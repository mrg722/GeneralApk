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
Fluidez (v2, en 3 pases sobre TODOS los cuadros antes de pintar):
  - Cabeza de Kratos: varias candidatas por cuadro (rk_head.candidates, la cabeza
    debe sobresalir del cuerpo) y se elige la secuencia mas coherente de cada clip
    (choose_heads, programacion dinamica); giro fino de 5 grados.
  - Cabeza de RONIN: sigue posicion E inclinacion de esa cabeza (girada y anclada
    al cuello, subpixel, supermuestreada), cara ~ ancho de la calva (HEAD_K=1.0),
    con media movil 1-2-1 en el clip y tope HEAD_POS_TOL / HEAD_ANG_TOL.
  - Ropa estable: guantelete = camino brazo->puno, sin faldon/taparrabo (sin partir
    nunca el cuerpo), sin islas ni puntitos, mediana temporal de etiquetas con
    compensacion de movimiento (vote_labels), luz con rangos fijos para todos los
    cuadros, rampa continua y tela lisa; aura de furia pegada al cuerpo = RONIN tenido.
  Metricas: rk_metrics.py (jitter de cabeza y parpadeo de clase por clip).
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
HEAD_K = 1.0           # ancho de la cara de RONIN / ancho de la cabeza de Kratos (antes 1.30: cabezon)
HEAD_OFF = (0.04, 0.16)  # centro de la cara de RONIN respecto del centro de la calva (anchos/altos), girado con ella
HEAD_OFF_BACK = (0.0, 0.02)  # nuca: sin caida de barbilla, centrada sobre la calva
HEAD_POS_TOL = 0.6     # suavizado de la cabeza: maximo corrimiento respecto del cuello de Kratos (px)
HEAD_ANG_TOL = 8.0     # y del giro (grados)
KRATOS_HEAD_W = 12.0   # ancho de la calva de Kratos (mediana de las plantillas; se recalcula en build)
HD_W_SSD, HD_W_RING, HD_W_U = 4.0, 5.0, 2.0   # costo de una candidata de cabeza (calce, cuerpo alrededor, altura)
HD_W_D, HD_W_A, HD_W_FLIP = 1.0, 0.6, 3.0     # costo de salto entre cuadros (px/5 sobre 5, giro/45, cambio de lado)
HAND_IN_SHIRT_MAX = 60  # mancha de piel (px) rodeada de polera -> polera
HAND_AREA = 0.016      # puno: area <= HAND_AREA*Hb^2 y largo <= HAND_EXT*Hb
HAND_EXT = 0.17
ARM_PATH = 0.34        # largo maximo del guantelete (camino brazo->puno) / alto del cuerpo
ARM_SLACK = 0.06
FOOT_MIN = 0.42        # el pie esta a >= 0.42 del alto del cuerpo desde la cintura (geodesico)
SKIRT_GAP = 3          # tela a mas de 3 px de la pierna = faldon
CLOTH_BLUR = 0.9       # desenfoque dentro de polera/pantalon (px)
SKIRT_LOWER_D = 0.32   # ... y a lo mas 0.32 del alto (geodesico por el cuerpo de abajo) de la cintura
SKIRT_REACH = 0.35     # largo maximo del faldon medido por la tela desde el cinto
SKIRT_DEPTH = 0.42     # el faldon cuelga hasta 0.42 del alto bajo la cintura (las botas se protegen por cercania)
AURA_GRAY = 0.15       # con mas de 15% del cuerpo bajo aura, la cabeza se busca solo por luz
AURA_MIX = 0.45        # cuanto del color del aura se mezcla sobre RONIN
ISLAND_MAX = 30        # islas de prenda mas chicas que esto (px) se funden con lo que las rodea
SASH_MIN = 20          # manchas de cinto mas chicas que esto no son cinto
VOTE_DIST = 2          # coherencia temporal: vecinos a 1 y 2 cuadros dentro del clip
VOTE_ITERS = 2
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
    # aura de furia (morada / fuego) PEGADA al cuerpo: el cuerpo de Kratos tenido.
    # Se segmenta como cuerpo (para que salga RONIN debajo) y al pintar se vuelve a
    # tenir con el color del aura; el fuego suelto/semitransparente de alrededor se conserva.
    aura = keep & (px[..., 3] > 200) & ndimage.binary_erosion(a, iterations=2)
    keep = keep & ~aura
    skin = (skin | tattoo | aura) & ~keep
    warm = warm & ~tattoo & ~keep & ~aura
    dark = a & ~skin & ~warm & ~keep
    return dict(a=a, skin=skin, tattoo=tattoo, warm=warm, dark=dark, keep=keep, aura=aura, h=h, s=s, v=v)


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


def mode_filter(labels, parts=(SHIRT, HAND, SASH, PANTS, FOOT)):
    """Mayoria 3x3 entre las partes del cuerpo: sin pixeles sueltos de otro color
    (los puntitos son los que mas parpadean entre cuadros)."""
    cnt = np.stack([ndimage.uniform_filter((labels == L).astype(np.float32), 3) for L in parts])
    win = np.array(parts, np.int8)[cnt.argmax(0)]
    own = np.zeros(labels.shape, np.float32)
    for i, L in enumerate(parts):
        own = np.where(labels == L, cnt[i], own)
    ch = np.isin(labels, parts) & (cnt.max(0) > own + 1e-6) & (own <= 3.5 / 9)
    out = labels.copy(); out[ch] = win[ch]
    return out


