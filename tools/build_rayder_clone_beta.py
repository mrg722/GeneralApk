#!/usr/bin/env python3
"""RAYDER CLON BETA (morado electrico) montado sobre una COPIA de los
movimientos del heroe KF de pelo blanco.

1. Recorta cada animacion de la hoja del Rayder clon (assets/characters/rayder/
   source/rayder_clone_hoja.png) por secciones (IDLE, CAMINAR, ... FINISHER).
2. Lo pinta de morado claro electrico (los rojos); la piel queda intacta.
3. Lee de animation.bin (respaldo de la APK) cuantos pasos y cuanto dura cada
   accion del heroe KF (idle A0, caminar A1, ..., habilidades A20-A24,
   transformacion A31) y, paso por paso, pone encima el recorte equivalente del
   clon. Asi el clon hereda la cadencia, los golpes y las habilidades del KF sin
   tocar al personaje KF original.
Salida: assets/characters/rayder/rayder_clon_beta_atlas.png + clips en
data/sprite_manifest.json (atlas "rayder_clon_beta").
"""
import colorsys, json, sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/characters/rayder/source/rayder_clone_hoja.png"
OUT = ROOT / "assets/characters/rayder/rayder_clon_beta_atlas.png"
MANIFEST = ROOT / "data/sprite_manifest.json"
sys.path.insert(0, str(ROOT / "tools/apk"))
from kf_decode import load as kf_load   # noqa: E402

CW, CH, PIVOT, COLS = 240, 120, (120, 112), 16
# seccion: (x0, x1, y0, y1)
SECTIONS = {
    "idle": (12, 328, 205, 284), "walk": (335, 628, 205, 284), "run": (635, 952, 205, 284),
    "dash": (958, 1322, 205, 284),
    "jump": (12, 188, 318, 402), "fall": (193, 328, 318, 402), "land": (335, 488, 318, 402),
    "crouch": (493, 636, 318, 402), "block": (641, 770, 318, 402), "hit": (775, 940, 318, 402),
    "hurt": (945, 1135, 318, 402), "death": (1140, 1525, 318, 402),
    "basic": (12, 272, 440, 521), "combo": (277, 515, 440, 521), "kick": (520, 705, 440, 521),
    "combo_adv": (710, 1187, 440, 521), "counter": (1192, 1525, 440, 521),
    "wave": (12, 708, 555, 646), "rage": (715, 1525, 555, 646),
    "teleport": (12, 215, 678, 781), "clones": (220, 540, 678, 781), "aerial": (546, 757, 678, 781),
    "finisher": (765, 1525, 678, 781),
}
UNIT = 44   # ancho tipico de una figura
UNIT_BY = {"death": 72, "finisher": 58, "wave": 50, "dash": 58, "aerial": 50}

# clip del juego -> (accion del heroe KF que da los pasos y tiempos, seccion del clon)
CLIPS = {
    "idle": (0, "idle", True), "walk": (1, "walk", True), "run": (2, "run", True), "dash": (3, "dash", False),
    "punch1": (6, "basic", False), "atk1": (6, "basic", False),
    "punch2": (7, "combo", False), "atk2": (7, "combo", False),
    "punch3": (8, "combo_adv", False), "atk3": (8, "combo_adv", False),
    "kick": (9, "kick", False), "atk4": (9, "kick", False),
    "special": (10, "wave", False), "energy": (10, "wave", False), "finisher": (10, "finisher", False),
    "hit": (11, "hit", False), "hit_high": (11, "hit", False), "hit_low": (11, "hurt", False),
    "knockdown": (13, "death", False), "air": (14, "fall", False), "airborne": (14, "fall", False),
    "getup": (16, "land", False), "defeat": (17, "death", False),
    "super": (19, "rage", False), "rage_attack": (19, "rage", False), "dash_attack": (3, "dash", False),
    "block": (15, "block", True), "recovery": (15, "idle", False),
    "skill1": (20, "counter", False), "skill2": (21, "teleport", False), "skill3": (22, "clones", False),
    "skill4": (23, "aerial", False), "skill5": (24, "finisher", False), "transform": (31, "rage", False),
    "victory": (18, "idle", False),
}


def purple(rgb):
    """Rojos -> morado claro electrico; piel, blancos y negros intactos."""
    out = rgb.copy()
    f = rgb.astype(np.float32) / 255.0
    mx, mn = f.max(-1), f.min(-1)
    sat = np.where(mx > 0, (mx - mn) / np.maximum(mx, 1e-6), 0)
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    hue = np.zeros_like(r)
    m = mx - mn > 1e-6
    hr = (mx == r) & m
    hue[hr] = ((g - b)[hr] / (mx - mn)[hr]) % 6 * 60
    hg = (mx == g) & m & ~hr
    hue[hg] = ((b - r)[hg] / (mx - mn)[hg] + 2) * 60
    hb = m & ~hr & ~hg
    hue[hb] = ((r - g)[hb] / (mx - mn)[hb] + 4) * 60
    warm = (hue < 25) | (hue > 330)
    skin = (hue >= 8) & (hue <= 40) & (sat < 0.62) & (mx > 0.42)
    red = warm & (sat > 0.45) & ~skin & (mx > 0.18)
    ys, xs = np.nonzero(red)
    for y, x in zip(ys, xs):
        v = min(1.0, mx[y, x] * 1.25 + 0.08)
        s = sat[y, x] * 0.62
        rr, gg, bb = colorsys.hsv_to_rgb(282 / 360, s, v)
        out[y, x] = (int(rr * 255), int(gg * 255), int(bb * 255))
    return out


