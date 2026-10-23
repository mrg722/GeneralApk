#!/usr/bin/env python3
"""RONIN-KRATOS: Kratos completo (cuadros, clips, habilidades, 4 armas, tiempos,
cajas y sonidos) con el cuerpo de RONIN.  Lee SIEMPRE las hojas actuales de
Kratos (data/kratos/kratos_*.txt + assets/kratos/actor/*.png): si Kratos cambia
(p.ej. el Cestus), se vuelve a correr y listo.

Como Kratos es un render 3D por cuadros (cada pieza "body" es su cuerpo entero
o un trozo grande, no un brazo suelto), el cambio de cuerpo es por PIXEL dentro
de cada pieza body, con el cuadro completo como contexto:
  1. Se arma el cuadro (solo piezas body) y se segmenta por material + eje del
     cuerpo: torso/brazos (piel palida + tatuaje), guanteletes, manos, cabeza,
     cinto, falda + muslos + botas (-> pantalon), pies.
  2. Cada region se repinta con la rampa de color REAL de RONIN (medida en sus
     imagenes, rk_ronin_parts.py) siguiendo la luz/sombra de Kratos pixel a
     pixel: polera ROJO CARMESI (pedido: RONIN-KRATOS va pintado de rojo),
     pantalon blanco calido, cinto dorado, piel, pies descalzos.
  3. La cabeza calva se tapa con la cabeza de RONIN (pelo blanco puntiagudo +
     cara, recortada de sus cuadros de combate), ajustada por la cara; el pelo
     sobresale: por eso cada pieza body se guarda con margen PAD y su `piece`
     se corrige (dx-PAD, dy-PAD, sw+2PAD, sh+2PAD; simetrico, vale con espejos y giro).
  4. Brazos/torso ~0.9 de ancho: se quita 1 px del contorno exterior de la polera
     donde la parte es gruesa.
  5. Armas, cadenas y efectos: intactos (siguen leyendo las hojas de Kratos).
Salida: assets/kratos/actor/95xx.png + data/kratos/ronin_kratos_<arma>.txt.
Ajustes manuales: tools/ronin_kratos/exceptions.json.
"""
import argparse, json, math, sys
from collections import deque
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from rk_common import ROOT, DATA, ACTOR, WEAPONS, CharFile, sheet, orient, hsv  # noqa: E402
from rk_inventory import inventory, pkey  # noqa: E402
import rk_ronin_parts  # noqa: E402
import rk_head  # noqa: E402

PAD = 12
HEAD_K = 1.30          # ancho de la cara de RONIN / ancho de la cabeza de Kratos
HEAD_OFF = (0.0, 0.05)  # desplazamiento de la cara (en anchos/altos de cabeza)
DEBUG = False
FIRST_ID = 9500
SHEET_W = 1024
LUMW = np.array([0.299, 0.587, 0.114], np.float32)
# etiquetas
BG, SHIRT, HAND, HEAD, SASH, PANTS, FOOT, KEEP = range(8)
LABEL_RGB = {BG: (40, 40, 40), SHIRT: (200, 30, 40), HAND: (250, 190, 140), HEAD: (255, 255, 0), SASH: (255, 170, 0),
             PANTS: (235, 235, 235), FOOT: (140, 80, 40), KEEP: (0, 160, 255)}


def load_exc():
    p = HERE / "exceptions.json"
    return json.loads(p.read_text()) if p.exists() else {}


# ---------------------------------------------------------------- materiales
def materials(px):
    h, s, v = hsv(px)
    a = px[..., 3] > 50
    skin = a & (s < 0.36) & (v > 0.22)
    red = (h < 16) | (h > 340)
    warm = a & ~skin & (s >= 0.36) & ((h < 55) | (h > 330))
    nb = ndimage.uniform_filter(skin.astype(np.float32), 5)
    tattoo = warm & red & (v > 0.45) & (s > 0.45) & (nb > 0.25)
    fire = a & (h >= 22) & (h < 70) & (s > 0.3) & (v > 0.88)
    purple = a & (h > 245) & (h < 335) & (s > 0.28) & (v > 0.3)
    keep = fire | purple
    # solo manchas grandes de efecto (los brillos dorados de botas/guanteletes no)
    kl, kn = ndimage.label(keep, structure=np.ones((3, 3)))
    if kn:
        sizes = ndimage.sum(np.ones_like(kl), kl, range(1, kn + 1))
        keep = np.isin(kl, 1 + np.nonzero(sizes >= 14)[0])
    skin = (skin | tattoo) & ~keep
    warm = warm & ~tattoo & ~keep
    dark = a & ~skin & ~warm & ~keep
    return dict(a=a, skin=skin, tattoo=tattoo, warm=warm, dark=dark, keep=keep, h=h, s=s, v=v)


