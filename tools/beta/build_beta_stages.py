"""Genera los escenarios del modo BETA desde nuevosSprites (sin cambiar tamanos).

Para cada nivel elegido:
  * compone las capas de tiles (mapdata/N.XML + map/K.png) a su resolucion
    original y la corta en trozos de <=2048 px de ancho (limite de textura
    seguro en Android) -> assets/beta/stages/<id>_<i>.png
  * calcula la franja caminable desde la capa [4]碰撞 (colision):
      8 = muro (fila superior e inferior de la franja), 9 = limite lateral,
      0 = suelo libre, 65535/-1 = vacio. Los bits 29/30 son espejo.
  * copia los fondos de parallax (image/*.png) que usa ese tipo de nivel
    segun data/GameSeting.lua (BoatGate, SnowOutGate, SnowInGate,
    FireOutGateBackRound) -> assets/beta/stages/bg/
  * escribe data/beta/stages/<id>.txt con todo lo anterior + musica y clima.

Uso: python3 tools/beta/build_beta_stages.py <carpeta nuevosSprites>
"""
import shutil
import sys
from collections import Counter
from pathlib import Path

from beta_map import parse_level, render_layer

ROOT = Path(__file__).resolve().parents[2]
OUT_IMG = ROOT / "assets/beta/stages"
OUT_DAT = ROOT / "data/beta/stages"
CHUNK = 2048
MASK = (1 << 29) | (1 << 30) | (1 << 31)

# id, nivel, nombre visible, tipo de fondo, musica, clima, color de cielo
STAGES = [
    ("barco",        101, "CUBIERTA DEL BARCO",   "barco",   "gate1music.ogg", "lluvia", (38, 44, 58)),
    ("nieve",        201, "MONTANA NEVADA",       "nieve",   "gate1music.ogg", "nieve",  (180, 196, 214)),
    ("cueva_helada", 282, "CUEVA HELADA",         "cueva",   "gate1music.ogg", "nieve",  (24, 32, 48)),
    ("volcan",       301, "CAMINO DEL VOLCAN",    "volcan",  "gate2music.ogg", "ceniza", (60, 22, 14)),
    ("arena_volcan", 142, "ARENA DEL VOLCAN",     "volcan",  "gate2music.ogg", "ceniza", (60, 22, 14)),
    ("nieve_larga",  351, "PASO DE LA MONTANA",   "nieve",   "gate1music.ogg", "nieve",  (180, 196, 214)),
]

# capa: archivo, factor de parallax (0 = fijo, 1 = como el suelo), y (px del
# nivel, borde superior), repetir en horizontal (1/0), separacion extra.
BACKGROUNDS = {
    "barco": [
        ("Sky.png", 0.05, 0, 1, 0),
        ("Sea.png", 0.15, 250, 1, 0),
        ("Boat_04.png", 0.20, 236, 1, 900),
        ("Boat_03.png", 0.25, 226, 1, 700),
        ("Boat_02.png", 0.32, 170, 1, 1300),
        ("Boat_01.png", 0.40, 140, 1, 1700),
    ],
    "nieve": [
        ("Xueshanwaibeijing.png", 0.08, 0, 1, 0),
        ("Xueshanwaizhongjing (4).png", 0.30, 150, 1, 300),
        ("Xueshanwaizhongjing (5).png", 0.38, 260, 1, 500),
        ("Xueshanwaizhongjing (1).png", 0.50, 110, 1, 900),
        ("Xueshanwaizhongjing (3).png", 0.55, 70, 1, 1300),
    ],
    "cueva": [
        ("Xueshanneibeijing.png", 0.08, 0, 1, 0),
        ("Xueshanneizhongjing (2).png", 0.30, 150, 1, 200),
        ("Xueshanneizhongjing (1).png", 0.45, 170, 1, 700),
        ("Xueshanneizhongjing (4).png", 0.55, 160, 1, 1100),
    ],
    "volcan": [
        ("Huoshanbeijing.png", 0.08, 0, 1, 0),
        ("HuoShanZhongJing_02.png", 0.30, 90, 1, 700),
        ("HuoShanZhongJing_01.png", 0.45, 130, 1, 1000),
    ],
}