def clean_islands(labels, maxa=None, parts=(SHIRT, PANTS, SASH)):
    """Islas chicas de una prenda dentro de otra (manchas rojas en el pantalon,
    franjas blancas en la manga) pasan a la prenda que las rodea.  Manos y pies no
    se tocan (son chicos a proposito)."""
    maxa = ISLAND_MAX if maxa is None else maxa
    out = labels.copy()
    for L in parts:
        lab, n = ndimage.label(out == L, structure=np.ones((3, 3)))
        if n == 0:
            continue
        sz = ndimage.sum(np.ones_like(lab), lab, range(1, n + 1))
        for i in np.nonzero(sz < maxa)[0] + 1:
            m = lab == i
            ys, xs = np.nonzero(m)
            y0, y1, x0, x1 = max(0, ys.min() - 2), ys.max() + 3, max(0, xs.min() - 2), xs.max() + 3
            mm = m[y0:y1, x0:x1]
            ring = ndimage.binary_dilation(mm, iterations=1) & ~mm
            nbl = out[y0:y1, x0:x1][ring]
            nbl = nbl[np.isin(nbl, VOTE)]
            if len(nbl) == 0:
                continue
            vals, cnt = np.unique(nbl, return_counts=True)
            k = np.argmax(cnt)
            if vals[k] != L and cnt[k] >= 0.6 * len(nbl):
                out[y0:y1, x0:x1][mm] = vals[k]
    return out


