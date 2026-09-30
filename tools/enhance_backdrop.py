#!/usr/bin/env python3
"""Pase de atmosfera para los fondos planos de District Fury.

No dibuja arquitectura nueva (eso es trabajo de arte). Lo que hace es tomar
las formas planas y darles la iluminacion que tiene el fondo de referencia:
niebla en el horizonte, bloom en las luces, reflejos en el suelo mojado,
gradiente de cielo, vineteado y grano fino.

Trabaja a 1280x720 (el x5 exacto del original, filtro nearest) para no
romper la rejilla de pixeles.
"""
import sys
import numpy as np
from PIL import Image, ImageFilter

HORIZON = 470           # el suelo jugable empieza aqui (ver Types.h)
PALETTES = {            # (luz calida, sombra fria) por stage
    1: ((255, 176, 96), (28, 34, 58)),
    2: ((255, 196, 110), (34, 36, 44)),
    3: ((150, 255, 190), (18, 40, 46)),
    4: ((150, 200, 255), (22, 30, 48)),
    5: ((255, 90, 110), (30, 14, 24)),
}

def enhance(path, out, stage):
    src = Image.open(path).convert('RGB')
    if src.size != (1280, 720):
        src = src.resize((1280, 720), Image.NEAREST)
    a = np.array(src).astype(np.float32)
    h, w, _ = a.shape
    warm, cool = (np.array(c, np.float32) for c in PALETTES.get(stage, PALETTES[1]))

    # 1) gradiente de cielo: arriba mas frio y profundo, abajo mas cerrado
    ramp = np.linspace(0.0, 1.0, h, dtype=np.float32)[:, None, None]
    a = a * (0.80 + 0.20 * ramp) + cool * 0.16 * (1.0 - ramp)

    # 2) niebla en el horizonte: separa las capas lejanas de las cercanas
    dist = np.abs(np.arange(h, dtype=np.float32) - HORIZON)[:, None, None]
    haze = np.clip(1.0 - dist / 120.0, 0.0, 1.0) ** 3
    a = a * (1.0 - 0.10 * haze) + cool * 0.28 * haze

    # 3) bloom: las luces existentes se expanden y tinen el aire
    lum = a.mean(axis=2)
    mask = np.clip((lum - 118.0) / 90.0, 0.0, 1.0)
    glow = Image.fromarray((mask[:, :, None] * a).astype(np.uint8)).filter(ImageFilter.GaussianBlur(14))
    a = np.clip(a + np.array(glow).astype(np.float32) * 0.40, 0, 255)

    # 4) suelo mojado: reflejo vertical de lo que hay sobre el horizonte
    top = Image.fromarray(a[:HORIZON].astype(np.uint8)).transpose(Image.FLIP_TOP_BOTTOM)
    top = top.resize((w, h - HORIZON)).filter(ImageFilter.GaussianBlur(3))
    refl = np.array(top).astype(np.float32)
    fade = np.linspace(0.30, 0.03, h - HORIZON, dtype=np.float32)[:, None, None]
    a[HORIZON:] = np.clip(a[HORIZON:] * (1.0 - fade * 0.55) + refl * fade, 0, 255)

    # 5) franja jugable: se oscurece para que los personajes destaquen
    play = np.clip((np.arange(h, dtype=np.float32) - 500.0) / 140.0, 0.0, 1.0)[:, None, None]
    a *= (1.0 - 0.16 * play)

    # 6) luz calida rasante desde el horizonte
    a += warm * (haze * 0.10)

    # 7) vineteado
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    r = np.sqrt(((xx - w / 2) / (w / 2)) ** 2 + ((yy - h / 2) / (h / 2)) ** 2)
    a *= (1.0 - 0.30 * np.clip(r - 0.55, 0, 1)[:, :, None])

    # 8) curva de contraste: las formas planas recuperan cuerpo
    n = np.clip(a / 255.0, 0, 1)
    a = (n * n * (3 - 2 * n)) * 255.0 * 0.85 + a * 0.15

    # 9) grano fino (sin ensuciar, solo rompe las zonas planas)
    rng = np.random.default_rng(stage * 977)
    a += rng.normal(0.0, 3.0, a.shape).astype(np.float32)

    Image.fromarray(np.clip(a, 0, 255).astype(np.uint8)).save(out)

if __name__ == '__main__':
    for p in sys.argv[1:]:
        stage = int(p.split('stage')[1][0]) if 'stage' in p else 1
        enhance(p, p, stage)
        print('mejorado:', p)
