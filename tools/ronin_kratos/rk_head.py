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
BGC = np.array([90.0, 120.0, 90.0], np.float32)      # gris-verde neutro bajo lo transparente


def flat(rgba):
    a = rgba[..., 3:4].astype(np.float32) / 255
    return rgba[..., :3].astype(np.float32) * a + BGC * (1 - a)


def templates(compose_fn, exc):
    key = id(exc)
    if key in _cache:
        return _cache[key]
    out = []
    for t in exc.get("head_templates", []):
        can = compose_fn(t["weapon"], t["frame"])
        x0, y0, x1, y1 = t["box"]
        crop = can[y0:y1 + 1, x0:x1 + 1].copy()
        out.append(dict(img=crop, name=f"{t['weapon']}:{t['frame']}", facing_left=bool(t.get("facing_left", False))))
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


def match(can, A, u, up, tpls, umax=0.4, force=None):
    """Mejor posicion de la cabeza.  Devuelve dict(cx, cy, w, h, ang, facing_left, mask, score)."""
    if not tpls:
        return None
    I = flat(can)
    body_ang = math.degrees(math.atan2(-up[1], up[0])) - 90
    angles = [0, -20, 20, -40, 40] if abs(body_ang) < 25 else [round(body_ang), round(body_ang) - 25, round(body_ang) + 25, 0]
    if force and "angles" in force:
        angles = force["angles"]
    I2 = (I ** 2).sum(-1)
    best = None
    H, W = A.shape
    for v in variants(tpls, angles):
        T = v["arr"]
        m = (T[..., 3] > 128).astype(np.float32)
        if m.sum() < 20 or T.shape[0] >= H or T.shape[1] >= W:
            continue
        Tf = flat(T)
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
        cover = signal.fftconvolve(A.astype(np.float32), m[::-1, ::-1], mode="valid") / n
        ok = ac & (uc < umax) & (cover > 0.75)
        if force and "center" in force:
            fx, fy = force["center"]
            yy, xx = np.mgrid[0:ssd.shape[0], 0:ssd.shape[1]]
            ok &= (np.abs(xx + cx0 - fx) <= 3) & (np.abs(yy + cy0 - fy) <= 3)
        if not ok.any():
            continue
        ssd = np.where(ok, ssd, np.inf)
        k = np.unravel_index(np.argmin(ssd), ssd.shape)
        sc = ssd[k]
        if best is None or sc < best["score"]:
            y, x = k
            mask = np.zeros(A.shape, bool)
            mask[y:y + th, x:x + tw] = m > 0
            ys, xs = np.nonzero(mask)
            best = dict(score=float(sc), cx=float(xs.mean()), cy=float(ys.mean()),
                        w=float(tpls[v["t"]]["img"].shape[1] * v["sc"]), h=float(tpls[v["t"]]["img"].shape[0] * v["sc"]),
                        ang=float(v["ang"]), facing_left=v["mirror"], mask=mask & A, tpl=v["t"])
    return best
