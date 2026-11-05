#!/usr/bin/env python3
"""Hojas de contacto numeradas del atlas de MAX (solo LEE los PNG).

Uso: python3 tools/max/hoja_contacto.py [atlas.png] [salida_dir] [desde] [hasta] [por_hoja]
Dibuja cada celda 270x175 (a la mitad) con su indice y una cruz en el pivote
(135,168). Escribe <salida_dir>/max_cells_<desde>_<hasta>.png.
"""
import os
import sys

from PIL import Image, ImageDraw

CW, CH, COLS = 270, 175, 14
PIV = (135, 168)


def main():
    atlas = sys.argv[1] if len(sys.argv) > 1 else "assets/characters/max/max_atlas.png"
    out = sys.argv[2] if len(sys.argv) > 2 else "/tmp/max_hojas"
    lo = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    hi = int(sys.argv[4]) if len(sys.argv) > 4 else -1
    per = int(sys.argv[5]) if len(sys.argv) > 5 else 42
    os.makedirs(out, exist_ok=True)
    im = Image.open(atlas).convert("RGBA")
    total = (im.width // CW) * (im.height // CH)
    if hi < 0 or hi >= total:
        hi = total - 1
    cols, k = 7, 0.5
    w, h = int(CW * k), int(CH * k)
    for start in range(lo, hi + 1, per):
        end = min(hi, start + per - 1)
        n = end - start + 1
        rows = (n + cols - 1) // cols
        sheet = Image.new("RGBA", (cols * w, rows * h), (60, 60, 70, 255))
        d = ImageDraw.Draw(sheet)
        for j, idx in enumerate(range(start, end + 1)):
            cx, cy = (idx % COLS) * CW, (idx // COLS) * CH
            cell = im.crop((cx, cy, cx + CW, cy + CH)).resize((w, h), Image.NEAREST)
            ox, oy = (j % cols) * w, (j // cols) * h
            bg = Image.new("RGBA", (w, h), (95, 95, 105, 255) if (j + j // cols) % 2 else (70, 70, 80, 255))
            bg.alpha_composite(cell)
            sheet.paste(bg, (ox, oy))
            px, py = ox + PIV[0] * k, oy + PIV[1] * k
            d.line((px - 4, py, px + 4, py), fill=(255, 0, 0, 255))
            d.line((px, py - 4, px, py + 4), fill=(255, 0, 0, 255))
            d.rectangle((ox, oy, ox + 22, oy + 11), fill=(0, 0, 0, 255))
            d.text((ox + 2, oy), str(idx), fill=(255, 255, 0, 255))
        path = f"{out}/max_cells_{start}_{end}.png"
        sheet.save(path)
        print(path)


if __name__ == "__main__":
    main()
