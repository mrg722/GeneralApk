#!/usr/bin/env python3
"""Metricas de fluidez de RONIN-KRATOS (cuadro a cuadro, dentro de cada clip),
medidas sobre lo que dibuja el motor (data/kratos/ronin_kratos_<arma>.txt):

  * Cabeza: centro/tamano de la cabeza de RONIN (pelo blanco + cara) en cada
    cuadro, y centro/angulo de la cabeza calva de Kratos en el mismo cuadro
    (rk_head: plantillas, la referencia del movimiento real).  Jitter =
    |delta(RONIN) - delta(Kratos)| entre cuadros consecutivos del clip (px).
  * Parpadeo de clase: % de pixeles del cuerpo cuya clase de RONIN (polera roja,
    blanco, piel, cinto, oscuro) cambia entre cuadros consecutivos, con
    compensacion de movimiento (cada pixel se busca en el cuadro siguiente a
    +-3 px por el color de Kratos: si Kratos es "lo mismo" ahi, RONIN tambien
    deberia serlo).  Se excluye el contorno de 1 px.

Uso: rk_metrics.py [--weapons espadas,...] [--tag nombre] [--onion]
Salida: out/metrics_<tag>.json (+ out/onion_<tag>_<arma>.png)."""
import argparse, json, math, sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from rk_common import DATA, WEAPONS, CharFile, orient, sheet, hsv, on_bg  # noqa: E402
import rk_build  # noqa: E402
from rk_inventory import pkey  # noqa: E402

OUT = HERE / "out"
CLIPS = ["idle", "walk", "run", "dash", "hit", "punch1", "punch2", "punch3", "kick"]
SHIRT, WHITE, SKIN, SASH, DARK, OTHER, HAIR = range(7)


def body_canvas(cf, fi, is_body, W=260, H=230, OX=130, OY=190):
    """Cuadro fi (solo piezas body) en un lienzo fijo con el origen (pies) en OX,OY."""
    can = np.zeros((H, W, 4), np.float32)
    for p in cf.frames[fi]["pieces"]:
        if not is_body(p):
            continue
        src = sheet(cf.images[p["img"]])[p["sy"]:p["sy"] + p["sh"], p["sx"]:p["sx"] + p["sw"]]
        o = orient(src, p["flip"])
        X, Y = p["dx"] + OX, p["dy"] + OY
        a = o[..., 3:4].astype(np.float32) / 255
        reg = can[Y:Y + o.shape[0], X:X + o.shape[1]]
        reg[..., :3] = o[..., :3] * a + reg[..., :3] * (1 - a)
        reg[..., 3:4] = 255 * (a + reg[..., 3:4] / 255 * (1 - a))
    return can.clip(0, 255).astype(np.uint8)


def ronin_classes(px):
    h, s, v = hsv(px)
    a = px[..., 3] > 100
    c = np.full(a.shape, OTHER, np.int8)
    red = (h < 22) | (h > 330)
    c[a & (v < 0.16)] = DARK
    c[a & (v >= 0.16) & red & (s > 0.5)] = SHIRT
    c[a & (s < 0.2) & (v > 0.4)] = WHITE
    c[a & (s < 0.25) & (v > 0.4) & (px[..., 2].astype(int) > px[..., 0].astype(int))] = HAIR   # pelo: blanco frio
    c[a & (h >= 8) & (h < 42) & (s >= 0.2) & (s <= 0.5) & (v > 0.4)] = SKIN
    c[a & (h >= 35) & (h < 62) & (s > 0.4) & (v > 0.35)] = SASH
    c[~a] = -1
    return c


_track = None


def kratos_head(cf, fi, ctx):
    """Cabeza de Kratos (referencia del movimiento real) en coordenadas de origen:
    la que eligio rk_build con continuidad temporal (out/head_track.json)."""
    global _track
    if _track is None:
        _track = json.loads((OUT / "head_track.json").read_text())
    t = _track.get(f"{ctx[3]}:{fi}")
    if t is None:
        return None
    x, y, ang, fl = t["kratos"]
    return dict(x=x, y=y, ang=ang, w=rk_build.KRATOS_HEAD_W)


