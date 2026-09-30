#!/usr/bin/env python3
"""Genera assets/characters/rayden_128.png (celda 128x128, pivote [64,126]).

Origen: assets/characters/rayden_clean.png (4x4, celda 96x96, pivote [48,95]),
que NO se modifica. Pasos:
  1. Reempaqueta los 16 frames originales sin escalar (offset +16,+31).
  2. Limpia las lineas negras semitransparentes que quedaron en los bordes de
     celda de la hoja original (alfa < 64 en las columnas/filas de borde).
  3. Deriva 6 frames nuevos SOLO a partir de pixeles de Rayden:
       16 anticipo   : guardia (1) desplazada 3 px atras y 1 px abajo
       17 golpe bajo : impacto (13) con las piernas flexionadas (12 px)
       18 aire subida: impacto (13) rotado 25 grados hacia atras
       19 aire caida : impacto (13) rotado 60 grados hacia atras
       20 levantarse : guardia (1) agachada (22 px de flexion)
       21 bloqueo    : guardia (3) 2 px atras con piernas algo flexionadas
Uso: python3 tools/build_rayden_sheet.py
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/characters/rayden_clean.png"
DST = ROOT / "assets/characters/rayden_128.png"
SC, DC = 96, 128
OX, OY = 16, 31               # pivote [48,95] -> [64,126]
PIVOT = (64, 126)
COLS, ROWS = 4, 6


def src_frame(sheet, i):
    r, c = divmod(i, 4)
    cell = sheet.crop((c * SC, r * SC, (c + 1) * SC, (r + 1) * SC))
    out = Image.new("RGBA", (DC, DC), (0, 0, 0, 0))
    out.paste(cell, (OX, OY))
    clean_borders(out)
    return out


def clean_borders(img):
    """Borra lineas de 1 px semitransparentes (restos de la grilla original):
    pixel con alfa < 64 cuyos vecinos perpendiculares son transparentes y que
    forma parte de un tramo recto de >= 12 px. La sombra de los pies (varias
    filas de grosor) no cumple la condicion y se conserva."""
    px = img.load()
    faint = lambda x, y: 0 < px[x, y][3] < 64
    clear = set()
    for horizontal in (True, False):
        for a in range(DC):
            run = []
            for b in range(DC + 1):
                x, y = (b, a) if horizontal else (a, b)
                ok = b < DC and faint(x, y)
                if ok:
                    if horizontal:
                        n1 = px[x, y - 1][3] if y > 0 else 0
                        n2 = px[x, y + 1][3] if y < DC - 1 else 0
                    else:
                        n1 = px[x - 1, y][3] if x > 0 else 0
                        n2 = px[x + 1, y][3] if x < DC - 1 else 0
                    ok = n1 < 64 and n2 < 64
                if ok:
                    run.append((x, y))
                else:
                    if len(run) >= 12: clear.update(run)
                    run = []
    # Franja de borde de la celda original (fila superior y columnas extremas):
    # ahi no hay cuerpo, solo restos de grilla de hasta 2 px de grosor.
    for y in range(DC):
        for x in range(DC):
            border = OY <= y <= OY + 2 or OX <= x <= OX + 1 or OX + SC - 2 <= x <= OX + SC - 1
            if border and faint(x, y):
                clear.add((x, y))
    for x, y in clear:
        px[x, y] = (0, 0, 0, 0)


def shift(img, dx, dy):
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    out.paste(img, (dx, dy), img)
    return out


def crouch(img, rows, knee_top, knee_bottom):
    """Flexiona las piernas quitando `rows` filas repartidas en la banda de
    rodillas y baja el torso: los pies quedan en la misma fila (pivote)."""
    band = list(range(knee_top, knee_bottom))
    step = max(1, len(band) // rows)
    drop = set(band[::step][:rows])
    keep = [y for y in range(DC) if y not in drop]
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    # se rellena desde abajo para que los pies no se muevan
    dst_y = DC - 1
    for y in reversed(keep):
        out.paste(img.crop((0, y, DC, y + 1)), (0, dst_y))
        dst_y -= 1
    return out


def rotate_back(img, degrees):
    # Facing derecha: "hacia atras" = antihorario alrededor de los pies.
    return img.rotate(degrees, resample=Image.NEAREST, center=PIVOT)


def main():
    sheet = Image.open(SRC).convert("RGBA")
    assert sheet.size == (4 * SC, 4 * SC), sheet.size
    frames = [src_frame(sheet, i) for i in range(16)]
    guard, guard2, hit = frames[1], frames[3], frames[13]
    frames += [
        shift(guard, -3, 1),                 # 16 anticipo
        crouch(hit, 12, 90, 116),             # 17 golpe bajo
        rotate_back(hit, 25),                # 18 aire subida
        rotate_back(hit, 60),                # 19 aire caida
        crouch(guard, 22, 84, 118),          # 20 levantarse
        shift(crouch(guard2, 5, 92, 114), -2, 0),  # 21 bloqueo
    ]
    out = Image.new("RGBA", (COLS * DC, ROWS * DC), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        r, c = divmod(i, COLS)
        out.paste(f, (c * DC, r * DC))
    out.save(DST)
    print(f"OK {DST.relative_to(ROOT)}: {out.size[0]}x{out.size[1]}, {len(frames)} frames")


if __name__ == "__main__":
    main()
