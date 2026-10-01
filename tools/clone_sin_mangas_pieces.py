#!/usr/bin/env python3
"""Despiece del Rayder clon SIN MANGAS (boceto final del jefe).

Fuentes:
  - kf_clone/canva_sin_mangas/cabezas.png: 8 cabezas generadas con Canva desde el
    boceto (fondo negro); se recorta solo cabeza y cuello (sin el cuello del chaleco).
  - source/hoja_rayder_clone_final.jpg: hoja del personaje (fondo crema). De las
    vistas de frente, lado y espalda y del desglose de vestuario salen torsos
    (chaleco + camiseta rota), brazos desnudos, guanteletes, pantalon y botas.

Devuelve piezas RGBA (fondo transparente) por parte del cuerpo; las usa
build_kf_clone_rig_canva.py con la formula del heroe KF.
"""
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
SHEET = ROOT / "assets/characters/rayder/source/hoja_rayder_clone_final.jpg"
HEADS = ROOT / "assets/characters/rayder/kf_clone/canva_sin_mangas/cabezas.png"
UP = 3   # la hoja es chica (1024 px): se amplia antes de recortar

# (x0, y0, x1, y1) en la hoja original
BOXES = {
    "torso": [(86, 96, 160, 204), (290, 90, 366, 204), (205, 92, 262, 204)],
    "sleeve": [(52, 112, 94, 192), (146, 112, 190, 200), (360, 115, 399, 196), (264, 115, 296, 196)],
    "forearm": [(366, 180, 401, 246), (262, 184, 293, 246)],
    "fist": [(687, 217, 750, 310), (146, 210, 184, 248), (892, 255, 923, 313)],
    "thigh": [(60, 198, 120, 300), (118, 198, 168, 300), (272, 200, 322, 300), (322, 200, 372, 300)],
    "shin": [(60, 288, 120, 352), (118, 288, 168, 352), (272, 290, 322, 352), (322, 290, 372, 352)],
    "belt": [(78, 184, 142, 198), (300, 190, 362, 204)],
    "boot": [(727, 235, 818, 320), (38, 340, 96, 392), (270, 338, 322, 390), (352, 338, 398, 390)],
}


def _cut_cream(img, box):
    x0, y0, x1, y1 = box
    px = np.array(img.crop(box).resize(((x1 - x0) * UP, (y1 - y0) * UP), Image.LANCZOS)).astype(int)
    bg = np.array([238, 233, 224])
    dist = np.abs(px - bg).sum(-1)
    lum = px.mean(-1)
    fg = (dist > 70) & ~((lum > 125) & (px.max(-1) - px.min(-1) < 34))   # fuera crema y sombras grises claras
    fg = ndimage.binary_opening(fg, iterations=1)
    fg = ndimage.binary_fill_holes(ndimage.binary_closing(fg, iterations=2))
    lab, n = ndimage.label(fg)
    if n == 0: return None
    sizes = ndimage.sum(fg, lab, range(1, n + 1))
    keep = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s >= sizes.max() * 0.25])
    ys, xs = np.nonzero(keep)
    sl = (slice(ys.min(), ys.max() + 1), slice(xs.min(), xs.max() + 1))
    return np.dstack([px[sl].astype(np.uint8), (keep[sl] * 255).astype(np.uint8)])


def heads():
    """8 cabezas en una grilla de 2 filas x 4 columnas."""
    img = np.array(Image.open(HEADS).convert("RGB")).astype(int)
    H, W = img.shape[:2]
    out = []
    for r in range(2):
        for c in range(4):
            cell = img[r * H // 2:(r + 1) * H // 2, c * W // 4:(c + 1) * W // 4]
            fg = ndimage.binary_fill_holes(ndimage.binary_closing(cell.max(-1) > 9, iterations=4))
            lab, n = ndimage.label(fg)
            if n == 0: continue
            sizes = ndimage.sum(fg, lab, range(1, n + 1))
            m = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s > 3000])
            ys, xs = np.nonzero(m)
            y0, y1 = ys.min(), ys.max() + 1
            y1 = y0 + int((y1 - y0) * 0.78)   # cabeza y cuello, sin el cuello del chaleco ni hombros
            m = m[y0:y1]
            xs = np.nonzero(m.any(0))[0]
            sl = (slice(y0, y1), slice(xs.min(), xs.max() + 1))
            out.append(np.dstack([cell[sl].astype(np.uint8), (m[:, sl[1]] * 255).astype(np.uint8)]))
    return out


def library():
    img = Image.open(SHEET).convert("RGB")
    lib = {g: [p for p in (_cut_cream(img, b) for b in boxes) if p is not None] for g, boxes in BOXES.items()}
    lib["head"] = heads()
    return lib


if __name__ == "__main__":
    import sys
    lib = library()
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "despiece_sin_mangas.png"
    rows = []
    for g, ps in lib.items():
        hgt = 160
        tiles = [Image.fromarray(p, "RGBA").resize((max(1, p.shape[1] * hgt // p.shape[0]), hgt)) for p in ps]
        row = Image.new("RGBA", (sum(t.width + 8 for t in tiles) + 8, hgt + 8), (70, 70, 90, 255))
        x = 4
        for t in tiles: row.paste(t, (x, 4), t); x += t.width + 8
        rows.append(row); print(g, len(ps))
    W = max(r.width for r in rows)
    sheet = Image.new("RGBA", (W, sum(r.height for r in rows)), (70, 70, 90, 255))
    y = 0
    for r in rows: sheet.paste(r, (0, y)); y += r.height
    sheet.save(out)