def segment(can, fexc=None, tpls=None, cands=False):
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
    if u_w > 0.5:
        # la piel del torso se junto con la de un muslo (mano apoyada): cintura
        # = la cintura tipica de Kratos de pie (ajustable por cuadro en exceptions.json)
        u_w = 0.40
        Um = Um & (u <= u_w + 0.02)
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
    # manos = componentes de piel chicas y compactas (punos).  La piel suelta
    # que no es puno es brazo (arriba de la cintura) o pierna (abajo): antes un
    # brazo de atras separado del torso salia color piel en un cuadro y rojo en otro.
    hand_c, upper_m, legskin = [], Um.copy(), np.zeros_like(A)
    for i, area, cu in comps:
        if i == U:
            continue
        m = lab == i
        ys_, xs_ = np.nonzero(m)
        ext = max(np.ptp(ys_), np.ptp(xs_)) + 1
        if area <= HAND_AREA * Hb * Hb and ext <= HAND_EXT * Hb:
            hand_c.append((m, cu))
        elif cu < u_w - 0.02:
            upper_m |= m
        else:
            legskin |= m
    # guantelete = el camino corto por cuero/metal entre la piel del brazo y el
    # puno (aunque el antebrazo cuelgue delante de la falda: antes salia blanco)
    redcloth = M["warm"] & ((h_ < 15) | (h_ > 335)) & ~ndimage.binary_dilation(orange, iterations=1)
    passable = A & ~skin & ~M["keep"] & ~(beltzone & ~orange) & ~redcloth
    DU = geodesic(passable | upper_m, upper_m)
    path_hands = np.zeros_like(A)
    for m, cu in hand_c:
        Dh = geodesic(passable | m, m)
        both = passable & (DU > 0) & (Dh > 0)
        if not both.any():
            continue
        S = np.where(both, DU + Dh, 1 << 30)
        L = S.min()
        if L <= ARM_PATH * Hb:
            sleeves |= both & (S <= L + max(4, ARM_SLACK * Hb))
            path_hands |= m
    sleeves = ndimage.binary_closing(sleeves, iterations=1) & A & ~skin & ~M["keep"]
    if fexc.get("no_sleeves"):
        sleeves[:] = False
    near_sl = ndimage.binary_dilation(sleeves, iterations=2)
    red_low = M["warm"] & ~orange & (u > u_w) & ~sleeves
    d_sl = ndimage.distance_transform_edt(~sleeves) if sleeves.any() else np.full(A.shape, 1e9)
    d_red = ndimage.distance_transform_edt(~red_low) if red_low.any() else np.full(A.shape, 1e9)
    hands = path_hands.copy()
    for m, cu in hand_c:
        if (m & near_sl).any() or cu < u_w - 0.02:
            hands |= m
        elif not (m & path_hands).any():
            legskin |= m
    # puno pegado al muslo: lo que queda mas cerca de la manga que de la falda/bota
    hands |= legskin & (d_sl <= 7) & (d_sl < d_red)
    legskin &= ~hands
    # banda del cinto: alrededor de la cintura, en las columnas del torso
    band = (u >= u_w - 0.035) & (u <= u_w + 0.05) & A & ~skin & ~M["keep"] & ~sleeves
    band &= (tp >= tlo) & (tp <= thi)
    band &= ndimage.distance_transform_edt(~Um) <= 5
    band |= sash
    band &= ~sleeves
    # el cinto es UNA banda: fuera los puntitos dorados sueltos (brillos del guantelete)
    bl, bn = ndimage.label(ndimage.binary_closing(band, iterations=1) & A, structure=np.ones((3, 3)))
    if bn:
        sz = ndimage.sum(np.ones_like(bl), bl, range(1, bn + 1))
        band &= np.isin(bl, 1 + np.nonzero(sz >= SASH_MIN)[0])
    lower = A & ~M["keep"] & (u > u_w - 0.02) & ~hands & ~band & ~sleeves
    lower &= ~(upper_m & (u <= u_w))
    upper_rest = A & ~M["keep"] & ~lower & ~band & ~hands
    labels[upper_rest] = SHIRT
    labels[hands] = HAND
    labels[band] = SASH
    labels[lower] = PANTS
    labels[M["keep"]] = KEEP
    # "manos" rodeadas de polera por todos lados son brillos de la piel del torso
    # (Kratos con luz dorada del Cestus), no punos: van con la polera
    hl, hn = ndimage.label(labels == HAND, structure=np.ones((3, 3)))
    for i in range(1, hn + 1):
        m = hl == i
        if m.sum() > HAND_IN_SHIRT_MAX:
            continue
        ring = ndimage.binary_dilation(m, iterations=2) & ~m
        if ring.sum() and (labels[ring] == SHIRT).mean() >= 0.85:
            labels[m] = SHIRT
    # pies: el extremo de cada pierna (distancia geodesica desde la cintura)
    seeds = lower & (u <= u_w + 0.06)
    D = geodesic(lower, seeds)
    foot = np.zeros_like(A)
    flen = max(3, int(round(0.10 * Hb)))
    if (D > 0).any():
        Dz = np.where(D >= 0, D, 0)
        Dm = ndimage.maximum_filter(Dz, size=int(0.3 * Hb) | 1)
        # el pie es el extremo LEJANO de la pierna (la punta de la falda, a ~0.25, no);
        # en una pierna muy doblada/escorzada (patada, golpe recibido) el camino
        # geodesico hasta el tobillo es corto aunque SI sea el pie: se puede bajar
        # el umbral por cuadro (exceptions.json "foot_min")
        foot_min = fexc.get("foot_min", FOOT_MIN)
        foot = lower & (D >= 0) & (D >= Dm - flen) & (Dm >= foot_min * Hb) & (D >= (foot_min - 0.07) * Hb)
        # sin rayas: el pie es una sola mancha (empeine + dedos)
        foot = ndimage.binary_closing(foot, iterations=2) & lower
    if fexc.get("no_feet"):
        foot[:] = False
    labels[foot & (labels == PANTS)] = FOOT
    # punta / faldon de la falda de Kratos: tela roja oscura lejos de las piernas
    # y del cinto (RONIN lleva pantalon: sin taparrabo ni capa)
    drop = np.zeros_like(A)
    if not fexc.get("keep_skirt"):
        h_, s_, v_ = M["h"], M["s"], M["v"]
        cloth_red = lower & ~foot & ((h_ < 16) | (h_ > 335)) & (s_ > 0.35) & (v_ < 0.62)
        cloth = cloth_red | (lower & ~foot & (v_ < 0.13) & ndimage.binary_dilation(cloth_red, iterations=2))
        legs = (legskin | (orange & lower) | foot) & (u > u_w + 0.06)
        if legs.any() and cloth.any():
            d_leg = ndimage.distance_transform_edt(~legs)
            d_band = ndimage.distance_transform_edt(~(band | (upper_rest & (u <= u_w))))
            d_foot = ndimage.distance_transform_edt(~foot) if foot.any() else np.full(A.shape, 1e9)
            cand = cloth & (d_leg > SKIRT_GAP) & (d_band > 4) & (u > u_w + 0.07) & (u < u_w + SKIRT_DEPTH) & (d_foot > 6)
            # la falda cuelga DEL CINTO: solo tela unida al cinto por tela (una bota
            # oscura de la pierna de atras no lo esta: antes se borraba y el pie quedaba suelto)
            seeds = cloth_red & (d_band <= 4)
            Dg = geodesic(cloth_red, seeds) if seeds.any() else np.full(A.shape, -1)
            reach = (Dg >= 0) & (Dg <= SKIRT_REACH * Hb)
            cand &= reach | (ndimage.binary_dilation(reach, iterations=2) & ~cloth_red)
            # y cerca de la cintura recorriendo el cuerpo de abajo: las botas quedan lejos
            cand &= (D >= 0) & (D <= SKIRT_LOWER_D * Hb)
            # solo lo que cuelga hacia afuera (toca el fondo), nunca huecos dentro de la
            # pierna, y NUNCA partiendo el cuerpo: si al quitarlo queda suelto un trozo
            # que no es tela (un pie, una bota), ese trozo no se quita
            cl, cn = ndimage.label(cand, structure=np.ones((3, 3)))
            outside = ndimage.binary_dilation(~A, iterations=1)
            clothish = cloth | (lower & ((h_ < 16) | (h_ > 335)) & (v_ < 0.62))
            for i in range(1, cn + 1):
                m = cl == i
                if m.sum() < 4 or not (m & outside).any():
                    continue
                trial = drop | m
                il, ino = ndimage.label(A & ~trial, structure=np.ones((3, 3)))
                if ino == 0:
                    continue
                sz = ndimage.sum(np.ones_like(il), il, range(1, ino + 1))
                big = 1 + int(np.argmax(sz))
                ok, extra = True, np.zeros_like(A)
                near = ndimage.binary_dilation(m, iterations=2)
                for j in range(1, ino + 1):
                    if j == big:
                        continue
                    mj = il == j
                    if not (mj & near).any():
                        continue          # ya estaba suelto (otra pieza), no es culpa de m
                    if (mj & foot).any() or (mj & clothish).sum() < 0.6 * mj.sum() or sz[j - 1] > 120:
                        ok = False; break
                    extra |= mj           # resto de falda que queda colgando: tambien fuera
                if ok:
                    drop |= m | extra
    labels[drop] = BG
    labels = clean_islands(mode_filter(labels))
    # cabeza: con cands=True solo se dan las posiciones posibles (se elige despues
    # con continuidad temporal en el clip, ver choose_heads); si no, la mejor
    head, hc = None, []
    if cands and tpls and not fexc.get("no_head"):
        aura_frac = M["aura"].sum() / max(1, A.sum())
        hc = rk_head.candidates(can, A, u, up, tpls, force=fexc.get("head_at"), gray=aura_frac > AURA_GRAY)
    elif tpls:
        head = rk_head.match(can, A, u, up, tpls, force=fexc.get("head_at"))
    else:
        head = find_head(Um, u, M, up, Hb, fexc)
    if fexc.get("no_head"):
        head = None
    if head is not None:
        labels[head["mask"] & np.isin(labels, [SHIRT, HAND, SASH])] = HEAD
    info = dict(up=up, u=u, u_w=u_w, Hb=Hb, head=head, head_cands=hc, M=M, sleeves=sleeves, beltzone=beltzone,
                orange=orange, drop=drop)
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
def lum_of(can, M):
    """Luz de Kratos para repintar: el tatuaje toma la luz de la piel vecina y se
    quita el ruido fino de la textura 3D (sin aplanar los volumenes grandes)."""
    lum = can[..., :3].astype(np.float32) @ LUMW
    sk = M["skin"] & ~M["tattoo"]
    num = ndimage.uniform_filter(np.where(sk, lum, 0), 7); den = ndimage.uniform_filter(sk.astype(np.float32), 7)
    lum = np.where(M["tattoo"], np.where(den > 0.05, num / np.maximum(den, 1e-3), lum) * 0.93, lum)
    return 0.6 * ndimage.median_filter(lum, size=3) + 0.4 * ndimage.gaussian_filter(lum, 0.8)


