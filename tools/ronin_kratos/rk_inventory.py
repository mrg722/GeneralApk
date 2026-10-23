#!/usr/bin/env python3
"""Fase 0: inventario de las piezas de Kratos (las 4 armas, leido en vivo de
data/kratos/kratos_*.txt).

Kratos NO es un rig de extremidades como el heroe KF: casi todas sus piezas
"corporales" son el cuerpo entero (o medio cuerpo) ya renderizado en 3D, y el
resto son armas, cadenas y efectos. Por eso la clasificacion es en dos niveles:
  - por pieza: body (cuerpo de Kratos), weapon (hoja metalica), chain, fx
    (fuego, estelas, brillos semitransparentes);
  - por pixel, dentro de las piezas body (ver rk_segment.py): cabeza, torso,
    brazo, guantelete, mano, falda, cinto, muslo, bota, pie...
Las piezas dudosas se fuerzan en exceptions.json ("piece_class").

Salida: tools/ronin_kratos/out/inventory.json y contact_pieces_<clase>.png.
"""
import json, sys
from collections import defaultdict
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

sys.path.insert(0, str(Path(__file__).resolve().parent))
from rk_common import ROOT, DATA, WEAPONS, CharFile, sheet, hsv, on_bg  # noqa: E402

HERE = Path(__file__).resolve().parent
OUT = HERE / "out"


def materials(px):
    """Mascaras de material de Kratos para un parche RGBA."""
    h, s, v = hsv(px)
    a = px[..., 3] > 128
    skin = a & (s < 0.36) & (v > 0.24)
    warm = a & ~skin & (s >= 0.36) & ((h < 50) | (h > 330)) & (v > 0.08)
    red = (h < 16) | (h > 340)
    nb = ndimage.uniform_filter(skin.astype(np.float32), 5)
    tattoo = warm & red & (v > 0.5) & (s > 0.45) & (nb > 0.3)
    gold = warm & (h >= 30) & (h < 62) & (v > 0.55)
    dark = a & (v <= 0.24) & ~warm
    fire = a & (h >= 25) & (h < 65) & (s > 0.35) & (v > 0.85)
    purple = a & (h > 250) & (h < 330) & (s > 0.3)
    cloth = a & red & (s > 0.45) & (v > 0.08) & (v < 0.6)
    steel = a & (h > 180) & (h < 260) & (s > 0.12)
    return dict(alpha=a, skin=skin | tattoo, tattoo=tattoo, warm=warm & ~tattoo, gold=gold, dark=dark,
                fire=fire, purple=purple, cloth=cloth, steel=steel)


def piece_stats(px):
    al = px[..., 3]
    n = max(1, int((al > 0).sum()))
    m = materials(px)
    solid = (al > 200).sum() / n
    na = max(1, int(m["alpha"].sum()))
    st = {k: float(m[k].sum() / na) for k in ("skin", "tattoo", "warm", "gold", "dark", "fire", "purple", "cloth", "steel")}
    st.update(solid=float(solid), area=int(n), opaque=int(na))
    return st


def classify_piece(st, w, h):
    """body = cuerpo de Kratos (entero o en partes grandes); weapon = hojas de
    acero / Cestus / Espadas; fx = fuego, estelas, brillos, cadenas."""
    if st["opaque"] < 30:
        return "fx"
    if st["solid"] < 0.4:
        # Kratos fantasma / con aura (semitransparente) sigue siendo su cuerpo
        if st["opaque"] > 2000 and st["skin"] > 0.2 and st["cloth"] > 0.06 and st["fire"] < 0.12:
            return "body"
        return "fx"
    if st["steel"] > 0.15:
        return "weapon"
    big = max(w, h) >= 45
    if big and st["skin"] > 0.10 and st["cloth"] > 0.07:
        # las Espadas del Caos encendidas se parecen (piel+rojo): son chicas y con fuego/oro
        bladelike = st["opaque"] < 1200 and (st["fire"] >= 0.05 or st["gold"] >= 0.06)
        if not bladelike:
            return "body"
    if st["gold"] > 0.12 or st["skin"] + st["dark"] > 0.6:
        return "weapon"
    return "fx"


def load_exceptions():
    p = HERE / "exceptions.json"
    return json.loads(p.read_text()) if p.exists() else {}


def pkey(k):
    return "%s:%d,%d,%d,%d" % k


def inventory():
    exc = load_exceptions().get("piece_class", {})
    pieces = {}
    for w in WEAPONS:
        cf = CharFile(DATA / f"kratos_{w}.txt")
        clip_of = defaultdict(set)
        for cn, fl in cf.clips.items():
            for f in fl:
                clip_of[f].add(cn)
        for fi, fr in enumerate(cf.frames):
            for p in fr["pieces"]:
                k = cf.key(p)
                e = pieces.setdefault(pkey(k), dict(sheet=k[0], rect=list(k[1:]), uses=0, weapons=set(), clips=set()))
                e["uses"] += 1; e["weapons"].add(w); e["clips"] |= clip_of[fi]
    for key, e in pieces.items():
        x, y, w, h = e["rect"]
        px = sheet(e["sheet"])[y:y + h, x:x + w]
        e["stats"] = piece_stats(px)
        e["auto"] = classify_piece(e["stats"], w, h)
        e["cls"] = exc.get(key, e["auto"])
        e["weapons"] = sorted(e["weapons"]); e["clips"] = sorted(e["clips"])
    return pieces


def contact(pieces, cls, path, cell=110, cols=16):
    items = [(k, e) for k, e in sorted(pieces.items(), key=lambda kv: (-kv[1]["uses"], kv[0])) if e["cls"] == cls]
    if not items:
        return
    from PIL import ImageDraw
    rows = (len(items) + cols - 1) // cols
    M = Image.new("RGB", (cols * cell, rows * (cell + 12)), (30, 30, 30))
    d = ImageDraw.Draw(M)
    for i, (k, e) in enumerate(items):
        x, y, w, h = e["rect"]
        px = sheet(e["sheet"])[y:y + h, x:x + w]
        im = Image.fromarray(on_bg(px))
        im.thumbnail((cell - 4, cell - 4))
        r, c = divmod(i, cols)
        M.paste(im, (c * cell + 2, r * (cell + 12) + 2))
        d.text((c * cell + 2, r * (cell + 12) + cell - 2), f"{e['sheet']}@{x},{y} x{e['uses']}", fill=(255, 255, 0))
    M.save(path)


def main():
    OUT.mkdir(exist_ok=True)
    pieces = inventory()
    cnt = defaultdict(int)
    for e in pieces.values():
        cnt[e["cls"]] += 1
    (OUT / "inventory.json").write_text(json.dumps(pieces, indent=0))
    for c in cnt:
        contact(pieces, c, OUT / f"contact_pieces_{c}.png")
    print("piezas unicas:", len(pieces), dict(cnt))


if __name__ == "__main__":
    main()
