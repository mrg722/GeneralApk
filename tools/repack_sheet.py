#!/usr/bin/env python3
"""Reempaqueta una hoja de celdas pequenas en celdas mayores SIN escalar.

Cada celda origen se coloca en la nueva celda de modo que su pivote caiga en
el pivote destino. Uso (Rayden 96x96 -> 128x128, pivote [64,126]):

    python3 tools/repack_sheet.py assets/characters/rayden_clean.png \
        assets/characters/rayden_128.png --grid 4 4 --src-cell 96 96 \
        --src-pivot 48 95 --dst-cell 128 128 --dst-pivot 64 126
"""
import argparse
from PIL import Image

ap = argparse.ArgumentParser()
ap.add_argument("src"); ap.add_argument("dst")
ap.add_argument("--grid", type=int, nargs=2, required=True)
ap.add_argument("--src-cell", type=int, nargs=2, required=True)
ap.add_argument("--src-pivot", type=int, nargs=2, required=True)
ap.add_argument("--dst-cell", type=int, nargs=2, required=True)
ap.add_argument("--dst-pivot", type=int, nargs=2, required=True)
a = ap.parse_args()

cols, rows = a.grid
(sw, sh), (dw, dh) = a.src_cell, a.dst_cell
ox, oy = a.dst_pivot[0] - a.src_pivot[0], a.dst_pivot[1] - a.src_pivot[1]
if ox < 0 or oy < 0 or ox + sw > dw or oy + sh > dh:
    raise SystemExit(f"la celda origen no cabe en la destino con offset ({ox},{oy})")

src = Image.open(a.src).convert("RGBA")
assert src.size == (cols * sw, rows * sh), f"hoja origen {src.size} no coincide con la grilla"
dst = Image.new("RGBA", (cols * dw, rows * dh), (0, 0, 0, 0))
for r in range(rows):
    for c in range(cols):
        cell = src.crop((c * sw, r * sh, (c + 1) * sw, (r + 1) * sh))
        dst.paste(cell, (c * dw + ox, r * dh + oy))
dst.save(a.dst)
print(f"OK {a.dst}: {dst.size[0]}x{dst.size[1]}, offset ({ox},{oy})")
