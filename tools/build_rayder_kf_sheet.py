#!/usr/bin/env python3
"""Atlas 'rayder_kf': hoja de movimientos estilo King Fighter del usuario
(assets/characters/rayder/source/rayder_movimientos_kf_negro.jpg, fondo negro).

Cada frame se ubica por la caja numerada que tiene bajo los pies (centro x) y
la linea de esas cajas marca el suelo de la fila. El fondo negro se quita por
inundacion desde los bordes y luego se recupera 1 px de contorno. Las
etiquetas (IDLE, WALK...) y los numeros se excluyen.
Uso: python3 tools/build_rayder_kf_sheet.py [fuente] [salida]
"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/characters/rayder/source/rayder_movimientos_kf_negro.jpg"
OUT = ROOT / "assets/characters/rayder/rayder_kf_atlas.png"
CW, CH = 224, 144          # celda
PIVOT = (112, 138)         # pies
SCALE = 0.83               # figura de pie ~120 px -> ~100 px (igual que Rayder)
COLS = 12

# fila: (y etiqueta, y techo de figuras, y de las cajas numeradas)
ROWS = [(12, 20, 153), (177, 168, 320), (343, 340, 469), (492, 490, 600), (623, 615, 735), (756, 752, 855),
        (879, 870, 1010)]
# accion: (fila, x inicio etiqueta, ancho etiqueta, centros x de sus frames)
ACTIONS = {
    "idle": (0, 0, 90, [35, 99, 162, 226, 290, 360]),
    "walk": (0, 410, 110, [440, 500, 567, 622, 686, 760, 826, 890]),
    "run": (0, 960, 130, [1025, 1107, 1186, 1268, 1340, 1414, 1485]),
    "jump": (1, 0, 100, [43, 115, 180, 246, 309, 369, 423]),
    "jump_forward": (1, 480, 170, [533, 591, 654, 718, 793, 840]),
    "jump_back": (1, 845, 150, [921, 985, 1059, 1123, 1190]),
    "landing": (1, 1225, 120, [1287, 1343, 1409, 1490]),
    "light_punch": (2, 0, 160, [50, 124, 211, 297]),
    "heavy_punch": (2, 365, 160, [398, 489, 580, 700]),
    "light_kick": (2, 745, 140, [795, 871, 940, 1024, 1093]),
    "heavy_kick": (2, 1145, 140, [1205, 1276, 1356, 1428, 1497]),
    "crouch_punch": (3, 0, 170, [48, 124, 198, 275]),
    "crouch_heavy_punch": (3, 330, 220, [370, 452, 560, 652]),
    "crouch_kick": (3, 695, 160, [754, 848, 938, 1023]),
    "crouch_heavy_kick": (3, 1080, 200, [1146, 1222, 1305, 1388, 1468]),
    "air_punch": (4, 0, 130, [55, 112, 171, 220, 265]),
    "air_kick": (4, 300, 110, [350, 414, 474, 511, 548]),
    "block": (4, 585, 100, [611, 670, 725, 789]),
    "hit_light": (4, 805, 130, [867, 942, 1004]),
    "hit_heavy": (4, 1040, 140, [1097, 1183, 1250]),
    "stagger": (4, 1280, 120, [1335, 1384, 1444, 1494]),
    "knockdown": (5, 0, 140, [54, 153, 231, 289, 355]),
    "getup": (5, 375, 110, [433, 491, 577, 640, 713]),
    "grab": (5, 745, 100, [790, 846, 925, 987]),
    "throw": (5, 1040, 110, [1086, 1161, 1240, 1334, 1410, 1486]),
    "special1": (6, 0, 260, [35, 98, 160, 228, 284, 328]),
    "special2": (6, 360, 230, [372, 434, 478, 510, 548, 600]),
    "special3": (6, 635, 200, [666, 726, 776, 821, 863, 901]),
    "super": (6, 935, 160, [968, 1035, 1109, 1192, 1269, 1343, 1418, 1478]),
}
# Frames del atlas anterior (rayder_atlas.png) que se agregan al final, en este orden.
LEGACY = [4, 5, 16, 29, 30, 40] + list(range(50, 102))

# Primer/ultimo frame de cada accion: ventana extra (estela, polvo o energia).
EDGE = 62


def background(crop):
    """Fondo = negro conectado a los bordes del recorte (la ropa negra interior queda)."""
    dark = crop.max(-1) <= 24
    seeds = np.zeros_like(dark)
    seeds[0, :] = seeds[-1, :] = seeds[:, 0] = seeds[:, -1] = True
    lab, _ = ndimage.label(dark)
    ids = np.unique(lab[seeds & dark])
    return np.isin(lab, ids[ids > 0])


def body_centers(img, top, ground, xs):
    """Ajusta los centros a los torsos reales (camisa blanca + piel; los efectos
    azules no cuentan) con k-medias en 1D partiendo de las cajas numeradas."""
    x0, x1 = max(0, xs[0] - 70), min(img.shape[1], xs[-1] + 70)
    band = img[top:ground, x0:x1]
    r, g, b = band[..., 0], band[..., 1], band[..., 2]
    shirt = (r > 165) & (g > 165) & (b > 160) & (band.max(-1) - band.min(-1) < 40)
    skin = (r > 110) & (r > g + 15) & (g > b + 5) & (r - b > 45)
    ys, cols = np.nonzero(shirt | skin)
    if len(cols) < 50: return xs
    pts = cols + x0
    c = np.array(xs, float)
    for _ in range(25):
        lab = np.argmin(np.abs(pts[:, None] - c[None, :]), axis=1)
        for k in range(len(c)):
            m = pts[lab == k]
            if len(m) > 30: c[k] = 0.5 * c[k] + 0.5 * np.median(m)
    # no dejar que dos centros se crucen o se peguen
    c = np.maximum.accumulate(c)
    for k in range(1, len(c)):
        if c[k] - c[k - 1] < 30: c[k] = c[k - 1] + 30
    return [int(round(v)) for v in c]


def main():
    src = Path(sys.argv[1]) if len(sys.argv) > 1 else SRC
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else OUT
    img = np.array(Image.open(src).convert("RGB")).astype(int)
    H, W, _ = img.shape
    work = img.copy()
    # borrar etiquetas
    for row, x0, w, _ in ACTIONS.values():
        ly = ROWS[row][0]
        work[max(0, ly - 13):ly + 13, max(0, x0):x0 + w] = 0
    cells, names = [], {}
    for name, (row, _, _, xs) in ACTIONS.items():
        _, top, boxy = ROWS[row]
        ground = boxy - 9
        names[name] = []
        xs = body_centers(img, top, ground, xs)
        for i, cx in enumerate(xs):
            left = (xs[i - 1] + cx) // 2 if i > 0 else cx - EDGE
            right = (xs[i + 1] + cx) // 2 if i + 1 < len(xs) else cx + EDGE
            left, right = max(0, left), min(W, right)
            crop = work[top:ground + 3, left:right].copy()
            fg = ~background(crop)
            # sin sombra del piso: grises oscuros poco saturados en las 10 filas de abajo
            sat = crop.max(-1) - crop.min(-1)
            low = (crop.max(-1) < 120) & (sat < 40)
            fg[-8:] &= ~low[-8:]
            # se quedan las piezas del propio frame: las que cruzan su columna
            # central o las sueltas (estelas) que no vienen cortadas del vecino
            lab, n = ndimage.label(fg, structure=np.ones((3, 3)))
            keep = np.zeros_like(fg)
            ccx = cx - left
            for k, sl in enumerate(ndimage.find_objects(lab), 1):
                comp = lab[sl] == k
                area = comp.sum()
                if area < 30: continue
                x0c, x1c = sl[1].start, sl[1].stop
                central = x0c <= ccx + 22 and x1c >= ccx - 22
                atEdge = x0c == 0 or x1c == crop.shape[1]
                if central or (not atEdge and area >= 60):
                    keep[sl] |= comp
            keep = ndimage.binary_fill_holes(keep)
            rgba = np.zeros(crop.shape[:2] + (4,), np.uint8)
            rgba[..., :3] = crop
            rgba[..., 3] = np.where(keep, 255, 0)
            piece = Image.fromarray(rgba, "RGBA")
            sw, sh = max(1, round(piece.width * SCALE)), max(1, round(piece.height * SCALE))
            small = piece.resize((sw, sh), Image.LANCZOS)
            small.putalpha(small.getchannel("A").point(lambda v: 0 if v < 40 else 255))
            cell = Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
            px = round(PIVOT[0] - (cx - left) * SCALE)
            py = round(PIVOT[1] - (ground - top) * SCALE)
            cell.paste(small, (px, py), small)
            names[name].append(len(cells))
            cells.append(cell)
    # Frames del atlas anterior que siguen siendo los mejores para algunas
    # acciones (gancho ascendente, onda, proyectil, en el aire, levantarse,
    # giro, victoria...): se copian a celdas de este tamano, pies con pies.
    old = Image.open(ROOT / "assets/characters/rayder/rayder_atlas.png").convert("RGBA")
    ocw, och, opiv = 160, 128, (80, 122)
    names["legacy"] = []
    for f in LEGACY:
        c = old.crop(((f % 10) * ocw, (f // 10) * och, (f % 10 + 1) * ocw, (f // 10 + 1) * och))
        # el set anterior era mas ancho y alto: misma complexion que la hoja nueva
        kx, ky = 0.93 * 0.84, 0.93
        c = c.resize((round(ocw * kx), round(och * ky)), Image.LANCZOS)
        c.putalpha(c.getchannel("A").point(lambda v: 0 if v < 40 else 255))
        cell = Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
        cell.paste(c, (round(PIVOT[0] - opiv[0] * kx), round(PIVOT[1] - opiv[1] * ky)), c)
        names["legacy"].append(len(cells))
        cells.append(cell)
    rows = (len(cells) + COLS - 1) // COLS
    atlas = Image.new("RGBA", (COLS * CW, rows * CH), (0, 0, 0, 0))
    for i, c in enumerate(cells):
        atlas.paste(c, ((i % COLS) * CW, (i // COLS) * CH))
    atlas.save(out)
    print(f"{len(cells)} frames, rejilla {COLS}x{rows}, celda {CW}x{CH} -> {out.relative_to(ROOT) if out.is_relative_to(ROOT) else out}")
    for k, v in names.items(): print(f"  {k}: {v[0]}-{v[-1]}")
    print("  legado: frame viejo f -> nuevo", names["legacy"][0], "+ indice en LEGACY")


if __name__ == "__main__":
    main()