# ---------------------------------------------------------------- composicion
def compose_body(cf, fi, body_keys):
    """Cuadro fi solo con piezas body.  Devuelve canvas, lista de (pieza, X, Y, oriented)."""
    items = []
    for p in cf.frames[fi]["pieces"]:
        if pkey(cf.key(p)) not in body_keys:
            continue
        src = sheet(cf.images[p["img"]])[p["sy"]:p["sy"] + p["sh"], p["sx"]:p["sx"] + p["sw"]]
        items.append((p, orient(src, p["flip"])))
    if not items:
        return None, []
    x0 = min(p["dx"] for p, _ in items); y0 = min(p["dy"] for p, _ in items)
    x1 = max(p["dx"] + o.shape[1] for p, o in items); y1 = max(p["dy"] + o.shape[0] for p, o in items)
    W, H = x1 - x0 + 2 * PAD, y1 - y0 + 2 * PAD
    can = np.zeros((H, W, 4), np.float32)
    placed = []
    for p, o in items:
        X, Y = p["dx"] - x0 + PAD, p["dy"] - y0 + PAD
        a = o[..., 3:4].astype(np.float32) / 255
        reg = can[Y:Y + o.shape[0], X:X + o.shape[1]]
        reg[..., :3] = o[..., :3] * a + reg[..., :3] * (1 - a)
        reg[..., 3:4] = 255 * (a + reg[..., 3:4] / 255 * (1 - a))
        placed.append((p, X, Y, o))
    return can.clip(0, 255).astype(np.uint8), placed


# ---------------------------------------------------------------- segmentacion
def geodesic(mask, seeds):
    D = np.full(mask.shape, -1, np.int32)
    q = deque()
    for y, x in zip(*np.nonzero(seeds & mask)):
        D[y, x] = 0; q.append((y, x))
    H, W = mask.shape
    while q:
        y, x = q.popleft()
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)):
            yy, xx = y + dy, x + dx
            if 0 <= yy < H and 0 <= xx < W and mask[yy, xx] and D[yy, xx] < 0:
                D[yy, xx] = D[y, x] + 1; q.append((yy, xx))
    return D


