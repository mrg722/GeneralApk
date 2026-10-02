#!/usr/bin/env python3
"""Vista previa de un personaje BETA desde data/beta/characters/<id>.txt (sin el juego).
Uso: python3 tools/beta/preview_beta.py <id> <salida.png> [clip ...]"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]


def load(cid):
    d = {"images": {}, "frames": [], "clips": {}}
    cur = None
    for line in (ROOT / "data/beta/characters" / f"{cid}.txt").read_text(encoding="utf-8").splitlines():
        t = line.split()
        if not t or t[0].startswith("#"): continue
        if t[0] == "image": d["images"][int(t[1])] = int(t[2])
        elif t[0] == "frame":
            cur = {"pieces": [], "body": list(map(int, t[3:7])), "attack": list(map(int, t[7:11]))}; d["frames"].append(cur)
        elif t[0] == "piece": cur["pieces"].append(list(map(int, t[1:])))
        elif t[0] == "clip": d["clips"][t[1]] = [(int(x.split(":")[0]), float(x.split(":")[1])) for x in t[4:]]
    return d


def render(d, fid, cache):
    fr = d["frames"][fid]
    out = Image.new("RGBA", (900, 700), (0, 0, 0, 0)); ox, oy = 450, 600
    for gi, sx, sy, sw, sh, x, y, flip in fr["pieces"]:
        aid = d["images"][gi]
        if aid not in cache: cache[aid] = Image.open(ROOT / "assets/beta/actor" / f"{aid}.png").convert("RGBA")
        p = cache[aid].crop((sx, sy, sx + sw, sy + sh))
        if flip & 1: p = p.transpose(Image.FLIP_LEFT_RIGHT)
        if flip & 2: p = p.transpose(Image.FLIP_TOP_BOTTOM)
        out.alpha_composite(p, (ox + x, oy + y))
    dr = ImageDraw.Draw(out)
    if any(fr["attack"]): a = fr["attack"]; dr.rectangle((ox + a[0], oy + a[1], ox + a[2], oy + a[3]), outline=(255, 40, 40, 255), width=2)
    b = fr["body"]; dr.rectangle((ox + b[0], oy + b[1], ox + b[2], oy + b[3]), outline=(60, 200, 255, 160))
    return out


def main():
    cid, out = sys.argv[1], sys.argv[2]
    clips = sys.argv[3:] or ["idle", "walk", "punch1", "punch2", "punch3", "kick", "ab0", "ab1", "ab2", "hit", "defeat"]
    d = load(cid); cache = {}
    rows = []
    for c in clips:
        if c not in d["clips"]: continue
        seq = d["clips"][c]; step = max(1, len(seq) // 8)
        tiles = [render(d, f, cache) for f, _ in seq[::step][:8]]
        row = Image.new("RGBA", (8 * 225, 175), (40, 44, 56, 255)); dr = ImageDraw.Draw(row); dr.text((2, 2), c, fill="yellow")
        for i, t in enumerate(tiles): t = t.resize((225, 175)); row.alpha_composite(t, (i * 225, 0))
        rows.append(row)
    sheet = Image.new("RGBA", (8 * 225, 175 * len(rows)))
    for i, r in enumerate(rows): sheet.paste(r, (0, i * 175))
    sheet.save(out)


if __name__ == "__main__":
    main()
