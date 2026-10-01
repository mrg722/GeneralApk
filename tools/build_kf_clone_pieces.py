#!/usr/bin/env python3
"""Rompecabezas: arma la hoja de piezas del Rayder clon BETA sobre la COPIA de
la hoja de piezas del heroe KF (imagen 1 de animation.bin: 208 piezas de
cabeza, torso, mangas, antebrazos, puños, piernas y botas).

Para cada pieza del KF:
  1. Se clasifica por su material (pelo blanco = cabeza, camisa amarilla = torso,
     gris = manga, piel = antebrazo, marron/rojo = pierna, gris-azul abajo = bota,
     oscuro pequeño = puño).
  2. Se toma la MISMA parte recortada del Rayder clon (figura de frente de su
     hoja, ~133 px de alto) y se gira y escala para seguir el eje y el largo de
     la pieza del KF.
  3. Se pega dentro de la silueta exacta de la pieza (asi encaja en cada cuadro
     y habilidad del KF), conservando el contorno y la luz/sombra del KF.
  4. Los rojos del clon pasan a morado claro electrico; la piel queda intacta.
Salida: assets/characters/rayder/kf_clone/img_1.png (misma medida y posiciones
que la hoja original: el juego la usa en lugar de la del KF solo para la copia).
"""
import colorsys, io, math, sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/apk"))
sys.path.insert(0, str(ROOT / "tools"))
from kf_decode import load  # noqa: E402
from build_rayder_clone_beta import purple  # noqa: E402

CLONE = ROOT / "assets/characters/rayder/source/rayder_clone_hoja.png"
OUT = ROOT / "assets/characters/rayder/kf_clone/img_1.png"
FIG = (1330, 1430, 186, 330)            # figura de frente del clon (x0, x1, y0, y1)
# partes del clon (x0, x1, y0, y1) en la hoja
PARTS = {
    "head": (1381, 1410, 191, 223), "torso": (1371, 1418, 217, 258), "sleeve": (1359, 1379, 221, 250),
    "forearm": (1362, 1379, 245, 259), "fist": (1358, 1374, 253, 273), "thigh": (1376, 1399, 257, 290),
    "shin": (1376, 1396, 286, 312), "boot": (1374, 1399, 306, 327),
}


def hsv(c):
    r, g, b = c[..., 0] / 255, c[..., 1] / 255, c[..., 2] / 255
    mx, mn = np.max(c[..., :3], -1) / 255, np.min(c[..., :3], -1) / 255
    s = np.where(mx > 0, (mx - mn) / np.maximum(mx, 1e-6), 0)
    h = np.zeros_like(mx); d = np.maximum(mx - mn, 1e-6)
    h = np.where(mx == r, ((g - b) / d) % 6, np.where(mx == g, (b - r) / d + 2, (r - g) / d + 4)) * 60
    return h, s, mx


def classify(px, mask):
    h, s, v = hsv(px.astype(float))
    m = mask & (v > 0.12)
    n = max(1, m.sum())
    white = ((s < 0.15) & (v > 0.82) & m).sum() / n
    yellow = ((h > 38) & (h < 64) & (s > 0.4) & m).sum() / n
    skin = ((h > 5) & (h < 32) & (s > 0.25) & (s < 0.62) & (v > 0.55) & m).sum() / n
    gray = ((s < 0.16) & (v > 0.3) & (v < 0.82) & m).sum() / n
    bluish = ((h > 170) & (h < 230) & (s > 0.08) & m).sum() / n
    pants = (((h < 30) | (h > 330)) & (s > 0.3) & (v < 0.6) & m).sum() / n
    big = max(mask.shape)
    if white > 0.12 and yellow > 0.04: return "torso_head"
    if white > 0.12: return "head"
    if yellow > 0.05: return "torso"
    if skin > 0.45: return "forearm"
    if bluish > 0.2: return "boot"
    if pants > 0.3: return "thigh" if big > 24 else "shin"
    if gray > 0.3: return "jacket" if big > 30 else "sleeve"
    if m.sum() < 90: return "fist"
    return "jacket" if gray > 0.15 else "other"


def axis(mask):
    ys, xs = np.nonzero(mask)
    if len(xs) < 3: return 0.0, 1.0, 1.0, (mask.shape[1] / 2, mask.shape[0] / 2)
    cx, cy = xs.mean(), ys.mean()
    cov = np.cov(np.vstack([xs - cx, ys - cy]))
    w, v = np.linalg.eigh(cov)
    ang = math.degrees(math.atan2(v[1, 1], v[0, 1]))
    major, minor = 4 * math.sqrt(max(w[1], 1e-3)), 4 * math.sqrt(max(w[0], 1e-3))
    return ang, major, minor, (cx, cy)


def fitted(part_img, part_mask, mask):
    """Gira y escala la parte del clon para cubrir la silueta `mask`."""
    a_t, maj_t, min_t, c_t = axis(mask)
    a_s, maj_s, min_s, c_s = axis(part_mask)
    src = Image.fromarray(np.dstack([part_img, (part_mask * 255).astype(np.uint8)]), "RGBA")
    # alinear el eje mayor del recorte con el horizontal, escalar y girar al de la pieza
    src = src.rotate(a_s, resample=Image.BICUBIC, expand=True)
    m2 = np.array(src)[..., 3] > 60
    _, maj2, min2, _ = axis(m2)
    kx, ky = maj_t / max(maj2, 1), min_t / max(min2, 1)
    src = src.resize((max(1, round(src.width * kx * 1.08)), max(1, round(src.height * ky * 1.08))), Image.LANCZOS)
    src = src.rotate(-a_t, resample=Image.BICUBIC, expand=True)
    arr = np.array(src)
    _, _, _, c2 = axis(arr[..., 3] > 60)
    out = np.zeros(mask.shape + (4,), np.uint8)
    ox, oy = round(c_t[0] - c2[0]), round(c_t[1] - c2[1])
    H, W = mask.shape
    sy0, sx0 = max(0, -oy), max(0, -ox)
    dy0, dx0 = max(0, oy), max(0, ox)
    hh = min(arr.shape[0] - sy0, H - dy0); ww = min(arr.shape[1] - sx0, W - dx0)
    if hh > 0 and ww > 0:
        out[dy0:dy0 + hh, dx0:dx0 + ww] = arr[sy0:sy0 + hh, sx0:sx0 + ww]
    return out