def segment(can, fexc=None, tpls=None):
    """Etiqueta cada pixel del cuadro.  Devuelve (labels, info)."""
    fexc = fexc or {}
    M = materials(can)
    A = M["a"]
    H, W = A.shape
    labels = np.zeros((H, W), np.int8)
    if A.sum() < 20:
        return labels, None
    yy, xx = np.mgrid[0:H, 0:W]
    # eje del cuerpo: de la tela/cuero (abajo) a la piel (arriba)
    up = np.array([0.0, -1.0])
    if M["skin"].sum() > 20 and M["warm"].sum() > 20:
        cs = np.array([xx[M["skin"]].mean(), yy[M["skin"]].mean()])
        cw = np.array([xx[M["warm"]].mean(), yy[M["warm"]].mean()])
        d = cs - cw
        n = np.linalg.norm(d)
        bw = np.ptp(xx[A]); bh = np.ptp(yy[A])
        if n > 4 and (bw > 1.25 * bh or d[1] > 0):
            ang = math.degrees(math.atan2(-d[1], d[0]))
            if not (55 < ang < 125):
                up = d / n
    if "up_deg" in fexc:
        a = math.radians(fexc["up_deg"]); up = np.array([math.cos(a), -math.sin(a)])
    down = -up
    proj = xx * down[0] + yy * down[1]
    pa = proj[A]
    top, bot = np.percentile(pa, 0.5), np.percentile(pa, 99.5)
    u = (proj - top) / max(1.0, bot - top)
    Hb = max(1.0, bot - top)

    skin = M["skin"]
    lab, n = ndimage.label(skin, structure=np.ones((3, 3)))
    comps = []
    for i in range(1, n + 1):
        m = lab == i
        comps.append((i, int(m.sum()), float(u[m].mean())))
    upper = [c for c in comps if c[2] < 0.55 and c[1] >= 15]
    U = max(upper or comps, key=lambda c: c[1])[0] if comps else 0
    Um = lab == U
    u_w = float(np.percentile(u[Um], 98.5)) if Um.any() else 0.45
    u_w = fexc.get("waist", u_w)
    tp = xx * -down[1] + yy * down[0]          # coordenada perpendicular al eje
    torso_cols = Um & (u > u_w - 0.12) & (u <= u_w)
    if torso_cols.any():
        tlo, thi = np.percentile(tp[torso_cols], 3) - 2, np.percentile(tp[torso_cols], 97) + 2
    else:
        tlo, thi = -1e9, 1e9
    # guanteletes (cuero naranja/dorado pegado al brazo) -> manga
    h_ = M["h"]
    orange = M["warm"] & (h_ >= 17) & (h_ < 56)
    beltzone = (np.abs(u - u_w) <= 0.05) & (tp >= tlo) & (tp <= thi)
    sash = orange & beltzone
    rest = orange & ~beltzone
    glab, gn = ndimage.label(rest, structure=np.ones((3, 3)))
    near_U = ndimage.binary_dilation(Um, iterations=3)
    sleeves = np.zeros_like(A)
    for i in range(1, gn + 1):
        m = glab == i
        if m.sum() < 5:
            continue
        if ((m & near_U).any() and u[m].mean() < 0.8 and u[m].min() < u_w + 0.05) or u[m].min() < u_w - 0.02:
            sleeves |= m
    sleeves = ndimage.binary_closing(sleeves, iterations=2) & A & ~skin & ~M["keep"]
    if DEBUG: print("sleeves", sleeves.sum(), "u_w", round(u_w,2), "gn", gn)
    if fexc.get("no_sleeves"):
        sleeves[:] = False
    near_sl = ndimage.binary_dilation(sleeves, iterations=2)
    red_low = M["warm"] & ~orange & (u > u_w) & ~sleeves
    d_sl = ndimage.distance_transform_edt(~sleeves) if sleeves.any() else np.full(A.shape, 1e9)
    d_red = ndimage.distance_transform_edt(~red_low) if red_low.any() else np.full(A.shape, 1e9)
    uarea = max(1, Um.sum())
    upper_m = Um.copy()
    hands = np.zeros_like(A); legskin = np.zeros_like(A)
    for i, area, cu in comps:
        if i == U:
            continue
        m = lab == i
        if area < 0.15 * uarea and ((m & near_sl).any() or cu < u_w - 0.02):
            hands |= m
        elif cu > u_w - 0.02:
            legskin |= m
            # puno pegado al muslo: lo que queda mas cerca de la manga que de la falda/bota
            hands |= m & (d_sl <= 7) & (d_sl < d_red)
        else:
            upper_m |= m
    # banda del cinto: alrededor de la cintura, en las columnas del torso
    band = (u >= u_w - 0.035) & (u <= u_w + 0.05) & A & ~skin & ~M["keep"] & ~sleeves
    band &= (tp >= tlo) & (tp <= thi)
    band &= ndimage.distance_transform_edt(~Um) <= 5
    band |= sash
    band &= ~sleeves
    lower = A & ~M["keep"] & (u > u_w - 0.02) & ~hands & ~band & ~sleeves
    lower &= ~(upper_m & (u <= u_w))
    upper_rest = A & ~M["keep"] & ~lower & ~band & ~hands
    labels[upper_rest] = SHIRT
    labels[hands] = HAND
    labels[band] = SASH
    labels[lower] = PANTS
    labels[M["keep"]] = KEEP
    # pies: el extremo de cada pierna (distancia geodesica desde la cintura)
    seeds = lower & (u <= u_w + 0.06)
    D = geodesic(lower, seeds)
    foot = np.zeros_like(A)
    flen = max(3, int(round(0.10 * Hb)))
    if (D > 0).any():
        Dz = np.where(D >= 0, D, 0)
        Dm = ndimage.maximum_filter(Dz, size=int(0.3 * Hb) | 1)
        foot = lower & (D >= 0) & (D >= Dm - flen) & (Dm >= 0.3 * Hb) & (D >= 0.25 * Hb)
    if fexc.get("no_feet"):
        foot[:] = False
    labels[foot & (labels == PANTS)] = FOOT
    # cabeza
    if tpls:
        head = rk_head.match(can, A, u, up, tpls, force=fexc.get("head_at"))
    else:
        head = find_head(Um, u, M, up, Hb, fexc)
    if fexc.get("no_head"):
        head = None
    if head is not None:
        labels[head["mask"] & np.isin(labels, [SHIRT, HAND, SASH])] = HEAD
    info = dict(up=up, u=u, u_w=u_w, Hb=Hb, head=head, M=M, sleeves=sleeves, beltzone=beltzone, orange=orange)
    return labels, info


