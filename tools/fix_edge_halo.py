#!/usr/bin/env python3
"""Quita el halo claro que rodea la silueta de los sprites (resto del fondo
blanco original mezclado en el borde). Solo actua en los 2 pixeles junto a la
zona transparente y solo sobre pixeles claros y poco saturados (lum >= 80,
saturacion < 40): los reemplaza por el color medio del interior cercano, o los
oscurece si no hay interior al lado. Se aplica si el anillo interior tiene
claramente mas pixeles claros que la capa siguiente (halo real, no ropa blanca).
Uso: python3 tools/fix_edge_halo.py [--dry-run]
"""
import sys
from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
FILES = [f"assets/enemies/{n}_clean.png" for n in
         ["punk", "charger", "brute", "enforcer", "chemical_soldier", "urban_ninja", "mutant", "armored_guard"]]
FILES += ["assets/characters/rayden_128.png", "assets/characters/rayder/rayder_atlas.png"]


def light(p): return p[3] > 0 and sum(p[:3]) / 3 >= 110 and max(p[:3]) - min(p[:3]) < 45


def fix(path, dry):
    im = Image.open(path).convert("RGBA"); px = im.load(); W, H = im.size
    dist = {}; q = deque()
    for y in range(H):
        for x in range(W):
            if px[x, y][3] == 0: dist[(x, y)] = 0; q.append((x, y))
    while q:
        x, y = q.popleft(); d = dist[(x, y)]
        if d >= 4: continue
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if 0 <= n[0] < W and 0 <= n[1] < H and n not in dist: dist[n] = d + 1; q.append(n)
    ring = [k for k, v in dist.items() if v in (1, 2)]
    inner = [k for k, v in dist.items() if v == 3]
    ring2 = [k for k, v in dist.items() if v == 2]
    r_ratio = sum(light(px[k]) for k in ring2) / max(1, len(ring2))
    i_ratio = sum(light(px[k]) for k in inner) / max(1, len(inner))
    if ("--force" not in sys.argv or "characters" in str(path)) and (r_ratio < i_ratio * 1.5 or r_ratio < 0.04):
        print(f"{path}: sin halo (anillo {r_ratio:.3f} vs interior {i_ratio:.3f})"); return
    changed = 0
    rim = lambda p: p[3] > 0 and sum(p[:3]) / 3 >= 80 and max(p[:3]) - min(p[:3]) < 40
    for (x, y) in ring:
        p = px[x, y]
        if not rim(p): continue
        acc = [0, 0, 0]; n = 0
        for dx in range(-2, 3):
            for dy in range(-2, 3):
                k = (x + dx, y + dy)
                if 0 <= k[0] < W and 0 <= k[1] < H and dist.get(k, 99) >= 3:
                    c = px[k]; acc[0] += c[0]; acc[1] += c[1]; acc[2] += c[2]; n += 1
        new = tuple(a // n for a in acc) if n else (p[0] * 2 // 5, p[1] * 2 // 5, p[2] * 2 // 5)
        px[x, y] = (*new, p[3]); changed += 1
    if not dry: im.save(path)
    print(f"{path}: halo corregido en {changed} px (anillo {r_ratio:.3f} vs interior {i_ratio:.3f})")


if __name__ == "__main__":
    for f in FILES: fix(ROOT / f, "--dry-run" in sys.argv)
