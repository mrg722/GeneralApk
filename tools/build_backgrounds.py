#!/usr/bin/env python3
"""Genera los fondos HD en assets/backgrounds/hd/ (alto 720, ancho >= 1280).

Fuentes a resolucion completa del usuario (assets/backgrounds/source/):
  stage1_scenario01 <- stage1_scenario01_barrio_bajo.jpg (1536x864)
  stage2_scenario01 <- stage2_scenario01_tuberias.png    (1672x941)
  stage2_scenario02 <- stage2_scenario02_fundicion.png   (1672x941)
Se reducen a 1600x900 -> recorte a 1600x720 conservando el suelo (parte baja)
para dejar margen de paneo horizontal. Los demas escenarios solo existen como
tiras 816x276 (assets/backgrounds/stageN_scenarioNN.png); se escalan a alto
720 en tiempo de ejecucion (no se generan copias: no ganarian detalle y
pesarian ~25 MB). Las miniaturas de las hojas de referencia
(~390 px de ancho) tienen menos resolucion que esas tiras y no se usan.
Uso: python3 tools/build_backgrounds.py
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/backgrounds/source"
OUT = ROOT / "assets/backgrounds/hd"
FULL = {
    "stage1_scenario01": "stage1_scenario01_barrio_bajo.jpg",
    "stage2_scenario01": "stage2_scenario01_tuberias.png",
    "stage2_scenario02": "stage2_scenario02_fundicion.png",
}


def from_full(path):
    im = Image.open(path).convert("RGB").resize((1600, 900), Image.LANCZOS)
    return im.crop((0, 900 - 720 - 60, 1600, 900 - 60))   # quita cielo, conserva suelo


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for stage in range(1, 6):
        for sc in range(1, 5):
            name = f"stage{stage}_scenario{sc:02d}"
            if name not in FULL: continue
            im, how = from_full(SRC / FULL[name]), "fuente completa"
            im.save(OUT / f"{name}.png", optimize=True)
            print(f"{name}: {im.size[0]}x{im.size[1]} ({how})")


if __name__ == "__main__":
    main()