def foreground(img, box):
    """Fondo = panel gris liso conectado al borde de la seccion. Se inunda desde
    el borde por pixeles parecidos al gris del panel; el contorno negro de cada
    figura frena la inundacion, asi la ropa negra queda dentro de la figura."""
    x0, x1, y0, y1 = box
    sec = img[y0:y1, x0:x1]
    est = np.stack([ndimage.median_filter(sec[..., c], size=(9, 61)) for c in range(3)], -1)
    near = np.abs(sec - est).max(-1) < 15
    lab, _ = ndimage.label(near)
    border = np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]])
    bg = np.isin(lab, np.unique(border[border > 0]))
    fg = ~bg
    fg = ndimage.binary_opening(fg, iterations=1)
    fg = ndimage.binary_fill_holes(fg)
    return fg


def segments(col, unit):
    segs, s, gap = [], None, 0
    for x, v in enumerate(list(col) + [0] * 6):
        if v > 1:
            if s is None: s = x
            gap = 0; e = x
        elif s is not None:
            gap += 1
            if gap >= 2: segs.append((s, e + 1)); s = None
    out = []
    for a, b in segs:
        w = b - a
        if w < 12: continue
        k = max(1, round(w / unit))
        cuts = [a]
        for i in range(1, k):
            c = a + w * i // k
            cuts.append(min(range(max(a + 5, c - 14), min(b - 5, c + 14)), key=lambda x: col[x]))
        cuts.append(b)
        out += [(cuts[i], cuts[i + 1]) for i in range(len(cuts) - 1)]
    return out


def main():
    img = np.array(Image.open(SRC).convert("RGB")).astype(float)
    painted = purple(img.astype(np.uint8))
    seqs = {}
    for name, (x0, x1, y0, y1) in SECTIONS.items():
        band = foreground(img, (x0, x1, y0, y1))
        col = ndimage.binary_opening(band, structure=np.ones((3, 3))).sum(0)
        frames = []
        ground = y1 - y0 - 1
        rows = np.nonzero(band.any(1))[0]
        if len(rows): ground = rows.max()
        areas = []
        for a, b in segments(col, UNIT_BY.get(name, UNIT)):
            mask = band[:, a:b]
            lab, n = ndimage.label(mask)
            if n == 0: continue
            sz = ndimage.sum(mask, lab, range(1, n + 1))
            keep = np.isin(lab, [i + 1 for i, v in enumerate(sz) if v >= 0.05 * sz.max()])
            if keep.sum() < 60: continue
            areas.append(keep.sum())
            rgba = np.dstack([painted[y0:y1, x0 + a:x0 + b], (keep * 255).astype(np.uint8)])
            body = keep[: max(1, int(keep.shape[0] * 0.6))]
            cx = np.average(np.arange(keep.shape[1]), weights=body.sum(0) + 1e-6)
            cell = Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
            piece = Image.fromarray(rgba, "RGBA")
            cell.paste(piece, (round(PIVOT[0] - cx), PIVOT[1] - ground), piece)
            frames.append(cell)
        # fuera los trozos sueltos (menos de un 35 % del recorte tipico de la seccion)
        if areas:
            med = float(np.median(areas))
            frames = [f for f, ar in zip(frames, areas) if ar >= 0.35 * med]
        seqs[name] = frames
        print(f"{name}: {len(frames)} recortes")
    # atlas: todos los recortes
    cells, index = [], {}
    for name, frames in seqs.items():
        index[name] = list(range(len(cells), len(cells) + len(frames)))
        cells += frames
    rows = (len(cells) + COLS - 1) // COLS
    atlas = Image.new("RGBA", (COLS * CW, rows * CH), (0, 0, 0, 0))
    for i, c in enumerate(cells):
        atlas.paste(c, ((i % COLS) * CW, (i // COLS) * CH))
    atlas.save(OUT)
    # clips: pasos y tiempos de la COPIA del heroe KF, recortes del clon encima
    sprites, _ = kf_load(ROOT / "apk_reference/king_fighter_iii/bin/animation.bin")
    hero = sprites[0]
    clips = {}
    for clip, (act, sec, loop) in CLIPS.items():
        steps = hero["actions"][act]["steps"] if act < len(hero["actions"]) else []
        src = index.get(sec) or index["idle"]
        if not steps: steps = [{"ticks": 2}] * len(src)
        n = len(steps)
        frames = [src[min(len(src) - 1, i * len(src) // n)] for i in range(n)]
        durs = [round(max(1, s["ticks"]) * 0.05, 3) for s in steps]
        clips[clip] = {"frames": frames, "durations": durs, "loop": loop}
    m = json.loads(MANIFEST.read_text())
    m["atlases"]["rayder_clon_beta"] = {"path": str(OUT.relative_to(ROOT)), "grid": [COLS, rows], "cell": [CW, CH],
                                         "pivot": list(PIVOT)}
    m["clips"]["rayder_clon_beta"] = clips
    MANIFEST.write_text(json.dumps(m, indent=2, ensure_ascii=False))
    print(f"atlas {len(cells)} recortes, {len(clips)} clips con la cadencia del heroe KF")


if __name__ == "__main__":
    main()
