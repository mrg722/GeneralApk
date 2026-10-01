#!/usr/bin/env python3
"""Despiece GRANDE del Rayder clon, con la misma formula del heroe KF.

El KF tiene 208 piezas en su hoja de cuerpo (cabezas, torsos, mangas,
antebrazos, puños, piernas, botas en muchos angulos). Aqui se arma una hoja
igual para el clon: para cada pieza del KF se busca, en TODAS las animaciones
recortadas del clon (137 cuadros a ~70 px, la misma escala que el KF), la
parte del cuerpo del clon que mejor encaja (misma forma, mismo angulo: se
prueban los 8 giros/espejos) y en la zona correcta del cuerpo (cabeza arriba,
botas abajo). Esa parte se recorta con la silueta de la pieza y pasa a ser la
pieza del clon. El juego arma los cuadros con las mismas posiciones, giros y
tiempos del KF.  Salida: kf_clone/img_1_rig_<color>.png/.json (formato rig).
"""
import argparse, io, json, sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage, signal

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/apk")); sys.path.insert(0, str(ROOT / "tools"))
from kf_decode import load  # noqa: E402
from build_kf_clone_pieces import classify, hsv  # noqa: E402

ATLAS = ROOT / "assets/characters/rayder/rayder_clon_beta_atlas.png"   # 137 recortes morados
CW, CH, COLS = 240, 120, 16
# zona vertical del cuerpo (0 = cabeza, 1 = pies) donde se busca cada parte
ZONE = {"head": (0.0, 0.3), "torso_head": (0.0, 0.5), "torso": (0.1, 0.55), "jacket": (0.1, 0.6),
        "sleeve": (0.12, 0.6), "forearm": (0.2, 0.7), "fist": (0.2, 0.75), "thigh": (0.4, 0.8),
        "shin": (0.55, 1.0), "boot": (0.75, 1.05)}


def transforms(a):
    out = []
    for k in range(4):
        r = np.rot90(a, k)
        out += [r, r[:, ::-1]]
    return out


def main(color):
    sprites, images = load(ROOT / "apk_reference/king_fighter_iii/bin/animation.bin")
    sheet = np.array(Image.open(io.BytesIO(images[1]["png"])).convert("RGBA"))
    clips = images[1]["clips"]
    atl = np.array(Image.open(ATLAS).convert("RGBA"))
    if color == "green":   # morado -> verde electrico
        h, s, v = hsv(atl.astype(float))
        sel = (h > 250) & (h < 320) & (s > 0.2) & (atl[..., 3] > 0)
        atl[..., :3][sel] = atl[..., :3][sel][:, [1, 0, 2]]          # R<->G: morado -> verde
    frames = []
    for f in range((atl.shape[0] // CH) * COLS):
        c = atl[(f // COLS) * CH:(f // COLS + 1) * CH, (f % COLS) * CW:(f % COLS + 1) * CW]
        a = c[..., 3] > 60
        if a.sum() < 400: continue
        ys = np.nonzero(a.any(1))[0]
        frames.append((c, a.astype(float), c[..., :3].astype(float).mean(-1), ys.min(), ys.max()))
    print(len(frames), "cuadros del clon")
    tiles, counts = [], {}
    for i, (x, y, w, h) in enumerate(clips):
        kf = sheet[y:y + h, x:x + w] if w > 0 and h > 0 else np.zeros((1, 1, 4), np.uint8)
        mask = kf[..., 3] > 0
        cls = classify(kf, mask) if mask.sum() >= 6 else "other"
        if cls not in ZONE:
            tiles.append(kf.copy()); counts["kf"] = counts.get("kf", 0) + 1; continue
        z0, z1 = ZONE[cls]
        lum = kf[..., :3].astype(float).mean(-1)
        best = (-9, None)
        for t, (m, l) in enumerate(zip(transforms(mask.astype(float)), transforms(np.where(mask, lum, 0)))):
            n = m.sum(); mh, mw = m.shape
            lm = l - (l[m > 0].mean() if n else 0); lm *= m
            for c, a, L, top, bot in frames:
                if mh >= a.shape[0] or mw >= a.shape[1]: continue
                inter = signal.fftconvolve(a, m[::-1, ::-1], mode="valid")
                area = signal.fftconvolve(a, np.ones_like(m), mode="valid")
                iou = inter / np.maximum(n + area - inter, 1)
                corr = signal.fftconvolve(L * a, lm[::-1, ::-1], mode="valid") / (np.abs(lm).sum() * 60 + 1)
                sc = iou + 0.15 * np.clip(corr, -1, 1)
                # zona del cuerpo
                cy = (np.arange(sc.shape[0]) + mh / 2 - top) / max(1, bot - top)
                sc[(cy < z0) | (cy > z1), :] = -9
                k = np.unravel_index(np.argmax(sc), sc.shape)
                if sc[k] > best[0]: best = (sc[k], (c, k, t))
        if best[1] is None:
            tiles.append(kf.copy()); counts["kf"] = counts.get("kf", 0) + 1; continue
        c, (py, px), t = best[1]
        tm = transforms(mask)[t]
        patch = c[py:py + tm.shape[0], px:px + tm.shape[1]].copy()
        # deshacer el giro para volver a la orientacion de la pieza del KF
        inv = {0: lambda a: a}
        k4, flip = t // 2, t % 2
        if flip: patch = patch[:, ::-1]
        patch = np.rot90(patch, -k4)
        lim = ndimage.binary_dilation(mask, iterations=1)[: patch.shape[0], : patch.shape[1]]
        tile = np.zeros_like(kf)
        tile[: patch.shape[0], : patch.shape[1]] = patch
        tile[~(lim & (tile[..., 3] > 0))] = 0
        # huecos donde el clon no cubre: el contorno del KF en oscuro
        hole = mask & (tile[..., 3] == 0)
        tile[hole] = [24, 20, 30, 255]
        tiles.append(tile)
        counts[cls] = counts.get(cls, 0) + 1
    W = 1024; xx = yy = rowh = 0; rects = []
    for tl in tiles:
        if xx + tl.shape[1] > W: xx = 0; yy += rowh + 1; rowh = 0
        rects.append((xx, yy, tl.shape[1], tl.shape[0])); xx += tl.shape[1] + 1; rowh = max(rowh, tl.shape[0])
    out = np.zeros((yy + rowh, W, 4), np.uint8)
    for (ax, ay, aw, ah), tl in zip(rects, tiles): out[ay:ay + ah, ax:ax + aw] = tl
    d = ROOT / "assets/characters/rayder/kf_clone"
    Image.fromarray(out, "RGBA").save(d / f"img_1_rig_{color}.png")
    (d / f"img_1_rig_{color}.json").write_text(json.dumps({"image": 1, "pad": 0, "clips": rects}))
    print(color, counts)


if __name__ == "__main__":
    ap = argparse.ArgumentParser(); ap.add_argument("--color", default="purple")
    main(ap.parse_args().color)