RAMP_OF = {SHIRT: "shirt", HAND: "skin", HEAD: "skin", SASH: "sash", PANTS: "pants", FOOT: "skin"}
BIAS = {SHIRT: 0.0, HAND: 0.1, HEAD: 0.1, SASH: 0.12, PANTS: 0.12, FOOT: 0.05}


def mat_sel(M, mat):
    return M[mat] if mat != "dark" else (~M["skin"] & ~M["warm"])


def lum_ranges(samples):
    """Rangos de luz FIJOS por (parte, material) para todos los cuadros: antes se
    normalizaba cuadro a cuadro y el mismo trozo cambiaba de tono entre cuadros."""
    rng = {}
    for k, vals in samples.items():
        v = np.concatenate(vals)
        if len(v) >= 50:
            rng[k] = (float(np.percentile(v, 4)), float(np.percentile(v, 97)))
    return rng


def paint(can, labels, info, R, shirt="crimson", ranges=None):
    M = info["M"]
    out = can.copy().astype(np.float32)
    lum = lum_of(can, M)
    for L, rn in RAMP_OF.items():
        rp = R[shirt] if rn == "shirt" else R[rn]
        sel_l = labels == L
        for mat in ("skin", "warm", "dark"):
            sel = sel_l & mat_sel(M, mat)
            if sel.sum() == 0:
                continue
            v = lum[sel]
            if ranges and (L, mat) in ranges:
                lo, hi = ranges[(L, mat)]
            else:
                lo, hi = np.percentile(v, 4), np.percentile(v, 97)
            if mat == "dark":
                t = np.clip((v - lo) / max(8, hi - lo), 0, 1)
                # botas/cuero oscuro de Kratos -> pantalon en sombra suave (no gris sucio)
                t = 0.22 + 0.5 * t if L == PANTS else 0.4 * t
            else:
                t = np.clip((v - lo) / max(12, hi - lo), 0, 1)
                t = np.clip(t * (1 - BIAS[L]) + BIAS[L], 0, 1)
            # rampa continua (interpolada): volumen suave, sin escalones de bloque
            x = t * (len(rp) - 1)
            i0 = np.clip(np.floor(x).astype(int), 0, len(rp) - 2)
            f = (x - i0)[:, None]
            out[sel, :3] = rp[i0] * (1 - f) + rp[i0 + 1] * f
    # aura: RONIN debajo, tenido con el color del aura de Kratos
    au = M.get("aura")
    if au is not None and au.any():
        al = AURA_MIX
        out[au, :3] = out[au, :3] * (1 - al) + can[au, :3].astype(np.float32) * al
    # tela lisa de RONIN: desenfoque suave DENTRO de cada prenda (quita el
    # moteado de la textura 3D y deja el volumen grande de luz/sombra)
    for L in (SHIRT, PANTS):
        m = (labels == L).astype(np.float32)
        if m.sum() == 0:
            continue
        den = ndimage.gaussian_filter(m, CLOTH_BLUR)
        for c in range(3):
            num = ndimage.gaussian_filter(out[..., c] * m, CLOTH_BLUR)
            out[..., c] = np.where(m > 0, num / np.maximum(den, 1e-3), out[..., c])
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


# ---------------------------------------------------------------- cabeza de RONIN
_head_cache = {}


def head_source(heads, which):
    """Cabeza fuente de RONIN (alta resolucion), premultiplicada, y su centro de cara."""
    if which not in _head_cache:
        hd = heads[which]
        if which == "back":
            # sin cara (pelo solo): el "ancho de cara" equivalente es el ancho
            # del bulto de pelo y el centro es su centroide (no hay piel que ubicar)
            am = hd[..., 3] > 40
            ys, xs = np.nonzero(am)
            fcx, fcy = float(xs.mean()), float(ys.mean())
            fw = float(xs.max() - xs.min() + 1)
        else:
            (fx0, fy0, fx1, fy1), (fcx, fcy) = rk_ronin_parts.face_box(hd)
            fw = fx1 - fx0 + 1
        a = hd[..., 3:4].astype(np.float32) / 255
        pm = np.dstack([hd[..., :3].astype(np.float32) * a, a[..., 0]])
        _head_cache[which] = (pm, (fcx, fcy), fw)
    return _head_cache[which]


