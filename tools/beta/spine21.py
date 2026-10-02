"""Mini runtime de Spine 2.1 (solo lectura y horneado a cuadros), sin dependencias
mas alla de numpy + PIL. Soporta lo que usan los jefes de nuevosSprites:
huesos (rotate/translate/scale con curvas), slots (attachment, color),
region, mesh y skinnedmesh, ffd (deformaciones) y drawOrder.

Formulas de spine-runtimes 2.1 (Bone.updateWorldTransform, RegionAttachment,
MeshAttachment, SkinnedMeshAttachment). Spine usa y hacia arriba; aqui todo se
devuelve con y hacia abajo (pantalla), con el origen del esqueleto en los pies.
"""
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image


# ------------------------------------------------------------------ atlas

def load_atlas(path):
    pages, regions = {}, {}
    lines = Path(path).read_text(encoding="utf-8").splitlines()
    page = None
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        i += 1
        if not line:
            page = None
            continue
        if page is None:
            page = line
            img = Image.open(Path(path).parent / page).convert("RGBA")
            pages[page] = np.asarray(img, dtype=np.float32) / 255.0
            while i < len(lines) and ":" in lines[i]:
                i += 1
            continue
        name = line
        r = {"page": page}
        while i < len(lines) and ":" in lines[i]:
            k, v = lines[i].split(":", 1)
            r[k.strip()] = v.strip()
            i += 1
        x, y = (int(t) for t in r["xy"].split(","))
        w, h = (int(t) for t in r["size"].split(","))
        ow, oh = (int(t) for t in r["orig"].split(","))
        offx, offy = (int(t) for t in r["offset"].split(","))
        rot = r.get("rotate", "false") == "true"
        ph, pw = pages[page].shape[:2]
        u, v = x / pw, y / ph
        if rot: u2, v2 = (x + h) / pw, (y + w) / ph
        else: u2, v2 = (x + w) / pw, (y + h) / ph
        regions[name] = dict(page=page, u=u, v=v, u2=u2, v2=v2, rotate=rot, w=w, h=h, ow=ow, oh=oh,
                             offx=offx, offy=offy)
    return pages, regions


# ------------------------------------------------------------------ curvas

def curve_percent(curve, p):
    if curve == "stepped": return 0.0
    if isinstance(curve, list):
        cx1, cy1, cx2, cy2 = curve
        # Resolver x(t) = p por biseccion y devolver y(t).
        lo, hi = 0.0, 1.0
        for _ in range(30):
            t = (lo + hi) / 2
            x = 3 * (1 - t) ** 2 * t * cx1 + 3 * (1 - t) * t * t * cx2 + t ** 3
            if x < p: lo = t
            else: hi = t
        t = (lo + hi) / 2
        return 3 * (1 - t) ** 2 * t * cy1 + 3 * (1 - t) * t * t * cy2 + t ** 3
    return p


def sample(keys, time):
    """(clave anterior, clave siguiente, porcentaje) para `time`."""
    if time <= keys[0]["time"]: return keys[0], None, 0.0
    if time >= keys[-1]["time"]: return keys[-1], None, 0.0
    for a, b in zip(keys, keys[1:]):
        if a["time"] <= time < b["time"]:
            p = (time - a["time"]) / max(1e-9, b["time"] - a["time"])
            return a, b, curve_percent(a.get("curve"), p)
    return keys[-1], None, 0.0


def parse_color(s):
    if not s: return np.array([1, 1, 1, 1], np.float32)
    return np.array([int(s[i:i + 2], 16) / 255.0 for i in (0, 2, 4, 6)], np.float32)


# ------------------------------------------------------------------ esqueleto

