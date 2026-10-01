#!/usr/bin/env python3
"""Rayder clon BETA = COPIA del heroe KF (sprites 0 y 1 de animation.bin) con
cada pieza vestida como el Rayder clon. Las formas, piezas, cuadros y tiempos
del heroe no cambian; solo el material de cada pieza:

  pelo blanco         -> negro con reflejo (mechon claro en el tono mas alto)
  chaqueta gris       -> cuero negro
  camiseta amarilla   -> camiseta blanca
  pantalon marron     -> negro
  llamas / acentos    -> morado claro electrico
  piel                -> intacta
Salida: data/kf_clone_palette.json (color exacto -> color nuevo), que usa el
juego al armar los cuadros de la copia (src/game/lab/KfReference.cpp).
"""
import colorsys, io, json, sys
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/apk"))
from kf_decode import load  # noqa: E402

PURPLE_H = 282 / 360


def classify(rgb):
    r, g, b = [c / 255 for c in rgb]
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    hd = h * 360
    if v < 0.06: return rgb                                           # contorno
    skin = (5 <= hd <= 32) and 0.25 <= s <= 0.62 and v > 0.55 and b > 0.3 * r
    if skin: return rgb
    if s < 0.13 and v > 0.84:                                          # blanco: pelo
        return tuple(int(c * 255) for c in colorsys.hsv_to_rgb(0.68, 0.18, 0.30 + (v - 0.84) * 2.0))
    if 38 <= hd <= 62 and s > 0.4:                                     # amarillo: camiseta
        return tuple(int(c * 255) for c in colorsys.hsv_to_rgb(0.6, 0.03, min(1, 0.55 + v * 0.45)))
    warm = hd < 38 or hd > 320
    if warm and s > 0.45 and v > 0.35:                                 # llamas y acentos
        return tuple(int(c * 255) for c in colorsys.hsv_to_rgb(PURPLE_H, s * 0.62, min(1, v * 1.2 + 0.1)))
    if warm and s > 0.3:                                               # rojo oscuro / marron: negro morado
        return tuple(int(c * 255) for c in colorsys.hsv_to_rgb(PURPLE_H, 0.35, v * 0.55))
    # grises y azulados: cuero / tela negra
    return tuple(int(c * 255) for c in colorsys.hsv_to_rgb(0.66, 0.10, 0.06 + v * 0.40))


def main():
    sprites, images = load(ROOT / "apk_reference/king_fighter_iii/bin/animation.bin")
    table = {}
    for sp in (0, 1):
        for img_id in sorted(set((m >> 16) & 0xFFFF for m in sprites[sp]["modules"])):
            im = Image.open(io.BytesIO(images[img_id]["png"]))
            if im.mode != "P": continue
            pal = im.getpalette()[: 3 * 256]
            used = np.unique(np.array(im))
            for i in (int(v) for v in used):
                if 3 * i + 3 > len(pal): continue
                c = tuple(pal[3 * i: 3 * i + 3])
                key = "%02x%02x%02x" % c
                if key not in table:
                    n = classify(c)
                    table[key] = "%02x%02x%02x" % tuple(int(x) for x in n)
    out = ROOT / "data/kf_clone_palette.json"
    out.write_text(json.dumps({"colors": table}, indent=0))
    print(len(table), "colores ->", out.relative_to(ROOT))


if __name__ == "__main__":
    main()