def ronin_head(can, kh, OX, OY):
    """Cabeza de RONIN (pelo blanco + cara) cerca de la cabeza de Kratos."""
    if kh is None:
        return None
    cls = ronin_classes(can)
    H, W = cls.shape
    cx, cy = kh["x"] + OX, kh["y"] + OY
    yy, xx = np.mgrid[0:H, 0:W]
    near = (np.abs(xx - cx) < 26) & (np.abs(yy - cy) < 26)
    # el pelo blanco frio solo existe en la cabeza de RONIN: su centro es la posicion de la cabeza
    hair = near & (cls == HAIR)
    if hair.sum() < 8:
        return None
    lab, n = ndimage.label(ndimage.binary_closing(hair, iterations=1) & near)
    sizes = ndimage.sum(np.ones_like(lab), lab, range(1, n + 1))
    k = 1 + int(np.argmax(sizes))
    mm = (lab == k) & hair
    ys, xs = np.nonzero(mm)
    face = ndimage.binary_dilation(mm, iterations=4) & (cls == SKIN)
    fw = float(np.ptp(np.nonzero(face)[1]) + 1) if face.any() else 0.0
    return dict(x=float(xs.mean()) - OX, y=float(ys.mean()) - OY, h=float(np.ptp(ys) + 1), w=float(np.ptp(xs) + 1), face_w=fw)


def interior(c, it=1):
    """Pixeles a >= it px del borde de su propia zona de color (no cuentan los
    bordes que se corren 1-2 px con el movimiento, si los trozos que cambian de color)."""
    out = np.zeros(c.shape, bool)
    for k in (SHIRT, WHITE, SKIN, SASH, HAIR):
        out |= ndimage.binary_erosion(c == k, iterations=it)
    return out


def flicker(K0, K1, R0, R1, r=3):
    """% de pixeles del cuerpo que cambian de clase de RONIN entre dos cuadros,
    con compensacion de movimiento guiada por el color de Kratos.
    Devuelve (bruto %, interior %, n): bruto cuenta todo pixel; interior solo
    los que estan dentro de una zona de color en ambos cuadros (trozos que
    cambian de color = deformacion; los bordes moviendose no)."""
    a0 = (K0[..., 3] > 100) & (R0[..., 3] > 100)
    a1 = (K1[..., 3] > 100) & (R1[..., 3] > 100)
    inner0 = ndimage.binary_erosion(a0, iterations=1)
    c0, c1 = ronin_classes(R0), ronin_classes(R1)
    k0 = K0[..., :3].astype(np.float32); k1 = K1[..., :3].astype(np.float32)
    i0, i1 = interior(c0), interior(c1)
    # alineacion global previa (en clips rapidos el cuerpo entero se corre mas que
    # +-r): se prueba sin corrimiento y con el corrimiento global que mejor superpone
    # el Kratos de ambos cuadros, y se queda la correspondencia con menos residuo
    bestg, g = np.inf, (0, 0)
    for gy in range(-12, 13, 2):
        for gx in range(-12, 13, 2):
            ks = np.roll(np.roll(k1, -gy, 0), -gx, 1); as_ = np.roll(np.roll(a1, -gy, 0), -gx, 1)
            ov = a0 & as_
            if ov.sum() < 0.5 * a0.sum():
                continue
            e = ((k0[ov] - ks[ov]) ** 2).sum(-1).mean() + 4.0 * (a0 ^ as_).sum() / max(1, a0.sum()) * 1e3
            if e < bestg:
                bestg, g = e, (gy, gx)
    res = None
    for gg in {(0, 0), g}:
        sh = lambda x: np.roll(np.roll(x, -gg[0], 0), -gg[1], 1)
        out = _local(k0, sh(k1), sh(a1), sh(c1), sh(i1), r)
        resid = float(np.median(np.minimum(out[0][a0], 1e6))) if a0.any() else 0.0
        if res is None or resid < res[0]:
            res = (resid,) + out
    _, best, bc, bi = res
    sel = inner0 & np.isfinite(best) & (best < 1e5) & (c0 >= 0) & (c0 != OTHER) & (bc >= 0) & (bc != OTHER)
    # la cabeza de RONIN es una pieza rigida aparte (se mide con el jitter): fuera del parpadeo del cuerpo
    sel &= ~ndimage.binary_dilation(c0 == HAIR, iterations=5) & (bc != HAIR)
    if sel.sum() == 0:
        return 0.0, 0.0, 0
    si = sel & i0 & bi
    fi = float((bc[si] != c0[si]).mean() * 100) if si.any() else 0.0
    return float((bc[sel] != c0[sel]).mean() * 100), fi, int(sel.sum())