def head_scale(heads, which, kratos_w):
    pm, fc, fw = head_source(heads, which)
    return HEAD_K * kratos_w / fw


def render_head(heads, which, k, ang, facing_left, tx, ty, SS=4):
    """Cabeza de RONIN girada `ang` (mismo sentido que PIL.rotate / las plantillas
    de Kratos), espejada si mira a la izquierda, escala k, con el centro de la cara
    en (tx, ty) con precision subpixel.  Se muestrea desde la imagen grande con
    supermuestreo SSxSS (sin el escalon de NEAREST).  Devuelve (spr RGBA, ox, oy)."""
    pm, (fcx, fcy), _ = head_source(heads, which)
    Hs, Ws = pm.shape[:2]
    t = math.radians(ang)
    c, s = math.cos(t), math.sin(t)
    mx = -1.0 if facing_left else 1.0
    # adelante: d = k*(mx*(x-fcx), y-fcy); T = (tx,ty) + Rccw(t) d, Rccw = [[c, s], [-s, c]] (y hacia abajo)
    corners = np.array([[0, 0], [Ws, 0], [0, Hs], [Ws, Hs]], np.float32)
    dx_ = k * mx * (corners[:, 0] - fcx); dy_ = k * (corners[:, 1] - fcy)
    X = tx + c * dx_ + s * dy_; Y = ty - s * dx_ + c * dy_
    x0, y0 = int(math.floor(X.min())) - 1, int(math.floor(Y.min())) - 1
    x1, y1 = int(math.ceil(X.max())) + 1, int(math.ceil(Y.max())) + 1
    W, H = x1 - x0, y1 - y0
    gy, gx = np.mgrid[0:H * SS, 0:W * SS].astype(np.float32)
    PX = x0 + (gx + 0.5) / SS - tx; PY = y0 + (gy + 0.5) / SS - ty
    # inversa: d = Rccw(-t) (P)
    ddx = c * PX - s * PY; ddy = s * PX + c * PY
    sx = fcx + ddx / (k * mx) - 0.5; sy = fcy + ddy / k - 0.5
    ch = [ndimage.map_coordinates(pm[..., i], [sy, sx], order=1, mode="constant", cval=0.0) for i in range(4)]
    acc = np.stack(ch, -1).reshape(H, SS, W, SS, 4).mean((1, 3))
    al = acc[..., 3]
    rgb = np.where(al[..., None] > 0.01, acc[..., :3] / np.maximum(al[..., None], 0.01), 0)
    spr = np.dstack([np.clip(rgb, 0, 255), np.where(al > 0.5, 255, 0)]).astype(np.uint8)
    return spr, x0, y0


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


# ---------------------------------------------------------------- pase 1: analisis por cuadro
def frame_sig(cf, fi, body_keys):
    return tuple((pkey(cf.key(p)), p["dx"], p["dy"], p["flip"]) for p in cf.frames[fi]["pieces"] if pkey(cf.key(p)) in body_keys)


def analyze(cf, fi, body_keys, exc, wname, tpls):
    """Cuadro armado (solo cuerpo) + etiquetas + cabeza de Kratos.  `off` pasa de
    coordenadas del lienzo a coordenadas del origen del personaje (pies)."""
    can, placed = compose_body(cf, fi, body_keys)
    if can is None:
        return None
    fexc = exc.get("frames", {}).get(f"{wname}:{fi}", {})
    labels, info = segment(can, fexc, tpls, cands=True)
    if info is None:
        return None
    p0, X0, Y0, _ = placed[0]
    off = (p0["dx"] - X0, p0["dy"] - Y0)
    for c in info["head_cands"]:
        c["ox"], c["oy"] = c["cx"] + off[0], c["cy"] + off[1]      # en coordenadas del origen
    return dict(can=can, placed=placed, labels=labels, info=info, off=off, fexc=fexc, head=None,
                which=fexc.get("head"), key=f"{wname}:{fi}")


def set_head(a, cand, tpls):
    """Fija la cabeza de Kratos elegida para el cuadro: giro fino, mascara y etiqueta."""
    info = a["info"]
    if cand is None:
        info["head"] = None; a["head"] = None
        return
    A = info["M"]["a"]
    if not (a["fexc"].get("head_at") or {}).get("angles"):
        c = rk_head.refine(a["can"], A, info["u"], tpls, cand)
    else:
        c = cand
    mask = rk_head.mask_of(c, A)
    info["head"] = dict(c, mask=mask)
    a["labels"][mask & np.isin(a["labels"], [SHIRT, HAND, SASH])] = HEAD
    fl = bool(a["fexc"]["facing_left"]) if "facing_left" in a["fexc"] else c["facing_left"]
    a["head"] = dict(x=c["cx"] + a["off"][0], y=c["cy"] + a["off"][1], ang=c["ang"], w=c["w"], h=c["h"], facing_left=fl)
    if a.get("which") is None:
        # sin override manual: nuca de Kratos (de espaldas) -> pelo trasero de RONIN,
        # cualquier otra plantilla -> cara de frente (evita que se vea la cara al reves)
        a["which"] = "back" if c.get("back") else "r4_c2"


def _unary(c, best):
    return (HD_W_SSD * (c["score"] / max(best, 1e-6) - 1) + HD_W_RING * max(0.0, c["rc"] - 0.35)
            + HD_W_U * max(0.0, c["u"]) + c.get("tcost", 0.0))


def _pair(c1, c2):
    d = math.hypot(c1["ox"] - c2["ox"], c1["oy"] - c2["oy"])
    da = abs((c1["ang"] - c2["ang"] + 180) % 360 - 180)
    return HD_W_D * max(0.0, d - 5) / 5 + HD_W_A * da / 45 + (HD_W_FLIP if c1["facing_left"] != c2["facing_left"] else 0)