def main():
    sprites, images = load(ROOT / "apk_reference/king_fighter_iii/bin/animation.bin")
    sheet = Image.open(io.BytesIO(images[1]["png"])).convert("RGBA")
    clips = images[1]["clips"]
    kf = np.array(sheet)
    clone = np.array(Image.open(CLONE).convert("RGB")).astype(float)
    # mascara de la figura del clon (fondo gris liso conectado al borde)
    from build_rayder_clone_beta import foreground
    fig = np.zeros(clone.shape[:2], bool)
    x0, x1, y0, y1 = FIG
    fig[y0:y1, x0:x1] = foreground(clone, FIG)
    painted = purple(clone.astype(np.uint8))
    parts = {}
    for name, (a, b, c, d) in PARTS.items():
        parts[name] = (painted[c:d, a:b], fig[c:d, a:b])
    parts["torso_head"] = parts["torso"]
    parts["jacket"] = parts["sleeve"]
    # material limpio del clon (tabla de colores) como base de cada pieza
    import json
    lut = json.loads((ROOT / "data/kf_clone_palette.json").read_text())["colors"]
    def base_of(px):
        b = px[..., :3].copy()
        for yy, xx in zip(*np.nonzero(px[..., 3])):
            k = "%02x%02x%02x" % tuple(int(c) for c in px[yy, xx, :3])
            if k in lut: b[yy, xx] = [int(lut[k][i:i + 2], 16) for i in (0, 2, 4)]
        return b.astype(float)
    # costuras del clon mas suaves: el morado se oscurece y desatura en la ropa
    for name in list(parts):
        im_, mk = parts[name]
        hh, ss, vv = hsv(im_.astype(float))
        pur = (hh > 250) & (hh < 320) & (ss > 0.2)
        soft = im_.astype(float).copy()
        gray = soft.mean(-1, keepdims=True)
        soft[pur] = (soft[pur] * 0.45 + gray[pur] * 0.55) * 0.8
        parts[name] = (soft.astype(np.uint8), mk)
    out = kf.copy()
    counts = {}
    for i, (x, y, w, h) in enumerate(clips):
        if w <= 0 or h <= 0: continue
        px = kf[y:y + h, x:x + w]
        mask = px[..., 3] > 0
        if mask.sum() < 4: continue
        cls = classify(px, mask)
        counts[cls] = counts.get(cls, 0) + 1
        if cls == "other": continue
        pimg, pmask = parts[cls]
        fit = fitted(pimg, pmask, mask)
        for c in range(3): fit[..., c] = ndimage.median_filter(fit[..., c], size=2)
        have = fit[..., 3] > 60
        # relleno: donde el recorte no llega, color medio de la parte del clon
        mean = pimg[pmask].mean(0) if pmask.any() else np.array([30, 28, 36])
        col = np.where(have[..., None], fit[..., :3], mean).astype(float)
        # luz y sombra del KF (relativa) + su contorno oscuro
        lum = px[..., :3].astype(float).mean(-1)
        rel = np.clip((lum + 20) / (np.median(lum[mask]) + 20), 0.55, 1.45)[..., None]
        col = np.clip(col * (0.25 + 0.75 * rel), 0, 255)
        outline = lum < 28
        res = px.copy()
        sel = mask & ~outline
        # cabeza/torso: la piel del KF (cara, cuello, manos) queda; el resto es del clon
        h_, s_, v_ = hsv(px.astype(float))
        skin = (h_ > 5) & (h_ < 32) & (s_ > 0.25) & (s_ < 0.62) & (v_ > 0.55)
        if cls in ("head", "torso", "torso_head", "forearm"): sel &= ~skin
        if cls in ("head", "torso_head"):
            # cabeza: solo el pelo (blanco del KF) pasa a ser el pelo del clon
            hair = (s_ < 0.18) & (v_ > 0.6)
            if cls == "head": sel &= hair
            else: col[hair] = col[hair]   # torso con pelo: todo del clon salvo piel
        base = base_of(px)
        mix = 0.6 if cls in ("head", "torso_head") else 0.45   # parte del clon / material limpio
        col = col * mix + base * (1 - mix)
        # contraste: volumen como el KF (luces mas claras, sombras mas oscuras)
        m_ = col[sel].mean() if sel.any() else 0
        col = np.clip((col - m_) * 1.45 + m_ * 1.05, 0, 255)
        # limpieza: sin puntos sueltos y con pocos colores, como el pixel art del KF
        for ch in range(3): col[..., ch] = ndimage.median_filter(col[..., ch], size=3)
        res[sel, :3] = col[sel].astype(np.uint8)
        if sel.sum() > 12:
            q = Image.fromarray(res[..., :3]).quantize(colors=7 if cls in ("head", "torso", "torso_head") else 5,
                                                         method=Image.Quantize.MEDIANCUT).convert("RGB")
            res[sel, :3] = np.array(q)[sel]
        out[y:y + h, x:x + w] = res
    OUT.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(out, "RGBA").save(OUT)
    print("piezas:", counts, "->", OUT.relative_to(ROOT))


if __name__ == "__main__":
    main()
