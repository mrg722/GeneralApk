#!/usr/bin/env python3
"""Rayder de espaldas (pantalla de eleccion de luchador), recortado de la hoja
de poses del usuario (fondo de cuadros gris). Salida: assets/characters/rayder/rayder_espalda.png"""
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/characters/rayder/source/rayder_extra_492d4123.jpg"
OUT = ROOT / "assets/characters/rayder/rayder_espalda.png"

img = np.array(Image.open(SRC).convert("RGB")).astype(int)
crop = img[530:1010, 40:330]
sat = crop.max(-1) - crop.min(-1)
light = (crop.min(-1) > 95) & (sat < 28)        # cuadros grises del fondo
lab, _ = ndimage.label(light)
edge = np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))
fg = ~np.isin(lab, edge[edge > 0])
fg = ndimage.binary_opening(fg, iterations=1)
lab2, n = ndimage.label(fg)
fg = lab2 == (np.argmax(ndimage.sum(fg, lab2, range(1, n + 1))) + 1)
fg = ndimage.binary_fill_holes(fg)
# fondo atrapado entre el brazo y el torso: zonas claras grandes en el tercio derecho
inner, m = ndimage.label(light & fg)
for k, sl in enumerate(ndimage.find_objects(inner), 1):
    comp = inner[sl] == k
    if comp.sum() > 120 and (sl[1].start + sl[1].stop) / 2 > 0.6 * crop.shape[1]:
        fg[sl] &= ~comp
ys, xs = np.nonzero(fg)
rgba = np.dstack([crop.astype(np.uint8), (fg * 255).astype(np.uint8)])[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
im = Image.fromarray(rgba, "RGBA")
# Mismo estilo que Rayden Cruz: pixel art de ~92 px de alto, ampliado sin suavizar.
small = im.resize((round(im.width * 92 / im.height), 92), Image.LANCZOS)
small.putalpha(small.getchannel("A").point(lambda v: 0 if v < 110 else 255))
small.save(OUT)
rgba = np.array(small)
print(OUT.relative_to(ROOT), rgba.shape[1], "x", rgba.shape[0])
