#!/usr/bin/env python3
"""Extrae poses nuevas de las hojas 'v3' (combos, carga/disparo) y las agrega
como celdas NUEVAS al atlas existente assets/characters/rayder/rayder_kf_atlas.png
(no se toca ninguna celda ya usada). Mismo tamano de celda (270x175), mismo
pivote (135,168) y misma escala/complexion que las poses ya presentes (se
calibra contra la celda 0, idle, altura de pie = 110 px).

Fuentes:
  A) rayder_cruz_combos_transparente_v3.png - RGB, fondo "transparente"
     exportado como damero gris/blanco horneado en los pixeles (no alpha real).
     Grid 6 cols x 4 filas, celda 256x256.
  B) rayder_cruz_carga_disparo_poder_transparente_v3.png - RGBA real (alpha
     autentico, incluye el aura azul como parte del arte). Grid 8 cols x 4
     filas, celda 192x256.

Uso: python3 tools/build_rayder_cruz_v3_extra.py
"""
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = Path(__file__).resolve().parents[1]
ATLAS = ROOT / "assets/characters/max/max_atlas.png"
SRC_A = ROOT / "assets/characters/rayder/source/rayder_cruz_combos_transparente_v3.png"
SRC_B = ROOT / "assets/characters/rayder/source/rayder_cruz_carga_disparo_poder_transparente_v3.png"
OUT_SHEET = ROOT / "tools/rayder/out/rayder_cruz_v3_cells.png"  # hoja de verificacion (no comiteada)

CW, CH = 270, 175
PIVOT = (135, 168)
STAND_H = 110
COLS = 14   # ancho fijo del atlas (igual que el script original)
FIRST_FREE = 245  # primer indice de celda totalmente libre (no referenciado por ningun clip)


CHECKER_TONES = [(253, 253, 253), (211, 211, 211)]  # los dos tonos exactos del damero


def bg_mask_checker(cell_rgb):
    """Fondo = pixeles que calzan (tolerancia chica) con los DOS tonos exactos
    del damero, en cualquier parte de la celda (no solo el borde): un damero
    puede quedar encerrado dentro del aura/estela sin tocar el marco."""
    c = cell_rgb.astype(int)
    near = np.zeros(c.shape[:2], bool)
    for t in CHECKER_TONES:
        near |= np.abs(c - np.array(t)).max(-1) < 9
    # tambien el borde conectado algo menos estricto (antialias del damero)
    sat = c.max(-1) - c.min(-1)
    soft = (sat < 10) & (c.min(-1) > 195)
    seeds = np.zeros(soft.shape, bool)
    seeds[0, :] = seeds[-1, :] = seeds[:, 0] = seeds[:, -1] = True
    lab, _ = ndimage.label(soft)
    ids = np.unique(lab[seeds & soft])
    bg = near | np.isin(lab, ids[ids > 0])
    return bg


def crop_bbox(alpha, pad=2):
    ys, xs = np.nonzero(alpha > 40)
    if len(ys) == 0:
        return None
    y0, y1 = max(0, ys.min() - pad), ys.max() + 1 + pad
    x0, x1 = max(0, xs.min() - pad), xs.max() + 1 + pad
    return y0, y1, x0, x1


def place_on_cell(rgba_img, bottom_y, cx, k):
    """Escala rgba_img por k y lo pega en una celda CWxCH con los pies en el
    pivote (como el resto del atlas)."""
    w, h = rgba_img.size
    small = rgba_img.resize((max(1, round(w * k)), max(1, round(h * k))), Image.LANCZOS)
    a = small.getchannel("A").point(lambda v: 0 if v < 40 else min(255, int(v * 1.2)))
    small.putalpha(a)
    cell = Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
    px = round(PIVOT[0] - cx * k)
    py = round(PIVOT[1] - bottom_y * k)
    cell.paste(small, (px, py), small)
    return cell


def extract_checker_cell(img, row, col, cellw, cellh, nrows_total=4):
    y0, x0 = row * cellh, col * cellw
    c = img[y0:y0 + cellh, x0:x0 + cellw]
    bg = bg_mask_checker(c)
    fg = ~bg
    fg = ndimage.binary_opening(fg, iterations=1)
    lab, n = ndimage.label(fg, structure=np.ones((3, 3)))
    if n:
        sizes = ndimage.sum(fg, lab, range(1, n + 1))
        fg = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s > 0.02 * sizes.max()])
    # solo rellenar huecos CHICOS (ropa/sombras internas), nunca bolsones
    # grandes de damero atrapados dentro del aura/estela
    filled = ndimage.binary_fill_holes(fg)
    holes, nh = ndimage.label(filled & ~fg)
    for k, sl in enumerate(ndimage.find_objects(holes), 1):
        h = holes[sl] == k
        if h.sum() <= 60:
            fg[sl] |= h
    alpha = (fg * 255).astype(np.uint8)
    rgba = np.dstack([c.astype(np.uint8), alpha])
    return Image.fromarray(rgba, "RGBA")


