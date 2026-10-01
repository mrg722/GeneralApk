#!/usr/bin/env python3
"""Rayder clon (rojo) armado con la FORMULA del heroe KF y su propio despiece.

Despiece: hojas generadas con Canva a partir del Rayder clon
(assets/characters/rayder/kf_clone/canva/: cabezas, torsos, brazos, piernas y
piezas base; ~50 piezas en varios angulos, fondo negro).

Formula: el heroe KF arma cada cuadro con piezas de su hoja de cuerpo (imagen 1
de animation.bin, 208 piezas) = recorte + posicion + giro/espejo. Para cada una
de esas 208 piezas se elige, entre las piezas del clon de la MISMA parte del
cuerpo, la que mejor encaja (forma, angulo y luces; 8 giros/espejos) y se
escala a su medida a RES veces la resolucion del KF (mas calidad). El juego
arma los cuadros con las mismas posiciones, giros y tiempos del KF.

Salida: kf_clone/rig_rojo.png (atlas de piezas) + kf_clone/rig_rojo.txt
("RES PAD" y luego "x y w h" por pieza del KF, en orden).
"""
import io, math, sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/apk")); sys.path.insert(0, str(ROOT / "tools"))
from kf_decode import load  # noqa: E402
from build_kf_clone_pieces import classify, hsv  # noqa: E402

CANVA = ROOT / "assets/characters/rayder/kf_clone/canva"
OUT = ROOT / "assets/characters/rayder/kf_clone"
RES = 3          # piezas a 3x la resolucion del KF
PAD = 4 * RES    # margen alrededor de cada pieza (la pieza del clon puede salir un poco)

# hoja -> grupo por fila (filas de arriba a abajo)
SHEETS = {
    "cabezas.png": ["head", "head"],
    "brazos.png": ["sleeve", "forearm", "fist"],
    "piernas.png": ["thigh", "shin", "boot"],
    "torsos.png": ["torso", "torso"],
    "piezas_base.png": ["head", "torso", "arm_mix", "leg_mix"],
}


# Etiquetas hechas a mano de las 208 piezas del cuerpo del heroe KF (imagen 1).
LABELS = {}
for g, ids in {
    "head": [11, 16, 27, 99, 115, 125, 138, 167, 169, 170, 179, 194, 205],
    "torso_head": [9, 80, 86, 94, 122, 124, 129, 130, 154, 168, 183, 199, 191],
    "torso": [7, 31, 41, 49, 58, 63, 101, 105, 131, 143, 144, 145, 147, 152, 156, 176, 177, 193],
    "sleeve": [0, 14, 33, 44, 59, 65, 84, 153, 201],
    "forearm": [10, 13, 18, 19, 26, 34, 35, 36, 45, 46, 54, 56, 57, 66, 67, 68, 69, 93, 97, 98, 100, 111, 160, 172,
                173, 178, 197, 202, 203, 204, 206, 207],
    "fist": [17, 23, 25, 55, 74, 85, 87, 104, 108, 112, 117, 121, 132, 133, 134, 164],
    "thigh": [3, 6, 22, 29, 39, 40, 47, 51, 60, 70, 72, 76, 79, 81, 89, 90, 96, 110, 123, 128, 135, 146, 148, 181,
              186, 187, 189, 195, 198, 200],
    "shin": [2, 5, 20, 21, 37, 38, 48, 50, 62, 71, 75, 82, 91, 95, 109, 118, 120, 126, 139, 151, 157, 161, 162, 165,
             174, 180, 188, 190, 196],
    "boot": [1, 4, 28, 30, 42, 43, 52, 53, 61, 64, 73, 77, 78, 116, 119, 127, 136, 140, 141, 149, 155, 163, 192],
}.items():
    for i in ids: LABELS[i] = g


