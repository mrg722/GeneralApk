#!/usr/bin/env python3
"""Despiece del Rayder clon + armado con la formula del KF (sin rellenar la
silueta del KF: cada pieza del clon se ve entera, con su propio diseño).

El heroe KF arma cada cuadro con piezas de su hoja (imagen 1 de animation.bin):
cada pieza = recorte + posicion + giro/espejo. Aqui:
  1. Se recorta el Rayder clon en piezas propias (cabeza de perfil, de frente y
     de espaldas, torso, manga, antebrazo, puño, muslo, pierna, bota) desde las
     vistas "SPRITE POR DIRECCION" de su hoja.
  2. Cada pieza del KF se reemplaza por la pieza del clon de la misma parte del
     cuerpo, escalada a su medida (sin deformarla) y orientada (de 4 variantes,
     la que mejor encaja con la pieza del KF).
  3. Se guarda una hoja de piezas propia (con margen alrededor de cada pieza)
     y su tabla de recortes; el juego arma los cuadros con las MISMAS
     posiciones, giros y tiempos del KF (src/game/lab/KfReference.cpp).
Color: --color purple (morado claro electrico) o green (verde electrico).
"""
import argparse, io, json, math, sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/apk"))
sys.path.insert(0, str(ROOT / "tools"))
from kf_decode import load  # noqa: E402
from build_rayder_clone_beta import foreground  # noqa: E402
from build_kf_clone_pieces import classify, hsv  # noqa: E402

CLONE = ROOT / "assets/characters/rayder/source/rayder_clone_hoja.png"
VIEWS = (20, 410, 815, 955)          # seccion "SPRITE POR DIRECCION"
# despiece (x0, x1, y0, y1) en la hoja del clon
PIECES = {
    "head_side": (345, 371, 823, 851), "head_front": (59, 86, 823, 851), "head_back": (149, 176, 823, 851),
    "torso": (344, 381, 844, 882), "torso_back": (137, 190, 840, 879), "sleeve": (368, 382, 851, 877),
    "forearm": (366, 381, 870, 885), "fist": (364, 381, 881, 896), "thigh": (359, 383, 881, 911),
    "shin": (360, 383, 904, 930), "boot": (365, 387, 925, 942),
}
PAD = 14


def recolor(rgb, color):
    """Rojos del clon -> morado o verde electrico claro; piel intacta."""
    h, s, v = hsv(rgb.astype(float))
    skin = (h >= 8) & (h <= 40) & (s < 0.62) & (v > 0.42)
    red = ((h < 25) | (h > 330)) & (s > 0.45) & ~skin & (v > 0.18)
    hue = {"purple": 282, "green": 128}[color]
    out = rgb.copy().astype(float)
    import colorsys
    for y, x in zip(*np.nonzero(red)):
        rr, gg, bb = colorsys.hsv_to_rgb(hue / 360, s[y, x] * 0.62, min(1.0, v[y, x] * 1.25 + 0.08))
        out[y, x] = (rr * 255, gg * 255, bb * 255)
    return out.astype(np.uint8)


def axis(mask):
    ys, xs = np.nonzero(mask)
    if len(xs) < 3: return 0.0, 1.0, (mask.shape[1] / 2, mask.shape[0] / 2)
    cx, cy = xs.mean(), ys.mean()
    w, v = np.linalg.eigh(np.cov(np.vstack([xs - cx, ys - cy])))
    return math.degrees(math.atan2(v[1, 1], v[0, 1])), 4 * math.sqrt(max(w[1], 1e-3)), (cx, cy)


def place(piece, mask, size, variant):
    """Pieza del clon orientada/escalada sobre un lienzo `size` (con margen) centrada en la del KF."""
    H, W = mask.shape
    a_t, len_t, c_t = axis(mask)
    a_s, len_s, _ = axis(piece[..., 3] > 60)
    img = Image.fromarray(piece, "RGBA")
    if variant & 1: img = img.transpose(Image.Transpose.FLIP_LEFT_RIGHT); a_s = 180 - a_s
    k = len_t / max(len_s, 1)
    area_k = math.sqrt(mask.sum() / max(1, (piece[..., 3] > 60).sum()))
    k = 0.5 * k + 0.5 * area_k                         # escala uniforme: largo y area de la pieza del KF
    img = img.resize((max(1, round(img.width * k)), max(1, round(img.height * k))), Image.BOX)
    rot = a_t - a_s + (180 if variant & 2 else 0)
    img = img.rotate(-rot, resample=Image.BICUBIC, expand=True)
    arr = np.array(img)
    arr[..., 3] = np.where(arr[..., 3] > 90, 255, 0)
    _, _, c2 = axis(arr[..., 3] > 0)
    out = np.zeros((H + 2 * PAD, W + 2 * PAD, 4), np.uint8)
    ox, oy = round(c_t[0] + PAD - c2[0]), round(c_t[1] + PAD - c2[1])
    sy0, sx0, dy0, dx0 = max(0, -oy), max(0, -ox), max(0, oy), max(0, ox)
    hh = min(arr.shape[0] - sy0, out.shape[0] - dy0); ww = min(arr.shape[1] - sx0, out.shape[1] - dx0)
    if hh > 0 and ww > 0: out[dy0:dy0 + hh, dx0:dx0 + ww] = arr[sy0:sy0 + hh, sx0:sx0 + ww]
    # la pieza ocupa el lugar de la del KF (+1 px): asi las piezas encajan sin taparse
    lim = np.zeros(out.shape[:2], bool)
    lim[PAD:PAD + H, PAD:PAD + W] = ndimage.binary_dilation(mask, iterations=1)
    out[~lim] = 0
    return out


