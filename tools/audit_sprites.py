#!/usr/bin/env python3
"""Auditoria de recorte de sprites (personajes, enemigos, jefes, NPC).

Por cada PNG informa:
  alfa       : si tiene canal alfa real (sin alfa = fondo pegado)
  borde_op   : % de pixeles opacos en el marco exterior (fondo sin quitar)
  halo       : % de pixeles de contorno claros (matte blanco en el borde)
  huecos     : manchas casi blancas pegadas a zona transparente (recorte malo)
  astillas   : piezas sueltas que tocan el borde de celda (restos del vecino)
Uso: python3 tools/audit_sprites.py [--json]
"""
import json, sys
from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GRIDS = {  # atlas con grilla: ruta -> (cols, rows)
    "assets/characters/rayden_clean.png": (4, 4),
    "assets/characters/rayden_128.png": (4, 6),
    "assets/characters/rayder/rayder_atlas.png": (10, 5),
}
for n in ["punk", "charger", "brute", "enforcer", "chemical_soldier", "urban_ninja", "mutant", "armored_guard"]:
    GRIDS[f"assets/enemies/{n}_clean.png"] = (4, 3)


def lum(p): return (p[0] + p[1] + p[2]) / 3


def near_white(p): return p[3] > 0 and min(p[:3]) >= 200 and max(p[:3]) - min(p[:3]) <= 30


def components(px, box, pred):
    x0, y0, x1, y1 = box
    seen, comps = set(), []
    for y in range(y0, y1):
        for x in range(x0, x1):
            if (x, y) in seen or not pred(px[x, y]): continue
            q, pts = deque([(x, y)]), []
            seen.add((x, y))
            while q:
                a, b = q.popleft(); pts.append((a, b))
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        n = (a + dx, b + dy)
                        if x0 <= n[0] < x1 and y0 <= n[1] < y1 and n not in seen and pred(px[n]):
                            seen.add(n); q.append(n)
            comps.append(pts)
    return comps


def touches_transparent(px, pts, W, H):
    for x, y in pts:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if 0 <= n[0] < W and 0 <= n[1] < H and px[n][3] == 0: return True
    return False


def audit(path):
    im = Image.open(path)
    has_alpha = im.mode in ("RGBA", "LA") or "transparency" in im.info
    im = im.convert("RGBA"); W, H = im.size; px = im.load()
    frame = [(x, 0) for x in range(W)] + [(x, H - 1) for x in range(W)] + [(0, y) for y in range(H)] + [(W - 1, y) for y in range(H)]
    border_op = sum(1 for p in frame if px[p][3] > 200) / len(frame) * 100
    edge = bright = 0
    for y in range(1, H - 1):
        for x in range(1, W - 1):
            p = px[x, y]
            if p[3] == 0: continue
            if any(px[x + dx, y + dy][3] == 0 for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                edge += 1
                if lum(p) >= 175: bright += 1
    halo = bright / edge * 100 if edge else 0
    holes = [c for c in components(px, (0, 0, W, H), lambda p: near_white(p) and p[3] > 120) if len(c) >= 6 and touches_transparent(px, c, W, H)]
    slivers = 0
    rel = str(path.relative_to(ROOT))
    if rel in GRIDS:
        cols, rows = GRIDS[rel]; cw, ch = W // cols, H // rows
        for r in range(rows):
            for c in range(cols):
                box = (c * cw, r * ch, (c + 1) * cw, (r + 1) * ch)
                comps = components(px, box, lambda p: p[3] > 0)
                if not comps: continue
                main = max(comps, key=len)
                for comp in comps:
                    if comp is main or len(comp) < 3: continue
                    xs = [p[0] for p in comp]; ys = [p[1] for p in comp]
                    if min(xs) == box[0] or max(xs) == box[2] - 1 or min(ys) == box[1] or max(ys) == box[3] - 1: slivers += 1
    return {"file": rel, "alfa": has_alpha, "borde_op": round(border_op, 1), "halo": round(halo, 1),
            "huecos": len(holes), "px_huecos": sum(len(h) for h in holes), "astillas": slivers}


def main():
    files = sorted(p for d in ["assets/characters", "assets/enemies", "assets/bosses", "assets/npc"] for p in (ROOT / d).rglob("*.png") if "source" not in p.parts)
    rows = [audit(p) for p in files]
    if "--json" in sys.argv: print(json.dumps(rows, indent=1)); return
    bad = 0
    for r in rows:
        issues = []
        if not r["alfa"]: issues.append("SIN ALFA")
        if r["borde_op"] > 5: issues.append(f"fondo en borde {r['borde_op']}%")
        if r["halo"] > 12: issues.append(f"halo claro {r['halo']}%")
        if r["huecos"]: issues.append(f"{r['huecos']} manchas blancas ({r['px_huecos']} px)")
        if r["astillas"]: issues.append(f"{r['astillas']} astillas de celda vecina")
        bad += bool(issues)
        print(f"{'FALLA' if issues else 'ok   '} {r['file']}: {', '.join(issues)}")
    print(f"\n{bad} de {len(rows)} archivos con problemas")


if __name__ == "__main__":
    main()
