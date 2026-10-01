#!/usr/bin/env python3
"""Atlas 'brakk_v2': hoja mejorada de Brakk del usuario
(assets/bosses/brakk/source/brakk_mejorado_b.png, fondo degradado rojo).

El fondo no es transparente: se estima a partir de las zonas lisas (poca
variacion local) con una convolucion normalizada y se resta; lo que difiere es
el personaje. Cada fila se corta por los huecos entre figuras (y los tramos
anchos por su valle mas vacio). Salida: assets/bosses/brakk/brakk_v2_atlas.png
"""
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/bosses/brakk/source/brakk_mejorado_b.png"
OUT = ROOT / "assets/bosses/brakk/brakk_v2_atlas.png"
CW, CH, PIVOT, COLS = 256, 176, (128, 168), 12
SCALE = 1.0
# filas de personaje: (techo, suelo)
ROWS = [(0, 136), (138, 268), (270, 402), (404, 532), (534, 666), (668, 792)]
ROW_UNIT = [66, 88, 90, 88, 72, 95]   # ancho tipico por fila (la primera esta mas apretada)
UNIT = 92   # ancho tipico de una figura


def foreground(img):
    gray = img.mean(-1)
    m = ndimage.uniform_filter(gray, 7); m2 = ndimage.uniform_filter(gray ** 2, 7)
    std = np.sqrt(np.maximum(m2 - m * m, 0))
    low = (std < 3.0).astype(float)
    bg = np.stack([ndimage.gaussian_filter(img[..., c] * low, 30) / np.maximum(ndimage.gaussian_filter(low, 30), 1e-3)
                   for c in range(3)], -1)
    diff = np.abs(img - bg).max(-1)
    r, g, b = img[..., 0], img[..., 1], img[..., 2]
    glow = (r > g + 60) & (r > b + 60) & (std < 9)     # resplandor rojo liso del fondo
    fg = (diff > 30) & ~glow
    fg = ndimage.binary_closing(fg, iterations=2)
    fg = ndimage.binary_fill_holes(fg)
    return ndimage.binary_opening(fg, iterations=1)


def segments(col, unit=UNIT):
    segs, s, gap = [], None, 0
    for x, v in enumerate(list(col) + [0] * 8):
        if v > 2:
            if s is None: s = x
            gap = 0; e = x
        elif s is not None:
            gap += 1
            if gap >= 3: segs.append((s, e + 1)); s = None
    out = []
    for a, b in segs:
        w = b - a
        if w < 30: continue
        k = max(1, round(w / unit))
        cuts = [a]
        for i in range(1, k):
            c = a + w * i // k
            cuts.append(min(range(c - 25, c + 25), key=lambda x: col[x]))
        cuts.append(b)
        out += [(cuts[i], cuts[i + 1]) for i in range(len(cuts) - 1)]
    return out


def main():
    img = np.array(Image.open(SRC).convert("RGB")).astype(float)
    fg = foreground(img)
    cells, info = [], []
    for r, (top, ground) in enumerate(ROWS):
        band = fg[top:ground]
        # columnas con "cuerpo" (no cuenta la cadena fina: se exige grosor)
        col = ndimage.binary_opening(band, structure=np.ones((5, 5))).sum(0)
        for a, b in segments(col, ROW_UNIT[r]):
            a2, b2 = max(0, a - 6), min(img.shape[1], b + 6)
            crop = img[top:ground, a2:b2].astype(np.uint8)
            mask = fg[top:ground, a2:b2].copy()
            lab, n = ndimage.label(mask)
            if n == 0: continue
            sizes = ndimage.sum(mask, lab, range(1, n + 1))
            keep = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s >= 0.04 * sizes.max()])
            rgba = np.dstack([crop, (keep * 255).astype(np.uint8)])
            piece = Image.fromarray(rgba, "RGBA")
            bodycols = ndimage.binary_opening(keep, structure=np.ones((5, 5))).sum(0)
            cx = int(np.average(np.arange(len(bodycols)), weights=bodycols + 1e-6)) if bodycols.sum() else piece.width // 2
            cell = Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
            cell.paste(piece, (PIVOT[0] - cx, PIVOT[1] - (ground - top)), piece)
            info.append((r, a2 + cx))
            cells.append(cell)
    rows = (len(cells) + COLS - 1) // COLS
    atlas = Image.new("RGBA", (COLS * CW, rows * CH), (0, 0, 0, 0))
    for i, c in enumerate(cells):
        atlas.paste(c, ((i % COLS) * CW, (i // COLS) * CH))
    atlas.save(OUT)
    print(f"{len(cells)} frames, rejilla {COLS}x{rows}, celda {CW}x{CH}")
    for i, (r, x) in enumerate(info): print(i, "fila", r + 1, "x", x, flush=True)


if __name__ == "__main__":
    main()
