#!/usr/bin/env python3
"""Fase 1: biblioteca de partes de RONIN (imagenes del usuario).

- Rampas de color por material, medidas en las piezas aisladas del turnaround
  (torso/brazos = polera, muslos = pantalon, cinto = sash, mano/pie = piel) y
  en los cuadros de combate de sheet_a (para promediar luz/sombra reales).
- Cabezas: 3/4 mirando a la derecha recortadas de varios cuadros de combate de
  sheet_a / sheet_b (cara + pelo), y de perfil (turnaround, espejada).
La polera de RONIN-KRATOS va en ROJO CARMESI (pedido del usuario): la rampa
roja se deriva de la azul marino real de RONIN cambiando el tono y
conservando el orden de luces y sombras (ver crimson_from_navy).
Salida: out/ronin_parts.npz + out/ronin_parts.png (hoja de revision).
"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

sys.path.insert(0, str(Path(__file__).resolve().parent))
from rk_common import hsv  # noqa: E402

HERE = Path(__file__).resolve().parent
OUT = HERE / "out"
SRC = Path("/tmp/claude-0/-home-user/05b0c2b9-43b9-5c8e-82ee-61da3b1aff5a/scratchpad/newchar")
# copia estable de las imagenes del usuario dentro del repo (el script no depende de /tmp)
LIB = HERE / "ronin_src"
FILES = {
    "head_side": "pieces/turnaround/head.png", "torso": "pieces/turnaround/torso.png",
    "upperarmA": "pieces/turnaround/upperarmA.png", "upperarmB": "pieces/turnaround/upperarmB.png",
    "forearmA": "pieces/turnaround/forearmA.png", "forearmB": "pieces/turnaround/forearmB.png",
    "sash": "pieces/turnaround/sash.png", "pelvis": "pieces/turnaround/pelvis.png",
    "thighA": "pieces/turnaround/thighA.png", "thighB": "pieces/turnaround/thighB.png",
    "shinA": "pieces/turnaround/shinA.png", "shinB": "pieces/turnaround/shinB.png",
    "punoA": "pieces/hand_sword/punoA.png",
}
FRAMES = ["r0_c0", "r0_c3", "r1_c0", "r1_c3", "r2_c3", "r3_c3", "r4_c2", "r5_c3", "r6_c0", "r6_c6", "r7_c0", "r7_c2"]
HEAD_FRAMES = ["r4_c2", "r5_c3", "r3_c3", "r2_c3"]   # cabezas despejadas (sin puno delante)


def load(name):
    p = LIB / (name + ".png")
    return np.array(Image.open(p).convert("RGBA"))


def sync_sources():
    """Copia (una vez) las imagenes del usuario a tools/ronin_kratos/ronin_src/."""
    LIB.mkdir(exist_ok=True)
    pairs = dict(FILES)
    for f in FRAMES + HEAD_FRAMES:
        pairs[f] = f"source/sheet2/extracted/{f}.png"
    for k, rel in pairs.items():
        dst = LIB / (k + ".png")
        if not dst.exists() and (SRC / rel).exists():
            Image.open(SRC / rel).save(dst)


def mats(px):
    h, s, v = hsv(px)
    a = px[..., 3] > 160
    inner = ndimage.binary_erosion(a, iterations=2)
    return dict(a=a, inner=inner,
                navy=inner & (h > 195) & (h < 260) & (s > 0.15) & (v > 0.06),
                skin=inner & (h > 8) & (h < 34) & (s > 0.2) & (s < 0.62) & (v > 0.45),
                white=inner & (s < 0.14) & (v > 0.45),
                yellow=inner & (h > 32) & (h < 58) & (s > 0.4) & (v > 0.45))


def ramp(cols, n=7):
    cols = np.asarray(cols, np.float32)
    lum = cols @ np.array([0.299, 0.587, 0.114], np.float32)
    o = np.argsort(lum)
    cols, lum = cols[o], lum[o]
    out = []
    for q in np.linspace(0.04, 0.97, n):
        i = int(q * (len(cols) - 1))
        lo, hi = max(0, i - len(cols) // (3 * n)), min(len(cols), i + len(cols) // (3 * n) + 1)
        out.append(cols[lo:hi].mean(0))
    return np.array(out)


def crimson_from_navy(navy):
    """Misma rampa de luces/sombras que la polera azul, en rojo carmesi:
    sombras a vino oscuro, luces a rojo vivo (orden de luminancia intacto)."""
    import colorsys
    out = []
    lum = navy @ np.array([0.299, 0.587, 0.114])
    t = (lum - lum.min()) / max(1e-6, lum.max() - lum.min())
    for k, c in enumerate(navy):
        hh = (356 - 4 * t[k]) / 360                   # carmesi (sombras un poco mas vino)
        ss = 0.86 - 0.12 * t[k]
        vv = 0.20 + 0.62 * t[k] ** 0.85
        out.append(np.array(colorsys.hsv_to_rgb(hh, ss, vv)) * 255)
    return np.array(out)


def extract_head(px):
    """Cabeza (pelo + cara + cuello) de un cuadro de combate de RONIN: el pelo
    blanco y la cara (componente de piel mas alto) hasta la barbilla/cuello;
    se descarta la polera azul del hombro que queda al lado."""
    m = mats(px)
    a = px[..., 3] > 40
    h_, s_, v_ = hsv(px)
    ys = np.nonzero(a.any(1))[0]
    top = ys.min()
    skin = a & (h_ > 8) & (h_ < 34) & (s_ > 0.2) & (v_ > 0.4)
    skin[top + 100:] = False
    lab, n = ndimage.label(skin)
    # cara = componente de piel cuyo punto mas alto es el mas alto
    best, by = 0, 1e9
    for i in range(1, n + 1):
        yy = np.nonzero(lab == i)[0]
        if len(yy) > 30 and yy.min() < by:
            best, by = i, yy.min()
    chin = np.nonzero(lab == best)[0].max()
    navyish = (h_ > 195) & (h_ < 265) & (s_ > 0.24) & (v_ < 0.55)
    keep = a & (np.arange(px.shape[0])[:, None] <= chin) & ~navyish
    keep = ndimage.binary_opening(keep, iterations=1) | (keep & ~ndimage.binary_dilation(navyish, iterations=1))
    lab, n = ndimage.label(keep)
    if n > 1:
        big = np.argmax(ndimage.sum(np.ones_like(lab), lab, range(1, n + 1))) + 1
        keep = lab == big
    head = px.copy()
    head[~keep] = 0
    ys, xs = np.nonzero(keep)
    return head[ys.min():ys.max() + 1, xs.min():xs.max() + 1]


def face_box(head):
    m = mats(head)
    sk = m["skin"]
    ys, xs = np.nonzero(sk)
    return (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())), (float(xs.mean()), float(ys.mean()))


def build():
    sync_sources()
    parts = {k: load(k) for k in list(FILES) + FRAMES + HEAD_FRAMES}
    navy, skin, white, yellow = [], [], [], []
    for k, px in parts.items():
        m = mats(px)
        navy.append(px[m["navy"]][:, :3]); skin.append(px[m["skin"]][:, :3])
        if k.startswith(("thigh", "shin", "pelvis", "r")):
            white.append(px[m["white"] & ~m["skin"]][:, :3])
        if k in ("sash", "pelvis") or k.startswith("r"):
            yellow.append(px[m["yellow"]][:, :3])
    R = dict(navy=ramp(np.concatenate(navy)), skin=ramp(np.concatenate(skin), 6),
             pants=ramp(np.concatenate(white), 6), sash=ramp(np.concatenate(yellow), 5))
    R["crimson"] = crimson_from_navy(R["navy"])
    heads = {}
    for f in HEAD_FRAMES:
        heads[f] = extract_head(parts[f])
    side = parts["head_side"][:, ::-1].copy()          # turnaround mira a la izquierda -> derecha
    heads["side"] = side
    return R, heads


def save_review(R, heads, path):
    from PIL import ImageDraw
    rows = []
    for k, r in R.items():
        sw = np.zeros((24, 24 * len(r), 3), np.uint8)
        for i, c in enumerate(r):
            sw[:, i * 24:(i + 1) * 24] = c
        rows.append((k, sw))
    W = 900
    H = 30 * len(rows) + 260
    M = Image.new("RGB", (W, H), (40, 40, 40)); d = ImageDraw.Draw(M)
    y = 4
    for k, sw in rows:
        d.text((4, y + 6), k, fill=(255, 255, 0)); M.paste(Image.fromarray(sw), (80, y)); y += 30
    x = 4
    for k, h in heads.items():
        im = Image.fromarray(h); im.thumbnail((200, 200))
        bg = Image.new("RGBA", im.size, (70, 110, 70, 255)); bg.alpha_composite(im)
        M.paste(bg.convert("RGB"), (x, y)); d.text((x, y + 205), k, fill=(255, 255, 0)); x += 210
    M.save(path)


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True)
    R, heads = build()
    save_review(R, heads, OUT / "ronin_parts.png")
    for k, r in R.items():
        print(k, r.astype(int).tolist())
