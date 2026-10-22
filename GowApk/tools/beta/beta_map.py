"""Lector de niveles de nuevosSprites (mapdata/N.XML + tilesets map/N.png).

Cada <Layer file="...[K]nombre.png" tilew="48" tileh="48" tilew_t=ancho_en_tiles
tileh_t=alto_en_tiles> trae los tiles separados por comas: -1 vacio; el resto es
el indice en el tileset map/K.png (48x48) con espejo en los bits altos:
bit 30 = espejo horizontal, bit 29 = espejo vertical. La capa [4]碰撞 es la de
colision (no se dibuja)."""
import re
from pathlib import Path
from PIL import Image

FLIP_X, FLIP_Y = 1 << 30, 1 << 29


def read_text(path):
    b = Path(path).read_bytes()
    if b[:3] == b"\xef\xbb\xbf": b = b[3:]
    try: return b.decode("utf-8")
    except UnicodeDecodeError: return b.decode("gb18030", "replace")


def parse_level(path):
    t = read_text(path)
    layers = []
    for m in re.finditer(r"<Layer ([^>]*)>\s*<Data>([^<]*)</Data>", t):
        at = dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))
        name = at["file"].split("\\")[-1]
        k = int(re.match(r"\[(\d+)\]", name).group(1))
        vals = [int(v) for v in m.group(2).replace("\n", "").replace("\r", "").split(",") if v.strip() != ""]
        layers.append({"tileset": k, "name": name, "tw": int(at["tilew"]), "th": int(at["tileh"]),
                       "cols": int(at["tilew_t"]), "rows": int(at["tileh_t"]), "data": vals})
    return layers


def render_layer(layer, tileset_dir, cache):
    k = layer["tileset"]
    if k not in cache: cache[k] = Image.open(Path(tileset_dir) / f"{k}.png").convert("RGBA")
    ts = cache[k]
    tw, th = layer["tw"], layer["th"]
    per_row = ts.width // tw
    out = Image.new("RGBA", (layer["cols"] * tw, layer["rows"] * th), (0, 0, 0, 0))
    for i, v in enumerate(layer["data"]):
        if v < 0: continue
        idx = v & ~(FLIP_X | FLIP_Y | (1 << 31))
        sx, sy = (idx % per_row) * tw, (idx // per_row) * th
        if sy + th > ts.height: continue
        tile = ts.crop((sx, sy, sx + tw, sy + th))
        if v & FLIP_X: tile = tile.transpose(Image.FLIP_LEFT_RIGHT)
        if v & FLIP_Y: tile = tile.transpose(Image.FLIP_TOP_BOTTOM)
        out.alpha_composite(tile, ((i % layer["cols"]) * tw, (i // layer["cols"]) * th))
    return out
