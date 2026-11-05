#!/usr/bin/env python3
"""Deriva el arco del miembro (LimbArc, V5) de cada ataque de MAX desde el arte.

Uso (raiz del repo): python3 tools/max/derivar_arcos.py [salida.tsv]
Solo LEE assets/characters/max/max_atlas.png y data/sprite_manifest.json.

Para cada movimiento toma el cuadro de impacto de su clip, busca el pixel opaco
(alfa > 128, cuerpo o efecto) mas adelantado en cada banda de altura de 20 px y
se queda con el mas lejano dentro de la banda pedida. Con la escala del cuadro
("scales") y 1 px de celda = 0.996 px estandar (SpriteScale de MAX = 1.2 * 0.83),
calcula respecto al pivote (hombro (0,-84) o cadera (0,-58)):
  L1 = distancia al pixel - r/2, theta1 = angulo (0 = al frente, + = arriba).
Imprime la tabla que copian los LimbArc de src/game/moves/Moves_Max.cpp.
"""
import json
import math
import sys

from PIL import Image

CW, CH, COLS, PIVX, PIVY = 270, 175, 14, 135, 168
K = 1.2 * 0.83

# movimiento: (clip, paso de impacto en el clip, pivote, banda (alto min, alto max en px estandar), r)
MOVES = {
    "Punch1":        ("punch1", 2, "hombro", (60, 110), 14),
    "Punch2":        ("punch2", 2, "hombro", (60, 110), 15),
    "PunchHook":     ("punch_hook", 2, "hombro", (50, 110), 18),
    "Punch3":        ("media_luna", 1, "hombro", (40, 150), 20),
    "Chain5":        ("punch3", 2, "hombro", (40, 140), 22),
    "Kick":          ("kick_front", 1, "cadera", (40, 100), 16),
    "Kick2":         ("kick2", 1, "cadera", (60, 140), 18),
    "Kick3":         ("kick3", 1, "cadera", (40, 150), 22),
    "Chain6":        ("kick", 3, "cadera", (30, 130), 22),
    "KickLauncher":  ("kick_launcher", 1, "cadera", (20, 150), 20),
    "FwdPunch":      ("fwd_punch", 1, "hombro", (60, 110), 18),
    "FwdKick":       ("fwd_kick", 1, "cadera", (30, 120), 18),
    "Uppercut":      ("uppercut", 2, "hombro", (20, 140), 18),
    "Sweep":         ("sweep", 1, "cadera", (0, 40), 16),
    "DashKick":      ("dash_kick", 1, "cadera", (0, 50), 16),
    "Stomp":         ("stomp", 1, "cadera", (0, 30), 15),
    "DashAttack":    ("dash_attack", 1, "hombro", (40, 120), 22),
    "AirPunch":      ("air_punch", 1, "hombro", (40, 130), 14),
    "AirPunch2":     ("air_punch2", 1, "hombro", (40, 130), 15),
    "AirKick":       ("air_kick", 1, "cadera", (20, 110), 18),
    "DiveKick":      ("dive_kick", 1, "cadera", (0, 80), 16),
    "AirSpike":      ("air_spike", 1, "hombro", (20, 130), 20),
    "RisingUppercut": ("rising_uppercut", 2, "hombro", (60, 170), 20),
    "Hurricane":     ("hurricane", 1, "cadera", (20, 120), 20),
    "DrillKick":     ("drill_kick", 1, "cadera", (20, 120), 18),
    "Signature1":    ("signature1", 2, "hombro", (50, 120), 24),
    "Signature2":    ("signature2", 1, "cadera", (0, 170), 30),
    "Signature3":    ("signature3", 1, "cadera", (0, 170), 34),
    "RageAttack":    ("rage_attack", 4, "hombro", (20, 160), 34),
    "Finisher":      ("finisher", 3, "hombro", (20, 160), 40),
}
PIVOTS = {"hombro": 84, "cadera": 58}


def front_by_band(im, idx, scale):
    cx, cy = (idx % COLS) * CW, (idx // COLS) * CH
    px = im.crop((cx, cy, cx + CW, cy + CH)).load()
    bands = {}
    for y in range(CH):
        h = (PIVY - y) * scale * K
        b = int(h // 20) * 20
        for x in range(CW - 1, -1, -1):
            if px[x, y][3] > 128:
                d = (x - PIVX) * scale * K
                if d > bands.get(b, (-1e9, 0))[0]:
                    bands[b] = (d, h)
                break
    return bands


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/max_arcos.tsv"
    im = Image.open("assets/characters/max/max_atlas.png").convert("RGBA")
    clips = json.load(open("data/sprite_manifest.json"))["clips"]["max"]
    rows = ["movimiento\tclip\tcelda\tescala\tpivote\tpunta_x\tpunta_alto\tL1\ttheta1\tr"]
    for name, (clip, step, piv, (lo, hi), r) in MOVES.items():
        c = clips[clip]
        step = min(step, len(c["frames"]) - 1)
        cell = c["frames"][step]
        scale = c.get("scales", [1.0] * len(c["frames"]))[step]
        bands = front_by_band(im, cell, scale)
        cand = [v for b, v in bands.items() if lo <= b < hi]
        if not cand:
            rows.append("%s\t%s\t%d\t%.2f\t%s\t-\t-\t-\t-\t%d" % (name, clip, cell, scale, piv, r))
            continue
        dx, h = max(cand)
        ph = PIVOTS[piv]
        L = math.hypot(dx, h - ph)
        th = math.degrees(math.atan2(h - ph, dx))
        rows.append("%s\t%s\t%d\t%.2f\t%s\t%.0f\t%.0f\t%.0f\t%.0f\t%d" % (
            name, clip, cell, scale, piv, dx, h, max(10.0, L - r / 2), th, r))
    open(out, "w").write("\n".join(rows) + "\n")
    print("\n".join(rows))


if __name__ == "__main__":
    main()
