#!/usr/bin/env python3
"""Fase 4: auditoria visual.  Para cada arma y cada clip: Kratos (arriba) y
RONIN-KRATOS (abajo), mismo cuadro, mirando a la derecha y volteado; armado
desde los .txt finales (data/kratos/ronin_kratos_<arma>.txt + hojas 95xx), o
sea lo mismo que dibuja el motor.  Ademas una hoja antes/despues del color
de la polera (azul marino original de RONIN vs rojo carmesi).
Salida: tools/ronin_kratos/out/audit_<arma>_<n>.png y audit_color.png
"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from rk_common import DATA, WEAPONS, CharFile, compose, on_bg  # noqa: E402

OUT = HERE / "out"
CELL_W, CELL_H = 150, 150


def fit(img, w, h):
    im = Image.fromarray(img)
    if im.width > w or im.height > h:
        im.thumbnail((w, h))
    return im


def cell(cf, fi, mirror):
    c, (ox, oy) = compose(cf, fi, mirror=mirror)
    return on_bg(c, (70, 105, 70))


def clip_rows(cfk, cfr):
    seen = set()
    for name, frames in cfk.clips.items():
        sig = tuple(frames)
        if sig in seen:
            continue
        seen.add(sig)
        uniq = []
        for f in frames:
            if f not in uniq:
                uniq.append(f)
        yield name, uniq


def audit_weapon(w, max_rows=6):
    cfk = CharFile(DATA / f"kratos_{w}.txt")
    cfr = CharFile(DATA / f"ronin_kratos_{w}.txt")
    rows = list(clip_rows(cfk, cfr))
    # cada fila: nombre + pares (K/R) derecha, y los mismos volteados en una segunda fila
    blocks = []
    for name, frames in rows:
        for mirror in (False, True):
            ims = []
            for f in frames:
                k = fit(cell(cfk, f, mirror), CELL_W, CELL_H)
                r = fit(cell(cfr, f, mirror), CELL_W, CELL_H)
                ims.append((f, k, r))
            blocks.append((name + (" (volteado)" if mirror else ""), ims))
    pages, page = [], []
    for b in blocks:
        page.append(b)
        if len(page) >= max_rows * 2:
            pages.append(page); page = []
    if page:
        pages.append(page)
    paths = []
    for n, page in enumerate(pages):
        cols = 12
        hrows = sum((len(ims) + cols - 1) // cols for _, ims in page)
        M = Image.new("RGB", (cols * CELL_W + 4, hrows * (2 * CELL_H + 14) + len(page) * 16), (25, 25, 25))
        d = ImageDraw.Draw(M)
        y = 0
        for name, ims in page:
            d.text((4, y + 2), f"{w} / {name}", fill=(255, 255, 0)); y += 16
            for i in range(0, len(ims), cols):
                for j, (f, k, r) in enumerate(ims[i:i + cols]):
                    x = j * CELL_W
                    M.paste(k, (x, y)); M.paste(r, (x, y + CELL_H))
                    d.text((x + 2, y + 2 * CELL_H), f"#{f}", fill=(180, 180, 180))
                y += 2 * CELL_H + 14
        p = OUT / f"audit_{w}_{n}.png"
        M.save(p); paths.append(p)
    return paths


def audit_frames(w, per_page=48, cols=8, cw=112, ch=120):
    """Hoja compacta de revision: todos los cuadros con cuerpo, Kratos | RONIN-KRATOS."""
    from rk_inventory import inventory, pkey
    inv = inventory()
    body = {k for k, e in inv.items() if e["cls"] == "body"}
    cfk = CharFile(DATA / f"kratos_{w}.txt")
    cfr = CharFile(DATA / f"ronin_kratos_{w}.txt")
    fis = [i for i, f in enumerate(cfk.frames) if any(pkey(cfk.key(p)) in body for p in f["pieces"])]
    paths = []
    for n in range(0, len(fis), per_page):
        chunk = fis[n:n + per_page]
        rows = (len(chunk) + cols - 1) // cols
        M = Image.new("RGB", (cols * (2 * cw + 6), rows * (ch + 12)), (25, 25, 25))
        d = ImageDraw.Draw(M)
        for i, fi in enumerate(chunk):
            r, c = divmod(i, cols)
            x, y = c * (2 * cw + 6), r * (ch + 12)
            M.paste(fit(cell(cfk, fi, False), cw, ch), (x, y))
            M.paste(fit(cell(cfr, fi, False), cw, ch), (x + cw, y))
            d.text((x + 2, y + ch), f"#{fi}", fill=(255, 255, 0))
        p = OUT / f"review_{w}_{n // per_page}.png"
        M.save(p); paths.append(p)
    return paths


def audit_color(frames=(0, 16, 80, 118, 150, 200)):
    """Antes (polera azul marino de RONIN) / despues (rojo carmesi)."""
    import rk_build
    exc = rk_build.load_exc(); inv = rk_build.inventory()
    body = {k for k, e in inv.items() if e["cls"] == "body"}
    R, heads = rk_build.rk_ronin_parts.build()
    cf = CharFile(DATA / "kratos_espadas.txt")
    cols = []
    for fi in frames:
        row = []
        for shirt in ("navy", "crimson"):
            res, dbg = rk_build.process_frame(cf, fi, body, R, heads, exc, "espadas", shirt, debug=True)
            if dbg is None:
                break
            can = dbg[0]
            H, W = can.shape[:2]
            big = np.zeros((H + 2 * rk_build.PAD, W + 2 * rk_build.PAD, 4), np.float32)
            _, placed = rk_build.compose_body(cf, fi, body)
            for p, X, Y, o in placed:
                t = res[rk_build.pkey(cf.key(p))][1].astype(np.float32)
                reg = big[Y:Y + t.shape[0], X:X + t.shape[1]]
                a = t[..., 3:4] / 255
                reg[..., :3] = t[..., :3] * a + reg[..., :3] * (1 - a)
                reg[..., 3:4] = np.maximum(reg[..., 3:4], t[..., 3:4])
            row.append(on_bg(big.clip(0, 255).astype(np.uint8), (70, 105, 70)))
        if len(row) == 2:
            cols.append(row)
    S = 2
    Wc = max(max(a.shape[1], b.shape[1]) for a, b in cols) * S
    Hc = max(max(a.shape[0], b.shape[0]) for a, b in cols) * S
    M = Image.new("RGB", (Wc * len(cols), 2 * Hc + 30), (25, 25, 25))
    d = ImageDraw.Draw(M)
    for i, (a, b) in enumerate(cols):
        for j, im in enumerate((a, b)):
            big = np.kron(im, np.ones((S, S, 1), np.uint8))
            M.paste(Image.fromarray(big), (i * Wc, j * (Hc + 15) + 15))
    d.text((4, 2), "ANTES: polera azul marino (RONIN original)", fill=(255, 255, 0))
    d.text((4, Hc + 17), "DESPUES: polera rojo carmesi (RONIN-KRATOS)", fill=(255, 255, 0))
    M.save(OUT / "audit_color.png")


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True)
    ws = sys.argv[1:] or WEAPONS
    for w in ws:
        for p in audit_frames(w) + audit_weapon(w):
            print(p.relative_to(HERE))
    audit_color()
    print("out/audit_color.png")