def choose_heads(an, cfs, sigs_w, weapons, tpls):
    """Cabeza de Kratos con continuidad temporal: por cada clip se elige, entre las
    candidatas de cada cuadro, la secuencia de menor costo (calce de plantilla +
    sobresale del cuerpo + saltos entre cuadros) con programacion dinamica.  Un
    cuadro que ya quedo fijado por un clip anterior entra con su eleccion."""
    fixed = {}
    for w in weapons:
        cf = cfs[w]
        for name, frames in cf.clips.items():
            seq = []
            for f in frames:
                sg = sigs_w[w].get(f)
                if sg in an and (not seq or seq[-1] != sg):
                    seq.append(sg)
            # tramos con candidatas
            runs, cur = [], []
            for sg in seq:
                if fixed.get(sg, 0) is None or (sg not in fixed and not an[sg]["info"]["head_cands"]):
                    if cur:
                        runs.append(cur); cur = []
                    continue
                cur.append(sg)
            if cur:
                runs.append(cur)
            for run in runs:
                C = [[fixed[sg]] if sg in fixed else an[sg]["info"]["head_cands"] for sg in run]
                U = []
                for sg, cs in zip(run, C):
                    best = an[sg]["info"]["head_cands"][0]["score"] if an[sg]["info"]["head_cands"] else cs[0]["score"]
                    U.append([0.0 if sg in fixed else _unary(c, best) for c in cs])
                cost = [U[0][:]]; back = [[-1] * len(C[0])]
                for i in range(1, len(run)):
                    ci, bi = [], []
                    for j, c in enumerate(C[i]):
                        vals = [cost[i - 1][k] + _pair(p, c) for k, p in enumerate(C[i - 1])]
                        k = int(np.argmin(vals)); ci.append(vals[k] + U[i][j]); bi.append(k)
                    cost.append(ci); back.append(bi)
                j = int(np.argmin(cost[-1]))
                for i in range(len(run) - 1, -1, -1):
                    if run[i] not in fixed:
                        fixed[run[i]] = C[i][j]
                    j = back[i][j]
    for sg, a in an.items():
        if sg not in fixed:
            hc = a["info"]["head_cands"]
            fixed[sg] = hc[0] if hc else None
        set_head(a, fixed[sg], tpls)


# ---------------------------------------------------------------- pase 2: coherencia temporal
def clip_neighbours(cf, sigs, dist=1):
    """Vecinos temporales de cada cuadro (por firma): cuadros a <= dist pasos dentro
    de cada clip (los clips en bucle se cierran)."""
    nb = {}
    for name, frames in cf.clips.items():
        loop = False
        for l in cf.lines:
            t = l.split()
            if len(t) > 2 and t[0] == "clip" and t[1] == name:
                loop = t[2] == "1"; break
        fr = []
        for f in frames:
            if sigs.get(f) and (not fr or fr[-1] != f):
                fr.append(f)
        n = len(fr)
        loop = loop and n > 2 and fr[0] != fr[-1]
        for i in range(n):
            for d in range(1, dist + 1):
                j = i + d
                if j >= n:
                    if not loop:
                        break
                    j %= n
                sa, sb = sigs[fr[i]], sigs[fr[j]]
                if sa != sb:
                    nb.setdefault(sa, set()).add(sb); nb.setdefault(sb, set()).add(sa)
    return nb


def smooth_heads(an, nb):
    """Media movil (1-2-1) de centro y giro de la cabeza entre cuadros vecinos del
    clip, con tope para no despegarla del cuello."""
    out = {}
    for s, a in an.items():
        h = a["head"]
        if h is None:
            continue
        xs, ys, angs, ws = [2 * h["x"]], [2 * h["y"]], [2 * h["ang"]], 2.0
        for t in nb.get(s, ()):
            g = an[t]["head"] if t in an else None
            if g is None or g["facing_left"] != h["facing_left"]:
                continue
            if abs(g["x"] - h["x"]) > 14 or abs(g["y"] - h["y"]) > 14:
                continue
            xs.append(g["x"]); ys.append(g["y"]); angs.append(g["ang"]); ws += 1
        mx, my, ma = sum(xs) / ws, sum(ys) / ws, sum(angs) / ws
        out[s] = dict(h, x=h["x"] + float(np.clip(mx - h["x"], -HEAD_POS_TOL, HEAD_POS_TOL)),
                      y=h["y"] + float(np.clip(my - h["y"], -HEAD_POS_TOL, HEAD_POS_TOL)),
                      ang=h["ang"] + float(np.clip(ma - h["ang"], -HEAD_ANG_TOL, HEAD_ANG_TOL)))
    return out


def _shift_into(arr, src_off, dst_off, shape, fill):
    """arr (lienzo de otro cuadro) llevado al lienzo del cuadro destino por el origen comun."""
    out = np.full(shape + arr.shape[2:], fill, arr.dtype)
    dx, dy = src_off[0] - dst_off[0], src_off[1] - dst_off[1]
    H, W = shape
    h, w = arr.shape[:2]
    x0, y0 = max(0, dx), max(0, dy)
    x1, y1 = min(W, dx + w), min(H, dy + h)
    if x1 > x0 and y1 > y0:
        out[y0:y1, x0:x1] = arr[y0 - dy:y1 - dy, x0 - dx:x1 - dx]
    return out


VOTE = [SHIRT, HAND, SASH, PANTS, FOOT]


