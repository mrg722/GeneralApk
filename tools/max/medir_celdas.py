#!/usr/bin/env python3
"""Mide el CUERPO de cada celda del atlas de MAX (solo LEE los PNG).

Uso: python3 tools/max/medir_celdas.py [atlas.png] [salida.tsv]

Pixel de cuerpo = alfa > 128 y NO efecto azul (H 190-230 grados, S > 0.5,
V > 0.5). Por celda escribe: indice, pixeles opacos, pixeles de cuerpo,
bbox del cuerpo (x0 y0 x1 y1, px de celda), alto del cuerpo (y1 - y0), ancho
y la "altura de cabeza" = pivote_y (168) - y0 (cuanto sube la cabeza sobre
los pies) y el ancho de la cabeza (fila superior + 14 px), que es lo que mejor
delata un cambio de set (chaqueta ~1.0x frente a camiseta ~1.6x).
"""
import colorsys
import sys

from PIL import Image

CW, CH, COLS = 270, 175, 14
PIVY = 168


def is_effect(r, g, b):
    h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
    return 190 <= h * 360 <= 230 and s > 0.5 and v > 0.5


def measure(im, idx):
    cx, cy = (idx % COLS) * CW, (idx // COLS) * CH
    cell = im.crop((cx, cy, cx + CW, cy + CH))
    px = cell.load()
    opaque = body = 0
    x0 = y0 = 10 ** 9
    x1 = y1 = -1
    rows = {}
    for y in range(CH):
        for x in range(CW):
            r, g, b, a = px[x, y]
            if a <= 128:
                continue
            opaque += 1
            if is_effect(r, g, b):
                continue
            body += 1
            x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x), max(y1, y)
            rows.setdefault(y, []).append(x)
    if body == 0:
        return dict(idx=idx, opaque=opaque, body=0, bbox=(0, 0, 0, 0), h=0, w=0, head=0, headw=0)
    # ancho de la cabeza: filas y0 .. y0+14, ancho medio del tramo mas ancho por fila
    ws = []
    for y in range(y0, min(CH, y0 + 15)):
        xs = rows.get(y)
        if xs:
            ws.append(max(xs) - min(xs) + 1)
    headw = sorted(ws)[len(ws) // 2] if ws else 0
    return dict(idx=idx, opaque=opaque, body=body, bbox=(x0, y0, x1, y1), h=y1 - y0 + 1, w=x1 - x0 + 1,
                head=PIVY - y0, headw=headw)


def measure_all(path):
    im = Image.open(path).convert("RGBA")
    total = (im.width // CW) * (im.height // CH)
    return [measure(im, i) for i in range(total)]


def main():
    atlas = sys.argv[1] if len(sys.argv) > 1 else "assets/characters/max/max_atlas.png"
    out = sys.argv[2] if len(sys.argv) > 2 else "/tmp/max_celdas.tsv"
    rows = measure_all(atlas)
    with open(out, "w") as f:
        f.write("idx\topaco\tcuerpo\tx0\ty0\tx1\ty1\talto\tancho\tcabeza_sobre_pies\tancho_cabeza\n")
        for r in rows:
            f.write("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n" % (
                r["idx"], r["opaque"], r["body"], *r["bbox"], r["h"], r["w"], r["head"], r["headw"]))
    print(out, len(rows), "celdas")


if __name__ == "__main__":
    main()