def find_head(Um, u, M, up, Hb, fexc):
    """La cabeza calva: el bulto redondo mas alto del torso (cuello angosto)."""
    if Um.sum() < 40:
        return None
    r = max(1, int(round(Hb * 0.022)))
    best = None
    for it in (r + 1, r, max(1, r - 1)):
        er = ndimage.binary_erosion(Um, iterations=it)
        lab, n = ndimage.label(er)
        for i in range(1, n + 1):
            m = lab == i
            area = m.sum()
            if area < 4:
                continue
            m2 = ndimage.binary_dilation(m, iterations=it) & Um
            a2 = m2.sum()
            if a2 > 0.35 * Um.sum() and n > 1:
                continue
            ys, xs = np.nonzero(m2)
            hgt = max(1, np.ptp(ys) + 1); wid = max(1, np.ptp(xs) + 1)
            size_ok = 0.07 * Hb <= max(hgt, wid) <= 0.24 * Hb
            cu = u[m2].mean()
            fill = a2 / (hgt * wid)
            score = -cu * 2 + (1 if size_ok else -2) + fill
            # barba/ojos oscuros pegados = cara
            ring = ndimage.binary_dilation(m2, iterations=2) & ~m2
            score += 0.8 * min(1.0, (ring & M["dark"]).sum() / 6)
            if best is None or score > best[0]:
                best = (score, m2, it)
        if best is not None and best[0] > 0:
            break
    if best is None:
        return None
    m2 = best[1]
    if m2.sum() > 0.5 * Um.sum():
        return None
    ys, xs = np.nonzero(m2)
    return dict(mask=m2, cx=float(xs.mean()), cy=float(ys.mean()), h=float(np.ptp(ys) + 1), w=float(np.ptp(xs) + 1))


# ---------------------------------------------------------------- pintura
def paint(can, labels, info, R, shirt="crimson"):
    M = info["M"]
    out = can.copy().astype(np.float32)
    lum = can[..., :3].astype(np.float32) @ LUMW
    # tatuaje: luz de la piel vecina (el rojo no deja rayas)
    sk = M["skin"] & ~M["tattoo"]
    num = ndimage.uniform_filter(np.where(sk, lum, 0), 7); den = ndimage.uniform_filter(sk.astype(np.float32), 7)
    lum = np.where(M["tattoo"], np.where(den > 0.05, num / np.maximum(den, 1e-3), lum) * 0.93, lum)
    # menos ruido de textura 3D: la tela de RONIN es lisa con sombras grandes
    lum = 0.55 * ndimage.median_filter(lum, size=3) + 0.45 * ndimage.gaussian_filter(lum, 1.0)
    ramps = {SHIRT: R[shirt], HAND: R["skin"], HEAD: R["skin"], SASH: R["sash"], PANTS: R["pants"], FOOT: R["skin"]}
    bias = {SHIRT: 0.0, HAND: 0.1, HEAD: 0.1, SASH: 0.12, PANTS: 0.12, FOOT: 0.05}
    for L, rp in ramps.items():
        sel_l = labels == L
        for mat in ("skin", "warm", "dark"):
            sel = sel_l & M[mat]
            if mat == "dark":
                sel = sel_l & ~M["skin"] & ~M["warm"]
            if sel.sum() == 0:
                continue
            v = lum[sel]
            lo, hi = np.percentile(v, 4), np.percentile(v, 97)
            if mat == "dark":
                t = np.clip((v - lo) / max(8, hi - lo), 0, 1) * 0.35
            else:
                t = np.clip((v - lo) / max(12, hi - lo), 0, 1)
                t = np.clip(t * (1 - bias[L]) + bias[L], 0, 1)
            idx = np.clip(np.round(t * (len(rp) - 1)), 0, len(rp) - 1).astype(int)
            out[sel, :3] = rp[idx]
    return out


