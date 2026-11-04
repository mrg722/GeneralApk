#!/usr/bin/env python3
"""Pone celdas del atlas de MAX lado a lado a tamano real (solo LEE los PNG).

Uso: python3 tools/max/ver_celdas.py salida.png atlas.png i [j k ...] [--escala s1,s2,...] [--rejilla]
Cada celda se dibuja sobre gris con una linea roja en los pies (pivote y=168) y
lineas cada 20 px; --escala reescala cada celda alrededor del pivote (para ver
el antes/despues de "scales").
"""
import sys

from PIL import Image, ImageDraw

CW, CH, COLS = 270, 175, 14
PIV = (135, 168)


def cell(im, idx):
    cx, cy = (idx % COLS) * CW, (idx // COLS) * CH
    return im.crop((cx, cy, cx + CW, cy + CH))


def main():
    args = sys.argv[1:]
    scales = None
    if "--escala" in args:
        k = args.index("--escala")
        scales = [float(x) for x in args[k + 1].split(",")]
        del args[k:k + 2]
    out, atlas = args[0], args[1]
    idxs = [int(x) for x in args[2:]]
    im = Image.open(atlas).convert("RGBA")
    H = 230
    sheet = Image.new("RGBA", (CW * len(idxs), H), (80, 80, 90, 255))
    d = ImageDraw.Draw(sheet)
    for j, idx in enumerate(idxs):
        c = cell(im, idx)
        s = scales[j] if scales and j < len(scales) else 1.0
        if abs(s - 1) > 1e-3:
            c = c.resize((max(1, int(CW * s)), max(1, int(CH * s))), Image.NEAREST)
        ox = j * CW + int(PIV[0] - PIV[0] * s)
        oy = 40 + int(PIV[1] - PIV[1] * s)
        sheet.alpha_composite(c, (max(0, ox), max(0, oy)) if s <= 1 else (ox, oy))
        for y in range(40 + PIV[1], -1, -20):
            d.line((j * CW, y, j * CW + CW - 1, y), fill=(255, 255, 255, 40))
        d.line((j * CW, 40 + PIV[1], j * CW + CW - 1, 40 + PIV[1]), fill=(255, 0, 0, 255))
        d.text((j * CW + 4, 2), "%d x%.2f" % (idx, s), fill=(255, 255, 0, 255))
    sheet.save(out)
    print(out)


if __name__ == "__main__":
    main()