def _drop_edge_bleed(alpha_bool):
    """La hoja B tiene filas pegadas: a veces una bota/mano de la fila de
    arriba asoma por el borde superior de la celda de abajo. Se descartan
    las piezas CHICAS que tocan el borde superior (bleed), conservando el
    resto (debris de explosiones, estelas sueltas, etc.)."""
    lab, n = ndimage.label(alpha_bool, structure=np.ones((3, 3)))
    if not n:
        return alpha_bool
    out = alpha_bool.copy()
    top_ids = np.unique(lab[0, :])
    for i in top_ids:
        if i == 0:
            continue
        comp = lab == i
        if comp.sum() < 150:
            out[comp] = False
    return out


def extract_alpha_cell(img_rgba, row, col, cellw, cellh):
    y0, x0 = row * cellh, col * cellw
    c = img_rgba[y0:y0 + cellh, x0:x0 + cellw]
    alpha = _drop_edge_bleed(c[..., 3] > 30)
    rgba = c.copy()
    rgba[..., 3] = (alpha * 255).astype(np.uint8)
    return Image.fromarray(rgba, "RGBA")


def extract_alpha_box(img_rgba, box):
    """Igual que extract_alpha_cell pero con una caja en pixeles explicita
    (poses de la hoja B que se salen del ancho de su celda de grilla)."""
    x0, y0, x1, y1 = box
    c = img_rgba[y0:y1, x0:x1]
    alpha = _drop_edge_bleed(c[..., 3] > 30)
    rgba = c.copy()
    rgba[..., 3] = (alpha * 255).astype(np.uint8)
    return Image.fromarray(rgba, "RGBA")


def calibrate(piece):
    a = np.array(piece)[..., 3]
    bb = crop_bbox(a, pad=0)
    if bb is None:
        return None
    y0, y1, x0, x1 = bb
    return y1 - y0, (x0 + x1) / 2.0, y1


