#!/usr/bin/env python3
"""Mide las poses sueltas de TITAN-X y TITAN-X MEJORADO (BRIEF F1.1, tarea 1).

Uso (desde la raiz del repo):  python3 tools/poses/medir_titanx.py [--cpp]

Para cada PNG de assets/bosses/titanx/ y assets/bosses/titanx_mejorado/:
- tamano, caja opaca (alfa > 20) del cuerpo (la componente conexa mayor) y de los
  restos sueltos (componentes de >= 100 px que no son el cuerpo: puno suelto de
  punch1, trozo de puno de slam, trozo de rage_aura...);
- `pie`: margen bajo el pie (filas transparentes al fondo del lienzo);
- `pie_tras` / `pie_del`: x del pie trasero / delantero (franja del 6 % inferior),
  con la pose vista mirando a la DERECHA (las nativas a la izquierda se espejan);
- `torso`: x del centro del torso = mediana de x de los pixeles del cuerpo en la
  franja 40-62 % de su alto (en coordenadas de la textura SIN espejar);
- `alto/idle1`: alto opaco relativo al reposo (criterio <= 1.15).
Con --cpp imprime ademas la tabla kAnchors que usa src/game/visual/Poses_TitanX.cpp.
Necesita PIL (si el python3 del PATH no lo trae, usar /usr/bin/python3).
"""
import os
import sys
from collections import deque

from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
FOLDERS = ["titanx", "titanx_mejorado"]
# Orientacion nativa (verificada a ojo): todas miran a la derecha salvo estas.
LEFT = {("titanx", "recoil"), ("titanx_mejorado", "shoot_orb")}


def components(img):
    w, h = img.size
    a = img.split()[3].load()
    seen = bytearray(w * h)
    out = []
    for x in range(w):
        for y in range(h):
            if a[x, y] <= 20 or seen[y * w + x]:
                continue
            q = deque([(x, y)])
            seen[y * w + x] = 1
            px = []
            while q:
                cx, cy = q.popleft()
                px.append((cx, cy))
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < w and 0 <= ny < h and not seen[ny * w + nx] and a[nx, ny] > 20:
                        seen[ny * w + nx] = 1
                        q.append((nx, ny))
            out.append(px)
    out.sort(key=len, reverse=True)
    return out


def bbox(px):
    xs = [p[0] for p in px]
    ys = [p[1] for p in px]
    return min(xs), min(ys), max(xs), max(ys)


def measure(folder, pose):
    path = os.path.join(ROOT, "assets", "bosses", folder, pose + ".png")
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    comps = components(img)
    body = comps[0]
    x0, y0, x1, y1 = bbox(body)
    bh = y1 - y0 + 1
    band = sorted(p[0] for p in body if y0 + 0.40 * bh <= p[1] <= y0 + 0.62 * bh)
    torso = band[len(band) // 2]
    feet = [p[0] for p in body if p[1] >= y1 - 0.06 * bh]
    left = (folder, pose) in LEFT
    rear = max(feet) if left else min(feet)
    front = min(feet) if left else max(feet)
    strays = [bbox(c) + (len(c),) for c in comps[1:] if len(c) >= 100]
    return dict(pose=pose, w=w, h=h, body=(x0, y0, x1, y1), pad=h - 1 - y1, torso=torso, rear=rear,
                front=front, left=left, strays=strays, height=bh)


def main():
    cpp = "--cpp" in sys.argv
    rows = []
    for folder in FOLDERS:
        poses = sorted(f[:-4] for f in os.listdir(os.path.join(ROOT, "assets", "bosses", folder)) if f.endswith(".png"))
        ms = [measure(folder, p) for p in poses]
        idle = next(m for m in ms if m["pose"] == "idle1")
        print(f"== {folder} (idle1 alto opaco {idle['height']})")
        print("pose        tam       cuerpo(x0,y0,x1,y1)   pie torso pie_tras pie_del alto/idle1 mira  restos")
        for m in ms:
            rel = m["height"] / idle["height"]
            print(f"{m['pose']:<11} {m['w']:>3}x{m['h']:<4} {str(m['body']):<21} {m['pad']:>3} {m['torso']:>5} "
                  f"{m['rear']:>8} {m['front']:>7} {rel:>10.3f}  {'izq' if m['left'] else 'der'}  {m['strays']}")
            rows.append((folder, m))
    if cpp:
        print("\n// folder, pose, ancho, alto, torsoX, margenPie, miraDerecha")
        for folder, m in rows:
            print(f'    {{"{folder}", "{m["pose"]}", {m["w"]}, {m["h"]}, {m["torso"]}, {m["pad"]}, '
                  f'{"false" if m["left"] else "true"}}},')


if __name__ == "__main__":
    main()
