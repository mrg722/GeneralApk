#!/usr/bin/env python3
"""Iguala la estatura de Rayder entre grupos de frames.

La hoja v2 del usuario dibuja cada fila a distinto tamano (golpes/patadas ~146 px
de alto, caminar ~122 px). El extractor usaba una sola escala, asi que al golpear
Rayder "crecia" un 20 % y se veia ancho y a saltos. Este paso escala, alrededor
del pivote de los pies (80,122), solo los grupos grandes del atlas ya limpio.
Es idempotente: si el jab ya mide como el caminar, no hace nada.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ATLAS = ROOT / "assets/characters/rayder/rayder_atlas.png"
CW, CH, PIVOT = 160, 128, (80, 122)
# frame del atlas -> estatura de su grupo en la hoja original (caminar = 122)
GROUPS = {range(50, 68): 146, range(85, 88): 133, range(88, 90): 133, range(90, 92): 148}
TARGET = 122


def solid_height(cell):
    bb = cell.getchannel("A").point(lambda v: 255 if v > 80 else 0).getbbox()
    return 0 if not bb else bb[3] - bb[1]


def main():
    atlas = Image.open(ATLAS).convert("RGBA")
    cell = lambda f: atlas.crop(((f % 10) * CW, (f // 10) * CH, (f % 10 + 1) * CW, (f // 10 + 1) * CH))
    if solid_height(cell(50)) < solid_height(cell(92)) * 1.08:
        print("ya normalizado"); return
    for frames, h in GROUPS.items():
        k = TARGET / h
        for f in frames:
            c = cell(f)
            small = c.resize((round(CW * k), round(CH * k)), Image.LANCZOS)
            small.putalpha(small.getchannel("A").point(lambda v: 0 if v < 24 else v))
            out = Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
            out.paste(small, (round(PIVOT[0] - PIVOT[0] * k), round(PIVOT[1] - PIVOT[1] * k)), small)
            atlas.paste(out, ((f % 10) * CW, (f // 10) * CH))
    atlas.save(ATLAS)
    print("estatura igualada:", {f"{r.start}-{r.stop - 1}": round(TARGET / h, 3) for r, h in GROUPS.items()})


if __name__ == "__main__":
    main()