def thin_and_outline(out, labels, A):
    """Brazos/torso un poco mas delgados (-1 px de contorno exterior donde la
    parte es gruesa) y contorno oscuro estilo pixel art de RONIN."""
    edt = ndimage.distance_transform_edt(A)
    thick = ndimage.maximum_filter(edt, size=5) >= 4.0
    edge = A & ~ndimage.binary_erosion(A)
    cut = edge & (labels == SHIRT) & thick
    out[cut, 3] = 0
    A2 = A & ~cut
    edge2 = A2 & ~ndimage.binary_erosion(A2)
    body = edge2 & np.isin(labels, [SHIRT, HAND, HEAD, SASH, PANTS, FOOT])
    out[body, :3] *= 0.5
    return out, cut


def head_sprite(heads, info, facing_left, which):
    """Cabeza de RONIN (pelo + cara) a tamano fijo, sobre la cara de Kratos."""
    hd = heads[which]
    (fx0, fy0, fx1, fy1), (fcx, fcy) = rk_ronin_parts.face_box(hd)
    h = info["head"]
    k = (h["w"] * HEAD_K) / max(1, (fx1 - fx0 + 1))
    img = Image.fromarray(hd, "RGBA")
    W2, H2 = max(1, round(img.width * k)), max(1, round(img.height * k))
    a = hd[..., 3:4].astype(np.float32) / 255
    pm = np.dstack([hd[..., :3] * a, hd[..., 3:4]]).astype(np.uint8)
    sm = np.array(Image.fromarray(pm, "RGBA").resize((W2, H2), Image.LANCZOS)).astype(np.float32)
    al = sm[..., 3:4] / 255
    rgb = np.where(al > 0.01, sm[..., :3] / np.maximum(al, 0.01), 0)
    alpha = np.where(sm[..., 3] > 110, 255, 0)
    spr = np.dstack([np.clip(rgb, 0, 255), alpha]).astype(np.uint8)
    cx, cy = fcx * k, fcy * k
    if facing_left:
        spr = spr[:, ::-1]; cx = spr.shape[1] - 1 - cx
    ang = h.get("ang", 0.0)
    if abs(ang) > 1:
        im = Image.fromarray(spr, "RGBA")
        c0 = np.array([im.width / 2, im.height / 2])
        im2 = im.rotate(ang, resample=Image.NEAREST, expand=True)
        c1 = np.array([im2.width / 2, im2.height / 2])
        v = np.array([cx, cy]) - c0
        t = math.radians(ang)
        v2 = np.array([v[0] * math.cos(t) + v[1] * math.sin(t), -v[0] * math.sin(t) + v[1] * math.cos(t)])
        cx, cy = c1 + v2
        spr = np.array(im2)
    # centro de la cara de RONIN sobre el centro de la cabeza de Kratos (+ ajuste)
    t = math.radians(ang)
    ox, oy = HEAD_OFF[0] * h["w"] * (-1 if facing_left else 1), HEAD_OFF[1] * h["h"]
    tx = h["cx"] + ox * math.cos(t) + oy * math.sin(t)
    ty = h["cy"] - ox * math.sin(t) + oy * math.cos(t)
    return spr, int(round(tx - cx)), int(round(ty - cy))