def _local(k0, k1, a1, c1, i1, r):
    best = np.full(k0.shape[:2], np.inf, np.float32); bc = np.full(k0.shape[:2], -1, np.int8); bi = np.zeros(k0.shape[:2], bool)
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            k1s = np.roll(np.roll(k1, -dy, 0), -dx, 1)
            a1s = np.roll(np.roll(a1, -dy, 0), -dx, 1)
            c1s = np.roll(np.roll(c1, -dy, 0), -dx, 1)
            i1s = np.roll(np.roll(i1, -dy, 0), -dx, 1)
            ssd = ndimage.uniform_filter(((k0 - k1s) ** 2).sum(-1), 3) + np.where(a1s, 0, 1e6) + 2.0 * (dx * dx + dy * dy)
            upd = ssd < best
            best = np.where(upd, ssd, best); bc = np.where(upd, c1s, bc); bi = np.where(upd, i1s, bi)
    return best, bc, bi


def from_git(ref, w):
    """RONIN-KRATOS tal como estaba en `ref` (txt + hojas 95xx) para medir el antes."""
    import io, subprocess, tempfile
    from rk_common import ROOT, _sheet_cache
    show = lambda path: subprocess.run(["git", "-C", str(ROOT), "show", f"{ref}:{path}"], capture_output=True, check=True).stdout
    tmp = Path(tempfile.mkdtemp()) / f"ronin_kratos_{w}.txt"
    tmp.write_bytes(show(f"data/kratos/ronin_kratos_{w}.txt"))
    cf = CharFile(tmp)
    for i, sid in cf.images.items():
        if int(sid) >= 9500:
            _sheet_cache["git:" + sid] = np.array(Image.open(io.BytesIO(show(f"assets/kratos/actor/{sid}.png"))).convert("RGBA"))
            cf.images[i] = "git:" + sid
    return cf


def measure(w, ctx_base, tag, onion=False, ref=None):
    body_keys, exc, tpls = ctx_base
    cfk = CharFile(DATA / f"kratos_{w}.txt")
    cfr = from_git(ref, w) if ref else CharFile(DATA / f"ronin_kratos_{w}.txt")
    base = max(cfk.images) + 1
    isb_k = lambda p: pkey(cfk.key(p)) in body_keys
    isb_r = lambda p: p["img"] >= base
    ctx = (body_keys, exc, tpls, w)
    res = {}
    onion_rows = []
    cache = {}
    for c in CLIPS:
        if c not in cfk.clips:
            continue
        fr = []
        for f in cfk.clips[c]:
            if not fr or fr[-1] != f:
                fr.append(f)
        rows = []
        for f in fr:
            if f not in cache:
                K = body_canvas(cfk, f, isb_k); R = body_canvas(cfr, f, isb_r)
                kh = kratos_head(cfk, f, ctx)
                rh = ronin_head(R, kh, 130, 190)
                cache[f] = (K, R, kh, rh)
            rows.append(cache[f])
        jit, jrot, flk, flki, sizes = [], [], [], [], []
        for i in range(len(rows) - 1):
            K0, R0, kh0, rh0 = rows[i]; K1, R1, kh1, rh1 = rows[i + 1]
            if kh0 and kh1 and rh0 and rh1:
                dk = np.array([kh1["x"] - kh0["x"], kh1["y"] - kh0["y"]])
                dr = np.array([rh1["x"] - rh0["x"], rh1["y"] - rh0["y"]])
                jit.append(float(np.abs(dr - dk).max()))
                # en el marco girado de la cabeza de Kratos: si RONIN sigue posicion E inclinacion,
                # su offset respecto de la cabeza de Kratos no cambia
                def rel(kh, rh):
                    t = math.radians(kh["ang"]); c, s = math.cos(t), math.sin(t)
                    d = np.array([rh["x"] - kh["x"], rh["y"] - kh["y"]])
                    return np.array([c * d[0] - s * d[1], s * d[0] + c * d[1]])
                jrot.append(float(np.abs(rel(kh1, rh1) - rel(kh0, rh0)).max()))
            fl, fli, n = flicker(K0, K1, R0, R1)
            if n:
                flk.append(fl); flki.append(fli)
        for K, R, kh, rh in rows:
            if kh and rh:
                sizes.append((rh["face_w"], kh["w"], rh["h"]))
        res[c] = dict(frames=fr, jitter_mean=float(np.mean(jit)) if jit else None, jitter_max=float(np.max(jit)) if jit else None,
                      jitter_le1=float(np.mean(np.array(jit) <= 1.0) * 100) if jit else None,
                      jitter_rot_mean=float(np.mean(jrot)) if jrot else None, jitter_rot_max=float(np.max(jrot)) if jrot else None,
                      jitter_rot_le1=float(np.mean(np.array(jrot) <= 1.0) * 100) if jrot else None,
                      flicker_mean=float(np.mean(flk)) if flk else None, flicker_max=float(np.max(flk)) if flk else None,
                      flicker_int_mean=float(np.mean(flki)) if flki else None, flicker_int_max=float(np.max(flki)) if flki else None,
                      face_w=float(np.mean([s[0] for s in sizes])) if sizes else None,
                      kratos_head_w=float(np.mean([s[1] for s in sizes])) if sizes else None,
                      ronin_head_h=float(np.mean([s[2] for s in sizes])) if sizes else None,
                      heads=[[None if kh is None else [round(kh["x"], 1), round(kh["y"], 1), kh["ang"]],
                              None if rh is None else [round(rh["x"], 1), round(rh["y"], 1)]] for _, _, kh, rh in rows])
        if onion:
            onion_rows.append((c, [r[1] for r in rows], [r[0] for r in rows]))
    if onion:
        onion_sheet(onion_rows, OUT / f"onion_{tag}_{w}.png", f"{tag} / {w}")
    return res


