#!/usr/bin/env python3
"""Efectos de poder azules del usuario (assets/fx/source/efectos_azules.jpg,
5 filas x 8 cuadros sobre damero morado). La transparencia sale del propio
brillo cian del efecto (el damero es morado oscuro), asi el resplandor queda
suave. Salida: assets/fx/efectos_azules.png (rejilla 8x5, celdas 128x96).
Filas: 0 formacion del proyectil, 1 impacto y explosion, 2 escudo y area,
3 estallido final, 4 chispas y detalles."""
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/fx/source/efectos_azules.jpg"
OUT = ROOT / "assets/fx/efectos_azules.png"
ROWS = [(58, 120), (154, 223), (254, 324), (358, 436), (470, 542)]
CW, CH = 128, 96

img = np.array(Image.open(SRC).convert("RGB")).astype(float)
SRC2 = ROOT / "assets/fx/source/efectos_azules_estelas.jpg"   # fila 6: estelas y esquiva (49-56)
atlas = Image.new("RGBA", (8 * CW, 6 * CH), (0, 0, 0, 0))
jobs = [(img, r, y) for r, y in enumerate(ROWS)]
img2 = np.array(Image.open(SRC2).convert("RGB")).astype(float)
jobs.append((img2, 5, (470, 542)))
for img, r, (y0, y1) in jobs:
    for c in range(8):
        cell = img[y0:y1, c * 128 + 2:(c + 1) * 128 - 2]
        g = cell[..., 1]
        a = np.clip((g - 75.0) / 110.0, 0, 1)
        rgba = np.dstack([cell, a * 255]).astype(np.uint8)
        piece = Image.fromarray(rgba, "RGBA")
        atlas.paste(piece, (c * CW + 2, r * CH + (CH - piece.height) // 2), piece)
atlas.save(OUT)
print(OUT.relative_to(ROOT))
