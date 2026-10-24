#!/usr/bin/env python3
"""Actualiza los clips de rayder_kf en data/sprite_manifest.json con las
celdas nuevas agregadas al atlas por build_rayder_cruz_v3_extra.py
(indices 245-285). Reescribe el JSON entero con json.dump(indent=2), que
reproduce EXACTAMENTE el formato ya usado en el archivo (verificado antes
de aplicar: dump(load(original)) == original byte a byte)."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANI = ROOT / "data/sprite_manifest.json"

d = json.load(open(MANI))
clips = d["clips"]["rayder_kf"]

def setclip(name, frames, durations, loop=None):
    assert len(frames) == len(durations), name
    entry = clips[name]
    entry["frames"] = frames
    entry["durations"] = durations
    if loop is not None:
        entry["loop"] = loop

# DASH: antes reusaba cuadros de 'run' (14,15,16) sin pose de impulso propia.
# Ahora: lean de carrera -> arranque -> empuje -> estela fantasma propia (arte
# horneado) -> embestida a fondo.
setclip("dash", [14, 245, 246, 247, 248], [0.03, 0.035, 0.04, 0.05, 0.06])

# KICK: patada giratoria completa (antes una patada simple de 5 cuadros).
setclip("kick", [249, 250, 251, 252, 253, 254], [0.05, 0.055, 0.06, 0.07, 0.08, 0.09])

# PUNCH3 (tercer golpe del combo/remate): ahora un remate con salto real.
setclip("punch3", [255, 256, 257, 258, 259], [0.05, 0.06, 0.08, 0.1, 0.12])

# FINISHER: carga -> disparo -> impacto explosivo (antes solo golpes sueltos).
setclip("finisher", [260, 261, 262, 263, 264, 265], [0.08, 0.08, 0.09, 0.1, 0.12, 0.16])

# RAGE_ATTACK (super): combo de golpes cargados + patada giratoria final,
# resuelve en los cuadros de siempre para no romper el enganche con el FX.
setclip("rage_attack", [266, 267, 268, 269, 270, 271, 141, 142, 143],
        [0.05, 0.06, 0.07, 0.08, 0.09, 0.1, 0.1, 0.12, 0.16])

# ENERGY (EnergyWave): carga progresiva real (idle->guardia->carga->aura
# completa) antes de los cuadros de liberacion que ya existian.
setclip("energy", [272, 273, 274, 275, 276, 126, 127],
        [0.05, 0.05, 0.06, 0.07, 0.09, 0.1, 0.14])

# KNOCKDOWN: antes 1 solo cuadro estatico; ahora impacto + voltereta en el
# aire -> cuadro de caida original.
setclip("knockdown", [277, 278, 279, 104], [0.08, 0.09, 0.1, 0.12])

# GETUP: se insertan 2 cuadros agachado-recuperandose antes de ponerse de pie.
setclip("getup", [107, 108, 109, 280, 281, 110, 111],
        [0.08, 0.08, 0.08, 0.09, 0.09, 0.08, 0.08])

# DASH_ATTACK: patada voladora en carrera (antes una embestida generica).
setclip("dash_attack", [282, 283, 284, 285], [0.06, 0.08, 0.1, 0.14])

json.dump(d, open(MANI, "w"), indent=2)
with open(MANI, "a") as f:
    f.write("\n")
print("sprite_manifest.json actualizado (rayder_kf: dash, kick, punch3, finisher, "
       "rage_attack, energy, knockdown, getup, dash_attack).")