def ordered_pieces(path, rows):
    """Recortes de una hoja (fondo negro) ordenados por fila y luego por x."""
    img = np.array(Image.open(path).convert("RGB")).astype(int)
    fg = img.max(-1) > 20
    fg = ndimage.binary_closing(fg, iterations=2)
    fg = ndimage.binary_fill_holes(fg)
    lab, n = ndimage.label(fg)
    out = []
    for k, sl in enumerate(ndimage.find_objects(lab)):
        m = lab[sl] == k + 1
        if m.sum() < 1500: continue
        r = min(rows - 1, int((sl[0].start + sl[0].stop) / 2 / img.shape[0] * rows))
        out.append((r, sl[1].start, np.dstack([img[sl].astype(np.uint8), (m * 255).astype(np.uint8)])))
    out.sort(key=lambda t: (t[0], t[1]))
    return [p for _, _, p in out]


def stack(head, torso, overlap=0.18):
    """Cabeza sobre torso (pieza combinada como las del KF)."""
    hw, hh = head.shape[1], head.shape[0]
    k = torso.shape[1] * 0.62 / hw
    hd = np.array(Image.fromarray(head, "RGBA").resize((max(1, round(hw * k)), max(1, round(hh * k))), Image.LANCZOS))
    W = max(torso.shape[1], hd.shape[1]); top = hd.shape[0] - round(hd.shape[0] * overlap)
    out = np.zeros((top + torso.shape[0], W, 4), np.uint8)
    tx = (W - torso.shape[1]) // 2
    out[top:top + torso.shape[0], tx:tx + torso.shape[1]] = torso
    hx = (W - hd.shape[1]) // 2
    reg = out[0:hd.shape[0], hx:hx + hd.shape[1]]
    a = hd[..., 3:4] > 110
    reg[:] = np.where(a, hd, reg)
    return out


def cut_pieces(path, groups):
    img = np.array(Image.open(path).convert("RGB")).astype(int)
    fg = img.max(-1) > 20
    fg = ndimage.binary_closing(fg, iterations=2)
    fg = ndimage.binary_fill_holes(fg)
    lab, n = ndimage.label(fg)
    objs = [(sl, k + 1) for k, sl in enumerate(ndimage.find_objects(lab)) if (lab[sl] == k + 1).sum() > 900]
    if not objs: return []
    # filas por posicion vertical
    cys = sorted((sl[0].start + sl[0].stop) / 2 for sl, _ in objs)
    H = img.shape[0]
    rows = len(groups)
    out = []
    for sl, k in objs:
        cy = (sl[0].start + sl[0].stop) / 2
        r = min(rows - 1, int(cy / H * rows))
        g = groups[r]
        m = lab[sl] == k
        rgba = np.dstack([img[sl].astype(np.uint8), (m * 255).astype(np.uint8)])
        if g == "arm_mix":   # piezas base fila 3: mangas, antebrazos, puños segun forma
            hh, ww = m.shape
            g = "fist" if max(hh, ww) < 140 else ("sleeve" if (rgba[..., :3].mean() < 70) else "forearm")
        if g == "leg_mix":
            g = "boot" if m.shape[0] < 140 else ("shin" if m.shape[0] < 200 else "thigh")
        out.append((g, rgba))
    return out


def recolor_red(px):
    """Colores del KF -> paleta del clon: amarillos/naranjas a rojo, grises a negro."""
    out = px.copy()
    h, s_, v = hsv(px.astype(float))
    a = px[..., 3] > 0
    warm = a & (s_ > 0.35) & ((h < 70) | (h > 320))
    gray = a & (s_ < 0.25) & (v > 0.15)
    out[warm, 0] = np.clip(v[warm] * 200, 0, 255); out[warm, 1] = (v[warm] * 30); out[warm, 2] = (v[warm] * 38)
    out[gray, :3] = (np.stack([v[gray]] * 3, -1) * 255 * 0.32).astype(np.uint8)
    return out


def axis(mask):
    ys, xs = np.nonzero(mask)
    if len(xs) < 3: return 0.0, 1.0, (mask.shape[1] / 2, mask.shape[0] / 2)
    cx, cy = xs.mean(), ys.mean()
    w, v = np.linalg.eigh(np.cov(np.vstack([xs - cx, ys - cy])))
    return math.degrees(math.atan2(v[1, 1], v[0, 1])), 4 * math.sqrt(max(w[1], 1e-3)), (cx, cy)