class Skeleton:
    def __init__(self, json_path, atlas_path):
        self.data = json.loads(Path(json_path).read_text(encoding="utf-8"))
        self.pages, self.regions = load_atlas(atlas_path)
        self.bones = self.data["bones"]
        self.bone_index = {b["name"]: i for i, b in enumerate(self.bones)}
        self.slots = self.data["slots"]
        self.slot_index = {s["name"]: i for i, s in enumerate(self.slots)}
        self.skin = self.data["skins"]["default"]
        self.animations = self.data["animations"]

    def duration(self, anim):
        a = self.animations[anim]
        t = 0.0
        for grp in ("bones", "slots"):
            for tl in a.get(grp, {}).values():
                for keys in tl.values(): t = max(t, max(k["time"] for k in keys))
        for skin in a.get("ffd", {}).values():
            for slot in skin.values():
                for keys in slot.values(): t = max(t, max(k["time"] for k in keys))
        for k in a.get("drawOrder", []) or []: t = max(t, k["time"])
        for k in a.get("events", []) or []: t = max(t, k["time"])
        return t

    def pose(self, anim, time):
        """Lista de triangulos a dibujar: (page, dst[3x2], uv[3x2], color)."""
        a = self.animations[anim]
        # Huesos: pose de reposo + linea de tiempo.
        local = []
        for b in self.bones:
            local.append(dict(x=b.get("x", 0.0), y=b.get("y", 0.0), rot=b.get("rotation", 0.0),
                              sx=b.get("scaleX", 1.0), sy=b.get("scaleY", 1.0)))
        for bname, tls in a.get("bones", {}).items():
            if bname not in self.bone_index: continue
            bi = self.bone_index[bname]
            bd, L = self.bones[bi], local[bi]
            if "rotate" in tls:
                k0, k1, p = sample(tls["rotate"], time)
                ang = k0["angle"]
                if k1 is not None:
                    d = k1["angle"] - k0["angle"]
                    d = (d + 180) % 360 - 180
                    ang = k0["angle"] + d * p
                L["rot"] = bd.get("rotation", 0.0) + ang
            if "translate" in tls:
                k0, k1, p = sample(tls["translate"], time)
                x, y = k0.get("x", 0), k0.get("y", 0)
                if k1 is not None: x += (k1.get("x", 0) - x) * p; y += (k1.get("y", 0) - y) * p
                L["x"], L["y"] = bd.get("x", 0.0) + x, bd.get("y", 0.0) + y
            if "scale" in tls:
                k0, k1, p = sample(tls["scale"], time)
                x, y = k0.get("x", 1), k0.get("y", 1)
                if k1 is not None: x += (k1.get("x", 1) - x) * p; y += (k1.get("y", 1) - y) * p
                L["sx"], L["sy"] = bd.get("scaleX", 1.0) * x, bd.get("scaleY", 1.0) * y
        world = []
        for i, b in enumerate(self.bones):
            L = local[i]
            if "parent" in b:
                P = world[self.bone_index[b["parent"]]]
                wx = L["x"] * P["m00"] + L["y"] * P["m01"] + P["x"]
                wy = L["x"] * P["m10"] + L["y"] * P["m11"] + P["y"]
                wsx = P["sx"] * L["sx"] if b.get("inheritScale", True) else L["sx"]
                wsy = P["sy"] * L["sy"] if b.get("inheritScale", True) else L["sy"]
                wr = P["rot"] + L["rot"] if b.get("inheritRotation", True) else L["rot"]
            else:
                wx, wy, wsx, wsy, wr = L["x"], L["y"], L["sx"], L["sy"], L["rot"]
            r = math.radians(wr)
            c, s = math.cos(r), math.sin(r)
            world.append(dict(x=wx, y=wy, sx=wsx, sy=wsy, rot=wr, m00=c * wsx, m10=s * wsx, m01=-s * wsy, m11=c * wsy))

        self.last_world = world
        # Slots: adjunto y color.
        att = [s.get("attachment") for s in self.slots]
        col = [parse_color(s.get("color")) for s in self.slots]
        for sname, tls in a.get("slots", {}).items():
            if sname not in self.slot_index: continue
            si = self.slot_index[sname]
            if "attachment" in tls:
                keys = tls["attachment"]
                if time >= keys[0]["time"]:
                    k = max((k for k in keys if k["time"] <= time), key=lambda k: k["time"])
                    att[si] = k["name"]
            if "color" in tls:
                k0, k1, p = sample(tls["color"], time)
                c0 = parse_color(k0["color"])
                if k1 is not None: c0 = c0 + (parse_color(k1["color"]) - c0) * p
                col[si] = c0

        # Orden de dibujo.
        order = list(range(len(self.slots)))
        dkeys = a.get("drawOrder") or []
        prev = [k for k in dkeys if k["time"] <= time]
        if prev:
            k = prev[-1]
            offs = k.get("offsets")
            if offs:
                n = len(self.slots)
                draw = [-1] * n
                unchanged = []
                orig = 0
                for o in offs:
                    si = self.slot_index[o["slot"]]
                    while orig != si:
                        unchanged.append(orig); orig += 1
                    draw[orig + o["offset"]] = orig
                    orig += 1
                while orig < n:
                    unchanged.append(orig); orig += 1
                for j in range(n - 1, -1, -1):
                    if draw[j] == -1: draw[j] = unchanged.pop()
                order = draw

        ffd = a.get("ffd", {}).get("default", {})
        tris = []
        for si in order:
            name = att[si]
            if not name: continue
            slot = self.slots[si]
            adata = self.skin.get(slot["name"], {}).get(name)
            if adata is None: continue
            kind = adata.get("type", "region")
            rname = adata.get("path", name)
            reg = self.regions.get(rname)
            if reg is None: continue
            B = world[self.bone_index[slot["bone"]]]
            color = col[si].copy()
            if kind == "region":
                verts, uvs = self._region(adata, reg, B)
                tri_idx = [0, 1, 2, 2, 3, 0]
            elif kind in ("mesh", "skinnedmesh"):
                deform = None
                fk = ffd.get(slot["name"], {}).get(name)
                if fk:
                    deform = self._ffd(fk, time)
                if kind == "mesh": verts = self._mesh(adata, B, deform)
                else: verts = self._skinned(adata, world, deform)
                uvs = self._mesh_uvs(adata, reg)
                tri_idx = adata["triangles"]
                c2 = adata.get("color")
                if c2: color = color * parse_color(c2)
            else:
                continue
            page = reg["page"]
            ph, pw = self.pages[page].shape[:2]
            for t in range(0, len(tri_idx), 3):
                ids = tri_idx[t:t + 3]
                dst = np.array([[verts[k][0], -verts[k][1]] for k in ids], np.float32)
                src = np.array([[uvs[k][0] * pw, uvs[k][1] * ph] for k in ids], np.float32)
                tris.append((page, dst, src, color))
        return tris

    def events(self, anim):
        return [(e["time"], e["name"]) for e in (self.animations[anim].get("events") or [])]

    # -- adjuntos
    def _region(self, a, reg, B):
        w, h = a["width"], a["height"]
        sx, sy = a.get("scaleX", 1.0), a.get("scaleY", 1.0)
        rsx = w / reg["ow"] * sx
        rsy = h / reg["oh"] * sy
        lx = -w / 2 * sx + reg["offx"] * rsx
        ly = -h / 2 * sy + reg["offy"] * rsy
        lx2 = lx + reg["w"] * rsx
        ly2 = ly + reg["h"] * rsy
        r = math.radians(a.get("rotation", 0.0))
        c, s = math.cos(r), math.sin(r)
        x, y = a.get("x", 0.0), a.get("y", 0.0)
        local = [(lx * c - ly * s + x, ly * c + lx * s + y),     # BL
                 (lx * c - ly2 * s + x, ly2 * c + lx * s + y),   # UL
                 (lx2 * c - ly2 * s + x, ly2 * c + lx2 * s + y),  # UR
                 (lx2 * c - ly * s + x, ly * c + lx2 * s + y)]   # BR
        verts = [(px * B["m00"] + py * B["m01"] + B["x"], px * B["m10"] + py * B["m11"] + B["y"]) for px, py in local]
        u, v, u2, v2 = reg["u"], reg["v"], reg["u2"], reg["v2"]
        if reg["rotate"]: uvs = [(u2, v2), (u, v2), (u, v), (u2, v)]
        else: uvs = [(u, v2), (u, v), (u2, v), (u2, v2)]
        return verts, uvs

    def _mesh_uvs(self, a, reg):
        ru = a["uvs"]
        u, v, u2, v2 = reg["u"], reg["v"], reg["u2"], reg["v2"]
        w, h = u2 - u, v2 - v
        out = []
        for i in range(0, len(ru), 2):
            if reg["rotate"]: out.append((u + ru[i + 1] * w, v + h - ru[i] * h))
            else: out.append((u + ru[i] * w, v + ru[i + 1] * h))
        return out

    def _ffd(self, keys, time):
        k0, k1, p = sample(keys, time)

        def vec(k):
            off = k.get("offset", 0)
            vals = k.get("vertices", [])
            return off, vals
        o0, v0 = vec(k0)
        n = o0 + len(v0)
        if k1 is not None:
            o1, v1 = vec(k1)
            n = max(n, o1 + len(v1))
        d = np.zeros(n, np.float32)
        d[o0:o0 + len(v0)] = v0
        if k1 is not None:
            d1 = np.zeros(n, np.float32)
            d1[o1:o1 + len(v1)] = v1
            d = d + (d1 - d) * p
        return d

    def _mesh(self, a, B, deform):
        v = np.array(a["vertices"], np.float32)
        if deform is not None:
            m = min(len(v), len(deform))
            v[:m] += deform[:m]
        out = []
        for i in range(0, len(v), 2):
            x, y = v[i], v[i + 1]
            out.append((x * B["m00"] + y * B["m01"] + B["x"], x * B["m10"] + y * B["m11"] + B["y"]))
        return out

    def _skinned(self, a, world, deform):
        raw = a["vertices"]
        out = []
        i = 0
        f = 0
        while i < len(raw):
            n = int(raw[i]); i += 1
            wx = wy = 0.0
            for _ in range(n):
                bi = int(raw[i]); vx = raw[i + 1]; vy = raw[i + 2]; w = raw[i + 3]; i += 4
                if deform is not None and f + 1 < len(deform):
                    vx += deform[f]; vy += deform[f + 1]
                f += 2
                B = world[bi]
                wx += (vx * B["m00"] + vy * B["m01"] + B["x"]) * w
                wy += (vx * B["m10"] + vy * B["m11"] + B["y"]) * w
            out.append((wx, wy))
        return out