def over(dst, spr, ox, oy):
    H, W = dst.shape[:2]
    sy0, sx0 = max(0, -oy), max(0, -ox)
    dy0, dx0 = max(0, oy), max(0, ox)
    hh = min(spr.shape[0] - sy0, H - dy0); ww = min(spr.shape[1] - sx0, W - dx0)
    if hh <= 0 or ww <= 0:
        return
    s = spr[sy0:sy0 + hh, sx0:sx0 + ww].astype(np.float32)
    d = dst[dy0:dy0 + hh, dx0:dx0 + ww]
    a = s[..., 3:4] / 255
    d[..., :3] = s[..., :3] * a + d[..., :3] * (1 - a)
    d[..., 3:4] = np.maximum(d[..., 3:4], s[..., 3:4])


# ---------------------------------------------------------------- un cuadro
def process_frame(cf, fi, body_keys, R, heads, exc, wname, shirt="crimson", debug=False, tpls=None):
    can, placed = compose_body(cf, fi, body_keys)
    if can is None:
        return {}, None
    fexc = exc.get("frames", {}).get(f"{wname}:{fi}", {})
    labels, info = segment(can, fexc, tpls)
    if info is None:
        return {}, None
    out = paint(can, labels, info, R, shirt)
    out, cut = thin_and_outline(out, labels, info["M"]["a"])
    # sin pintar donde la pieza es efecto
    keep = labels == KEEP
    out[keep] = can[keep]
    flips = [p["flip"] & 1 for p, *_ in placed]
    facing_left = sum(flips) * 2 > len(flips)
    if info["head"] is not None and "facing_left" in info["head"]:
        facing_left = info["head"]["facing_left"]
    if "facing_left" in fexc:
        facing_left = bool(fexc["facing_left"])
    head = None
    if info["head"] is not None and not fexc.get("no_head"):
        which = fexc.get("head", "r4_c2")
        head = head_sprite(heads, info, facing_left, which)
    # recorte por pieza (en pantalla, con margen PAD)
    res = {}
    hmask = info["head"]["mask"] if info["head"] is not None else None
    host = None
    if hmask is not None:
        best = -1
        for j, (p, X, Y, o) in enumerate(placed):
            m = np.zeros_like(hmask); m[Y:Y + o.shape[0], X:X + o.shape[1]] = o[..., 3] > 60
            c = (m & hmask).sum()
            if c > best:
                best, host = c, j
    for j, (p, X, Y, o) in enumerate(placed):
        hh, ww = o.shape[:2]
        tile = np.zeros((hh + 2 * PAD, ww + 2 * PAD, 4), np.float32)
        reg = out[Y:Y + hh, X:X + ww]
        own = o[..., 3] > 0
        t = tile[PAD:PAD + hh, PAD:PAD + ww]
        t[..., :3] = np.where(own[..., None], reg[..., :3], 0)
        # alfa: la del original (respeta piezas semitransparentes) salvo lo adelgazado
        t[..., 3] = np.where(own & ~cut[Y:Y + hh, X:X + ww], o[..., 3], 0)
        if head is not None and j == host:
            spr, ox, oy = head
            over(tile, spr, ox - X + PAD, oy - Y + PAD)
        res[pkey(cf.key(p))] = (p["flip"], tile.clip(0, 255).astype(np.uint8))
    dbg = (can, labels, out, info) if debug else None
    return res, dbg


def unorient(tile, flip):
    t = tile
    if flip & 4:
        t = np.rot90(t, -1)
    if flip & 2:
        t = t[::-1]
    if flip & 1:
        t = t[:, ::-1]
    return np.ascontiguousarray(t)


# ---------------------------------------------------------------- empaquetar y escribir
def pack(tiles):
    """tiles: {key: array}.  Devuelve {key: (sheet_n, x, y, w, h)}, [sheets]."""
    order = sorted(tiles, key=lambda k: -tiles[k].shape[0])
    sheets, place = [], {}
    cur, x, y, rowh = [], 0, 0, 0
    MAXH = 1024

    def flush():
        if not cur:
            return
        Hh = max(yy + t.shape[0] for _, _, yy, t in cur)
        S = np.zeros((Hh, SHEET_W, 4), np.uint8)
        for k, xx, yy, t in cur:
            S[yy:yy + t.shape[0], xx:xx + t.shape[1]] = t
        sheets.append(S)

    for k in order:
        t = tiles[k]
        h, w = t.shape[:2]
        if x + w > SHEET_W:
            x, y, rowh = 0, y + rowh + 1, 0
        if y + h > MAXH:
            flush(); cur = []; x, y, rowh = 0, 0, 0
        cur.append((k, x, y, t))
        place[k] = (len(sheets), x, y, w, h)
        x += w + 1; rowh = max(rowh, h)
    flush()
    return place, sheets


