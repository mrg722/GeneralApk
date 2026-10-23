#!/usr/bin/env python3
"""RONIN: arma sus cuadros (assets/kratos/actor/9xxx.png) y data/kratos/ronin.txt
a partir de las hojas del usuario.

Uso:
    python3 -I tools/ronin/build_ronin.py <carpeta_hojas> [--audit <carpeta>] [--montage <carpeta>] [--dry]

<carpeta_hojas> debe tener:
    sheet_a.png  sheet_b.png  sheet_c.png      (poses: combos, patadas, salto, guardias)
    ki_carga_a.png  ki_carga_b.png             (carga de ki: manos arriba -> empuje)
    fx_ki.png                                  (esfera / rayo / explosion, fondo negro)
    fx_gold.png                                (efectos dorados: estelas, medialunas, polvo)
    manoespada_reposo.png  manoespada_swing.png (mano con espada; o --sword <carpeta>)

Pasos (todo con logica, nada a ojo salvo la eleccion de que cuadro va en que clip):
 1. Cada hoja se separa en figuras por componente conectado (alpha>128, 8-vecinos).
    Las figuras que se tocan se separan con una costura vertical de costo minimo
    (programacion dinamica tipo seam carving) que cruza la menor cantidad de pixeles
    opacos; los pixeles semitransparentes del borde van a la figura opaca mas cercana
    y los trocitos sueltos (<1500 px) a la figura mas cercana si estan a <=14 px.
 2. Escala: por fila de cada hoja, la guardia de referencia (alto total, pies ->
    punta del pelo) se lleva a GUARD_H px (= 15% menos que el RONIN anterior; sin
    las puntas del pelo mide ~110); el eje horizontal se multiplica ademas por
    SLIM (adelgazar levemente). Antes se limpia el borde (dehalo).
 3. Pies: y = fila opaca mas baja. Con los dos pies en el suelo, x = talon del pie
    de atras + K (K medido en la guardia de la misma fila), asi el pie de apoyo no
    salta entre cuadros de un mismo golpe. Con un pie (patadas) o en el aire, el
    ancla sale del centro del abdomen (franja azul sobre el cinturon) + T.
 4. Cajas: cuerpo = torso+piernas (sin pelo, sin cinta); ataque = la parte del
    sprite que sobresale delante del torso (puno/pie extendido) en los cuadros de
    impacto, o la extension visible del rayo/explosion de ki.
"""
import argparse
import json
import os
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage as ndi

GUARD_H = 100          # alto total de la guardia en px de cuadro (x1.25 en pantalla); ~ alto del Punk
SLIM = 0.90            # factor horizontal (mas delgado)
FIRST_ID = 9100        # ids de imagen de RONIN: 9100..
SHEETS = ["sheet_a", "sheet_b", "sheet_c", "ki_carga_a", "ki_carga_b"]
# figuras esperadas por fila (para separar las que se tocan)
EXPECTED = {"sheet_a": [6, 6, 6, 6, 5, 5],"sheet_b": [5, 5, 5, 5], "sheet_c": [6, 6, 6, 6, 6],
            "ki_carga_a": [9, 7, 7], "ki_carga_b": [9, 7, 6, 7]}


# ---------------------------------------------------------------- extraccion
def seam_split(mask, x_lo, x_hi):
    """Costura vertical (una x por fila) entre x_lo y x_hi que cruza la menor
    cantidad de pixeles opacos. Devuelve un array con la x de corte por fila."""
    h = mask.shape[0]
    xs = np.arange(x_lo, x_hi)
    cost = mask[:, x_lo:x_hi].astype(np.float64) * 100.0 + 0.01
    acc = cost.copy()
    back = np.zeros(cost.shape, dtype=np.int64)
    w = cost.shape[1]
    for y in range(1, h):
        prev = acc[y - 1]
        cand = np.stack([np.r_[np.inf, prev[:-1]], prev, np.r_[prev[1:], np.inf]])
        arg = np.argmin(cand, axis=0)
        acc[y] += cand[arg, np.arange(w)]
        back[y] = np.arange(w) + arg - 1
    seam = np.zeros(h, dtype=np.int64)
    seam[-1] = int(np.argmin(acc[-1]))
    for y in range(h - 1, 0, -1):
        seam[y - 1] = back[y, seam[y]]
    return xs[seam]