# ------------------------------------------------------------------ rasterizado

def render(skel, tris, pad=2):
    """Dibuja los triangulos (bilineal, alfa normal). Devuelve (imagen RGBA, ox, oy)
    con ox/oy = posicion del origen (pies) dentro de la imagen."""
    if not tris: return None, 0, 0
    allp = np.concatenate([t[1] for t in tris])
    x0, y0 = np.floor(allp.min(0)) - pad
    x1, y1 = np.ceil(allp.max(0)) + pad
    W, H = int(x1 - x0), int(y1 - y0)
    canvas = np.zeros((H, W, 4), np.float32)
    for page, dst, src, color in tris:
        tex = skel.pages[page]
        d = dst - np.array([x0, y0], np.float32)
        bx0, by0 = np.floor(d.min(0)).astype(int)
        bx1, by1 = np.ceil(d.max(0)).astype(int)
        bx0, by0 = max(bx0, 0), max(by0, 0)
        bx1, by1 = min(bx1, W), min(by1, H)
        if bx1 <= bx0 or by1 <= by0: continue
        (ax, ay), (bxp, byp), (cx, cy) = d
        den = (byp - cy) * (ax - cx) + (cx - bxp) * (ay - cy)
        if abs(den) < 1e-6: continue
        gy, gx = np.mgrid[by0:by1, bx0:bx1].astype(np.float32)
        px, py = gx + 0.5, gy + 0.5
        l1 = ((byp - cy) * (px - cx) + (cx - bxp) * (py - cy)) / den
        l2 = ((cy - ay) * (px - cx) + (ax - cx) * (py - cy)) / den
        l3 = 1.0 - l1 - l2
        eps = -1e-4
        inside = (l1 >= eps) & (l2 >= eps) & (l3 >= eps)
        if not inside.any(): continue
        su = l1 * src[0, 0] + l2 * src[1, 0] + l3 * src[2, 0] - 0.5
        sv = l1 * src[0, 1] + l2 * src[1, 1] + l3 * src[2, 1] - 0.5
        th, tw = tex.shape[:2]
        su = np.clip(su, 0, tw - 1.001); sv = np.clip(sv, 0, th - 1.001)
        iu, iv = su.astype(int), sv.astype(int)
        fu, fv = (su - iu)[..., None], (sv - iv)[..., None]
        c00 = tex[iv, iu]; c10 = tex[iv, iu + 1]; c01 = tex[iv + 1, iu]; c11 = tex[iv + 1, iu + 1]
        # bilineal con alfa premultiplicado (evita halos oscuros en los bordes)
        def pm(c): return np.concatenate([c[..., :3] * c[..., 3:4], c[..., 3:4]], -1)
        s = pm(c00) * (1 - fu) * (1 - fv) + pm(c10) * fu * (1 - fv) + pm(c01) * (1 - fu) * fv + pm(c11) * fu * fv
        s = s * np.concatenate([color[:3] * color[3], color[3:4]])
        s[~inside] = 0
        region = canvas[by0:by1, bx0:bx1]
        region[:] = s + region * (1 - s[..., 3:4])
    a = canvas[..., 3:4]
    rgb = np.where(a > 1e-6, canvas[..., :3] / np.maximum(a, 1e-6), 0)
    img = np.concatenate([rgb, a], -1)
    out = Image.fromarray((np.clip(img, 0, 1) * 255 + 0.5).astype(np.uint8), "RGBA")
    return out, -x0, -y0