def crop_box(imgs):
    m = np.zeros(imgs[0].shape[:2], bool)
    for im in imgs:
        m |= im[..., 3] > 0
    ys, xs = np.nonzero(m)
    return max(0, ys.min() - 3), ys.max() + 4, max(0, xs.min() - 3), xs.max() + 4


def onion_pair(a, b):
    """a (opaco) + b (siguiente) encima al 50% teñido de cian: lo que salta se ve doble."""
    base = on_bg(a, (60, 60, 60)).astype(np.float32)
    bb = on_bg(b, (60, 60, 60)).astype(np.float32)
    al = (b[..., 3:4] > 0) * 0.5
    tint = bb * np.array([0.55, 1.0, 1.0])
    return (base * (1 - al) + tint * al).clip(0, 255).astype(np.uint8)


def onion_sheet(rows, path, title, S=2, maxc=9):
    """Por clip: fila 1 = RONIN-KRATOS cuadro i + i+1 superpuestos; fila 2 = lo mismo en Kratos;
    ultima celda = todos los cuadros del clip superpuestos."""
    cells = []
    for c, R, K in rows:
        y0, y1, x0, x1 = crop_box(R + K)
        rr, kk = [], []
        n = min(len(R) - 1, maxc)
        for i in range(n):
            rr.append(onion_pair(R[i][y0:y1, x0:x1], R[i + 1][y0:y1, x0:x1]))
            kk.append(onion_pair(K[i][y0:y1, x0:x1], K[i + 1][y0:y1, x0:x1]))
        for L, out in ((R, rr), (K, kk)):
            acc = np.zeros((y1 - y0, x1 - x0, 3), np.float32)
            for im in L:
                acc += on_bg(im[y0:y1, x0:x1], (60, 60, 60))
            out.append((acc / len(L)).astype(np.uint8))
        cells.append((c, rr, kk))
    cw = max(r[0].shape[1] for _, r, _ in cells) * S
    ch = max(r[0].shape[0] for _, r, _ in cells) * S
    ncol = max(len(r) for _, r, _ in cells)
    M = Image.new("RGB", (ncol * (cw + 2) + 70, len(cells) * (2 * ch + 6) + 16), (20, 20, 20))
    d = ImageDraw.Draw(M)
    d.text((4, 2), title + "  (cada celda: cuadro i + i+1 al 50% cian; ultima: clip entero)  fila sup RONIN-KRATOS, inf Kratos", fill=(255, 255, 0))
    y = 16
    for c, rr, kk in cells:
        d.text((4, y + 4), c, fill=(255, 255, 0))
        for j, (a, b) in enumerate(zip(rr, kk)):
            for k, im in enumerate((a, b)):
                big = Image.fromarray(np.kron(im, np.ones((S, S, 1), np.uint8)))
                M.paste(big, (70 + j * (cw + 2), y + k * ch))
        y += 2 * ch + 6
    M.save(path)