def strip(v):
    return -1 if v < 0 else (v & ~MASK)


def walk_band(col, cols, rows):
    """Filas de muro (8) arriba/abajo -> rango de pies; columnas 9 -> limites."""
    cell = lambda c, r: strip(col["data"][r * cols + c])
    tops, bots = Counter(), Counter()
    for c in range(cols):
        wall_rows = [r for r in range(rows) if cell(c, r) == 8]
        free_rows = [r for r in range(rows) if cell(c, r) == 0]
        if not wall_rows or not free_rows:
            continue
        above = [r for r in wall_rows if r < min(free_rows)]
        below = [r for r in wall_rows if r > max(free_rows)]
        if above: tops[max(above)] += 1
        if below: bots[min(below)] += 1
    top = tops.most_common(1)[0][0] if tops else rows // 2
    bot = bots.most_common(1)[0][0] if bots else rows - 1
    # Limites laterales: columnas marcadas con 9 en la franja.
    side = sorted({c for c in range(cols) for r in range(top + 1, bot) if cell(c, r) == 9})
    th = col["th"]
    xmin, xmax = 0, cols * col["tw"]
    left = [c for c in side if c < cols // 4]
    right = [c for c in side if c > cols * 3 // 4]
    if left: xmin = (max(left) + 1) * col["tw"]
    if right: xmax = min(right) * col["tw"]
    return (top + 1) * th, bot * th, xmin, xmax


def main(src):
    src = Path(src)
    OUT_IMG.mkdir(parents=True, exist_ok=True)
    (OUT_IMG / "bg").mkdir(exist_ok=True)
    OUT_DAT.mkdir(parents=True, exist_ok=True)
    cache = {}
    for sid, level, name, kind, music, weather, sky in STAGES:
        layers = parse_level(src / "mapdata" / f"{level}.XML")
        col = next(l for l in layers if l["tileset"] == 4)
        img = None
        for l in layers:
            if l["tileset"] == 4:
                continue
            r = render_layer(l, src / "map", cache)
            if img is None: img = r
            else: img.alpha_composite(r)
        w, h = img.size
        y0, y1, xmin, xmax = walk_band(col, col["cols"], col["rows"])
        lines = [f"stage {sid}", f"name {name}", f"source mapdata/{level}.XML",
                 f"size {w} {h}", f"walk {y0} {y1} {xmin} {xmax}",
                 f"sky {sky[0]} {sky[1]} {sky[2]}", f"music {music}", f"weather {weather}"]
        for i, x in enumerate(range(0, w, CHUNK)):
            part = img.crop((x, 0, min(w, x + CHUNK), h))
            fn = f"{sid}_{i}.png"
            part.save(OUT_IMG / fn, optimize=True)
            lines.append(f"chunk {fn} {x}")
        for fn, par, y, rep, gap in BACKGROUNDS[kind]:
            safe = fn.replace(" (", "_").replace(")", "")
            shutil.copyfile(src / "image" / fn, OUT_IMG / "bg" / safe)
            lines.append(f"layer bg/{safe} {par} {y} {rep} {gap}")
        (OUT_DAT / f"{sid}.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
        print(f"{sid:13s} nivel {level} {w}x{h} pies {y0}..{y1} x {xmin}..{xmax}")

    # Musica y efectos de sonido (ogg originales, se copian tal cual).
    aud = ROOT / "assets/beta/audio"
    aud.mkdir(parents=True, exist_ok=True)
    for f in sorted((src / "sound").glob("*.ogg")):
        shutil.copyfile(f, aud / f.name)


if __name__ == "__main__":
    main(sys.argv[1])
