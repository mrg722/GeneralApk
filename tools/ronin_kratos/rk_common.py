"""Utilidades comunes de RONIN-KRATOS: leer los .txt de Kratos, cargar sus
hojas empaquetadas, recortar piezas con su espejo/giro y armar cuadros igual
que src/rendering/Animator.h (DrawPieces)."""
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data/kratos"
ACTOR = ROOT / "assets/kratos/actor"
WEAPONS = ["espadas", "rayo", "garras", "cesto"]

_sheet_cache = {}


def sheet(img_id):
    """Hoja RGBA (uint8) de assets/kratos/actor/<id>.png."""
    img_id = str(img_id)
    if img_id not in _sheet_cache:
        _sheet_cache[img_id] = np.array(Image.open(ACTOR / f"{img_id}.png").convert("RGBA"))
    return _sheet_cache[img_id]


class CharFile:
    """Un .txt de personaje por piezas, conservando cada linea tal cual."""

    def __init__(self, path):
        self.path = Path(path)
        self.lines = self.path.read_text().splitlines()
        self.images = {}            # idx -> id (str)
        self.frames = []            # [{'line': i, 'pieces': [{'line': j, 'img','sx','sy','sw','sh','dx','dy','flip'}]}]
        self.clips = {}             # nombre -> [frame idx]
        for i, l in enumerate(self.lines):
            t = l.split()
            if not t or t[0].startswith("#"):
                continue
            if t[0] == "image":
                self.images[int(t[1])] = t[2]
            elif t[0] == "frame":
                self.frames.append({"line": i, "pieces": []})
            elif t[0] == "piece":
                v = list(map(int, t[1:9]))
                self.frames[-1]["pieces"].append(dict(line=i, img=v[0], sx=v[1], sy=v[2], sw=v[3], sh=v[4],
                                                      dx=v[5], dy=v[6], flip=v[7]))
            elif t[0] == "clip":
                self.clips[t[1]] = [int(x.split(":")[0]) for x in t[4:]]

    def key(self, p):
        """Pieza unica = (id de hoja, recorte)."""
        return (self.images[p["img"]], p["sx"], p["sy"], p["sw"], p["sh"])


def orient(patch, flip):
    """Recorte tal como se ve en pantalla (mirando a la derecha)."""
    if flip & 1:
        patch = patch[:, ::-1]
    if flip & 2:
        patch = patch[::-1]
    if flip & 4:
        patch = np.rot90(patch, 1)
    return patch


def unorient_coords(flip, sw, sh):
    """Funcion (yy, xx) en pantalla -> (y, x) en el recorte de la hoja."""
    def f(yy, xx):
        y, x = yy, xx
        if flip & 4:           # deshacer rot90 antihorario: pantalla (h=sw, w=sh)
            # rot90(P,1)[i, j] = P[j, W-1-i] con W = ancho tras espejos (= sw)
            y, x = xx, sw - 1 - yy
        if flip & 2:
            y = sh - 1 - y
        if flip & 1:
            x = sw - 1 - x
        return y, x
    return f


def piece_pixels(p, cf, src=None):
    img = src if src is not None else sheet(cf.images[p["img"]])
    return img[p["sy"]:p["sy"] + p["sh"], p["sx"]:p["sx"] + p["sw"]]


def compose(cf, fi, getter=None, pad=8, mirror=False, with_index=False):
    """Arma el cuadro fi. getter(p) -> (patch RGBA del recorte sin orientar, dx, dy).
    Devuelve (canvas RGBA, origen (ox, oy) de los pies en el lienzo[, indice de pieza por pixel])."""
    items = []
    for k, p in enumerate(cf.frames[fi]["pieces"]):
        if getter:
            patch, dx, dy = getter(p)
        else:
            patch, dx, dy = piece_pixels(p, cf), p["dx"], p["dy"]
        o = orient(patch, p["flip"])
        items.append((o, dx, dy, k))
    if not items:
        c = np.zeros((4, 4, 4), np.uint8)
        return (c, (2, 2), np.full((4, 4), -1)) if with_index else (c, (2, 2))
    x0 = min(d for _, d, _, _ in items); y0 = min(d for _, _, d, _ in items)
    x1 = max(d + o.shape[1] for o, d, _, _ in items); y1 = max(d + o.shape[0] for o, _, d, _ in items)
    W, H = x1 - x0 + 2 * pad, y1 - y0 + 2 * pad
    can = np.zeros((H, W, 4), np.float32)
    idx = np.full((H, W), -1, np.int32)
    for o, dx, dy, k in items:
        X, Y = dx - x0 + pad, dy - y0 + pad
        a = o[..., 3:4].astype(np.float32) / 255
        reg = can[Y:Y + o.shape[0], X:X + o.shape[1]]
        reg[..., :3] = o[..., :3] * a + reg[..., :3] * (1 - a)
        reg[..., 3:4] = 255 * (a + reg[..., 3:4] / 255 * (1 - a))
        idx[Y:Y + o.shape[0], X:X + o.shape[1]][o[..., 3] > 0] = k
    can = can.clip(0, 255).astype(np.uint8)
    ox, oy = -x0 + pad, -y0 + pad
    if mirror:
        can = can[:, ::-1]; idx = idx[:, ::-1]; ox = W - ox
    return (can, (ox, oy), idx) if with_index else (can, (ox, oy))


def on_bg(rgba, color=(70, 110, 70)):
    a = rgba[..., 3:4].astype(np.float32) / 255
    return (rgba[..., :3] * a + np.array(color, np.float32) * (1 - a)).astype(np.uint8)


def hsv(c):
    c = c[..., :3].astype(np.float32) / 255
    r, g, b = c[..., 0], c[..., 1], c[..., 2]
    mx, mn = c.max(-1), c.min(-1)
    d = np.maximum(mx - mn, 1e-6)
    s = np.where(mx > 0, (mx - mn) / np.maximum(mx, 1e-6), 0)
    h = np.where(mx == r, ((g - b) / d) % 6, np.where(mx == g, (b - r) / d + 2, (r - g) / d + 4)) * 60
    h = np.where(mx - mn < 1e-6, 0, h)
    return h, s, mx