def main():
    atlas = Image.open(ATLAS).convert("RGBA")
    aw, ah = atlas.size
    rows_now = ah // CH

    # --- Fuente A: combos_transparente_v3 (damero horneado) ---
    imgA = np.array(Image.open(SRC_A).convert("RGB"))
    A_COLS, A_ROWS, A_CW, A_CH = 6, 4, 256, 256
    idleA = extract_checker_cell(imgA, 0, 0, A_CW, A_CH)
    hA, cxA, bottomA = calibrate(idleA)
    kA = STAND_H / hA
    print(f"[A] calib: altura {hA}px -> k={kA:.4f}")

    def getA(row, col):
        piece = extract_checker_cell(imgA, row, col, A_CW, A_CH)
        a = np.array(piece)[..., 3]
        bb = crop_bbox(a)
        if bb is None:
            return Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
        y0, y1, x0, x1 = bb
        sub = piece.crop((x0, y0, x1, y1))
        return place_on_cell(sub, bottomA - y0, cxA - x0, kA)

    # --- Fuente B: carga_disparo_poder (alpha real) ---
    imgB = np.array(Image.open(SRC_B).convert("RGBA"))
    B_COLS, B_ROWS, B_CW, B_CH = 8, 4, 192, 256
    idleB = extract_alpha_cell(imgB, 0, 0, B_CW, B_CH)
    hB, cxB, bottomB = calibrate(idleB)
    kB = STAND_H / hB
    print(f"[B] calib: altura {hB}px -> k={kB:.4f}")

    def getB(row, col):
        piece = extract_alpha_cell(imgB, row, col, B_CW, B_CH)
        a = np.array(piece)[..., 3]
        bb = crop_bbox(a)
        if bb is None:
            return Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
        y0, y1, x0, x1 = bb
        sub = piece.crop((x0, y0, x1, y1))
        return place_on_cell(sub, bottomB - y0, cxB - x0, kB)

    def getB_box(box):
        """Como getB pero con una caja en pixeles (pose que desborda su celda)."""
        piece = extract_alpha_box(imgB, box)
        a = np.array(piece)[..., 3]
        bb = crop_bbox(a)
        if bb is None:
            return Image.new("RGBA", (CW, CH), (0, 0, 0, 0))
        y0, y1, x0, x1 = bb
        sub = piece.crop((x0, y0, x1, y1))
        return place_on_cell(sub, bottomB - y0, cxB - x0, kB)

    # ------------------------------------------------------------------
    # Plan de celdas nuevas (indices consecutivos desde FIRST_FREE).
    # Cada entrada: (nombre_para_el_log, fuente, (fila,col))
    plan = [
        # DASH: arranque + estela fantasma propia + embestida (en vez de
        # reusar cuadros de 'run'). (fuente B, fila0: idle..dash)
        ("dash_ready",  "B", (0, 1)),
        ("dash_thrust", "B", (0, 5)),
        ("dash_ghost",  "B", (0, 6)),
        ("dash_lunge",  "B", (0, 7)),
        # KICK (patada giratoria/frontal mejorada)
        ("kick_f1", "A", (1, 0)),
        ("kick_f2", "A", (1, 1)),
        ("kick_f3", "A", (1, 2)),
        ("kick_f4", "A", (1, 3)),
        ("kick_f5", "A", (1, 4)),
        ("kick_f6", "A", (1, 5)),
        # PUNCH3 / remate con salto (patada voladora)
        ("remate1", "A", (2, 0)),
        ("remate2", "A", (2, 1)),
        ("remate3", "A", (2, 2)),
        ("remate4", "A", (2, 3)),
        ("remate5", "A", (2, 4)),
        # FINISHER: carga -> disparo -> impacto explosivo
        ("fin_charge1", "B", (0, 2)),
        ("fin_charge2", "B", (0, 3)),
        ("fin_charge3", "B", (0, 4)),
        ("fin_shot1",   "A", (2, 5)),
        ("fin_impact1", "Bbox", (0, 760, 300, 1024)),
        ("fin_impact2", "A", (3, 0)),
        # RAGE_ATTACK / super: combo de golpes con brillo + patada giratoria final
        ("rage1", "B", (1, 0)),
        ("rage2", "B", (1, 1)),
        ("rage3", "B", (1, 2)),
        ("rage4", "B", (1, 3)),
        ("rage5", "B", (1, 7)),
        ("rage6", "A", (1, 5)),
        # ENERGY (EnergyWave): carga progresiva real (idle->guardia->carga->aura)
        ("energy1", "B", (0, 0)),
        ("energy2", "B", (0, 1)),
        ("energy3", "B", (0, 2)),
        ("energy4", "B", (0, 3)),
        ("energy5", "B", (0, 4)),
        # KNOCKDOWN: impacto + caida + aterrizaje (antes 1 solo cuadro)
        ("kd1", "Bbox", (0, 760, 300, 1024)),
        ("kd2", "A", (2, 2)),
        ("kd3", "B", (2, 6)),
        # GETUP: recuperacion agachado (se agregan al final de la ya existente)
        ("getup_extra1", "A", (3, 1)),
        ("getup_extra2", "A", (3, 2)),
        # DASH_ATTACK: patada voladora de salto (remate en movimiento)
        ("dashatk1", "A", (2, 0)),
        ("dashatk2", "A", (2, 1)),
        ("dashatk3", "B", (2, 0)),
        ("dashatk4", "B", (2, 1)),
    ]

    cells = []
    for nm, src, loc in plan:
        if src == "A":
            piece = getA(*loc)
        elif src == "B":
            piece = getB(*loc)
        else:
            piece = getB_box(loc)
        cells.append((nm, piece))

    # celdas totales necesarias
    needed_last_index = FIRST_FREE + len(cells) - 1
    needed_rows = (needed_last_index // COLS) + 1
    if needed_rows > rows_now:
        new_atlas = Image.new("RGBA", (aw, needed_rows * CH), (0, 0, 0, 0))
        new_atlas.paste(atlas, (0, 0))
        atlas = new_atlas

    index_map = {}
    for i, (nm, piece) in enumerate(cells):
        idx = FIRST_FREE + i
        index_map[nm] = idx
        x = (idx % COLS) * CW
        y = (idx // COLS) * CH
        atlas.paste(piece, (x, y), piece)

    atlas.save(ATLAS)
    print(f"Atlas actualizado: {ATLAS.relative_to(ROOT)} ({atlas.size[0]}x{atlas.size[1]}, {atlas.size[1]//CH} filas)")
    print("Indices nuevos:")
    for nm, idx in index_map.items():
        print(f"  {nm}: {idx}")

    # hoja de verificacion: cada celda nueva con su nombre (no se comitea)
    OUT_SHEET.parent.mkdir(parents=True, exist_ok=True)
    n = len(cells)
    sheet_cols = 8
    sheet_rows = (n + sheet_cols - 1) // sheet_cols
    from PIL import ImageDraw
    sheet = Image.new("RGBA", (sheet_cols * CW, sheet_rows * (CH + 16)), (30, 30, 30, 255))
    d = ImageDraw.Draw(sheet)
    for i, (nm, piece) in enumerate(cells):
        cx, cy = (i % sheet_cols) * CW, (i // sheet_cols) * (CH + 16)
        sheet.paste(piece, (cx, cy), piece)
        d.text((cx + 2, cy + CH), f"{nm} #{FIRST_FREE+i}", fill=(255, 255, 0, 255))
    sheet.save(OUT_SHEET)
    print(f"Hoja de verificacion: {OUT_SHEET}")

    import json
    (ROOT / "tools/rayder/out/index_map.json").write_text(json.dumps(index_map, indent=2))


if __name__ == "__main__":
    main()
