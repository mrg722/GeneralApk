"""Cabeza de Kratos por plantillas (metodo despiece: se prueba cada plantilla
con espejo, escala y giro sobre el cuadro y se queda la que mejor calza).

Las plantillas son recortes de la cabeza calva de Kratos en cuadros de
referencia (exceptions.json -> head_templates: arma, cuadro, caja x0,y0,x1,y1
en el cuadro armado solo con piezas body).  Se recortan EN VIVO de las hojas
actuales de Kratos.  Como Kratos esta renderizado siempre a la misma escala,
la cabeza mide lo mismo en todos los cuadros: la cabeza de RONIN se pega con
tamano fijo (no salta de tamano entre cuadros)."""
import math
import numpy as np
from PIL import Image
from scipy import ndimage, signal

_cache = {}
RING = 3          # anillo alrededor de la cabeza (px)
RING_MAX = 0.70   # a lo mas 70% del anillo puede ser cuerpo (cuello/hombro); el pecho da ~100%
BGC = np.array([90.0, 120.0, 90.0], np.float32)      # gris-verde neutro bajo lo transparente


def flat(rgba, gray=False):
    a = rgba[..., 3:4].astype(np.float32) / 255
    f = rgba[..., :3].astype(np.float32) * a + BGC * (1 - a)
    if gray:
        # cuerpo tenido por un aura (morado / fuego): se compara solo la luz
        l = f @ np.array([0.299, 0.587, 0.114], np.float32)
        f = np.repeat(l[..., None], 3, -1)
    return f


def templates(compose_fn, exc):
    key = id(exc)
    if key in _cache:
        return _cache[key]
    out = []
    for t in exc.get("head_templates", []):
        can = compose_fn(t["weapon"], t["frame"])
        x0, y0, x1, y1 = t["box"]
        crop = can[y0:y1 + 1, x0:x1 + 1].copy()
        out.append(dict(img=crop, name=f"{t['weapon']}:{t['frame']}", facing_left=bool(t.get("facing_left", False)), cost=float(t.get("cost", 0.0))))
    _cache[key] = out
    return out


def variants(tpls, angles):
    for ti, t in enumerate(tpls):
        for mirror in (False, True):
            for sc in (0.9, 1.0, 1.1):
                for ang in angles:
                    im = Image.fromarray(t["img"], "RGBA")
                    if mirror:
                        im = im.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                    if sc != 1.0:
                        im = im.resize((max(3, round(im.width * sc)), max(3, round(im.height * sc))), Image.BILINEAR)
                    if ang:
                        im = im.rotate(ang, resample=Image.BILINEAR, expand=True)
                    arr = np.array(im)
                    yield dict(t=ti, mirror=mirror ^ t["facing_left"], sc=sc, ang=ang, arr=arr)


def _angles(up, force):
    if force and "angles" in force:
        return list(force["angles"])
    body_ang = math.degrees(math.atan2(-up[1], up[0])) - 90
    base = [0, -20, 20, -40, 40]
    if abs(body_ang) >= 25:
        base = [0, round(body_ang), round(body_ang) - 25, round(body_ang) + 25]
    return base


def candidates(can, A, u, up, tpls, umax=0.45, force=None, K=10, gray=False):
    """Varias posiciones posibles de la cabeza (minimos locales del calce de las
    plantillas, distintas entre si), para elegir despues con continuidad temporal
    dentro del clip.  Cada una: dict(cx, cy, w, h, ang, facing_left, mask, score, rc, u)."""
    if not tpls:
        return []
    I = flat(can, gray)
    I2 = (I ** 2).sum(-1)
    c = _search(I, I2, A, u, umax, force, variants(tpls, _angles(up, force)), tpls, per_variant=3, gray=gray)
    c.sort(key=lambda d: d["score"])
    kept = []
    for d in c:
        if all((d["cx"] - k["cx"]) ** 2 + (d["cy"] - k["cy"]) ** 2 > 16 for k in kept):
            kept.append(d)
        if len(kept) >= K:
            break
    return kept


def refine(can, A, u, tpls, cand, umax=0.6):
    """Giro fino (pasos de 5 grados) de una candidata, misma plantilla/espejo/escala,
    a +-2 px de su posicion."""
    gray = cand.get("gray", False)
    I = flat(can, gray)
    I2 = (I ** 2).sum(-1)
    t = tpls[cand["tpl"]]
    fine = []
    for d in (-10, -5, 0, 5, 10):
        for v in variants([t], [cand["ang"] + d]):
            if v["mirror"] == cand["facing_left"] and v["sc"] == cand["sc"]:
                v["t"] = cand["tpl"]; fine.append(v)
    force = dict(center=[cand["cx"], cand["cy"]], radius=2)
    c = _search(I, I2, A, u, umax, force, fine, tpls, per_variant=1, ring_max=1.01, gray=gray)
    if not c:
        return cand
    b = min(c, key=lambda d: d["score"])
    return b if b["score"] <= cand["score"] else cand