def setup():
    """Todo lo que necesita process_frame: excepciones, piezas body, rampas,
    cabezas de RONIN y plantillas de cabeza de Kratos."""
    exc = load_exc()
    inv = inventory()
    body_keys = {k for k, e in inv.items() if e["cls"] == "body"}
    R, heads = rk_ronin_parts.build()
    cfs = {w: CharFile(DATA / f"kratos_{w}.txt") for w in WEAPONS}
    tpls = rk_head.templates(lambda w, f: compose_body(cfs[w], f, body_keys)[0], exc)
    return exc, body_keys, R, heads, cfs, tpls


def build(weapons, shirt="crimson", write=True):
    exc, body_keys, R, heads, cfs, tpls = setup()
    tiles, ctx = {}, {}
    for w in weapons:
        cf = cfs[w]
        for fi in range(len(cf.frames)):
            need = [p for p in cf.frames[fi]["pieces"] if pkey(cf.key(p)) in body_keys and pkey(cf.key(p)) not in tiles]
            if not need:
                continue
            res, _ = process_frame(cf, fi, body_keys, R, heads, exc, w, shirt, tpls=tpls)
            for k, (flip, t) in res.items():
                if k not in tiles:
                    tiles[k] = unorient(t, flip)
                    ctx[k] = f"{w}:{fi}"
    place, sheets = pack(tiles)
    if write:
        for n, S in enumerate(sheets):
            Image.fromarray(S, "RGBA").save(ACTOR / f"{FIRST_ID + n}.png", optimize=True)
        for w in weapons:
            write_txt(cfs[w], w, place, len(sheets))
    return tiles, place, sheets, ctx


def write_txt(cf, w, place, nsheets):
    base = max(cf.images) + 1
    lines = list(cf.lines)
    # cabecera: id y nombre propios
    for i, l in enumerate(lines):
        t = l.split()
        if not t:
            continue
        if t[0] == "character":
            t[1] = f"ronin_kratos_{w}"; lines[i] = " ".join(t)
        elif t[0] == "name":
            nm = {"espadas": "ESPADAS DEL CAOS", "rayo": "CADENA DEL RAYO", "garras": "GARRAS DE HADES", "cesto": "CESTUS DE NEMEA"}[w]
            lines[i] = f"name RONIN-KRATOS ({nm})"
        elif t[0] == "#" or l.startswith("#"):
            lines[i] = f"# RONIN-KRATOS ({w}): copia de kratos_{w}.txt con las piezas del cuerpo de RONIN (tools/ronin_kratos/rk_build.py)"
    last_img = max(i for i, l in enumerate(lines) if l.startswith("image "))
    extra = [f"image {base + n} {FIRST_ID + n}" for n in range(nsheets)]
    for fr in cf.frames:
        for p in fr["pieces"]:
            k = pkey(cf.key(p))
            if k in place:
                n, x, y, ww, hh = place[k]
                lines[p["line"]] = (f"piece {base + n} {x} {y} {ww} {hh} {p['dx'] - PAD} {p['dy'] - PAD} {p['flip']}")
    lines = lines[:last_img + 1] + extra + lines[last_img + 1:]
    (DATA / f"ronin_kratos_{w}.txt").write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--weapons", default=",".join(WEAPONS))
    ap.add_argument("--shirt", default="crimson")
    a = ap.parse_args()
    ws = a.weapons.split(",")
    tiles, place, sheets, _ = build(ws if set(ws) == set(WEAPONS) else WEAPONS, a.shirt)
    print("piezas body repintadas:", len(tiles), "hojas:", [FIRST_ID + i for i in range(len(sheets))])
