#!/usr/bin/env python3
"""Quita el halo blanco de los atlas de personajes.

Diagnostico (19-09-2026): los PNG de enemigos fueron compuestos sobre fondo
blanco y conservan ese blanco mezclado en el borde. El juego ya corregia en
tiempo de carga los pixeles semitransparentes, pero el borde con alfa alto
(200-255) seguia claro y se veia como una linea blanca alrededor del sprite.

Correccion en dos pasos, sin tocar el interior del dibujo:
1. "Unpremultiply" contra blanco para todo pixel con alfa parcial.
2. Para los pixeles del contorno que siguen siendo mucho mas claros que el
   interior vecino, se reemplaza el color por la mediana de sus vecinos
   interiores. El alfa NO se toca: la silueta queda igual.
"""
import sys, glob
import numpy as np
from PIL import Image

def edge_mask(alpha):
    e = np.zeros(alpha.shape, bool)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if dy == 0 and dx == 0:
                continue
            e |= (alpha > 0) & (np.roll(np.roll(alpha, dy, 0), dx, 1) == 0)
    return e

def clean(path, ratio=1.45, report=False):
    im = Image.open(path).convert('RGBA')
    a = np.array(im).astype(np.float64)
    alpha = a[:, :, 3]
    rgb = a[:, :, :3]

    # 0) los pixeles casi invisibles (alfa < 8) solo aportan restos del matte
    #    blanco: al escalar o filtrar reaparecen como un halo tenue.
    ghost = (alpha > 0) & (alpha < 8)
    a[ghost] = 0
    alpha = a[:, :, 3]
    rgb = a[:, :, :3]

    # 1) unpremultiply contra blanco en alfa parcial
    partial = (alpha > 0) & (alpha < 255)
    if partial.any():
        af = (alpha[partial] / 255.0)[:, None]
        rgb[partial] = np.clip((rgb[partial] - 255.0 * (1.0 - af)) / af, 0, 255)

    # 2) contorno residual mas claro que el interior
    edge = edge_mask(alpha)
    inner = (alpha > 200) & ~edge
    if not inner.any():
        return 0
    lum = rgb.mean(axis=2)
    base = lum[inner].mean()
    bright = edge & (lum > base * ratio)
    fixed = 0
    ys, xs = np.nonzero(bright)
    h, w = alpha.shape
    for y, x in zip(ys, xs):
        vals = []
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                ny, nx = y + dy, x + dx
                if 0 <= ny < h and 0 <= nx < w and inner[ny, nx]:
                    vals.append(rgb[ny, nx])
        if vals:
            rgb[y, x] = np.median(np.array(vals), axis=0)
            fixed += 1
    if fixed or partial.any():
        a[:, :, :3] = rgb
        Image.fromarray(a.astype(np.uint8)).save(path)
    if report:
        print(f"{path}: borde corregido={fixed} (luminancia interior {base:.0f})")
    return fixed

if __name__ == '__main__':
    targets = sys.argv[1:] or sorted(glob.glob('assets/enemies/*.png'))
    total = 0
    for p in targets:
        total += clean(p, report=True)
    print('pixeles de contorno corregidos:', total)
