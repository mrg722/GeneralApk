#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Arreglo de medios para los packs de MAX (rayder/) y RONIN (ronin/) (BRIEF F1.11).

PROBLEMA
    Los WAV de assets/audio/sfx/rayder/ y ronin/ (sintetizados por
    genera_sonidos_rayder.py / genera_sonidos_ronin.py) tienen el 96-98 % de la
    energia entre 120 y 500 Hz y varios > 60 % bajo 120 Hz: en el altavoz de un
    telefono casi no se oyen.

QUE HACE (por archivo, sin tocar los originales)
    1. lee el WAV (estereo) y lo pasa a mono (el pan lo pone el motor)
    2. quita el silencio inicial (silbido tenia 7 ms) y el DC (paso alto 20 Hz)
    3. anade un chasquido sintetico de 3 ms (ruido 2-5 kHz) al inicio
       (mas suave en los sonidos que no son golpes: carga, silbido, esquiva...)
    4. realce de +4 dB en 1.5-3 kHz (filtro de pico RBJ centrado en 2.1 kHz)
    5. saturacion armonica en paralelo (tanh, como un "bass enhancer"): el grave
       genera armonicos y se suma solo su parte 500 Hz-3 kHz, con la ganancia
       justa para llegar al 30 % de energia en medios. El grave original se
       conserva tal cual: el caracter (tono, duracion, envolvente) es el mismo.
    6. pico <= -3 dBFS, st50 igual al original (si el pico lo permite), fundido
       de 2 ms al final; mono 44.1 kHz 16 bit
    Escribe con los MISMOS nombres en rayder_v2/ y ronin_v2/.

    Determinista: el chasquido usa una semilla fija por archivo (crc32).

USO
    python3 tools/audio/mejora_medios.py [carpeta_sfx]
"""

import glob
import importlib.util
import os
import sys
import wave
import zlib

import numpy as np
from scipy import signal

AQUI = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location("genera_banco_combate",
                                               os.path.join(AQUI, "genera_banco_combate.py"))
gb = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gb)

SR = gb.SR
SEMILLA = 20261104
REALCE_DB = 4.0
PICO_MAX_DB = -3.0
CRESTA_MAX_DB = 6.5   # pico/st50 maximo tras el realce (el original ronda 4-5 dB)
PACKS = (("rayder", "rayder_v2"), ("ronin", "ronin_v2"))
# sonidos que no son impactos: chasquido mas suave
SUAVES = ("carga_poder", "silbido", "esquiva", "furia", "salto", "espada_desenvaine", "disparo_poder")


def lee(ruta):
    with wave.open(ruta, "rb") as w:
        nch, sr, sw, n = w.getnchannels(), w.getframerate(), w.getsampwidth(), w.getnframes()
        d = w.readframes(n)
    assert sw == 2 and sr == SR, ruta
    x = np.frombuffer(d, dtype="<i2").astype(np.float64) / 32768.0
    if nch > 1:
        x = x.reshape(-1, nch).mean(axis=1)
    return x


def pico_rbj(x, f0, db_, q):
    a = 10 ** (db_ / 40.0)
    w0 = 2 * np.pi * f0 / SR
    al = np.sin(w0) / (2 * q)
    b = np.array([1 + al * a, -2 * np.cos(w0), 1 - al * a])
    aa = np.array([1 + al / a, -2 * np.cos(w0), 1 - al / a])
    return signal.lfilter(b / aa[0], aa / aa[0], x)


def cresta_db(x):
    return 20 * np.log10((np.max(np.abs(x)) + 1e-12) / (gb.st50(x) + 1e-12))


def densifica(x, cresta_max=CRESTA_MAX_DB):
    """
    Saturacion tanh final (el mismo "bass enhancer"): baja la relacion pico/st50
    para que el realce no deje el sonido mas flojo que el original al limitar
    el pico a -3 dBFS. Sube la saturacion de a poco hasta la cresta objetivo.
    """
    pk = np.max(np.abs(x)) + 1e-12
    if cresta_db(x) <= cresta_max:
        return x
    y = x
    for d in np.arange(1.0, 8.01, 0.25):
        y = np.tanh(d * x / pk) / np.tanh(d) * pk
        if cresta_db(y) <= cresta_max:
            break
    return y


def mejora(x, nombre, rng):
    x = x - np.mean(x)
    x = signal.sosfilt(gb._sos("high", 20.0, 2), x)
    pk0 = np.max(np.abs(x)) + 1e-12
    st0 = gb.st50(x)
    primero = int(np.argmax(np.abs(x) >= 10 ** (-45 / 20.0) * pk0))
    x = x[primero:]
    n = len(x)
    k = gb.ns(0.003)
    nivel = 0.12 if nombre in SUAVES else 0.35
    ch = gb.pb(rng.uniform(-1, 1, n), 2000, 5000, 2)
    env = np.zeros(n)
    env[:k] = np.exp(-np.arange(k) / (k / 3.0))
    ch = ch * env
    x = x + nivel * pk0 * ch / (np.max(np.abs(ch)) + 1e-12)
    x = pico_rbj(x, 2100.0, REALCE_DB, 0.9)
    medios_antes = gb.fraccion_medios(x)
    x, g = gb.realza_medios(x)
    x = densifica(x)
    if gb.fraccion_medios(x) < 0.29:   # la saturacion casi siempre sube medios; por si acaso
        x, g2 = gb.realza_medios(x)
        g += g2
    gain = st0 / (gb.st50(x) + 1e-12)
    gain = min(gain, 10 ** (PICO_MAX_DB / 20.0) / (np.max(np.abs(x)) + 1e-12))
    x = x * gain
    kk = gb.ns(0.002)
    x[-kk:] *= np.linspace(1.0, 0.0, kk)
    return x, medios_antes, g


def bandas_bajas(x):
    """Espectro < 500 Hz (para comparar el caracter grave con el original)."""
    X = np.abs(np.fft.rfft(x, 1 << 15))
    f = np.fft.rfftfreq(1 << 15, 1.0 / SR)
    m = f < 500
    return X[m] / (np.linalg.norm(X[m]) + 1e-12)


def main(argv):
    raiz = os.path.dirname(os.path.dirname(AQUI))
    sfx = argv[0] if argv else os.path.join(raiz, "assets", "audio", "sfx")
    print("%-34s %8s %8s %7s %9s %11s" % ("archivo", "med_orig", "med_eq", "realce", "simil<500", "st50 o->v2"))
    for origen, destino in PACKS:
        for ruta in sorted(glob.glob(os.path.join(sfx, origen, "*.wav"))):
            nom = os.path.splitext(os.path.basename(ruta))[0]
            rel = "%s/%s.wav" % (destino, nom)
            rng = np.random.default_rng((SEMILLA ^ zlib.crc32(rel.encode("utf-8"))) & 0xFFFFFFFF)
            x0 = lee(ruta)
            y, med_eq, g = mejora(x0, nom, rng)
            gb.escribe(os.path.join(sfx, rel), y)
            sim = float(np.dot(bandas_bajas(x0), bandas_bajas(y)))
            print("%-34s %7.1f%% %7.1f%% %7.2f %9.3f %5.1f->%5.1f" % (
                rel, 100 * gb.fraccion_medios(x0), 100 * med_eq, g, sim,
                20 * np.log10(gb.st50(x0)), 20 * np.log10(gb.st50(y))))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
