"""Lector del formato de animacion de nuevosSprites (anim/N.xml, editor "乐堂").

Estructura (igual idea que la formula KF):
  ImageNameList  -> imagenes "[ID]nombre.png" = actor/ID.png
  ModuleList     -> recortes (imageId, x, y, w, h) dentro de esas imagenes
  Frames         -> cuadros: piezas <Sprite module_id x y flip>, caja de cuerpo
                    (cx0,cy0)-(cx1,cy1) y caja de ataque (ax0,ay0)-(ax1,ay1),
                    coordenadas relativas a los pies (0,0), y negativa hacia arriba
  Actions        -> acciones: <Sequence frameid duration flag> + MechModel
"""
import re
from pathlib import Path
from PIL import Image

FLIPS = {0: (False, False), 1: (True, False), 2: (False, True), 3: (True, True)}


def read_text(path):
    b = Path(path).read_bytes()
    if b[:3] == b"\xef\xbb\xbf": b = b[3:]
    try: return b.decode("utf-8")
    except UnicodeDecodeError: return b.decode("gb18030", "replace")


def attrs(s):
    return {k: v for k, v in re.findall(r'(\w+)="([^"]*)"', s)}


def parse(path):
    t = read_text(path)
    a = {"name": re.search(r'FileType="([^"]*)"', t).group(1), "images": [], "modules": [], "frames": [], "actions": []}
    for fn in re.findall(r'<filename name="([^"]*)"', t):
        base = fn.split("\\")[-1]
        m = re.match(r"\[(\d+)\](.*)\.png", base)
        a["images"].append({"id": int(m.group(1)), "name": m.group(2)} if m else {"id": -1, "name": base})
    for m in re.findall(r"<Module ([^/]*)/>", t):
        d = attrs(m); a["modules"].append({k: int(d[k]) for k in ("imageId", "x", "y", "w", "h")})
    for fm in re.finditer(r"<Frame ([^>]*)>(.*?)</Frame>", t, re.S):
        d = attrs(fm.group(1))
        sprites = [{k: int(v) for k, v in attrs(s).items()} for s in re.findall(r"<Sprite ([^/]*)/>", fm.group(2))]
        a["frames"].append({"name": d.get("name", ""), "sprites": sprites,
                            "body": [int(d.get(k, 0)) for k in ("cx0", "cy0", "cx1", "cy1")],
                            "attack": [int(d.get(k, 0)) for k in ("ax0", "ay0", "ax1", "ay1")]})
    for am in re.finditer(r'<Action name="([^"]*)">(.*?)</Action>', t, re.S):
        seq = [{k: int(v) for k, v in attrs(s).items()} for s in re.findall(r"<Sequence ([^/]*)/>", am.group(2))]
        mech = re.search(r"<MechModel ([^/]*)/>", am.group(2))
        a["actions"].append({"name": am.group(1), "seq": seq, "mech": {k: int(v) for k, v in attrs(mech.group(1)).items()} if mech else {}})
    return a


class Composer:
    """Arma cuadros con las piezas originales (sin escalar ni deformar)."""
    def __init__(self, actor_dir, anim):
        self.anim = anim
        self.imgs = {}
        for i, im in enumerate(anim["images"]):
            p = Path(actor_dir) / f'{im["id"]}.png'
            self.imgs[i] = Image.open(p).convert("RGBA") if p.exists() else None

    def piece(self, mod_id, flip):
        m = self.anim["modules"][mod_id]
        src = self.imgs.get(m["imageId"])
        if src is None or m["w"] <= 0 or m["h"] <= 0: return None
        p = src.crop((m["x"], m["y"], m["x"] + m["w"], m["y"] + m["h"]))
        fx, fy = FLIPS.get(flip & 3, (False, False))
        if fx: p = p.transpose(Image.FLIP_LEFT_RIGHT)
        if fy: p = p.transpose(Image.FLIP_TOP_BOTTOM)
        return p

    def frame(self, fid):
        """Devuelve (imagen, ox, oy): ox,oy = posicion de los pies dentro de la imagen."""
        fr = self.anim["frames"][fid]
        parts = []
        for s in fr["sprites"]:
            if s.get("module_id", -1) < 0 or s["module_id"] >= len(self.anim["modules"]): continue
            p = self.piece(s["module_id"], s.get("flip", 0))
            if p is not None: parts.append((p, s["x"], s["y"]))
        if not parts: return None, 0, 0
        x0 = min(x for _, x, _ in parts); y0 = min(y for _, _, y in parts)
        x1 = max(x + p.width for p, x, _ in parts); y1 = max(y + p.height for p, _, y in parts)
        img = Image.new("RGBA", (x1 - x0, y1 - y0), (0, 0, 0, 0))
        for p, x, y in parts: img.alpha_composite(p, (x - x0, y - y0))
        return img, -x0, -y0
