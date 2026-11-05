#!/usr/bin/env python3
"""Mide las poses sueltas del CLON ANTIGUO (assets/bosses/rayder_clone) y de
GRINDER (assets/bosses/grinder) para F1.2.

Por cada PNG: tamano, bbox opaco (alfa > 16), islas opacas (componentes
4-conexas) con su bbox y area, y el "source rect" propuesto = bbox de la isla
principal crecida con las islas que la tocan o estan a <= 3 px (las islas
separadas del cuerpo -- restos de recorte -- quedan fuera y se listan).
Una isla grande que toca el borde de la textura y no es el cuerpo (la pierna
suelta de kick.png) es un resto de recorte aunque este pegada al cuerpo.
Los restos PEGADOS al cuerpo (trozo de bota en el borde derecho de hurt.png y
ready.png) no son islas: se cortan con un segundo rectangulo (CORTES) y la pose
se dibuja en 2 piezas. Tambien da la columna del centro del torso (mediana de x opacas en la franja
35-60 % de altura del cuerpo), que es el ancla horizontal.

Uso: python3 tools/poses/medir_clon_grinder.py [--tabla]
  --tabla  imprime ademas las filas {carpeta, pose, {x, y, w, h}, offX, pieAbajo}
           para comparar con la tabla de src/game/visual/Poses_CloneGrinder.cpp.
"""
import os
import sys
from collections import deque

from PIL import Image

RAIZ = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
CARPETAS = ["rayder_clone", "grinder"]
ALFA = 16
MARGEN = 3   # una isla a <= MARGEN px del cuerpo cuenta como cuerpo


def islas(img):
    w, h = img.size
    a = img.getchannel("A").load()
    vis = bytearray(w * h)
    res = []
    for y in range(h):
        for x in range(w):
            if a[x, y] <= ALFA or vis[y * w + x]:
                continue
            q = deque([(x, y)])
            vis[y * w + x] = 1
            x0 = x1 = x
            y0 = y1 = y
            n = 0
            while q:
                cx, cy = q.popleft()
                n += 1
                x0, x1, y0, y1 = min(x0, cx), max(x1, cx), min(y0, cy), max(y1, cy)
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < w and 0 <= ny < h and not vis[ny * w + nx] and a[nx, ny] > ALFA:
                        vis[ny * w + nx] = 1
                        q.append((nx, ny))
            res.append({"area": n, "bbox": (x0, y0, x1 + 1, y1 + 1)})
    res.sort(key=lambda i: -i["area"])
    return res


# pose -> (fila de corte, x maxima de la parte de abajo): la pieza de arriba es
# el cuerpo hasta `fila`; la de abajo, columnas [x0, xmax) desde `fila`.
CORTES = {("rayder_clone", "hurt"): (186, 154), ("rayder_clone", "ready"): (216, 162)}


def cerca(b, c, m):
    return not (b[2] + m < c[0] or c[2] + m < b[0] or b[3] + m < c[1] or c[3] + m < b[1])


def torso_x(img, box):
    a = img.getchannel("A").load()
    x0, y0, x1, y1 = box
    hh = y1 - y0
    xs = []
    for y in range(y0 + int(hh * .35), y0 + int(hh * .60)):
        for x in range(x0, x1):
            if a[x, y] > ALFA:
                xs.append(x)
    xs.sort()
    return xs[len(xs) // 2] if xs else (x0 + x1) // 2


def medir(carpeta, archivo):
    img = Image.open(os.path.join(RAIZ, "assets", "bosses", carpeta, archivo)).convert("RGBA")
    isl = islas(img)
    box = list(isl[0]["bbox"])
    restantes = isl[1:]
    cambiado = True
    while cambiado:
        cambiado = False
        nuevas = []
        for i in restantes:
            b = i["bbox"]
            borde = b[0] == 0 or b[1] == 0 or b[2] == img.width or b[3] == img.height
            if cerca(box, i["bbox"], MARGEN) and i["area"] >= 4 and not (borde and i["area"] > 200):
                b = i["bbox"]
                box = [min(box[0], b[0]), min(box[1], b[1]), max(box[2], b[2]), max(box[3], b[3])]
                cambiado = True
            else:
                nuevas.append(i)
        restantes = nuevas
    fuera = [i for i in restantes if i["area"] >= 3]
    return img, isl, tuple(box), fuera


def main():
    tabla = "--tabla" in sys.argv
    filas = []
    for carpeta in CARPETAS:
        d = os.path.join(RAIZ, "assets", "bosses", carpeta)
        print(f"== {carpeta}")
        for f in sorted(os.listdir(d)):
            if not f.endswith(".png"):
                continue
            img, isl, box, fuera = medir(carpeta, f)
            w, h = img.size
            bb = img.getchannel("A").point(lambda v: 255 if v > ALFA else 0).getbbox()
            tx = torso_x(img, box)
            print(f"{f:16s} {w:4d}x{h:<4d} bbox={bb} islas={len(isl)} cuerpo={box} torsoX={tx}"
                  f" centroSrc={(box[0] + box[2]) / 2:.1f}")
            for i in fuera[:8]:
                print(f"     fuera: area={i['area']:5d} bbox={i['bbox']}")
            corte = CORTES.get((carpeta, f[:-4]))
            if corte:
                a = img.getchannel("A").load()
                resto = sum(1 for y in range(corte[0], box[3]) for x in range(corte[1], box[2]) if a[x, y] > ALFA)
                print(f"     corte: fila {corte[0]}, abajo x < {corte[1]} (quita {resto} px opacos)")
            offx = (box[0] + box[2]) / 2 - tx
            filas.append(f'    {{"{carpeta}", "{f[:-4]}", {{{box[0]}, {box[1]}, {box[2] - box[0]}, {box[3] - box[1]}}}, '
                         f'ancla {{{tx}, {box[3]}}}, offX {offx:.1f}}},')
    if tabla:
        print("\n".join(filas))


if __name__ == "__main__":
    main()