def place(piece, mask, variant, grow):
    """Pieza del clon orientada y escalada para cubrir `mask` (ya a RES)."""
    H, W = mask.shape
    a_t, len_t, c_t = axis(mask)
    pm = piece[..., 3] > 60
    a_s, len_s, _ = axis(pm)
    img = Image.fromarray(piece, "RGBA")
    if variant & 1: img = img.transpose(Image.Transpose.FLIP_LEFT_RIGHT); a_s = 180 - a_s
    k = 0.5 * len_t / max(len_s, 1) + 0.5 * math.sqrt(mask.sum() / max(1, pm.sum()))
    k *= grow
    img = img.resize((max(1, round(img.width * k)), max(1, round(img.height * k))), Image.LANCZOS)
    img = img.rotate(-(a_t - a_s + (180 if variant & 2 else 0)), resample=Image.BICUBIC, expand=True)
    arr = np.array(img)
    arr[..., 3] = np.where(arr[..., 3] > 110, 255, 0)
    _, _, c2 = axis(arr[..., 3] > 0)
    out = np.zeros((H + 2 * PAD, W + 2 * PAD, 4), np.uint8)
    ox, oy = round(c_t[0] + PAD - c2[0]), round(c_t[1] + PAD - c2[1])
    sy0, sx0, dy0, dx0 = max(0, -oy), max(0, -ox), max(0, oy), max(0, ox)
    hh = min(arr.shape[0] - sy0, out.shape[0] - dy0); ww = min(arr.shape[1] - sx0, out.shape[1] - dx0)
    if hh > 0 and ww > 0: out[dy0:dy0 + hh, dx0:dx0 + ww] = arr[sy0:sy0 + hh, sx0:sx0 + ww]
    return out


def place_upright(piece, mask, flip):
    """Cabezas y torsos: derechos (sin girar), a la altura y centro de la pieza del KF."""
    H, W = mask.shape
    ys, xs = np.nonzero(mask)
    img = Image.fromarray(piece, "RGBA")
    if flip: img = img.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    pm = np.array(img)[..., 3] > 60
    pys, pxs = np.nonzero(pm)
    k = (ys.max() - ys.min() + 1) / max(1, pys.max() - pys.min() + 1)
    k = min(k, (xs.max() - xs.min() + 1) * 1.25 / max(1, pxs.max() - pxs.min() + 1))
    img = img.resize((max(1, round(img.width * k)), max(1, round(img.height * k))), Image.LANCZOS)
    arr = np.array(img); arr[..., 3] = np.where(arr[..., 3] > 110, 255, 0)
    ays, axs = np.nonzero(arr[..., 3] > 0)
    out = np.zeros((H + 2 * PAD, W + 2 * PAD, 4), np.uint8)
    ox = round(xs.mean() + PAD - axs.mean()); oy = round(ys.max() + PAD - ays.max())
    sy0, sx0, dy0, dx0 = max(0, -oy), max(0, -ox), max(0, oy), max(0, ox)
    hh = min(arr.shape[0] - sy0, out.shape[0] - dy0); ww = min(arr.shape[1] - sx0, out.shape[1] - dx0)
    if hh > 0 and ww > 0: out[dy0:dy0 + hh, dx0:dx0 + ww] = arr[sy0:sy0 + hh, sx0:sx0 + ww]
    return out


def score(cand, kf, mask):
    c = cand[PAD:PAD + mask.shape[0], PAD:PAD + mask.shape[1]]
    cm = c[..., 3] > 0
    iou = (cm & mask).sum() / max(1, (cm | mask).sum())
    both = cm & mask
    corr = 0.0
    if both.sum() > 20:
        a = c[..., :3].mean(-1)[both]; b = kf[..., :3].astype(float).mean(-1)[both]
        if a.std() > 1 and b.std() > 1: corr = float(np.corrcoef(a, b)[0, 1])
    return iou + 0.3 * (corr if np.isfinite(corr) else 0)