def compare_sheet(w, ref, clips=("idle", "walk", "dash", "punch1"), S=2, maxf=8):
    """Kratos | RONIN-KRATOS antes (commit `ref`) | ahora, lado a lado, por clip."""
    from rk_inventory import inventory
    body_keys = {k for k, e in inventory().items() if e["cls"] == "body"}
    cfk = CharFile(DATA / f"kratos_{w}.txt")
    cfb = from_git(ref, w)
    cfa = CharFile(DATA / f"ronin_kratos_{w}.txt")
    base = max(cfk.images) + 1
    rows = []
    for c in clips:
        fr = []
        for f in cfk.clips[c]:
            if not fr or fr[-1] != f:
                fr.append(f)
        fr = fr[:maxf]
        trip = [[body_canvas(cfk, f, lambda p: pkey(cfk.key(p)) in body_keys) for f in fr],
                [body_canvas(cfb, f, lambda p: p["img"] >= base) for f in fr],
                [body_canvas(cfa, f, lambda p: p["img"] >= base) for f in fr]]
        y0, y1, x0, x1 = crop_box(trip[0] + trip[1] + trip[2])
        rows.append((c, [[on_bg(im[y0:y1, x0:x1], (70, 105, 70)) for im in L] for L in trip]))
    cw = max(r[1][0][0].shape[1] for r in rows) * S
    ch = max(r[1][0][0].shape[0] for r in rows) * S
    ncol = max(len(r[1][0]) for r in rows)
    M = Image.new("RGB", (90 + ncol * (cw + 2), len(rows) * (3 * ch + 22) + 18), (20, 20, 20))
    d = ImageDraw.Draw(M)
    d.text((4, 2), f"{w}: por clip, fila 1 Kratos / fila 2 RONIN-KRATOS antes ({ref}) / fila 3 ahora", fill=(255, 255, 0))
    y = 18
    for c, trip in rows:
        d.text((4, y + 4), c, fill=(255, 255, 0))
        for k, lab in enumerate(("Kratos", "antes", "ahora")):
            d.text((4, y + k * ch + ch // 2), lab, fill=(180, 180, 180))
            for j, im in enumerate(trip[k]):
                M.paste(Image.fromarray(np.kron(im, np.ones((S, S, 1), np.uint8))), (90 + j * (cw + 2), y + k * ch))
        y += 3 * ch + 22
    path = OUT / f"compare_{w}.png"
    M.save(path)
    return path


def summary(allres):
    lines = []
    for w, res in allres.items():
        for c, r in res.items():
            lines.append(f"{w:8s} {c:7s} jitter cabeza media {r['jitter_mean'] if r['jitter_mean'] is None else round(r['jitter_mean'], 2)} "
                         f"max {r['jitter_max'] if r['jitter_max'] is None else round(r['jitter_max'], 2)} "
                         f"(<=1px {r['jitter_le1'] if r['jitter_le1'] is None else round(r['jitter_le1'])}%)  "
                         f"girado media {r['jitter_rot_mean'] if r['jitter_rot_mean'] is None else round(r['jitter_rot_mean'], 2)} "
                         f"max {r['jitter_rot_max'] if r['jitter_rot_max'] is None else round(r['jitter_rot_max'], 2)} "
                         f"(<=1px {r['jitter_rot_le1'] if r['jitter_rot_le1'] is None else round(r['jitter_rot_le1'])}%)  "
                         f"parpadeo interior {round(r['flicker_int_mean'] or 0, 2)}% (max {round(r['flicker_int_max'] or 0, 2)}%) "
                         f"bruto {round(r['flicker_mean'] or 0, 2)}%  "
                         f"cara RONIN {round(r['face_w'] or 0, 1)} / cabeza Kratos {round(r['kratos_head_w'] or 0, 1)} / alto cabeza RONIN {round(r['ronin_head_h'] or 0, 1)}")
    return "\n".join(lines)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--weapons", default=",".join(WEAPONS))
    ap.add_argument("--tag", default="now")
    ap.add_argument("--onion", action="store_true")
    ap.add_argument("--ref", default=None, help="medir RONIN-KRATOS de ese commit (el antes)")
    ap.add_argument("--compare", default=None, help="hoja Kratos | antes (ese commit) | ahora")
    a = ap.parse_args()
    OUT.mkdir(exist_ok=True)
    if a.compare:
        for w in a.weapons.split(","):
            print(compare_sheet(w, a.compare))
        sys.exit(0)
    exc, body_keys, R, heads, cfs, tpls = rk_build.setup()
    allres = {}
    for w in a.weapons.split(","):
        allres[w] = measure(w, (body_keys, exc, tpls), a.tag, a.onion, a.ref)
    (OUT / f"metrics_{a.tag}.json").write_text(json.dumps(allres, indent=1))
    print(summary(allres))
