#!/usr/bin/env python3
"""Limpia recortes de atlas con grilla (enemigos):
  1. Fondo atrapado: manchas claras NEUTRAS (gris/blanco/azulado, sin tinte de
     piel ni de sangre) de >= 8 px -> transparentes, mas el anillo claro que
     las rodea. Se conservan si estan rodeadas de color de efecto saturado
     (cortes de energia, gas), que tienen nucleo blanco legitimo.
  2. Astillas: piezas sueltas de la celda vecina (tocan el borde de la celda o
     estan lejos del cuerpo y son pequenas) -> transparentes.
Uso: python3 tools/clean_sprite_cutouts.py [--dry-run]
"""
import sys
from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ATLASES = {f"assets/enemies/{n}_clean.png": (4, 3) for n in
           ["punk", "charger", "brute", "enforcer", "chemical_soldier", "urban_ninja", "mutant", "armored_guard"]}


def leftover(p):
    r, g, b, a = p
    return a > 120 and min(r, g, b) >= 160 and max(r, g, b) - min(r, g, b) <= 40 and b >= r - 6


def effect_color(p):
    r, g, b, a = p
    return a > 60 and max(r, g, b) - min(r, g, b) >= 70 and (b > r + 40 or g > r + 40)


def comps(px, box, pred, eight=True):
    x0, y0, x1, y1 = box
    seen, out = set(), []
    nb = [(dx, dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if (dx or dy)] if eight else [(1, 0), (-1, 0), (0, 1), (0, -1)]
    for y in range(y0, y1):
        for x in range(x0, x1):
            if (x, y) in seen or not pred(px[x, y]): continue
            q, pts = deque([(x, y)]), []
            seen.add((x, y))
            while q:
                a, b = q.popleft(); pts.append((a, b))
                for dx, dy in nb:
                    n = (a + dx, b + dy)
                    if x0 <= n[0] < x1 and y0 <= n[1] < y1 and n not in seen and pred(px[n]):
                        seen.add(n); q.append(n)
            out.append(pts)
    return out


def ring(pts, W, H):
    s = set(pts); out = set()
    for x, y in pts:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                n = (x + dx, y + dy)
                if n not in s and 0 <= n[0] < W and 0 <= n[1] < H: out.add(n)
    return out


# Solo astillas (sin quitar zonas claras): hojas de personaje generadas.
SLIVERS_ONLY = {"assets/characters/rayden_128.png": (4, 6), "assets/characters/rayder/rayder_atlas.png": (10, 11)}


def clean(path, cols, rows, dry, gaps_pass=True):
    im = Image.open(path).convert("RGBA"); px = im.load(); W, H = im.size
    gaps = slivers = removed = 0
    # 1. fondo atrapado
    for c in (comps(px, (0, 0, W, H), leftover) if gaps_pass else []):
        if len(c) < 8: continue
        border = ring(c, W, H)
        fx = sum(1 for p in border if effect_color(px[p]))
        if fx > len(border) * 0.25: continue
        gaps += 1
        kill = set(c) | {p for p in border if px[p][3] > 0 and sum(px[p][:3]) / 3 >= 150}
        for p in kill: px[p] = (0, 0, 0, 0)
        removed += len(kill)
    # 2. astillas por celda
    cw, ch = W // cols, H // rows
    for r in range(rows):
        for cc in range(cols):
            box = (cc * cw, r * ch, (cc + 1) * cw, (r + 1) * ch)
            parts = comps(px, box, lambda p: p[3] > 0)
            if not parts: continue
            main = max(parts, key=len)
            mx0 = min(p[0] for p in main); mx1 = max(p[0] for p in main)
            my0 = min(p[1] for p in main); my1 = max(p[1] for p in main)
            for part in parts:
                if part is main: continue
                xs = [p[0] for p in part]; ys = [p[1] for p in part]
                edge = min(xs) == box[0] or max(xs) == box[2] - 1 or min(ys) == box[1] or max(ys) == box[3] - 1
                far = min(xs) > mx1 + 6 or max(xs) < mx0 - 6 or min(ys) > my1 + 3 or max(ys) < my0 - 6
                small = len(part) < len(main) * 0.12
                # Las piezas de efecto (saturadas o brillantes) solo se quitan si
                # tocan el borde de la celda: suelen ser parte legitima del golpe.
                fx = sum(1 for p in part if effect_color(px[p]) or sum(px[p][:3]) / 3 > 170) > len(part) * 0.4
                if (edge and small) or (far and small and not fx):
                    slivers += 1; removed += len(part)
                    for p in part: px[p] = (0, 0, 0, 0)
    if not dry: im.save(path)
    print(f"{'(simulado) ' if dry else ''}{path.relative_to(ROOT)}: {gaps} fondos atrapados, {slivers} astillas, {removed} px")


def main():
    dry = "--dry-run" in sys.argv
    for rel, (c, r) in ATLASES.items():
        clean(ROOT / rel, c, r, dry)
    for rel, (c, r) in SLIVERS_ONLY.items():
        clean(ROOT / rel, c, r, dry, gaps_pass=False)


if __name__ == "__main__":
    main()
