#!/usr/bin/env python3
"""Recorta los botones tactiles de las hojas del usuario (assets/ui/source/).

Las hojas no traen transparencia (fondo degradado), asi que cada boton se
recorta con una mascara circular de borde suave. Los botones con texto en
ingles o sin sentido (ATAQUE, DCR, BR-CO, SKILL...) no se usan: el juego dibuja
encima su propia etiqueta en espanol. Salida: assets/ui/touch/*.png
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/ui/source/botones_kit_azul_verde_rojo.png"
OUT = ROOT / "assets/ui/touch"

# nombre: (centro x, centro y, radio) en la hoja de 1536x1024
BUTTONS = {
    "golpe": (378, 578, 66),          # GOLPE rojo (texto ya en espanol)
    "dash": (98, 705, 66),            # DASH azul con corredor
    "bloq": (238, 705, 66),           # BLOQ verde con escudo
    "puno_azul": (60, 822, 36),
    "corredor_azul": (145, 822, 36),
    "escudo_azul": (233, 822, 36),
    "mira_verde": (320, 822, 36),
    "espiral_azul": (405, 822, 36),
    "calavera_verde": (490, 822, 36),
    "mano_roja": (575, 822, 36),
    "estallido_rojo": (660, 822, 36),
    "puno_verde": (60, 896, 36),
    "estallido_verde": (660, 896, 36),
    "puno_gris": (60, 970, 36),
    "palanca_base": (196, 212, 178),  # base del joystick (se borra la perilla dibujada)
    "palanca_perilla": (645, 230, 50),
}


def cut(img, cx, cy, r, feather=3):
    box = (cx - r, cy - r, cx + r, cy + r)
    c = img.crop(box).convert("RGBA")
    m = Image.new("L", c.size, 0)
    ImageDraw.Draw(m).ellipse((feather, feather, 2 * r - feather, 2 * r - feather), fill=255)
    m = m.filter(ImageFilter.GaussianBlur(feather / 2))
    c.putalpha(m)
    return c


def main():
    img = Image.open(SRC).convert("RGBA")
    OUT.mkdir(parents=True, exist_ok=True)
    for name, (cx, cy, r) in BUTTONS.items():
        c = cut(img, cx, cy, r)
        if name == "palanca_base":
            # La base trae una perilla dibujada en el centro: se tapa con el fondo
            # oscuro del aro para que solo se vea la perilla que se mueve.
            d = ImageDraw.Draw(c)
            k = 78
            d.ellipse((r - k, r - k, r + k, r + k), fill=(14, 20, 30, 235))
        c.save(OUT / f"{name}.png")
    print(f"{len(BUTTONS)} botones -> {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
