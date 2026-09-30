#!/usr/bin/env python3
"""Genera assets/characters/rayder/rayder_atlas.png desde la hoja del usuario
assets/characters/rayder/source/rayder_poses_50_fondo.png (1536x1024 RGBA,
50 poses numeradas en 5 filas, fondo transparente).

La hoja NO es una grilla uniforme, asi que:
  1. Cada pose se delimita por la posicion de su numero (etiqueta blanca).
  2. Se borran los numeros (digitos blancos + contorno negro).
  3. Cada pieza conectada de la fila se asigna a la pose cuya franja contiene
     su centro, asi los brazos/efectos que cruzan la frontera no se cortan.
  4. Escala x0.5 (LANCZOS, alfa premultiplicado), alfa residual < 24 fuera.
  5. Alineacion: centro del torso -> x=80; suelo de la fila -> y=122.
Salida: 10x5 celdas de 160x128, pivote [80,122]; frame i = pose i+1.
Despues ejecutar tools/clean_sprite_cutouts.py (quita astillas de celda).
Uso: python3 tools/build_rayder_sheet.py
"""
from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/characters/rayder/source/rayder_poses_50_fondo.png"
DST = ROOT / "assets/characters/rayder/rayder_atlas.png"
CW, CH, PIVOT, SCALE = 160, 128, (80, 122), 0.5
# (y de la etiqueta, x de cada etiqueta) medidos sobre la hoja original.
LABELS = [
    (12,  [12, 162, 307, 446, 563, 726, 920, 1124, 1266, 1425]),
    (221, [13, 163, 304, 450, 647, 775, 914, 1078, 1248, 1411]),
    (430, [12, 167, 319, 471, 618, 787, 957, 1097, 1226, 1393]),
    (644, [12, 150, 307, 450, 605, 758, 896, 1068, 1226, 1363]),
    (832, [12, 173, 310, 465, 631, 802, 947, 1071, 1221, 1388]),
]
BANDS = [(0, 215), (215, 420), (420, 636), (636, 822), (822, 1024)]


def is_white(p): return p[3] > 150 and min(p[:3]) > 200


def flood(px, seeds, pred, box):
    x0, y0, x1, y1 = box
    seen = set(s for s in seeds if pred(px[s])); q = deque(seen)
    while q:
        a, b = q.popleft()
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                n = (a + dx, b + dy)
                if x0 <= n[0] < x1 and y0 <= n[1] < y1 and n not in seen and pred(px[n]):
                    seen.add(n); q.append(n)
    return seen


def erase_labels(img):
    px = img.load()
    for ly, xs in LABELS:
        for lx in xs:
            box = (lx - 3, ly - 3, lx + 30, ly + 22)
            seeds = [(x, y) for x in range(box[0], box[2]) for y in range(box[1], box[3])]
            digits = flood(px, seeds, is_white, box)
            if not digits: continue
            dx0 = min(p[0] for p in digits) - 6; dx1 = max(p[0] for p in digits) + 7
            dy0 = min(p[1] for p in digits) - 6; dy1 = max(p[1] for p in digits) + 7
            for x in range(dx0, dx1):
                for y in range(dy0, dy1):
                    p = px[x, y]
                    # digitos, contorno negro y la caja semitransparente del numero
                    if is_white(p) or (p[3] > 0 and max(p[:3]) < 90) or 0 < p[3] < 200:
                        px[x, y] = (0, 0, 0, 0)


def pieces(px, box):
    x0, y0, x1, y1 = box
    solid = lambda p: p[3] > 100
    seen, out = set(), []
    for y in range(y0, y1):
        for x in range(x0, x1):
            if (x, y) in seen or not solid(px[x, y]): continue
            comp = flood(px, [(x, y)], solid, box)
            seen |= comp; out.append(comp)
    return out


def solid_bbox(img, thr=200):
    return img.getchannel("A").point(lambda v: 255 if v >= thr else 0).getbbox()


def main():
    sheet = Image.open(SRC).convert("RGBA"); W, H = sheet.size
    erase_labels(sheet)
    px = sheet.load()
    out = Image.new("RGBA", (10 * CW, 5 * CH), (0, 0, 0, 0))
    for r, (band, (_, xs)) in enumerate(zip(BANDS, LABELS)):
        box = (0, band[0], W, band[1])
        # Lineas de corte: el valle de menor densidad solida cerca de cada etiqueta.
        colsum = [sum(1 for y in range(band[0], band[1]) if px[x, y][3] > 200) for x in range(W)]
        cuts = [0]
        for lx in xs[1:]:
            lo, hi = max(cuts[-1] + 20, lx - 70), min(W - 1, lx + 30)
            cuts.append(min(range(lo, hi), key=lambda x: (colsum[x], abs(x - (lx - 4)))))
        cuts.append(W)
        region = lambda x: max(i for i in range(len(xs)) if x >= cuts[i])
        poses = [Image.new("RGBA", (W, band[1] - band[0]), (0, 0, 0, 0)) for _ in xs]
        loads = [p.load() for p in poses]
        taken = set()
        # Piezas solidas (alfa > 100): una pieza que cae en una sola franja se
        # asigna entera por su centro (no se corta un brazo que cruza); si la
        # pieza abarca varias poses fusionadas por el brillo, se corta.
        for comp in pieces(px, box):
            if len(comp) < 6: continue
            taken |= comp
            xs_c = [p[0] for p in comp]
            span = {region(min(xs_c)), region(max(xs_c))}
            big = len(span) > 1 and len(comp) > 4000
            cx = sum(xs_c) / len(comp)
            for x, y in comp:
                k = region(x) if big else region(cx)
                loads[k][x, y - band[0]] = px[x, y]
        # Brillos tenues (alfa <= 100): por linea de corte.
        for y in range(band[0], band[1]):
            for x in range(W):
                p = px[x, y]
                if 0 < p[3] and (x, y) not in taken:
                    loads[region(x)][x, y - band[0]] = p
        bboxes = [solid_bbox(p) for p in poses]
        ground = max(b[3] for b in bboxes if b)
        for c, pose in enumerate(poses):
            b = bboxes[c]
            if not b: continue
            torso = pose.crop((b[0], b[1] + (b[3] - b[1]) * 3 // 10, b[2], b[1] + (b[3] - b[1]) * 6 // 10))
            tb = solid_bbox(torso)
            cx = b[0] + ((tb[0] + tb[2]) / 2 if tb else (b[2] - b[0]) / 2)
            full = pose.getchannel("A").getbbox()
            crop = pose.crop(full)
            small = crop.resize((max(1, round(crop.width * SCALE)), max(1, round(crop.height * SCALE))), Image.LANCZOS)
            small.putalpha(small.getchannel("A").point(lambda v: 0 if v < 24 else v))
            ox = round(PIVOT[0] - (cx - full[0]) * SCALE)
            oy = round(PIVOT[1] - (ground - full[1]) * SCALE)
            cell = Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
            cell.paste(small, (ox, oy), small)
            out.paste(cell, (c * CW, r * CH))
    DST.parent.mkdir(parents=True, exist_ok=True)
    out.save(DST)
    print(f"OK {DST.relative_to(ROOT)}: {out.size[0]}x{out.size[1]}, 50 frames")


if __name__ == "__main__":
    main()