def mask_of(cand, A):
    """Mascara (en el lienzo) de una candidata, recortada al cuerpo."""
    y, x, mb = cand["mpos"]
    m = np.zeros(A.shape, bool)
    m[y:y + mb.shape[0], x:x + mb.shape[1]] = mb
    return m & A


def match(can, A, u, up, tpls, umax=0.4, force=None):
    """La mejor candidata (sin contexto temporal), con giro fino."""
    c = candidates(can, A, u, up, tpls, umax, force, K=1)
    if not c:
        return None
    b = refine(can, A, u, tpls, c[0]) if not (force and "angles" in force) else c[0]
    return dict(b, mask=mask_of(b, A))


def _sub(a, b, c):
    d = a - 2 * b + c
    return 0.0 if not np.isfinite(d) or d <= 1e-6 else float(np.clip((a - c) / (2 * d), -0.5, 0.5))


def _search(I, I2, A, u, umax, force, vars_, tpls, per_variant=1, ring_max=None, gray=False):
    out = []
    H, W = A.shape
    Af = A.astype(np.float32)
    ring_max = RING_MAX if ring_max is None else ring_max
    for v in vars_:
        T = v["arr"]
        m = (T[..., 3] > 128).astype(np.float32)
        if m.sum() < 20 or T.shape[0] >= H or T.shape[1] >= W:
            continue
        Tf = flat(T, gray)
        n = m.sum()
        # SSD con mascara via FFT
        s_i2 = signal.fftconvolve(I2, m[::-1, ::-1], mode="valid")
        cross = sum(signal.fftconvolve(I[..., c], (Tf[..., c] * m)[::-1, ::-1], mode="valid") for c in range(3))
        t2 = ((Tf ** 2).sum(-1) * m).sum()
        ssd = (s_i2 - 2 * cross + t2) / n
        # el centro debe caer en el cuerpo, en su mitad de arriba
        th, tw = T.shape[:2]
        cy0, cx0 = th // 2, tw // 2
        uc = u[cy0:cy0 + ssd.shape[0], cx0:cx0 + ssd.shape[1]]
        ac = A[cy0:cy0 + ssd.shape[0], cx0:cx0 + ssd.shape[1]]
        cover = signal.fftconvolve(Af, m[::-1, ::-1], mode="valid") / n
        ok = ac & (uc < umax) & (cover > 0.75)
        # la cabeza SOBRESALE del cuerpo: alrededor de la plantilla hay fondo (salvo
        # el cuello).  El pecho/espalda calzan en color pero estan rodeados de cuerpo.
        mb = m > 0
        ring = (ndimage.binary_dilation(mb, iterations=RING) & ~mb).astype(np.float32)
        rc = signal.fftconvolve(Af, ring[::-1, ::-1], mode="valid") / max(1.0, ring.sum())
        rc = rc[:ssd.shape[0], :ssd.shape[1]]
        ok &= rc < ring_max
        score = ssd * (0.75 + 0.5 * rc)
        if force and "center" in force:
            fx, fy = force["center"]
            r = force.get("radius", 3)
            yy, xx = np.mgrid[0:ssd.shape[0], 0:ssd.shape[1]]
            ok &= (np.abs(xx + cx0 - fx) <= r) & (np.abs(yy + cy0 - fy) <= r)
        if not ok.any():
            continue
        score = np.where(ok, score, np.inf)
        if per_variant == 1:
            locs = [np.unravel_index(np.argmin(score), score.shape)]
        else:
            lm = (ndimage.minimum_filter(score, size=7, mode="constant", cval=np.inf) == score) & np.isfinite(score)
            ys_, xs_ = np.nonzero(lm)
            o = np.argsort(score[ys_, xs_])[:per_variant]
            locs = list(zip(ys_[o], xs_[o]))
        Hs, Ws = score.shape
        for (y, x) in locs:
            sc = score[y, x]
            ys, xs = np.nonzero(mb)
            ys = ys + y; xs = xs + x
            # subpixel: parabola por el minimo en cada eje
            ox = _sub(score[y, x - 1], sc, score[y, x + 1]) if 0 < x < Ws - 1 else 0.0
            oy = _sub(score[y - 1, x], sc, score[y + 1, x]) if 0 < y < Hs - 1 else 0.0
            out.append(dict(score=float(sc), cx=float(xs.mean()) + ox, cy=float(ys.mean()) + oy,
                            w=float(tpls[v["t"]]["img"].shape[1] * v["sc"]), h=float(tpls[v["t"]]["img"].shape[0] * v["sc"]),
                            ang=float(v["ang"]), facing_left=v["mirror"], mpos=(int(y), int(x), mb), tpl=v["t"], sc=v["sc"],
                            rc=float(rc[y, x]), u=float(uc[y, x]), gray=gray, tcost=tpls[v["t"]].get("cost", 0.0)))
    return out