def extract_figures(path, expected=None):
    im = np.array(Image.open(path).convert("RGBA"))
    alpha = im[..., 3]
    core = alpha > 128
    lab, n = ndi.label(core, structure=np.ones((3, 3)))
    areas = ndi.sum(core, lab, index=np.arange(1, n + 1))
    big = [i + 1 for i in range(n) if areas[i] >= 1500]
    # filas: agrupar por centro vertical
    objs = ndi.find_objects(lab)
    figs = []
    for i in big:
        s = objs[i - 1]
        figs.append({"lab": i, "y0": s[0].start, "y1": s[0].stop, "x0": s[1].start, "x1": s[1].stop})
    # filas (por el borde inferior) y, por fila, cuantas figuras faltan: los
    # componentes mas anchos se reparten esas figuras extra (se tocan entre si).
    hs = sorted(f["y1"] - f["y0"] for f in figs)
    med_h = hs[len(hs) // 2]
    figs.sort(key=lambda f: f["y1"])
    rows, cur = [], []
    for f in figs:
        if cur and f["y1"] - cur[-1]["y1"] > 0.4 * med_h:
            rows.append(cur)
            cur = []
        cur.append(f)
    if cur:
        rows.append(cur)
    for ri, row in enumerate(rows):
        for f in row:
            f["k"] = 1
        if expected and ri < len(expected):
            for _ in range(max(0, expected[ri] - len(row))):
                worst = max(row, key=lambda f: (f["x1"] - f["x0"]) / f["k"])
                worst["k"] += 1
    next_lab = n + 1
    out = []
    for f in figs:
        w = f["x1"] - f["x0"]
        k = f["k"]
        if k >= 2:
            sub = lab[f["y0"]:f["y1"], f["x0"]:f["x1"]] == f["lab"]
            cuts = []
            for j in range(1, k):
                ex = int(j * w / k)
                lo, hi = max(1, ex - int(0.3 * w / k)), min(w - 1, ex + int(0.3 * w / k))
                cuts.append(seam_split(sub, lo, hi))
            xx = np.arange(w)[None, :]
            part = np.zeros(sub.shape, dtype=np.int64)
            for c in cuts:
                part += xx >= c[:, None]
            for j in range(k):
                m = sub & (part == j)
                if m.sum() < 1500:
                    continue
                region = lab[f["y0"]:f["y1"], f["x0"]:f["x1"]]
                region[m] = next_lab
                ys, xs = np.nonzero(m)
                out.append({"lab": next_lab, "y0": f["y0"] + ys.min(), "y1": f["y0"] + ys.max() + 1,
                            "x0": f["x0"] + xs.min(), "x1": f["x0"] + xs.max() + 1})
                next_lab += 1
        else:
            out.append(f)
    figs = out
    # trocitos sueltos -> figura mas cercana (<=14 px); semitransparentes -> idem (<=3 px)
    fig_labels = np.zeros_like(lab)
    for idx, f in enumerate(figs):
        fig_labels[lab == f["lab"]] = idx + 1
    small = (lab > 0) & (fig_labels == 0)
    dist, (iy, ix) = ndi.distance_transform_edt(fig_labels == 0, return_indices=True)
    near = fig_labels[iy, ix]
    attach_small = small & (dist <= 14)
    fig_labels[attach_small] = near[attach_small]
    soft = (alpha > 0) & (fig_labels == 0)
    dist, (iy, ix) = ndi.distance_transform_edt(fig_labels == 0, return_indices=True)
    near = fig_labels[iy, ix]
    attach_soft = soft & (dist <= 3)
    fig_labels[attach_soft] = near[attach_soft]
    # recorte final de cada figura (solo sus pixeles)
    result = []
    for idx, f in enumerate(figs):
        m = fig_labels == idx + 1
        ys, xs = np.nonzero(m)
        y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
        crop = im[y0:y1, x0:x1].copy()
        crop[~m[y0:y1, x0:x1]] = 0
        result.append({"img": crop, "x0": int(x0), "y0": int(y0), "cx": (x0 + x1) / 2, "cy": (y0 + y1) / 2,
                       "h": int(y1 - y0)})
    # filas por centro vertical (corte cuando el salto supera 40% del alto mediano)
    hs = sorted(r["h"] for r in result)
    med_h = hs[len(hs) // 2]
    result.sort(key=lambda r: r["y0"] + r["h"])
    rows, cur = [], []
    for r in result:
        bottom = r["y0"] + r["h"]
        if cur and bottom - (cur[-1]["y0"] + cur[-1]["h"]) > 0.4 * med_h:
            rows.append(cur)
            cur = []
        cur.append(r)
    if cur:
        rows.append(cur)
    named = {}
    for ri, row in enumerate(rows):
        row.sort(key=lambda r: r["cx"])
        if expected and ri < len(expected) and len(row) != expected[ri]:
            print("  aviso: %s fila %d tiene %d figuras (esperaba %d)" % (os.path.basename(path), ri, len(row), expected[ri]))
        for ci, r in enumerate(row):
            named["r%dc%d" % (ri, ci)] = r
    return named


def montage(named, path, cols=8, cell=200):
    keys = sorted(named, key=lambda k: (int(k[1:k.index("c")]), int(k[k.index("c") + 1:])))
    rows = (len(keys) + cols - 1) // cols
    canvas = Image.new("RGB", (cols * cell, rows * cell), (0, 140, 0))
    d = ImageDraw.Draw(canvas)
    for i, k in enumerate(keys):
        im = Image.fromarray(named[k]["img"])
        s = min((cell - 16) / im.width, (cell - 16) / im.height)
        im = im.resize((max(1, int(im.width * s)), max(1, int(im.height * s))), Image.LANCZOS)
        x, y = (i % cols) * cell, (i // cols) * cell
        canvas.paste(im, (x + 8, y + 14), im)
        d.text((x + 3, y + 1), k, fill=(255, 255, 0))
    canvas.save(path)




# ---------------------------------------------------------------- utilidades
def resize_rgba(arr, w, h):
    """Escala con alfa premultiplicado (sin halos oscuros en el borde)."""
    im = Image.fromarray(arr, "RGBA").convert("RGBa")
    im = im.resize((max(1, int(round(w))), max(1, int(round(h)))), Image.LANCZOS)
    out = np.array(im.convert("RGBA"))
    out[out[..., 3] < 8] = 0
    return out


def masks(img):
    a = img[..., 3] > 128
    rgb = img[..., :3].astype(np.int32)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    mx = rgb.max(-1)
    mn = rgb.min(-1)
    sat = (mx - mn) / np.maximum(mx, 1)
    navy = a & (mx < 125) & (b > r + 10)
    belt = a & (r > 160) & (g > 110) & (b < 95) & (r - b > 100)
    skin = a & (r > 160) & (r > g + 18) & (g > b + 8) & (b >= 80)
    light = a & (mx > 140) & (sat < 0.25)
    return a, navy, skin, belt, light


def crop_alpha(img, thr=0):
    ys, xs = np.nonzero(img[..., 3] > thr)
    if len(ys) == 0:
        return img[:1, :1], 0, 0
    return img[ys.min():ys.max() + 1, xs.min():xs.max() + 1], int(xs.min()), int(ys.min())


def comp_centroids(mask, min_area=12):
    lab, n = ndi.label(ndi.binary_dilation(mask, iterations=1))
    out = []
    for i in range(1, n + 1):
        m = (lab == i) & mask
        s = m.sum()
        if s < min_area:
            continue
        ys, xs = np.nonzero(m)
        out.append({"cx": xs.mean(), "cy": ys.mean(), "x0": xs.min(), "x1": xs.max(),
                    "y0": ys.min(), "y1": ys.max(), "n": s})
    return out


def dehalo(img, opaque=128):
    """Halo de fondo en el borde (sheet_c): el color de cada pixel semitransparente
    pasa a ser el del pixel opaco mas cercano (cKDTree); el alfa no se toca."""
    from scipy.spatial import cKDTree
    a = img[..., 3]
    op = np.argwhere(a >= opaque)
    semi = np.argwhere((a > 0) & (a < opaque))
    if len(op) == 0 or len(semi) == 0:
        return img
    _, idx = cKDTree(op).query(semi)
    out = img.copy()
    src = op[idx]
    out[semi[:, 0], semi[:, 1], :3] = img[src[:, 0], src[:, 1], :3]
    return out


def edge_contrast(img, opaque=128):
    """Medida de halo: cuanto mas claro que su pixel opaco vecino es el borde (0 = sin halo)."""
    a = img[..., 3]
    semi = (a > 0) & (a < opaque)
    _, (iy, ix) = ndi.distance_transform_edt(a < opaque, return_indices=True)
    lum = img[..., :3].astype(int).max(-1)
    near = img[iy, ix, :3].astype(int).max(-1)
    d = (lum - near)[semi]
    return float(np.clip(d, 0, None).mean()) if d.size else 0.0


# ---------------------------------------------------------------- figuras normalizadas
class Body:
    """Una figura de las hojas ya escalada, con sus medidas (en px de juego)."""

    def __init__(self, sheet, key, src, scale):
        h, w = src.shape[:2]
        self.sheet, self.key = sheet, key
        self.img = resize_rgba(src, w * scale * SLIM, h * scale)
        a, navy, skin, belt, light = masks(self.img)
        self.a, self.navy, self.skin, self.belt, self.light = a, navy, skin, belt, light
        ys, xs = np.nonzero(a)
        self.top, self.bottom = int(ys.min()), int(ys.max())
        rows = a[max(0, self.bottom - 5):self.bottom + 1]
        cols = np.nonzero(rows.any(0))[0]
        self.heel, self.toe = int(cols.min()), int(cols.max())
        ny, nx = np.nonzero(navy)
        self.navy_cx = float(nx.mean()) if len(nx) else float(xs.mean())
        self.navy_top = int(ny.min()) if len(ny) else self.top
        by, bx = np.nonzero(belt)
        self.belt_top = int(np.percentile(by, 10)) if len(by) else (self.navy_top + 40)
        band = navy[max(0, self.belt_top - 14):self.belt_top + 1]
        bc = np.nonzero(band.any(0))[0]
        self.torso_front = int(bc.max()) if len(bc) else int(self.navy_cx + 10)
        # centro del abdomen (franja azul justo encima del cinturon: sin brazos ni cinta)
        bys, bxs = np.nonzero(band)
        self.torso_cx = float(bxs.mean()) if len(bxs) else self.navy_cx
        # pies en el suelo: grupos separados en las 5 filas de abajo
        lab, n = ndi.label(rows.any(0))
        self.feet_down = n

    def hands(self, where):
        """Manos: 'top' = las mas altas (manos arriba), 'front' = las de mas adelante.
        Devuelve (x, y) del borde util (arriba o adelante) del grupo de piel."""
        sk = self.skin.copy()
        sk[int(self.bottom - 0.18 * (self.bottom - self.top)):] = False      # sin pies
        cs = comp_centroids(sk)
        if not cs:
            return (float(self.toe), float(self.top))
        if where == "top":
            c = min(cs, key=lambda c: c["y0"])
            return (c["cx"], float(c["y0"]))
        if where == "chest":
            cs2 = [c for c in cs if c["cy"] > self.navy_top] or cs
            c = max(cs2, key=lambda c: c["x1"])
            return (c["cx"], c["cy"])
        c = max(cs, key=lambda c: c["x1"])
        return (float(c["x1"]), c["cy"])


def guard_height(img):
    a = img[..., 3] > 128
    ys = np.nonzero(a.any(1))[0]
    return ys.max() - ys.min() + 1


# ---------------------------------------------------------------- efectos
def fx_from_black(rgb):
    """Brillo aditivo sobre negro -> RGBA: alpha = max(R,G,B), color des-premultiplicado."""
    rgb = rgb.astype(np.float64)
    al = rgb.max(-1)
    col = np.where(al[..., None] > 0, rgb * 255.0 / np.maximum(al[..., None], 1), 0)
    out = np.dstack([np.clip(col, 0, 255), al]).astype(np.uint8)
    out[al < 14] = 0
    return out


def split_columns(band_alpha, k):
    """Corta una banda de efectos en k columnas por los minimos de su perfil."""
    prof = ndi.uniform_filter1d((band_alpha > 40).sum(0).astype(float), 21)
    xs = np.nonzero(prof > 0.5)[0]
    x0, x1 = xs.min(), xs.max() + 1
    w = x1 - x0
    cuts = [x0]
    for j in range(1, k):
        ex = x0 + j * w / k
        lo, hi = int(ex - 0.3 * w / k), int(ex + 0.3 * w / k)
        cuts.append(lo + int(np.argmin(prof[lo:hi])))
    cuts.append(x1)
    return list(zip(cuts[:-1], cuts[1:]))


def load_fx_ki(path):
    rgba = fx_from_black(np.array(Image.open(path).convert("RGB")))
    al = rgba[..., 3]
    bands = [(0, 215, 9, True), (215, 455, 7, False), (455, 700, 6, False), (700, 1000, 6, False)]
    cells = []
    for y0, y1, k, by_comp in bands:
        band = rgba[y0:y1]
        # primero por componentes (las chispas sueltas van con la etapa mas cercana);
        # si las etapas se tocan, por los minimos del perfil de columnas.
        lab, n = ndi.label(ndi.binary_dilation(band[..., 3] > 40, iterations=4))
        objs = ndi.find_objects(lab)
        areas = ndi.sum(band[..., 3] > 40, lab, index=np.arange(1, n + 1))
        seeds = sorted(np.argsort(areas)[::-1][:k], key=lambda i: objs[i][1].start)
        if by_comp and n >= k:
            cx = [(objs[i][1].start + objs[i][1].stop) / 2 for i in seeds]
            owner = np.zeros(n + 1, dtype=np.int64)
            for i in range(n):
                c = (objs[i][1].start + objs[i][1].stop) / 2
                owner[i + 1] = int(np.argmin([abs(c - x) for x in cx]))
            for j in range(k):
                part = band.copy()
                part[(owner[lab] != j) | (lab == 0)] = 0
                cell, _, _ = crop_alpha(part, 20)
                cells.append(cell)
        else:
            for c0, c1 in split_columns(band[..., 3], k):
                cell, _, _ = crop_alpha(band[:, c0:c1], 20)
                cells.append(cell)
    return {"sphere": cells[:16], "beam": cells[16:22], "expl": cells[22:28]}


# Regiones de la hoja de efectos dorados (1536x1024), por fila de la hoja.
GOLD_REGIONS = {
    "fist": [(15, 80, 150, 165), (170, 80, 340, 165), (350, 40, 590, 170), (610, 10, 900, 195),
             (20, 265, 210, 350), (220, 245, 400, 350), (405, 240, 670, 365), (680, 205, 955, 400)],
    "moon": [(950, 60, 1010, 120), (1010, 30, 1090, 170), (1080, 20, 1200, 200), (1195, 20, 1340, 210),
             (1340, 0, 1530, 225), (985, 285, 1140, 400), (1030, 225, 1330, 460), (1250, 225, 1530, 460)],
    "streak": [(1110, 470, 1330, 530)],
    "spin": [(1310, 480, 1530, 645)],
    "ghost": [(20, 400, 240, 520), (240, 400, 400, 520), (400, 400, 560, 520), (550, 430, 680, 520),
              (650, 430, 790, 520), (770, 430, 900, 520)],
    "dust": [(15, 560, 150, 650), (155, 560, 330, 655), (340, 565, 500, 655), (505, 555, 740, 655),
             (745, 555, 900, 655), (855, 590, 960, 660), (955, 590, 1065, 660), (1070, 590, 1260, 660)],
    "star": [(15, 730, 85, 810), (95, 735, 180, 815), (195, 710, 300, 830), (310, 690, 450, 840),
             (460, 690, 620, 840), (630, 670, 890, 860)],
    "flare": [(905, 680, 1005, 840), (1010, 680, 1110, 840), (1120, 680, 1250, 845), (1240, 680, 1370, 845),
              (1365, 665, 1515, 855)],
    "ring": [(25, 925, 150, 955), (165, 905, 385, 965), (395, 880, 780, 980), (790, 875, 1170, 990),
             (1170, 875, 1525, 990)],
}


def load_fx_gold(path):
    im = np.array(Image.open(path).convert("RGBA"))
    # cada efecto = su region; dentro de ella se descartan los pedazos que tocan el
    # borde de la region y son chicos (son del efecto vecino, no de este)
    out = {}
    for name, regs in GOLD_REGIONS.items():
        out[name] = []
        for (x0, y0, x1, y1) in regs:
            part = im[y0:y1, x0:x1].copy()
            lab, n = ndi.label(part[..., 3] > 20, structure=np.ones((3, 3)))
            if n:
                areas = ndi.sum(part[..., 3] > 20, lab, index=np.arange(1, n + 1))
                big = areas.max()
                for i, o in enumerate(ndi.find_objects(lab)):
                    edge = o[0].start == 0 or o[1].start == 0 or o[0].stop == part.shape[0] or o[1].stop == part.shape[1]
                    if edge and areas[i] < 0.3 * big:
                        part[lab == i + 1] = 0
            cell, _, _ = crop_alpha(part, 20)
            out[name].append(cell)
    return out


# ---------------------------------------------------------------- cuadros y clips
# Guardia de referencia por FILA de cada hoja (las hojas no tienen la misma escala
# entre filas: p.ej. la fila 3 de sheet_b esta dibujada ~12% mas chica que la 0).
# Su alto total se lleva a GUARD_H; K = talon de atras -> centro entre los pies.
# Filas sin guardia usan la de otra fila: (hoja, fila) -> figura de referencia.
REF = {("a", 0): "r0c0", ("a", 1): "r1c0", ("a", 2): "r2c0", ("a", 3): "r3c0", ("a", 4): "r4c0", ("a", 5): "r4c0",
       ("a", 6): "r4c0", ("a", 7): "r4c0",
       ("b", 0): "r0c0", ("b", 1): "r1c0", ("b", 2): "r2c0", ("b", 3): "r3c0",
       # sheet_c fila 4: misma cabeza que la fila 0 pero cuerpo mas bajo -> escala de la fila 0
       ("c", 0): "r0c0", ("c", 1): "r1c0", ("c", 2): "r2c0", ("c", 3): "r3c0", ("c", 4): "r0c0",
       ("ka", 0): "r0c0", ("ka", 1): "r0c0", ("ka", 2): "r2c6",
       # ki_carga_b: la guardia de la fila 0 esta erguida (otra postura); todas usan la final
       ("kb", 0): "r3c6", ("kb", 1): "r3c6", ("kb", 2): "r3c6", ("kb", 3): "r3c6"}


def row_of(key):
    return int(key[1:key.index("c")])


def F(src, t, hit=False, anchor="heel", lift=0, dx=0, fx=(), body=True, note=""):
    sheet, key = src.split(":")
    return {"sheet": sheet, "key": key, "t": t, "hit": hit, "anchor": anchor, "lift": lift,
            "dx": dx, "fx": list(fx), "body": body}


def ki_wave(sh, charge, push, hold, back, t=0.05):
    """Onda de ki: esfera creciendo sobre las manos (16 etapas repartidas en los
    cuadros de carga), empuje de palmas con el rayo (6 etapas) y explosion (6
    etapas) en la punta del rayo; despues vuelve a guardia."""
    frames = []
    n = len(charge)
    for i, (k, where) in enumerate(charge):
        st = int(round(i * 15 / max(1, n - 1)))
        frames.append(F("%s:%s" % (sh, k), t, fx=[("sphere", st, where)]))
    for i, k in enumerate(push):
        frames.append(F("%s:%s" % (sh, k), t, hit=True, fx=[("beam", i, "palm")]))
    for i in range(6):
        frames.append(F("%s:%s" % (sh, hold), t, hit=True, fx=[("expl", i, "tip")]))
    for k in back:
        frames.append(F("%s:%s" % (sh, k), 0.07))
    return frames


# carga en orden: (cuadro, donde va la esfera: "chest" = entre las palmas, "top" = sobre las manos)
KI_A = [F("ka:r0c0", 0.04)] + ki_wave(
    "ka", charge=[("r0c1", "chest"), ("r0c2", "chest"), ("r0c3", "chest"), ("r0c5", "top"), ("r0c6", "top"),
                  ("r0c7", "top"), ("r1c0", "top"), ("r1c1", "top"), ("r1c2", "top"), ("r1c3", "top")],
    push=["r1c4", "r1c5", "r1c6", "r2c0", "r2c1", "r2c2"], hold="r2c2",
    back=["r2c3", "r2c4", "r2c5", "r2c6"])
KI_B = [F("kb:r0c0", 0.05)] + ki_wave(
    "kb", charge=[("r0c1", "chest"), ("r0c2", "chest"), ("r0c3", "chest"), ("r0c4", "chest"), ("r0c5", "chest"),
                  ("r0c6", "chest"), ("r0c7", "top"), ("r0c8", "top"), ("r1c0", "chest"), ("r1c1", "chest"),
                  ("r1c2", "chest"), ("r1c3", "chest")],
    push=["r1c4", "r1c5", "r1c6", "r2c0", "r2c1", "r2c2"], hold="r2c3",
    back=["r2c4", "r2c5", "r3c0", "r3c1", "r3c2", "r3c3", "r3c4", "r3c5", "r3c6"], t=0.055)
# el super arranca con el anillo de energia en el suelo
for _i in range(4):
    KI_B[_i]["fx"].append(("ring", _i + 1, "ground"))

GUARD = "b:r0c0"
CLIPS = [
    # guardia respirando: 4 guardias casi iguales de sheet_b (hombros/pies apenas distintos)
    ("idle", 1, [F("b:r0c0", 0.16), F("b:r0c2", 0.16), F("b:r1c3", 0.16),
                 F("b:r1c4", 0.16), F("b:r1c3", 0.16), F("b:r0c2", 0.16)]),
    # caminar en guardia (estilo KOF): ciclo de 6 guardias de sheet_b, pies alineados
    ("walk", 1, [F("b:r0c4", 0.1), F("b:r1c0", 0.1), F("b:r1c1", 0.1),
                 F("b:r2c0", 0.1), F("b:r2c1", 0.1), F("b:r2c4", 0.1)]),
    ("run", 1, [F("b:r0c4", 0.07), F("b:r1c0", 0.07), F("b:r1c1", 0.07),
                F("b:r2c0", 0.07), F("b:r2c1", 0.07), F("b:r2c4", 0.07)]),
    # impulso: se agacha y sale hacia adelante (sheet_c fila 4) con polvo y estela
    ("dash", 0, [F("c:r4c4", 0.05, fx=[("dust_a", 0, "behind")]),
                 F("c:r4c3", 0.07, fx=[("streak", 0, "behind"), ("ghost", 0, "behind")]),
                 F("c:r4c3", 0.07, fx=[("dust", 2, "behind")])]),
    ("punch1", 0, [F("a:r0c1", 0.05), F("a:r0c2", 0.04), F("a:r0c3", 0.06, hit=True),
                   F("a:r0c4", 0.08, hit=True), F("a:r0c5", 0.07)]),
    ("punch2", 0, [F("a:r1c1", 0.05), F("a:r1c2", 0.07, hit=True), F("a:r1c3", 0.05),
                   F("a:r1c4", 0.07, hit=True), F("a:r1c5", 0.07)]),
    ("punch3", 0, [F("b:r2c1", 0.06), F("b:r2c2", 0.12, hit=True, fx=[("star", 2, "hit")]), F("b:r2c3", 0.1)]),
    ("kick", 0, [F("a:r2c1", 0.06), F("a:r2c2", 0.06), F("a:r2c3", 0.12, hit=True),
                 F("a:r2c4", 0.08), F("a:r2c5", 0.06)]),
    ("dash_attack", 0, [F("a:r3c1", 0.06), F("a:r3c2", 0.06, hit=True), F("a:r3c3", 0.12, hit=True),
                        F("a:r3c4", 0.08), F("a:r3c5", 0.06)]),
    # patada voladora: se agacha, rodillazo en el aire, patada, cae
    ("finisher", 0, [F("a:r5c0", 0.07), F("a:r5c1", 0.08, hit=True, anchor="torso", lift=14),
                     F("a:r5c2", 0.08, hit=True, anchor="torso", lift=22),
                     F("a:r5c3", 0.12, hit=True, anchor="torso", lift=16, fx=[("star", 3, "hit")]),
                     F("a:r5c4", 0.1, fx=[("dust_a", 1, "feet")])]),
    ("jump", 0, [F("a:r4c0", 0.03), F("a:r4c1", 0.08), F("a:r4c2", 0.16, anchor="torso", lift=24),
                 F("a:r4c3", 0.1, fx=[("dust_a", 1, "feet")]), F("a:r4c4", 0.08)]),
    ("air", 0, [F("a:r4c2", 0.12, anchor="torso", lift=10)]),
    # PENDIENTE DE ARTE: no hay pose de caer por los aires; se usa el encogido del salto
    ("airborne", 0, [F("a:r4c1", 0.12, anchor="torso", lift=10)]),
    # bloqueo: antebrazos cruzados delante de la cara (ki_carga_a)
    ("block", 1, [F("ka:r0c4", 0.2)]),
    # PENDIENTE DE ARTE (golpe recibido): se tapa la cara / se dobla de la carga de ki
    ("hit", 0, [F("kb:r0c6", 0.14)]),
    ("hit_high", 0, [F("kb:r0c6", 0.14)]),
    ("hit_low", 0, [F("kb:r0c3", 0.14)]),
    ("recovery", 0, [F(GUARD, 0.1)]),
    # PENDIENTE DE ARTE (derribo/derrota en el suelo): rodilla en tierra de sheet_a
    ("knockdown", 0, [F("a:r5c0", 0.2, fx=[("dust", 0, "feet")])]),
    ("getup", 0, [F("a:r5c0", 0.12), F("a:r4c3", 0.1), F("a:r4c4", 0.1)]),
    ("defeat", 0, [F("a:r5c0", 0.4)]),
    # PENDIENTE DE ARTE (victoria): manos abiertas de la carga de ki con el aura
    ("victory", 1, [F("ka:r0c8", 0.2, fx=[("ring", 2, "ground")]), F("ka:r0c8", 0.2, fx=[("ring", 3, "ground")]),
                    F("ka:r0c8", 0.2, fx=[("ring", 4, "ground")])]),
    ("special", 0, KI_A), ("energy", 0, KI_A), ("ab0", 0, KI_A),
    ("super", 0, KI_B), ("rage_attack", 0, KI_B),
    # ESPADA RAPIDA: el directo de sheet_b con la mano-espada encima del puno
    ("ab1", 0, [F("b:r1c1", 0.06), F("b:r1c2", 0.05, fx=[("sword", 0, "fist")]),
                F("b:r1c2", 0.12, hit=True, fx=[("sword", 1, "fist")]),
                F("b:r1c2", 0.08, hit=True, fx=[("sword", 1, "fist"), ("moon", 3, "hit")]),
                F("b:r1c3", 0.08)]),
    # TORMENTA: combo de patadas de sheet_c con medialunas doradas en cada impacto
    ("ab2", 0, [F("c:r0c1", 0.05), F("c:r0c2", 0.07, hit=True, fx=[("moon", 1, "hit")]),
                F("c:r0c3", 0.07, hit=True, fx=[("moon", 2, "hit")]), F("c:r0c4", 0.05),
                F("c:r2c1", 0.05), F("c:r2c2", 0.07, hit=True, fx=[("moon", 1, "hit")]),
                F("c:r2c3", 0.08, hit=True, fx=[("moon", 3, "hit")]), F("c:r2c4", 0.07, hit=True),
                F("c:r1c2", 0.05), F("c:r1c3", 0.07, hit=True, fx=[("moon", 2, "hit")]),
                F("c:r1c4", 0.07, hit=True), F("c:r1c1", 0.09, hit=True, fx=[("moon", 4, "hit")]),
                F("c:r3c1", 0.08, hit=True), F("c:r4c1", 0.1, hit=True), F("c:r1c5", 0.07)]),
    # RAFAGA: combo rapido de punos de sheet_b con el puno en llamas doradas
    ("ab3", 0, [F("b:r3c0", 0.04), F("b:r3c1", 0.06, hit=True, fx=[("fist", 0, "fist")]),
                F("b:r3c2", 0.06, hit=True, fx=[("fist", 1, "fist")]), F("b:r3c3", 0.04),
                F("b:r0c3", 0.06, hit=True, fx=[("fist", 2, "fist")]), F("b:r0c1", 0.04),
                F("b:r1c2", 0.07, hit=True, fx=[("fist", 5, "fist")]), F("b:r2c3", 0.04),
                F("c:r4c2", 0.06, hit=True, fx=[("fist", 4, "fist")]),
                F("b:r2c2", 0.1, hit=True, fx=[("fist", 7, "fist")]), F("b:r3c4", 0.08)]),
    # RODILLAZO: salto con rodilla (sheet_c fila 3) y patada voladora, cae agachado
    ("ab4", 0, [F("c:r4c4", 0.06), F("c:r3c2", 0.07, anchor="torso", lift=10),
                F("c:r3c4", 0.1, hit=True, anchor="torso", lift=26, fx=[("star", 4, "hit")]),
                F("c:r3c3", 0.11, hit=True, anchor="torso", lift=18, fx=[("star", 5, "hit")]),
                F("c:r4c4", 0.08, fx=[("dust_a", 0, "feet")]), F("c:r4c5", 0.06)]),
]
ABILITIES = [("ab0", "ONDA DE KI"), ("ab1", "ESPADA RAPIDA"), ("ab2", "TORMENTA"), ("ab3", "RAFAGA"),
             ("ab4", "RODILLAZO")]

# escala de cada efecto (px de juego por px de la hoja) y su ancho objetivo
FX_W = {"sphere": 46, "beam": 150, "expl": 96, "ring": 120, "flare": 46, "moon": 44, "fist": 64,
        "star": 34, "streak": 60, "ghost": 70, "dust": 50, "dust_a": 52}


class Builder:
    def __init__(self, sheets_dir, sword_dir):
        self.figs = {}
        for short, name in [("a", "sheet_a"), ("b", "sheet_b"), ("c", "sheet_c"), ("ka", "ki_carga_a"), ("kb", "ki_carga_b")]:
            raw = extract_figures(os.path.join(sheets_dir, name + ".png"), EXPECTED.get(name))
            self.figs[short] = raw
        self.scale, self.K, self.T = {}, {}, {}
        self.bodies = {}
        for (short, row), key in REF.items():
            src = self.figs[short][key]["img"]
            self.scale[(short, row)] = GUARD_H / guard_height(src)
        for (short, row), key in REF.items():
            ref = self.body(short, key)
            self.K[(short, row)] = (ref.heel + ref.toe) / 2.0 - ref.heel
            self.T[(short, row)] = (ref.heel + self.K[(short, row)]) - ref.torso_cx
        self.ki = load_fx_ki(os.path.join(sheets_dir, "fx_ki.png"))
        gold = load_fx_gold(os.path.join(sheets_dir, "fx_gold.png"))
        self.fx = dict(self.ki)
        self.fx.update(gold)
        # polvo de la fila 7 de sheet_a (piezas sueltas)
        self.fx["dust_a"] = [self.figs["a"]["r7c5"]["img"], self.figs["a"]["r7c6"]["img"]]
        self.fx_scale = {}
        for k, cells in self.fx.items():
            if k in FX_W:
                self.fx_scale[k] = FX_W[k] / max(c.shape[1] for c in cells)
        sw = []
        for n in ["manoespada_reposo.png", "manoespada_swing.png"]:
            p = os.path.join(sword_dir, n)
            sw.append(np.array(Image.open(p).convert("RGBA"))[:, ::-1].copy() if os.path.exists(p) else None)
        self.sword = sw

    def body(self, sheet, key):
        if (sheet, key) not in self.bodies:
            src = dehalo(self.figs[sheet][key]["img"])
            self.bodies[(sheet, key)] = Body(sheet, key, src, self.scale[(sheet, row_of(key))])
        return self.bodies[(sheet, key)]

    def anchor(self, b, f):
        """Pies: con los dos pies en el suelo, talon de atras + K (el pie de apoyo no
        se mueve); con un pie (patadas) o ninguno (saltos), el centro del abdomen + T,
        para que el torso no salte de lugar entre cuadros del mismo golpe."""
        rk = (b.sheet, row_of(b.key))
        mode = f["anchor"]
        if mode == "heel" and (b.feet_down < 2 or f["lift"] > 0):
            mode = "torso"
        if mode == "torso":
            ax = b.torso_cx + self.T[rk]
        else:
            ax = b.heel + self.K[rk]
        return ax - f["dx"], b.bottom + 1 + f["lift"]

    def fx_img(self, kind, idx):
        img = self.fx[kind][idx]
        s = self.fx_scale[kind]
        return resize_rgba(img, img.shape[1] * s, img.shape[0] * s)

    def extremity(self, b):
        """Puno/pie extendido: la parte del sprite delante del torso (sin el pie de
        apoyo en el suelo); caja = los pixeles de los ultimos 22 px de esa extension."""
        a = b.a.copy()
        a[int(b.bottom - 0.08 * (b.bottom - b.top)):] = False
        ys, xs = np.nonzero(a & (np.arange(a.shape[1])[None, :] > b.torso_front + 6))
        if len(xs) == 0:
            return None
        xm = xs.max()
        sel = xs >= xm - 22
        return [float(xs[sel].min()), float(ys[sel].min()), float(xm + 1), float(ys[sel].max() + 1)]

    def compose(self, f):
        """Devuelve (piezas [(img, x, y)], caja_cuerpo, caja_ataque) en coords de pies."""
        b = self.body(f["sheet"], f["key"])
        ax, ay = self.anchor(b, f)
        bx, by = -ax, -ay                      # esquina sup. izq. del cuerpo
        under, over = [], []
        attack = None
        ext = self.extremity(b)
        ext_g = [ext[0] + bx, ext[1] + by, ext[2] + bx, ext[3] + by] if ext else None

        def add_box(box):
            nonlocal attack
            if attack is None:
                attack = list(box)
            else:
                attack = [min(attack[0], box[0]), min(attack[1], box[1]), max(attack[2], box[2]), max(attack[3], box[3])]

        def fx_box(img, x, y, thr=64):
            ys, xs = np.nonzero(img[..., 3] > thr)
            return [x + xs.min(), y + ys.min(), x + xs.max() + 1, y + ys.max() + 1]

        for kind, idx, where in f["fx"]:
            if kind == "sword":
                sw = self.sword[idx]
                if sw is None:
                    continue
                # mano = el grupo de piel mas grande (la estela dorada tiene tonos de piel sueltos)
                _, _, sk, _, _ = masks(sw)
                hand = max(comp_centroids(sk), key=lambda c: c["n"])
                hand_h = hand["y1"] - hand["y0"] + 1
                fx_, fy_ = b.hands("front")
                # alto del puno en el cuadro: grupo de piel mas adelantado
                cs = comp_centroids(b.skin)
                c = max(cs, key=lambda c: c["x1"])
                fist_h = c["y1"] - c["y0"] + 1
                s = 1.15 * fist_h / hand_h
                img = resize_rgba(sw, sw.shape[1] * s, sw.shape[0] * s)
                x = bx + c["cx"] - hand["cx"] * s
                y = by + c["cy"] - hand["cy"] * s
                over.append((("sword", idx, round(s, 4)), img, x, y))
                if f["hit"]:
                    bb = fx_box(img, x, y)
                    bb[0] = max(bb[0], bx + b.torso_front)
                    add_box(bb)
                continue
            img = self.fx_img(kind, idx)
            h, w = img.shape[:2]
            if kind == "sphere":
                if where == "top":
                    hx, hy = b.hands("top")
                    cx, cy = bx + hx, by + hy - h * 0.30
                else:
                    hx, hy = b.hands("chest")
                    cx, cy = bx + hx + 4, by + hy
                x, y = cx - w / 2, cy - h / 2
                over.append(((kind, idx), img, x, y))
            elif kind == "beam":
                px, py = b.hands("front")
                x, y = bx + px - 6, by + py - h / 2
                over.append(((kind, idx), img, x, y))
                if f["hit"]:
                    add_box(fx_box(img, x, y))
            elif kind == "expl":
                px, py = b.hands("front")
                beam_w = self.fx_img("beam", 5).shape[1]
                cx = bx + px - 6 + beam_w * 0.85
                x, y = cx - w / 2, by + py - h * 0.55
                over.append(((kind, idx), img, x, y))
                if f["hit"]:
                    add_box(fx_box(img, x, y))
            elif kind in ("ring", "flare"):
                x, y = -w / 2, -h + (h * 0.45 if kind == "ring" else 2)
                under.append(((kind, idx), img, x, y))
            elif kind in ("dust", "dust_a", "streak", "ghost"):
                if where == "behind":
                    x = bx + b.heel - w + 6
                    y = (-h * 1.6 - 30) if kind == "streak" else (-h + 2 if kind != "ghost" else -h - 4)
                    under.append(((kind, idx), img, x, y))
                else:
                    over.append(((kind, idx), img, -w / 2, -h + 3))
            elif kind == "fist":
                hx, hy = b.hands("front")
                x, y = bx + hx - w + 4, by + hy - h / 2
                over.append(((kind, idx), img, x, y))
            elif kind in ("moon", "star"):
                if ext_g:
                    cx, cy = (ext_g[0] + ext_g[2]) / 2 + (6 if kind == "moon" else 4), (ext_g[1] + ext_g[3]) / 2
                else:
                    cx, cy = 30, -60
                x, y = cx - w / 2, cy - h / 2
                over.append(((kind, idx), img, x, y))
                if f["hit"] and kind == "moon":
                    add_box(fx_box(img, x, y))
        if f["hit"] and attack is None:
            if ext_g is None:
                print("  aviso: %s:%s marcado como golpe sin extension" % (f["sheet"], f["key"]))
            else:
                add_box([ext_g[0] - 2, ext_g[1] - 3, ext_g[2] + 3, ext_g[3] + 3])
        elif f["hit"] and ext_g and not any(k[0][0] in ("beam", "expl") for k in over):
            add_box([ext_g[0] - 2, ext_g[1] - 3, ext_g[2] + 3, ext_g[3] + 3])
        # caja de cuerpo: desde la cara (sin el pelo) hasta los pies, ancho del cuerpo
        hair = b.light.copy()
        hair[b.navy_top:] = False
        bodym = b.a & ~hair & ~b.belt
        cols = np.nonzero(bodym)[1]
        x0, x1 = np.percentile(cols, 4), np.percentile(cols, 96)
        x1 = min(x1, b.torso_front + 10)
        top = max(b.top, b.navy_top - 14)
        bodybox = [bx + x0, by + top, bx + x1, by + b.bottom + 1]
        pieces = under + [(("body", f["sheet"], f["key"]), b.img, bx, by)] + over
        return pieces, bodybox, attack


def render_frame(pieces, size=(420, 260), origin=(140, 220), boxes=None, scale=1.0):
    can = Image.new("RGBA", size, (34, 38, 48, 255))
    d = ImageDraw.Draw(can)
    ox, oy = origin
    d.line([(0, oy), (size[0], oy)], fill=(90, 100, 120))
    for _, img, x, y in pieces:
        im = Image.fromarray(img)
        can.alpha_composite(im, (int(round(ox + x)), int(round(oy + y))))
    d.line([(ox, oy - 6), (ox, oy + 6)], fill=(255, 80, 80))
    if boxes:
        bb, ab = boxes
        d.rectangle([ox + bb[0], oy + bb[1], ox + bb[2], oy + bb[3]], outline=(80, 220, 120))
        if ab:
            d.rectangle([ox + ab[0], oy + ab[1], ox + ab[2], oy + ab[3]], outline=(255, 60, 60))
    return can


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sheets", help="carpeta con las hojas (ver docstring)")
    ap.add_argument("--sword", default=None, help="carpeta con manoespada_reposo.png / manoespada_swing.png (default: <sheets>)")
    ap.add_argument("--repo", default=os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    ap.add_argument("--audit", default=None, help="carpeta donde dejar las hojas de auditoria")
    ap.add_argument("--montage", default=None, help="carpeta: montaje con etiquetas de las figuras de cada hoja")
    ap.add_argument("--dry", action="store_true", help="no escribe en el repo")
    args = ap.parse_args()
    B = Builder(args.sheets, args.sword or args.sheets)
    if args.montage:
        os.makedirs(args.montage, exist_ok=True)
        for short, figs in B.figs.items():
            montage(figs, os.path.join(args.montage, "fig_%s.png" % short))
    print("escala por fila:", {"%s%d" % k: round(float(v), 4) for k, v in B.scale.items()})

    images = {}          # clave -> (id, img)
    frames = []          # (piezas[(img_idx, x, y)], cuerpo, ataque)
    frame_key = {}
    clip_frames = []
    used = {}
    for name, loop, fl in CLIPS:
        idxs = []
        for f in fl:
            sig = json.dumps([f["sheet"], f["key"], f["anchor"], f["lift"], f["dx"], f["hit"], f["fx"]])
            if sig not in frame_key:
                pieces, bodybox, attack = B.compose(f)
                pl = []
                for key, img, x, y in pieces:
                    if key not in images:
                        images[key] = (len(images), img)
                    pl.append((images[key][0], img.shape[1], img.shape[0], int(round(x)), int(round(y))))
                frame_key[sig] = len(frames)
                frames.append((pl, bodybox, attack, pieces))
            idxs.append((frame_key[sig], f["t"]))
            used.setdefault("%s:%s" % (f["sheet"], f["key"]), set()).add(name)
        clip_frames.append((name, loop, idxs))
    print("cuadros:", len(frames), "imagenes:", len(images))

    if args.audit:
        os.makedirs(args.audit, exist_ok=True)
        for name, loop, idxs in clip_frames:
            per = 5
            cols = min(len(idxs), per)
            rows = (len(idxs) + per - 1) // per
            TW, TH = 330, 200
            sheet = Image.new("RGBA", (cols * TW * 2, rows * TH * 2), (20, 20, 26, 255))
            for i, (fi, t) in enumerate(idxs):
                pl, bb, ab, pieces = frames[fi]
                tile = render_frame(pieces, size=(TW, TH), origin=(110, 180), boxes=(bb, ab))
                tile = tile.resize((TW * 2, TH * 2), Image.NEAREST)
                ImageDraw.Draw(tile).text((4, 4), "%s #%d f%d %.2fs" % (name, i, fi, t), fill=(255, 220, 90))
                sheet.alpha_composite(tile, ((i % per) * TW * 2, (i // per) * TH * 2))
            sheet.convert("RGB").save(os.path.join(args.audit, "clip_%s.png" % name))
            # papel cebolla: todos los cuerpos del clip encimados (deteccion de saltos)
            onion = Image.new("RGBA", (330, 200), (34, 38, 48, 255))
            for fi, t in idxs:
                for key, img, x, y in frames[fi][3]:
                    if key[0] == "body":
                        im = img.copy()
                        im[..., 3] = (im[..., 3] * 0.3).astype(np.uint8)
                        onion.alpha_composite(Image.fromarray(im), (int(round(110 + x)), int(round(180 + y))))
            d = ImageDraw.Draw(onion)
            d.line([(0, 180), (330, 180)], fill=(90, 100, 120))
            d.line([(110, 174), (110, 186)], fill=(255, 80, 80))
            onion.resize((660, 400), Image.NEAREST).convert("RGB").save(os.path.join(args.audit, "onion_%s.png" % name))
        with open(os.path.join(args.audit, "usados.json"), "w") as fh:
            json.dump({k: sorted(v) for k, v in sorted(used.items())}, fh, indent=1)
    if args.dry:
        return

    actor = os.path.join(args.repo, "assets", "kratos", "actor")
    ids = {}
    for key, (i, img) in sorted(images.items(), key=lambda kv: kv[1][0]):
        ids[i] = FIRST_ID + i
        Image.fromarray(img).save(os.path.join(actor, "%d.png" % ids[i]), optimize=True)
    out = ["# RONIN: artista marcial (pelo blanco, polera azul, pantalon blanco, cinturon amarillo).",
           "# GENERADO por tools/ronin/build_ronin.py desde las hojas del usuario: no editar a mano.",
           "# Todos los cuadros miran a la DERECHA (ficha facesRightByDefault=true).",
           "# Guardia normalizada a %d px de alto y x%.2f de ancho; pies en (0,0), y negativa hacia arriba." % (GUARD_H, SLIM),
           "# Cajas: cuerpo = cara->pies sin el pelo; ataque = puno/pie extendido delante del torso,",
           "# o la extension visible del rayo/explosion de ki (calculadas desde los pixeles).",
           "# PENDIENTE DE ARTE: hit/hit_high/hit_low, knockdown, getup, defeat, victory y airborne",
           "# reusan poses parecidas de las hojas (no hay dibujo propio de recibir golpe/caer/ganar).",
           "character ronin hero 120", "name RONIN"]
    for i in sorted(ids):
        out.append("image %d %d" % (i, ids[i]))
    out.append("")
    for fi, (pl, bb, ab, _) in enumerate(frames):
        b = [int(round(v)) for v in bb]
        a = [int(round(v)) for v in ab] if ab else [0, 0, 0, 0]
        out.append("frame %d %d %d %d %d %d %d %d %d %d" % (fi, len(pl), b[0], b[1], b[2], b[3], a[0], a[1], a[2], a[3]))
        for (img, w, h, x, y) in pl:
            out.append("piece %d 0 0 %d %d %d %d 0" % (img, w, h, x, y))
    out.append("")
    for name, loop, idxs in clip_frames:
        out.append("clip %s %d %d %s" % (name, loop, len(idxs), " ".join("%d:%g" % (fi, t) for fi, t in idxs)))
    out.append("")
    for clip, label in ABILITIES:
        out.append("ability %s %s" % (clip, label))
    out += ["sound hurt sound36", "sound die sound34", "sound dash sound70", ""]
    with open(os.path.join(args.repo, "data", "kratos", "ronin.txt"), "w") as fh:
        fh.write("\n".join(out))
    print("escrito ronin.txt e imagenes %d..%d" % (FIRST_ID, FIRST_ID + len(ids) - 1))


if __name__ == "__main__":
    main()