def score(cand, kf, mask):
    """Encaje: solapamiento de siluetas + parecido de luces/sombras."""
    c = cand[PAD:PAD + mask.shape[0], PAD:PAD + mask.shape[1]]
    cm = c[..., 3] > 0
    inter = (cm & mask).sum(); union = (cm | mask).sum()
    iou = inter / max(1, union)
    both = cm & mask
    if both.sum() > 8:
        a = c[..., :3].mean(-1)[both]; b = kf[..., :3].astype(float).mean(-1)[both]
        corr = np.corrcoef(a, b)[0, 1] if a.std() > 1 and b.std() > 1 else 0
    else:
        corr = 0
    return iou + 0.35 * (corr if np.isfinite(corr) else 0)


def build(color, out_png, out_json, pieces_png=None):
    sprites, images = load(ROOT / "apk_reference/king_fighter_iii/bin/animation.bin")
    sheet = np.array(Image.open(io.BytesIO(images[1]["png"])).convert("RGBA"))
    clips = images[1]["clips"]
    src = np.array(Image.open(CLONE).convert("RGB")).astype(float)
    fg = np.zeros(src.shape[:2], bool)
    x0, x1, y0, y1 = VIEWS
    fg[y0:y1, x0:x1] = foreground(src, VIEWS)
    painted = recolor(src.astype(np.uint8), color)
    pieces = {}
    for name, (a, b, c, d) in PIECES.items():
        m = fg[c:d, a:b]
        lab, n = ndimage.label(m)
        if n: m = lab == (np.argmax(ndimage.sum(m, lab, range(1, n + 1))) + 1)
        pieces[name] = np.dstack([painted[c:d, a:b], (m * 255).astype(np.uint8)])
    if pieces_png:   # hoja del despiece (plantilla)
        wsum = sum(p.shape[1] + 4 for p in pieces.values()); hmax = max(p.shape[0] for p in pieces.values())
        sheetp = Image.new("RGBA", (wsum, hmax), (0, 0, 0, 0)); xx = 0
        for p in pieces.values():
            sheetp.paste(Image.fromarray(p, "RGBA"), (xx, 0)); xx += p.shape[1] + 4
        sheetp.save(pieces_png)
    tiles, table, counts = [], [], {}
    for i, (x, y, w, h) in enumerate(clips):
        kf = sheet[y:y + h, x:x + w] if w > 0 and h > 0 else np.zeros((1, 1, 4), np.uint8)
        mask = kf[..., 3] > 0
        cls = classify(kf, mask) if mask.sum() >= 4 else "other"
        if cls in ("head", "torso_head"):
            hh, ss, vv = hsv(kf.astype(float))
            skin = ((hh > 5) & (hh < 32) & (ss > 0.25) & (vv > 0.5) & mask).sum() / max(1, mask.sum())
            name = "head_back" if skin < 0.04 else "head_side"
            if cls == "torso_head": name = "torso" if skin >= 0.04 else "torso_back"
        elif cls == "torso": name = "torso"
        elif cls == "jacket": name = "torso_back"
        elif cls in pieces: name = cls
        else: name = None
        counts[name or "kf"] = counts.get(name or "kf", 0) + 1
        if name is None:
            tile = np.zeros((h + 2 * PAD, w + 2 * PAD, 4), np.uint8)
            rgb = recolor(kf[..., :3], color)
            tile[PAD:PAD + h, PAD:PAD + w] = np.dstack([rgb, kf[..., 3]])
        else:
            cands = [place(pieces[name], mask, None, v) for v in range(4)]
            tile = max(cands, key=lambda t: score(t, kf, mask))
        tiles.append(tile)
    # empaquetar
    W = 1024; xx = yy = rowh = 0; rects = []
    for t in tiles:
        if xx + t.shape[1] > W: xx = 0; yy += rowh + 1; rowh = 0
        rects.append((xx, yy, t.shape[1], t.shape[0])); xx += t.shape[1] + 1; rowh = max(rowh, t.shape[0])
    atlas = np.zeros((yy + rowh, W, 4), np.uint8)
    for (ax, ay, aw, ah), t in zip(rects, tiles): atlas[ay:ay + ah, ax:ax + aw] = t
    Image.fromarray(atlas, "RGBA").save(out_png)
    out_json.write_text(json.dumps({"image": 1, "pad": PAD, "clips": rects}))
    print(color, counts, "->", out_png.relative_to(ROOT))


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--color", default="purple")
    a = ap.parse_args()
    d = ROOT / "assets/characters/rayder/kf_clone"
    d.mkdir(parents=True, exist_ok=True)
    build(a.color, d / f"img_1_rig_{a.color}.png", d / f"img_1_rig_{a.color}.json", d / f"despiece_{a.color}.png")