def flow_to(a, b, r=4):
    """Correspondencia del cuadro a al cuadro b (en el lienzo de a, por el origen
    comun): cada pixel de a busca en b (+-r px) el trozo de Kratos de color mas
    parecido.  Devuelve (dy, dx, ok)."""
    shape = a["can"].shape[:2]
    kb = _shift_into(b["can"], b["off"], a["off"], shape, 0).astype(np.float32)
    ka = a["can"].astype(np.float32)
    best = np.full(shape, np.inf, np.float32)
    by = np.zeros(shape, np.int8); bx = np.zeros(shape, np.int8)
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            ks = np.roll(np.roll(kb, -dy, 0), -dx, 1)
            d = ((ka[..., :3] - ks[..., :3]) ** 2).sum(-1) + np.where(ks[..., 3] > 100, 0, 1e6)
            ssd = ndimage.uniform_filter(d, 3) + 3.0 * (dx * dx + dy * dy)
            upd = ssd < best
            best = np.where(upd, ssd, best); by = np.where(upd, dy, by); bx = np.where(upd, dx, bx)
    return by, bx, best < 3 * 22.0 ** 2


def warp_labels(a, b, flow):
    """Etiquetas del cuadro b llevadas al cuadro a con la correspondencia `flow`."""
    shape = a["can"].shape[:2]
    lb = _shift_into(b["labels"], b["off"], a["off"], shape, BG)
    by, bx, ok = flow
    H, W = shape
    yy, xx = np.mgrid[0:H, 0:W]
    out = lb[(yy + by) % H, (xx + bx) % W]
    return np.where(ok, out, -1)


def vote_labels(an, nb, iters=1):
    """Mediana temporal de las etiquetas: si los vecinos del clip coinciden (con
    compensacion de movimiento) en otra parte, el pixel se corrige.  Misma zona de
    Kratos -> mismo color de RONIN en cuadros seguidos."""
    flows = {}
    total = set()
    for _ in range(iters):
        total |= _vote_once(an, nb, flows)
    return len(total)


def _vote_once(an, nb, flows):
    new = {}
    for s, a in an.items():
        lab = a["labels"]
        nbs = [an[t] for t in nb.get(s, ()) if t in an]
        if not nbs:
            continue
        votes = np.zeros((len(VOTE),) + lab.shape, np.float32)
        own = np.isin(lab, VOTE)
        for i, L in enumerate(VOTE):
            votes[i] += (lab == L) * 1.5
        for t in nb.get(s, ()):
            if t not in an:
                continue
            b = an[t]
            if (s, t) not in flows:
                flows[(s, t)] = flow_to(a, b)
            wl = warp_labels(a, b, flows[(s, t)])
            for i, L in enumerate(VOTE):
                votes[i] += (wl == L)
        win = np.array(VOTE, np.int8)[votes.argmax(0)]
        # solo cambia si gana con ventaja (lo propio pesa 1.5)
        top = votes.max(0)
        mine = np.zeros(lab.shape, np.float32)
        for i, L in enumerate(VOTE):
            mine = np.where(lab == L, votes[i], mine)
        ch = own & (top > mine + 0.5) & (top >= 2)
        if ch.any():
            lab2 = lab.copy(); lab2[ch] = win[ch]
            new[s] = lab2
    for s, lab2 in new.items():
        an[s]["labels"] = lab2
    return set(new)


# ---------------------------------------------------------------- pase 3: pintar y recortar
def render_frame(a, R, heads, shirt, ranges, head):
    can, placed, labels, info = a["can"], a["placed"], a["labels"], a["info"]
    drop = info["drop"]
    A = info["M"]["a"] & ~drop
    out = paint(can, labels, info, R, shirt, ranges)
    out, cut = thin_and_outline(out, labels, A & ~drop)
    cut = cut | drop
    keep = labels == KEEP
    out[keep] = can[keep]
    spr = None
    hmask = info["head"]["mask"] if info["head"] is not None else None
    if head is not None:
        k = head["k"]
        t = math.radians(head["ang"])
        hoff = HEAD_OFF_BACK if a["which"] == "back" else HEAD_OFF
        ox, oy = hoff[0] * head["w"] * (-1 if head["facing_left"] else 1), hoff[1] * head["h"]
        tx = head["x"] - a["off"][0] + ox * math.cos(t) + oy * math.sin(t)
        ty = head["y"] - a["off"][1] - ox * math.sin(t) + oy * math.cos(t)
        spr = render_head(heads, a["which"], k, head["ang"], head["facing_left"], tx, ty)
        # lo que sobra de la calva de Kratos fuera de la cabeza de RONIN (y lejos del
        # cuello/hombros) se quita: si no, asoma un bulto color piel detras del pelo
        if hmask is not None:
            cov = np.zeros(hmask.shape, bool)
            sp, sx0, sy0 = spr
            H, W = hmask.shape
            ys0, xs0 = max(0, sy0), max(0, sx0)
            ys1, xs1 = min(H, sy0 + sp.shape[0]), min(W, sx0 + sp.shape[1])
            if ys1 > ys0 and xs1 > xs0:
                cov[ys0:ys1, xs0:xs1] = sp[ys0 - sy0:ys1 - sy0, xs0 - sx0:xs1 - sx0, 3] > 0
            rest = A & ~hmask
            far = ndimage.distance_transform_edt(~rest) > 2
            cut = cut | (hmask & (labels == HEAD) & ~cov & far)
    host = None
    if hmask is not None:
        best = -1
        for j, (p, X, Y, o) in enumerate(placed):
            m = np.zeros_like(hmask); m[Y:Y + o.shape[0], X:X + o.shape[1]] = o[..., 3] > 60
            c = (m & hmask).sum()
            if c > best:
                best, host = c, j
    res = {}
    for j, (p, X, Y, o) in enumerate(placed):
        hh, ww = o.shape[:2]
        tile = np.zeros((hh + 2 * PAD, ww + 2 * PAD, 4), np.float32)
        reg = out[Y:Y + hh, X:X + ww]
        own = o[..., 3] > 0
        tt = tile[PAD:PAD + hh, PAD:PAD + ww]
        tt[..., :3] = np.where(own[..., None], reg[..., :3], 0)
        # alfa: la del original (respeta piezas semitransparentes) salvo lo adelgazado
        tt[..., 3] = np.where(own & ~cut[Y:Y + hh, X:X + ww], o[..., 3], 0)
        if spr is not None and j == host:
            sp, sx0, sy0 = spr
            over(tile, sp, sx0 - X + PAD, sy0 - Y + PAD)
        res[pkey(cf_key_of(p, a))] = (p["flip"], tile.clip(0, 255).astype(np.uint8))
    return res, out