def main():
    heads = ordered_pieces(CANVA / "cabezas.png", 2)        # 8: perfil, 3/4, frente, 3/4 abajo | arriba, grito, nuca, abajo
    arms = ordered_pieces(CANVA / "brazos.png", 3)          # 4 mangas | 4 antebrazos | 4 puños/manos
    legs = ordered_pieces(CANVA / "piernas.png", 3)         # muslos | piernas con bota | botas
    torsos = ordered_pieces(CANVA / "torsos.png", 2)        # frente, lado, 3/4 | espalda, inclinado adelante, atras
    base = ordered_pieces(CANVA / "piezas_base.png", 4)
    lib = {"head": heads[:], "torso": torsos[:], "sleeve": arms[0:4], "forearm": arms[4:8], "fist": arms[8:12],
           "thigh": legs[0:3], "shin": legs[3:6],
           # sin la bota vista desde la suela (casi toda blanca): se ve como una mancha gris
           "boot": [b for b in legs[6:] if b[..., :3][b[..., 3] > 0].mean() < 110]}
    # cabeza + torso: frente, perfil, 3/4, nuca, inclinado (perfil), atras (perfil arriba)
    pair = [(2, 0), (0, 1), (1, 2), (6, 3), (0, 4), (4, 5), (5, 4), (3, 2)]
    lib["torso_head"] = [stack(heads[h], torsos[t]) for h, t in pair if h < len(heads) and t < len(torsos)]
    print({g: len(v) for g, v in lib.items()}, "base", len(base))
    group_of = {g: [g] for g in lib}
    sprites, images = load(ROOT / "apk_reference/king_fighter_iii/bin/animation.bin")
    sheet = np.array(Image.open(io.BytesIO(images[1]["png"])).convert("RGBA"))
    tiles = []
    for (x, y, w, h) in images[1]["clips"]:
        kf = sheet[y:y + h, x:x + w] if w > 0 and h > 0 else np.zeros((1, 1, 4), np.uint8)
        big = np.array(Image.fromarray(kf, "RGBA").resize((max(1, w * RES), max(1, h * RES)), Image.NEAREST))
        mask = big[..., 3] > 0
        cls = LABELS.get(len(tiles), "other")
        cands_src = [p for g in group_of.get(cls, []) for p in lib.get(g, [])]
        if not cands_src:   # cinturones, cintas y efectos: la pieza del KF con colores del clon
            t = np.zeros((mask.shape[0] + 2 * PAD, mask.shape[1] + 2 * PAD, 4), np.uint8)
            t[PAD:PAD + mask.shape[0], PAD:PAD + mask.shape[1]] = recolor_red(big)
            tiles.append(t); continue
        best, bs = None, -9
        upright = cls in ("head", "torso_head", "torso")
        for p in cands_src:
            for v in (range(2) if upright else range(4)):
                c = place_upright(p, mask, v) if upright else place(p, mask, v, 1.0)
                sc = score(c, big, mask)
                if sc > bs: best, bs = c, sc
        # recorte: sobre la silueta del KF + un margen (las cabezas con mas margen: pelo)
        grow = 3 * RES if cls in ("head", "torso_head") else RES
        lim = np.zeros(best.shape[:2], bool)
        lim[PAD:PAD + mask.shape[0], PAD:PAD + mask.shape[1]] = mask
        lim = ndimage.binary_dilation(lim, iterations=grow)
        best[~lim] = 0
        tiles.append(best)
    W = 4096; xx = yy = rowh = 0; rects = []
    for t in tiles:
        if xx + t.shape[1] > W: xx = 0; yy += rowh + 1; rowh = 0
        rects.append((xx, yy, t.shape[1], t.shape[0])); xx += t.shape[1] + 1; rowh = max(rowh, t.shape[0])
    atlas = np.zeros((yy + rowh, W, 4), np.uint8)
    for (ax, ay, aw, ah), t in zip(rects, tiles): atlas[ay:ay + ah, ax:ax + aw] = t
    Image.fromarray(atlas, "RGBA").save(OUT / "rig_rojo.png")
    (OUT / "rig_rojo.txt").write_text(f"{RES} {PAD}\n" + "\n".join(f"{a} {b} {c} {d}" for a, b, c, d in rects) + "\n")
    print("atlas", atlas.shape, "piezas", len(tiles))


if __name__ == "__main__":
    main()