def cf_key_of(p, a):
    return a["keys"][id(p)]


def process_frame(cf, fi, body_keys, R, heads, exc, wname, shirt="crimson", debug=False, tpls=None):
    """Un cuadro suelto (sin coherencia temporal): para revisiones rapidas."""
    a = analyze(cf, fi, body_keys, exc, wname, tpls)
    if a is None:
        return {}, None
    a["keys"] = {id(p): cf.key(p) for p, *_ in a["placed"]}
    hc = a["info"]["head_cands"]
    set_head(a, hc[0] if hc else None, tpls)
    head = None
    if a["head"] is not None:
        head = dict(a["head"], k=head_scale(heads, a["which"], KRATOS_HEAD_W))
    res, out = render_frame(a, R, heads, shirt, None, head)
    return res, ((a["can"], a["labels"], out, a["info"]) if debug else None)


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
    """Pase 1: analiza cada cuadro distinto (las 4 armas comparten muchos).
    Pase 2: coherencia temporal dentro de cada clip (etiquetas y cabeza).
    Pase 3: pinta, pega la cabeza de RONIN y recorta las piezas."""
    global KRATOS_HEAD_W
    exc, body_keys, R, heads, cfs, tpls = setup()
    KRATOS_HEAD_W = float(np.median([t["img"].shape[1] for t in tpls]))
    an, sigs_w = {}, {}
    for w in weapons:
        cf = cfs[w]
        sigs = {}
        for fi in range(len(cf.frames)):
            s = frame_sig(cf, fi, body_keys)
            if not s:
                continue
            if s not in an:
                a = analyze(cf, fi, body_keys, exc, w, tpls)
                if a is None:
                    continue
                a["keys"] = {id(p): cf.key(p) for p, *_ in a["placed"]}
                an[s] = a
            sigs[fi] = s
        sigs_w[w] = sigs
    choose_heads(an, cfs, sigs_w, weapons, tpls)
    nb, nbv = {}, {}
    for w in weapons:
        for s, t in clip_neighbours(cfs[w], sigs_w[w]).items():
            nb.setdefault(s, set()).update(t)
        for s, t in clip_neighbours(cfs[w], sigs_w[w], VOTE_DIST).items():
            nbv.setdefault(s, set()).update(t)
    changed = vote_labels(an, nbv, VOTE_ITERS)
    for a in an.values():
        a["labels"] = clean_islands(a["labels"])
    samples = {}
    for a in an.values():
        M = a["info"]["M"]
        lum = lum_of(a["can"], M)
        for L in RAMP_OF:
            sel_l = a["labels"] == L
            for mat in ("skin", "warm", "dark"):
                v = lum[sel_l & mat_sel(M, mat)]
                if len(v):
                    samples.setdefault((L, mat), []).append(v[::2])
    ranges = lum_ranges(samples)
    hs = smooth_heads(an, nb)
    tiles, ctx, track = {}, {}, {}
    for s, a in an.items():
        if all(pkey(k) in tiles for k in a["keys"].values()):
            continue
        head = hs.get(s)
        if head is not None:
            head = dict(head, k=head_scale(heads, a["which"], KRATOS_HEAD_W))
        res, _ = render_frame(a, R, heads, shirt, ranges, head)
        for k, (flip, t) in res.items():
            if k not in tiles:
                tiles[k] = unorient(t, flip)
                ctx[k] = a["key"]
    # pista de la cabeza por cuadro (arma:cuadro): Kratos (elegida) y RONIN (suavizada)
    for w in weapons:
        for fi, sg in sigs_w[w].items():
            a = an.get(sg)
            if a is not None and a["head"] is not None and sg in hs:
                h, g = a["head"], hs[sg]
                track[f"{w}:{fi}"] = dict(kratos=[round(h["x"], 2), round(h["y"], 2), h["ang"], h["facing_left"]],
                                          ronin=[round(g["x"], 2), round(g["y"], 2), round(g["ang"], 2)])
    place, sheets = pack(tiles)
    if write:
        for n, S in enumerate(sheets):
            Image.fromarray(S, "RGBA").save(ACTOR / f"{FIRST_ID + n}.png", optimize=True)
        for w in weapons:
            write_txt(cfs[w], w, place, len(sheets))
        OUT = HERE / "out"
        OUT.mkdir(exist_ok=True)
        (OUT / "head_track.json").write_text(json.dumps(track, indent=0))
    print(f"cuadros analizados {len(an)}, corregidos por coherencia temporal {changed}, cabezas {len(hs)}")
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
