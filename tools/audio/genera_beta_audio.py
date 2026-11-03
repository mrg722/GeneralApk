#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Audio ORIGINAL del modo BETA (GowApk) - brief F1.13.

QUE ES
    Sintetiza por codigo (osciladores, ruido filtrado, Karplus-Strong y
    formantes) TODO el audio que carga GowApk/src/BetaMode.cpp desde
    GowApk/assets/audio/: los efectos soundNN.ogg y la musica (menu.ogg,
    gate1music.ogg, gate2music.ogg, gameCG.ogg, music_001/002/003.ogg).
    Reemplaza, con los MISMOS nombres de archivo, el audio extraido del juego
    original (con copyright) que habia antes. No usa muestras externas, no
    transcribe melodias existentes y no usa IA de audio: las melodias salen
    de un generador determinista (semilla por archivo) sobre el modo frigio.

TECNICA
    Efectos (estilo del banco de F1.11): chasquido 2-6 kHz + cuerpo 300-1200
    Hz + sub opcional, cortes = barrido de ruido + resonancias metalicas,
    voces = sierra con contorno de tono por 3 formantes, rugidos = sierra
    grave con aspereza (modulacion 30-70 Hz) + ruido, UI = tonos breves y
    pulsadas de lira. Pico -3 dBFS, sin silencio inicial.
    Musica ("epico griego"): modo frigio, drones de quinta, percusion tipo
    taiko (receta de tools/audio/genera_intro_ronin.py: tono cayendo +
    transitorio de ruido), coro "aah" (sierra por 3 filtros paso banda en
    800/1150/2900 Hz), lira (Karplus-Strong con retardo fraccional) y aulos
    (caña: pulso estrecho filtrado). Bucles sin corte: la cola que pasa del
    final se suma al principio.

PROCEDENCIA VERIFICABLE
    Cada archivo se genera de forma determinista (semilla = crc32 del
    nombre) y se codifica a OGG Vorbis -q:a 3 con ffmpeg en modo bitexact.
    El script escribe GowApk/assets/audio/PROCEDENCIA.txt con
    nombre -> md5 -> receta.

USO
    python3 tools/audio/genera_beta_audio.py              # genera todo
    python3 tools/audio/genera_beta_audio.py --verificar  # exit 1 si algun
        .ogg de GowApk/assets/audio no es el generado por este script
    python3 tools/audio/genera_beta_audio.py --informe    # analisis por archivo
    python3 tools/audio/genera_beta_audio.py --solo sound61,menu   # parcial
"""

import argparse
import hashlib
import os
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

import numpy as np
from scipy import signal

SR = 44100
VERSION = "genera_beta_audio v1"
ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / "GowApk" / "assets" / "audio"
PICO_SFX_DB = -3.0
PICO_MUSICA_DB = -1.5

# ---------------------------------------------------------------------------
# UTILIDADES
# ---------------------------------------------------------------------------


class Ctx:
    """Generador aleatorio propio de cada archivo (determinista)."""

    def __init__(self, nombre):
        self.rng = np.random.default_rng(zlib.crc32(nombre.encode()) ^ 0x5EED)

    def ruido(self, n):
        return self.rng.uniform(-1.0, 1.0, n)


def ns(seg):
    return max(1, int(round(seg * SR)))


def tt(n):
    return np.arange(n, dtype=np.float64) / SR


def _sos(tipo, f):
    nyq = SR / 2.0
    if tipo == "band":
        lo = max(20.0, min(f[0], nyq * 0.95))
        hi = max(lo * 1.05, min(f[1], nyq * 0.98))
        return signal.butter(2, [lo / nyq, hi / nyq], btype="band", output="sos")
    return signal.butter(2, max(20.0, min(f, nyq * 0.98)) / nyq, btype=tipo, output="sos")


def pb(x, lo, hi):
    return signal.sosfilt(_sos("band", (lo, hi)), x)


def pbajo(x, f):
    return signal.sosfilt(_sos("low", f), x)


def palto(x, f):
    return signal.sosfilt(_sos("high", f), x)


def env(n, ataque=0.001, caida=0.1, curva=1.0):
    t = tt(n)
    sub = np.clip(np.arange(n) / max(1, ns(ataque)), 0.0, 1.0)
    return sub * np.exp(-curva * t / max(caida, 1e-4))


def env_adsr(n, a, r, s=1.0):
    e = np.full(n, s)
    na, nr = min(n, ns(a)), min(n, ns(r))
    e[:na] *= np.linspace(0.0, 1.0, na) ** 1.5
    if nr > 0:
        e[n - nr:] *= np.linspace(1.0, 0.0, nr) ** 1.5
    return e


def barrido(f0, f1, n, curva="exp"):
    if curva == "exp":
        return f0 * np.exp(np.log(f1 / f0) * np.linspace(0.0, 1.0, n))
    return np.linspace(f0, f1, n)


def fase(frec):
    return 2.0 * np.pi * np.cumsum(frec) / SR


def sierra_f(frec):
    ph = np.cumsum(frec) / SR
    return 2.0 * (ph - np.floor(ph)) - 1.0


def contorno(puntos, n):
    """puntos = [(t_rel 0..1, valor)] -> curva interpolada de n muestras."""
    xs = [p[0] for p in puntos]
    ys = [p[1] for p in puntos]
    return np.interp(np.linspace(0.0, 1.0, n), xs, ys)


def mezcla(dest, x, ini):
    i = int(ini)
    if i >= len(dest):
        return
    m = min(len(x), len(dest) - i)
    dest[i:i + m] += x[:m]


def reverb_ir(c, dur=1.2, brillo=4500.0, predelay=0.012):
    n = ns(dur)
    ir = c.ruido(n) * np.exp(-4.5 * tt(n) / dur)
    ir = palto(pbajo(ir, brillo), 150.0)
    ir = np.concatenate([np.zeros(ns(predelay)), ir])
    return ir / (np.sqrt(np.sum(ir ** 2)) + 1e-9)


def con_cola(c, x, mezcla_h=0.25, dur=0.8, brillo=5000.0):
    if mezcla_h <= 0:
        return x
    y = np.concatenate([x, np.zeros(ns(dur))])
    h = signal.fftconvolve(y, reverb_ir(c, dur, brillo))[:len(y)]
    h *= (np.max(np.abs(x)) + 1e-9) / (np.max(np.abs(h)) + 1e-9)
    return y + h * mezcla_h


def recorta_final(x, umbral_db=-60.0):
    lim = np.max(np.abs(x)) * 10 ** (umbral_db / 20.0)
    idx = np.nonzero(np.abs(x) > lim)[0]
    if len(idx) == 0:
        return x
    return x[: min(len(x), idx[-1] + ns(0.01))]


def quita_inicio(x, umbral_db=-40.0):
    """Sin silencio inicial: arranca en la primera muestra audible."""
    lim = np.max(np.abs(x)) * 10 ** (umbral_db / 20.0)
    idx = np.nonzero(np.abs(x) > lim)[0]
    return x[idx[0]:] if len(idx) else x


def bordes(x, ms_ini=0.5, ms_fin=6.0):
    y = x.copy()
    a, b = min(len(y) // 2, ns(ms_ini / 1000.0)), min(len(y) // 2, ns(ms_fin / 1000.0))
    if a > 1:
        y[:a] *= np.linspace(0.0, 1.0, a)
    if b > 1:
        y[-b:] *= np.linspace(1.0, 0.0, b)
    return y


def satura(x, empuje=1.6):
    p = np.max(np.abs(x)) + 1e-9
    return np.tanh(empuje * x / p) / np.tanh(empuje)


def normaliza(x, pico_db):
    p = np.max(np.abs(x))
    return x if p < 1e-9 else x * (10 ** (pico_db / 20.0) / p)


# Formantes de vocales (F1, F2, F3) y ganancias relativas.
VOCALES = {
    "a": ((800, 1150, 2900), (1.0, 0.55, 0.22)),
    "o": ((450, 800, 2830), (1.0, 0.45, 0.12)),
    "u": ((325, 700, 2530), (1.0, 0.3, 0.08)),
    "e": ((500, 1750, 2600), (1.0, 0.4, 0.2)),
}


def formantes(x, vocal, escala=1.0, ancho=0.18):
    fs, gs = VOCALES[vocal]
    y = np.zeros_like(x)
    for f, g in zip(fs, gs):
        f *= escala
        y += g * pb(x, f * (1 - ancho), f * (1 + ancho))
    return y


# ---------------------------------------------------------------------------
# RECETAS DE EFECTOS (devuelven mono)
# ---------------------------------------------------------------------------


def r_golpe(c, peso=0.5, brillo=1.0, sub=0.0, dur=0.28, cola=0.12):
    """Golpe: chasquido 2-6 kHz + cuerpo 300-1200 Hz con tono cayendo + sub."""
    n = ns(dur)
    t = tt(n)
    click = pb(c.ruido(n), 2000, 6000) * env(n, 0.0003, 0.012 + 0.01 * brillo, 1.0) * 1.3 * brillo
    f0 = 1200 - 700 * peso
    cuerpo_f = barrido(f0, f0 * 0.35, n)
    cuerpo = (np.sin(fase(cuerpo_f)) + 0.3 * np.sin(2 * fase(cuerpo_f))) * env(n, 0.0005, 0.05 + 0.06 * peso, 1.3)
    golpe_ruido = pb(c.ruido(n), 300, 1200) * env(n, 0.0005, 0.03 + 0.04 * peso, 1.5) * 0.7
    s = np.sin(fase(barrido(90 - 30 * peso, 42, n))) * env(n, 0.002, 0.1 + 0.1 * peso, 1.2) * sub
    x = click + cuerpo * (0.6 + 0.4 * peso) + golpe_ruido + s
    return con_cola(c, satura(x, 1.4 + peso), cola, 0.35)


def r_whoosh(c, f0=500, f1=2500, dur=0.3, nivel=1.0, bandas=8):
    """Barrido de aire: bandas de ruido cuya ganancia se mueve en el tiempo."""
    n = ns(dur)
    base = c.ruido(n)
    fr = np.geomspace(min(f0, f1), max(f0, f1), bandas)
    pos = np.linspace(0.0, 1.0, n)
    x = np.zeros(n)
    for i, f in enumerate(fr):
        centro = i / max(1, bandas - 1)
        if f1 < f0:
            centro = 1.0 - centro
        g = np.exp(-((pos - (0.15 + 0.6 * centro)) ** 2) / 0.03)
        x += pb(base, f * 0.8, f * 1.25) * g
    e = np.sin(np.pi * np.clip(pos * 1.1, 0, 1)) ** 0.7
    e[: ns(0.004)] *= np.linspace(0.3, 1.0, ns(0.004))
    return x * e * nivel


def r_metal(c, f=2400.0, dur=0.5, n_parciales=5, nivel=0.6):
    """Resonancias inarmonicas (filo/metal que vibra)."""
    n = ns(dur)
    t = tt(n)
    x = np.zeros(n)
    rat = [1.0, 1.51, 2.27, 2.76, 3.43, 4.1]
    for k in range(n_parciales):
        x += np.sin(2 * np.pi * f * rat[k] * t + c.rng.uniform(0, 6.28)) * env(n, 0.0005, dur * (0.5 - 0.06 * k), 2.0) / (1 + k)
    return x * nivel


def r_corte(c, peso=0.4, metal=0.5, dur=0.32, f_metal=2600.0, impacto=0.6):
    """Tajo de arma: whoosh rapido + filo metalico + impacto corto."""
    w = r_whoosh(c, 700, 4200, dur * 0.55, 0.8)
    n = ns(dur)
    x = np.zeros(n)
    mezcla(x, w, 0)
    i0 = ns(dur * 0.06)
    mezcla(x, r_golpe(c, peso, 1.2, 0.3 * peso, dur * 0.7, 0.0) * impacto, i0)
    mezcla(x, r_metal(c, f_metal, dur * 0.8, 5, metal), i0)
    return con_cola(c, x, 0.15, 0.4)


def r_cadena(c, dur=0.36):
    """Latigazo de cadenas: whoosh + varios eslabones metalicos breves."""
    n = ns(dur)
    x = np.zeros(n)
    mezcla(x, r_whoosh(c, 400, 3000, dur * 0.7, 0.7), 0)
    for k in range(5):
        i = ns(dur * (0.25 + 0.09 * k + c.rng.uniform(0, 0.03)))
        mezcla(x, r_metal(c, 3000 + 400 * c.rng.uniform(-1, 1), 0.09, 3, 0.35 - 0.04 * k), i)
    return x


def r_energia(c, f0=150.0, f1=900.0, dur=0.8, chispas=0.4):
    """Carga de energia: tono FM ascendente + tremolo que acelera + chispas."""
    n = ns(dur)
    fr = barrido(f0, f1, n)
    trem = 0.6 + 0.4 * np.sin(fase(barrido(6, 28, n)))
    tono = np.sin(fase(fr) + 1.8 * np.sin(fase(fr * 1.5))) * trem
    tono += 0.35 * np.sin(fase(fr * 0.5))
    ch = pb(c.ruido(n), 3000, 9000) * (c.rng.uniform(0, 1, n) > 0.97) * chispas * 3
    e = np.maximum(env_adsr(n, dur * 0.7, dur * 0.12), 0.45 * env(n, 0.0005, 0.06, 1.0))
    x = (tono * 0.8 + pbajo(ch, 9000)) * e
    mezcla(x, pb(c.ruido(ns(0.05)), 1500, 6000) * env(ns(0.05), 0.0003, 0.015, 1.0) * 0.5, 0)
    return x


def r_estallido(c, dur=0.9, grave=60.0, brillo=1.0):
    """Estallido de energia/magia: golpe ancho + boom + anillo + cola."""
    n = ns(dur)
    t = tt(n)
    x = r_golpe(c, 0.9, brillo, 0.0, dur, 0.0)
    x = np.pad(x, (0, max(0, n - len(x))))[:n]
    boom = np.sin(fase(barrido(grave * 2.4, grave, n))) * env(n, 0.001, 0.22, 1.2)
    anillo = np.sin(2 * np.pi * 820 * t) * np.sin(2 * np.pi * 230 * t) * env(n, 0.002, 0.12, 1.6) * 0.5
    sh = pb(c.ruido(n), 1500, 7000) * env(n, 0.002, 0.25, 2.0) * 0.4 * brillo
    return con_cola(c, x + boom + anillo + sh, 0.35, 0.9)


def r_rayo(c, dur=0.45):
    """Descarga electrica: zumbido de pulso irregular + crepitar + golpe."""
    n = ns(dur)
    fr = 110 * (1 + 0.25 * pbajo(c.ruido(n), 30) * 8)
    zumb = np.sign(np.sin(fase(fr))) * 0.5
    zumb = pb(zumb, 200, 5000)
    crep = c.ruido(n) * (c.rng.uniform(0, 1, n) > 0.92)
    crep = palto(crep, 2500) * 1.5
    x = (zumb + crep) * env(n, 0.0005, dur * 0.35, 1.0)
    g = r_golpe(c, 0.5, 1.3, 0.2, 0.25, 0.0)
    mezcla(x, g, 0)
    return con_cola(c, x, 0.2, 0.5, 7000)


def r_garra(c, cortes=3, dur=0.32):
    """Zarpazo: varios cortes cortos de ruido agudo + cuerpo sordo."""
    n = ns(dur)
    x = np.zeros(n)
    for k in range(cortes):
        w = r_whoosh(c, 1800, 6500, 0.07, 0.9 - 0.15 * k, 5)
        mezcla(x, w, ns(0.025 * k))
    mezcla(x, r_golpe(c, 0.35, 1.0, 0.0, 0.2, 0.0) * 0.8, ns(0.03))
    return x


def r_voz(c, f0=160.0, curva=((0, 1.0), (0.15, 1.15), (1, 0.7)), dur=0.4, vocal="a",
          aspereza=0.2, aliento=0.25, escala_formante=1.0, vibrato=0.0):
    """Quejido/grito sintetico (no humano real): sierra + formantes."""
    n = ns(dur)
    t = tt(n)
    fr = f0 * contorno(curva, n) * (1 + vibrato * np.sin(2 * np.pi * 5.5 * t))
    fr *= 1 + 0.01 * pbajo(c.ruido(n), 12) * 10
    src = sierra_f(fr)
    if aspereza > 0:
        src *= 1 - aspereza + aspereza * np.abs(np.sin(fase(fr * 0.5)))
        src = np.tanh(src * (1 + 3 * aspereza))
    src += c.ruido(n) * aliento
    y = formantes(src, vocal, escala_formante)
    e = env_adsr(n, 0.008, dur * 0.4)
    return y * e


def r_rugido(c, f0=70.0, dur=1.3, vocal="o", aspereza=0.7, caida=0.75):
    """Rugido de bestia/jefe: voz grave muy aspera + retumbo de ruido."""
    n = ns(dur)
    x = r_voz(c, f0, ((0, 0.8), (0.2, 1.15), (0.6, 1.0), (1, caida)), dur, vocal, aspereza, 0.35, 0.7)
    rumble = pbajo(c.ruido(n), 260) * env_adsr(n, 0.03, dur * 0.5) * 1.5
    y = x / (np.max(np.abs(x)) + 1e-9) + rumble / (np.max(np.abs(rumble)) + 1e-9) * 0.5
    y = satura(y, 2.0)
    return con_cola(c, y, 0.3, 0.9, 3000)


def r_siseo(c, dur=0.6, grito=0.6, f0=900.0):
    """Siseo de serpiente + chillido agudo (medusa)."""
    n = ns(dur)
    sis = pb(c.ruido(n), 4000, 9000) * env_adsr(n, 0.01, dur * 0.5)
    gr = r_voz(c, f0, ((0, 0.9), (0.3, 1.2), (1, 0.75)), dur, "e", 0.15, 0.1, 1.5, 0.03)
    gr = gr / (np.max(np.abs(gr)) + 1e-9)
    return con_cola(c, sis * 0.7 + gr * grito, 0.25, 0.6)


def r_relincho(c, dur=0.7, f0=520.0, baja=0.6):
    """Relincho sintetico: tono con vibrato rapido y ancho que cae."""
    n = ns(dur)
    t = tt(n)
    base = f0 * contorno(((0, 1.0), (0.2, 1.35), (1, baja)), n)
    fr = base * (1 + 0.12 * np.sin(2 * np.pi * 14 * t))
    src = sierra_f(fr) + 0.2 * c.ruido(n)
    y = formantes(src, "e", 1.2) * env_adsr(n, 0.01, dur * 0.5)
    return con_cola(c, y, 0.2, 0.6)


def r_huesos(c, golpes=5, dur=0.4, f=1800.0):
    """Traqueteo de huesos/madera: clics breves resonantes en serie."""
    n = ns(dur)
    x = np.zeros(n)
    tpos = 0.0
    for k in range(golpes):
        m = ns(0.05)
        fk = f * c.rng.uniform(0.7, 1.4)
        clic = pb(c.ruido(m), fk * 0.7, fk * 1.4) * env(m, 0.0003, 0.012, 1.0)
        clic += np.sin(2 * np.pi * fk * 0.5 * tt(m)) * env(m, 0.0003, 0.015, 1.0) * 0.5
        mezcla(x, clic * (1.0 - 0.12 * k), ns(tpos))
        tpos += c.rng.uniform(0.025, 0.07)
    return x


def lira_nota(c, f, dur=1.2, brillo=0.5, nivel=1.0):
    """Karplus-Strong con retardo fraccional (paso todo) via lfilter."""
    n = ns(dur)
    d = SR / f - 0.5
    N = int(np.floor(d))
    frac = d - N
    if frac < 0.1:
        N -= 1
        frac += 1.0
    ca = (1 - frac) / (1 + frac)
    g = 0.996 - 0.004 * (1 - brillo)
    # H = (1 + ca z^-1) / ((1 + ca z^-1) - g/2 (1 + z^-1)(ca + z^-1) z^-N)
    a = np.zeros(N + 3)
    a[0] = 1.0
    a[1] += ca
    a[N] -= g * 0.5 * ca
    a[N + 1] -= g * 0.5 * (1 + ca)
    a[N + 2] -= g * 0.5
    b = np.array([1.0, ca])
    exc = np.zeros(n)
    m = N + 1
    exc[:m] = pbajo(c.ruido(m), 1500 + 6000 * brillo)
    y = signal.lfilter(b, a, exc)
    y = palto(y, 80.0)
    return y * env_adsr(n, 0.0005, min(0.15, dur * 0.3)) * nivel


def r_ui(c, notas, paso=0.07, dur_nota=0.25, timbre="lira", nivel=1.0):
    """Sonidos de interfaz: notas breves de lira o tono puro."""
    total = paso * (len(notas) - 1) + dur_nota
    x = np.zeros(ns(total))
    for k, f in enumerate(notas):
        if timbre == "lira":
            nota = lira_nota(c, f, dur_nota, 0.8)
        else:
            m = ns(dur_nota)
            nota = (np.sin(2 * np.pi * f * tt(m)) + 0.25 * np.sin(4 * np.pi * f * tt(m))) * env(m, 0.0008, dur_nota * 0.35, 1.0)
        mezcla(x, nota * nivel, ns(paso * k))
    return x


def r_gong(c, f0=110.0, dur=2.2, mazo=0.8):
    """Gong de bronce: parciales inarmonicos con batido + golpe de mazo."""
    n = ns(dur)
    t = tt(n)
    rat = [1.0, 1.48, 1.98, 2.52, 3.14, 3.92, 4.73]
    x = np.zeros(n)
    for k, r in enumerate(rat):
        f = f0 * r * (1 + 0.004 * np.sin(2 * np.pi * (0.7 + 0.3 * k) * t))
        x += np.sin(fase(f) + c.rng.uniform(0, 6.28)) * env(n, 0.003 + 0.004 * k, dur * (0.6 / (1 + 0.35 * k)), 1.0) / (1 + 0.5 * k)
    x += pb(c.ruido(n), 300, 3000) * env(n, 0.0005, 0.03, 1.0) * mazo
    return con_cola(c, x, 0.3, 1.2, 4000)


def r_cuerno(c, f0=147.0, dur=1.3, quinta=True, nivel=1.0):
    """Cuerno de guerra: sierra filtrada con brillo que abre y cierra."""
    n = ns(dur)
    t = tt(n)
    x = np.zeros(n)
    voces = [(1.0, 1.0), (1.5, 0.6)] if quinta else [(1.0, 1.0)]
    voces.append((0.5, 0.5))
    for r, g in voces:
        for det in (-0.004, 0.004):
            fr = f0 * r * (1 + det) * contorno(((0, 0.94), (0.08, 1.0), (1, 0.99)), n)
            x += sierra_f(fr) * g
    corte = contorno(((0, 300), (0.25, 2200), (0.8, 1200), (1, 500)), n)
    y = np.zeros(n)
    blk = 2048
    for i in range(0, n, blk):   # filtro que varia por bloques (bordes suaves por el solape)
        seg = x[max(0, i - 512):i + blk]
        f = pbajo(seg, float(corte[min(n - 1, i)]))
        y[i:i + blk] = f[(i - max(0, i - 512)):][: len(y[i:i + blk])]
    return con_cola(c, y * env_adsr(n, 0.06, dur * 0.35) * nivel, 0.35, 1.0, 3500)


def r_pocion(c, dur=0.8):
    """Pocion: burbujas (senos con tono que sube) + arpegio brillante."""
    n = ns(dur)
    x = np.zeros(n)
    for k in range(9):
        m = ns(0.06)
        f = c.rng.uniform(500, 1400)
        b = np.sin(fase(barrido(f, f * 1.8, m))) * env(m, 0.001, 0.02, 1.0)
        mezcla(x, b * 0.6, ns(c.rng.uniform(0, dur * 0.5)))
    mezcla(x, r_ui(c, [880.0, 1108.7, 1318.5, 1760.0], 0.06, 0.4, "lira", 0.8), ns(0.05))
    return con_cola(c, x, 0.3, 0.7, 7000)


def r_taiko(c, f_ini=150.0, f_fin=60.0, caida=0.16, nivel=1.0, dur=0.6):
    """Taiko (receta de genera_intro_ronin.capa_percusion)."""
    n = ns(dur)
    fr = barrido(f_ini, f_fin, n)
    cuerpo = (np.sin(fase(fr)) + 0.25 * np.sin(2 * fase(fr))) * env(n, 0.001, caida, 1.4)
    click = pb(c.ruido(n), 1200, 7000) * env(n, 0.0005, 0.02, 3.0) * 0.5
    piel = pb(c.ruido(n), 150, 700) * env(n, 0.001, caida * 0.4, 2.0) * 0.4
    return (cuerpo * nivel + click + piel) * 0.9


def r_fanfarria(c, raiz=220.0, dur=1.8):
    """Remate/victoria: acorde mayor de cuerno + arpegio de lira + gong."""
    n = ns(dur)
    x = np.zeros(n)
    mezcla(x, r_cuerno(c, raiz, dur * 0.8, True, 0.6), 0)
    mezcla(x, r_cuerno(c, raiz * 1.26, dur * 0.8, False, 0.4), 0)
    mezcla(x, r_ui(c, [raiz * 2, raiz * 2.52, raiz * 3, raiz * 4], 0.07, 0.7, "lira", 0.9), ns(0.02))
    mezcla(x, r_taiko(c, 140, 55, 0.25, 1.2, 0.8), 0)
    mezcla(x, r_gong(c, raiz * 0.5, dur, 0.4) * 0.5, ns(0.05))
    return x


def r_paso(c, dur=0.18):
    """Pisada / aterrizaje: golpe sordo grave sin brillo."""
    x = r_golpe(c, 0.85, 0.35, 0.6, dur, 0.0)
    return pbajo(x, 2500)


def r_sting(c, dur=1.2):
    """Aviso dramatico (QTE): whoosh inverso + taiko + coro breve."""
    n = ns(dur)
    x = np.zeros(n)
    mezcla(x, r_taiko(c, 170, 55, 0.3, 1.3, 0.9), 0)
    mezcla(x, r_whoosh(c, 3000, 300, 0.35, 0.6), 0)
    v = r_voz(c, 220, ((0, 1), (1, 1)), 0.8, "a", 0.0, 0.05, 1.0, 0.02)
    v += r_voz(c, 330, ((0, 1), (1, 1)), 0.8, "a", 0.0, 0.05, 1.0, 0.02)
    mezcla(x, v / (np.max(np.abs(v)) + 1e-9) * 0.6, ns(0.02))
    return con_cola(c, x, 0.3, 0.9)


# ---------------------------------------------------------------------------
# TABLA DE EFECTOS: nombre -> (uso detectado, receta)
# Uso deducido de BetaMode.cpp, GowApk/data/** y los clipsound/sound de
# data/kratos/*.txt (ver informe F1.13). Los no referenciados hoy reciben
# variantes genericas para que ningun archivo quede con audio original.
# ---------------------------------------------------------------------------

def _sfx_tabla():
    T = {}

    def d(nombre, uso, receta, fn):
        T[nombre] = (uso, receta, fn)

    # --- Interfaz y flujo (BetaMode.cpp) ---
    d("sound2", "UI volver/cancelar (ESC)", "ui lira 2 notas descendentes", lambda c: r_ui(c, [659.3, 440.0], 0.07, 0.25))
    d("sound13", "UI mover cursor", "ui tono 1320 Hz 60 ms", lambda c: r_ui(c, [1318.5], 0.05, 0.06, "seno"))
    d("sound14", "UI confirmar / heroe entra", "ui lira 3 notas ascendentes", lambda c: r_ui(c, [440.0, 659.3, 880.0], 0.06, 0.35))
    d("sound24", "gong de inicio de pelea / aviso QTE", "gong 110 Hz 2.2 s", lambda c: r_gong(c, 110.0, 2.2))
    d("sound25", "inicio de QTE", "sting: whoosh + taiko + coro", lambda c: r_sting(c))
    d("sound44", "inicio de oleada / fase 2 del jefe", "cuerno de quinta 147 Hz", lambda c: r_cuerno(c, 146.8, 1.3))
    d("sound45", "aparece el jefe", "cuerno grave + rugido + taiko", lambda c: _jefe_entra(c))
    d("sound48", "victoria / remate", "fanfarria mayor + gong", lambda c: r_fanfarria(c, 220.0, 1.8))
    d("sound6", "pocion (vida/magia)", "burbujas + arpegio de lira", lambda c: r_pocion(c))

    # --- Kratos y RONIN-KRATOS (data/kratos/*.txt) ---
    d("sound61", "espadas tajo 1 / QTE acierto", "corte ligero metal 2.6 kHz", lambda c: r_corte(c, 0.3, 0.5, 0.3, 2600))
    d("sound62", "espadas tajo 2", "corte medio metal 2.3 kHz", lambda c: r_corte(c, 0.45, 0.55, 0.32, 2300))
    d("sound63", "espadas tajo 3 / ataque en carrera", "corte fuerte metal 2.0 kHz", lambda c: r_corte(c, 0.6, 0.6, 0.36, 2000))
    d("sound64", "golpe pesado / patada / remate", "golpe pesado con sub", lambda c: r_golpe(c, 0.85, 1.0, 0.7, 0.4, 0.15))
    d("sound65", "carga de energia (habilidades)", "energia FM 150->900 Hz", lambda c: r_energia(c, 150, 900, 0.7))
    d("sound75", "cadenas de las espadas", "whoosh + eslabones metalicos", lambda c: r_cadena(c))
    d("sound73", "cesto puñetazo 1", "golpe medio brillante", lambda c: r_golpe(c, 0.55, 1.2, 0.3, 0.3, 0.12))
    d("sound74", "cesto puñetazo 3 / carrera", "golpe fuerte + metal grave", lambda c: _cesto_fuerte(c))
    d("sound70", "dash / salto", "whoosh 400->2200 Hz", lambda c: r_whoosh(c, 400, 2200, 0.26))
    d("sound34", "Kratos muere", "voz grave 'o' cayendo 1.1 s", lambda c: r_voz(c, 120, ((0, 1.1), (0.2, 1.2), (1, 0.55)), 1.1, "o", 0.35, 0.3, 0.9))
    d("sound36", "Kratos herido", "voz 'a' corta 0.32 s", lambda c: r_voz(c, 150, ((0, 1.05), (0.25, 1.2), (1, 0.8)), 0.32, "a", 0.3, 0.25, 0.95))
    d("sound37", "furia / super (inicio)", "rugido 95 Hz + energia", lambda c: _furia(c))
    d("sound38", "impacto de magia (habilidad)", "estallido de energia", lambda c: r_estallido(c, 0.8, 60.0, 1.0))
    d("sound3", "RONIN-KRATOS rayo: golpe electrico", "descarga electrica + golpe", lambda c: r_rayo(c, 0.42))
    d("sound4", "RONIN-KRATOS garras: zarpazo", "3 cortes agudos + cuerpo", lambda c: r_garra(c, 3, 0.3))

    # --- Enemigos (GowApk/data/characters) ---
    d("sound31", "medusa herida", "siseo + chillido corto", lambda c: r_siseo(c, 0.45, 0.6, 950))
    d("sound32", "medusa muere", "siseo + chillido largo", lambda c: r_siseo(c, 1.1, 0.8, 800))
    d("sound39", "centauro herido", "relincho corto", lambda c: r_relincho(c, 0.45, 560, 0.75))
    d("sound40", "centauro muere", "relincho largo que cae", lambda c: r_relincho(c, 1.0, 520, 0.45))
    d("sound66", "no-muerto muere (esqueleto/excavador/momia/ave)", "huesos + gemido grave", lambda c: _no_muerto(c, True))
    d("sound67", "no-muerto herido", "huesos breves", lambda c: _no_muerto(c, False))
    d("sound68", "bruto/bestia herido", "gruñido aspero 85 Hz", lambda c: r_voz(c, 85, ((0, 1), (0.3, 1.15), (1, 0.8)), 0.4, "o", 0.6, 0.3, 0.75))
    d("sound69", "bruto/bestia muere", "rugido 70 Hz 1.1 s", lambda c: r_rugido(c, 70, 1.1, "o", 0.6))

    # --- Jefes (GowApk/data/bosses: hurt/die y ataques) ---
    d("sound51", "tentaculos muere / ataque 2", "rugido humedo 55 Hz 1.6 s", lambda c: r_rugido(c, 55, 1.6, "u", 0.75, 0.6))
    d("sound52", "tentaculos herido / ataque 1", "rugido 65 Hz 0.8 s", lambda c: r_rugido(c, 65, 0.8, "u", 0.7, 0.8))
    d("sound55", "titan muere / ataque 2", "rugido 45 Hz + estallido", lambda c: _titan(c, 1.8))
    d("sound56", "titan herido / ataque 1", "rugido 60 Hz 0.9 s", lambda c: r_rugido(c, 60, 0.9, "o", 0.8, 0.85))
    d("sound59", "poseidon herido / ataque 1", "rugido 80 Hz + ola", lambda c: _poseidon(c, 0.9))
    d("sound60", "poseidon muere / ataque 2", "rugido 70 Hz + ola larga", lambda c: _poseidon(c, 1.7))

    # --- Sin referencia hoy (variantes genericas originales) ---
    genericos = [
        (1, "golpe ligero", lambda c: r_golpe(c, 0.25, 1.1, 0.0, 0.22)),
        (5, "golpe medio", lambda c: r_golpe(c, 0.5, 1.0, 0.2, 0.3)),
        (7, "whoosh corto", lambda c: r_whoosh(c, 600, 3000, 0.2)),
        (8, "golpe ligero 2", lambda c: r_golpe(c, 0.3, 1.2, 0.0, 0.22)),
        (9, "corte ligero", lambda c: r_corte(c, 0.3, 0.4, 0.28, 2900)),
        (10, "golpe pesado", lambda c: r_golpe(c, 0.8, 0.9, 0.6, 0.38)),
        (11, "estallido", lambda c: r_estallido(c, 0.9, 55.0, 0.8)),
        (12, "whoosh largo", lambda c: r_whoosh(c, 300, 2500, 0.4)),
        (15, "metal choca", lambda c: r_metal(c, 1900, 0.6, 6, 0.9) + r_golpe(c, 0.4, 1.0, 0.0, 0.6, 0.0)[: ns(0.6)]),
        (16, "energia corta", lambda c: r_energia(c, 220, 1100, 0.5)),
        (17, "pisada", lambda c: r_paso(c, 0.18)),
        (18, "pisada 2", lambda c: r_paso(c, 0.2)),
        (19, "corte medio", lambda c: r_corte(c, 0.5, 0.5, 0.32, 2200)),
        (20, "rugido bestia", lambda c: r_rugido(c, 75, 1.0, "a", 0.6)),
        (21, "voz herida", lambda c: r_voz(c, 170, ((0, 1), (0.3, 1.2), (1, 0.8)), 0.3, "a", 0.25, 0.25)),
        (22, "golpe medio 2", lambda c: r_golpe(c, 0.55, 1.0, 0.25, 0.3)),
        (23, "whoosh medio", lambda c: r_whoosh(c, 500, 2600, 0.3)),
        (26, "estallido grave", lambda c: r_estallido(c, 1.0, 45.0, 0.7)),
        (27, "garra", lambda c: r_garra(c, 2, 0.26)),
        (28, "golpe pesado 2", lambda c: r_golpe(c, 0.75, 0.9, 0.5, 0.35)),
        (29, "rayo", lambda c: r_rayo(c, 0.4)),
        (30, "ui tono", lambda c: r_ui(c, [987.8], 0.05, 0.08, "seno")),
        (33, "cadena", lambda c: r_cadena(c, 0.34)),
        (35, "energia larga", lambda c: r_energia(c, 120, 700, 0.9)),
        (41, "corte fuerte", lambda c: r_corte(c, 0.7, 0.6, 0.36, 1800)),
        (42, "huesos", lambda c: r_huesos(c, 6, 0.42)),
        (43, "voz muere", lambda c: r_voz(c, 130, ((0, 1.1), (0.2, 1.2), (1, 0.5)), 0.9, "o", 0.3, 0.3)),
        (46, "gong agudo", lambda c: r_gong(c, 196.0, 1.6)),
        (47, "golpe ligero 3", lambda c: r_golpe(c, 0.2, 1.3, 0.0, 0.2)),
        (49, "cuerno corto", lambda c: r_cuerno(c, 196.0, 0.8)),
        (50, "ui lira", lambda c: r_ui(c, [523.3, 784.0], 0.07, 0.3)),
        (53, "whoosh grave", lambda c: r_whoosh(c, 200, 1400, 0.35)),
        (54, "golpe sordo", lambda c: r_paso(c, 0.24)),
        (57, "estallido brillante", lambda c: r_estallido(c, 0.7, 70.0, 1.2)),
        (58, "corte metal", lambda c: r_corte(c, 0.4, 0.7, 0.34, 3100)),
        (71, "energia aguda", lambda c: r_energia(c, 300, 1500, 0.5)),
        (72, "golpe medio 3", lambda c: r_golpe(c, 0.6, 1.0, 0.3, 0.32)),
    ]
    for num, receta, fn in genericos:
        d(f"sound{num}", "sin referencia en codigo/datos (variante generica)", receta, fn)
    return T


def _jefe_entra(c):
    x = np.zeros(ns(2.6))
    mezcla(x, r_cuerno(c, 73.4, 2.0, True, 0.9), 0)
    mezcla(x, r_taiko(c, 130, 45, 0.35, 1.4, 1.0), 0)
    mezcla(x, r_rugido(c, 60, 1.5, "o", 0.7) * 0.7, ns(0.35))
    return x


def _cesto_fuerte(c):
    x = r_golpe(c, 0.75, 1.1, 0.5, 0.38, 0.12)
    mezcla(x, r_metal(c, 1500, 0.25, 4, 0.35), 0)
    return x


def _furia(c):
    x = np.zeros(ns(1.4))
    mezcla(x, r_rugido(c, 95, 1.1, "a", 0.6), 0)
    mezcla(x, r_energia(c, 100, 600, 1.2) * 0.6, ns(0.05))
    return x


def _no_muerto(c, muere):
    if not muere:
        x = r_huesos(c, 4, 0.25)
        mezcla(x, r_golpe(c, 0.3, 0.6, 0.0, 0.15, 0.0) * 0.5, 0)
        return x
    x = np.zeros(ns(1.0))
    mezcla(x, r_huesos(c, 8, 0.6), 0)
    mezcla(x, r_voz(c, 110, ((0, 1.0), (1, 0.6)), 0.9, "u", 0.5, 0.5, 0.9) * 0.6, ns(0.05))
    return x


def _titan(c, dur):
    x = r_rugido(c, 45, dur, "o", 0.85, 0.6)
    mezcla(x, r_estallido(c, 1.0, 40.0, 0.6) * 0.7, 0)
    return x


def _poseidon(c, dur):
    x = r_rugido(c, 80, dur, "a", 0.6, 0.7)
    ola = r_whoosh(c, 150, 1200, dur, 0.9, 6)
    mezcla(x, ola / (np.max(np.abs(ola)) + 1e-9) * 0.5, 0)
    return x


def termina_sfx(x):
    x = np.asarray(x, dtype=np.float64)
    x = quita_inicio(x)
    x = recorta_final(x)
    x = bordes(x)
    return normaliza(x, PICO_SFX_DB)


# ---------------------------------------------------------------------------
# MUSICA
# ---------------------------------------------------------------------------

FRIGIO = [0, 1, 3, 5, 7, 8, 10]


def grado_a_midi(raiz, g):
    return raiz + 12 * (g // 7) + FRIGIO[g % 7]


def hz(midi):
    return 440.0 * 2 ** ((midi - 69) / 12.0)


def acorde(raiz, g, octava=0, mayor=False):
    notas = [grado_a_midi(raiz, g + k) + 12 * octava for k in (0, 2, 4)]
    if mayor and g % 7 == 0:   # tercera de picardia (frigio dominante)
        notas[1] += 1
    return notas


def drone(c, f, dur, nivel=1.0):
    """Drone de quinta: sierras (raiz + quinta + octava baja) muy filtradas."""
    n = ns(dur)
    t = tt(n)
    x = np.zeros(n)
    for r, g in ((1.0, 1.0), (1.5, 0.6), (0.5, 0.7)):
        for det in (-0.002, 0.002):
            x += sierra_f(np.full(n, f * r * (1 + det))) * g
    x = pbajo(pbajo(x, 700), 900)
    x *= 0.85 + 0.15 * np.sin(2 * np.pi * 0.13 * t + c.rng.uniform(0, 6.28))
    return x * env_adsr(n, min(0.6, dur * 0.3), min(0.8, dur * 0.3)) * nivel


def coro(c, midi_notas, dur, nivel=1.0, vocal="a", ataque=0.5):
    """Coro 'aah': por voz 3 sierras desafinadas -> 3 formantes paso banda."""
    n = ns(dur)
    t = tt(n)
    x = np.zeros(n)
    for m in midi_notas:
        f = hz(m)
        src = np.zeros(n)
        for det in (-0.006, 0.0, 0.007):
            vib = 1 + 0.006 * np.sin(2 * np.pi * (5.0 + c.rng.uniform(-0.6, 0.6)) * t + c.rng.uniform(0, 6.28))
            src += sierra_f(f * (1 + det) * vib)
        src += c.ruido(n) * 0.08
        x += formantes(src, vocal, 1.0 if f < 300 else 1.12, 0.12)
    return x * env_adsr(n, min(ataque, dur * 0.4), min(0.7, dur * 0.4)) * nivel


def aulos_nota(c, f, dur, nivel=1.0):
    """Aulos (caña doble): pulso estrecho con vibrato, filtrado nasal."""
    n = ns(dur)
    t = tt(n)
    vib = 1 + 0.008 * np.sin(2 * np.pi * 5.5 * t) * np.clip(t / 0.25, 0, 1)
    ph = np.cumsum(f * vib) / SR
    ph -= np.floor(ph)
    pul = np.where(ph < 0.22, 1.0, -0.28) + 0.05 * c.ruido(n)
    y = pb(pul, 400, 3800) + 0.6 * pb(pul, f * 2.0 * 0.9, f * 2.0 * 1.1)
    return y * env_adsr(n, 0.03, min(0.12, dur * 0.4)) * nivel


def tambor_mano(c, nivel=1.0):
    """Tambor de marco (tof): golpe agudo corto para corcheas."""
    n = ns(0.18)
    t = tt(n)
    x = np.sin(2 * np.pi * barrido(320, 220, n) * t) * env(n, 0.0005, 0.05, 1.5)
    x += pb(c.ruido(n), 900, 5000) * env(n, 0.0003, 0.02, 1.5) * 0.6
    return x * nivel


class Pista:
    def __init__(self, c, dur, cola=3.0):
        self.c = c
        self.dur = dur
        self.n = ns(dur)
        self.total = self.n + ns(cola)
        self.secos = np.zeros((self.total, 2))
        self.envio = np.zeros((self.total, 2))

    def pon(self, x, ini_s, nivel=1.0, pan=0.0, reverb=0.3):
        i = ns(ini_s) if ini_s > 0 else 0
        if i >= self.total:
            return
        m = min(len(x), self.total - i)
        gl, gr = np.cos((pan + 1) * np.pi / 4), np.sin((pan + 1) * np.pi / 4)
        seg = x[:m] * nivel
        self.secos[i:i + m, 0] += seg * gl
        self.secos[i:i + m, 1] += seg * gr
        self.envio[i:i + m, 0] += seg * gl * reverb
        self.envio[i:i + m, 1] += seg * gr * reverb

    def mezcla_final(self, bucle=True, fade_s=0.0, rev_dur=2.4, rev_mezcla=0.55):
        h = np.zeros_like(self.envio)
        for ch in range(2):
            ir = reverb_ir(self.c, rev_dur, 4200.0, 0.02 + 0.007 * ch)
            h[:, ch] = signal.fftconvolve(self.envio[:, ch], ir)[: self.total]
        x = self.secos + h * rev_mezcla
        x = signal.sosfilt(_sos("high", 45.0), x, axis=0)
        if bucle:   # la cola que pasa del final se suma al principio: bucle sin corte
            cola = x[self.n:]
            x = x[: self.n].copy()
            m = min(len(cola), self.n)
            x[:m] += cola[:m]
        else:
            x = x[: self.n].copy()
            if fade_s > 0:
                k = ns(fade_s)
                x[-k:] *= np.linspace(1.0, 0.0, k)[:, None] ** 1.5
            x[: ns(0.003)] *= np.linspace(0, 1, ns(0.003))[:, None]
        p = np.max(np.abs(x)) + 1e-9
        x = np.tanh(1.3 * x / p) / np.tanh(1.3)   # compresion suave del pico
        return normaliza(x, PICO_MUSICA_DB)


def melodia(c, compases, raiz, octava=1, notas_por_compas=(4, 6, 3, 5), cierre=0):
    """Motivo frigio determinista: frases de 2 compases, A A' B A."""
    ritmos = {
        3: [1.5, 1.5, 1.0],
        4: [1.0, 1.0, 1.0, 1.0],
        5: [1.0, 0.5, 0.5, 1.0, 1.0],
        6: [0.5, 0.5, 1.0, 0.5, 0.5, 1.0],
    }
    def frase():
        notas = []
        g = c.rng.choice([0, 2, 4, 7])
        for _ in range(2):
            k = int(c.rng.choice(notas_por_compas))
            for dur in ritmos[k]:
                g = int(np.clip(g + c.rng.choice([-2, -1, -1, 1, 1, 2, 0, 3]), -2, 9))
                notas.append((g, dur))
        notas[-1] = (cierre if c.rng.uniform() < 0.6 else 4, notas[-1][1])
        return notas
    A, B = frase(), frase()
    A2 = list(A)
    A2[-2] = ((A2[-2][0] + 1), A2[-2][1])   # variacion
    plan = [A, A2, B, A]
    sec = []
    for k in range(compases // 2):
        sec.append(plan[k % 4])
    return [[(grado_a_midi(raiz, g) + 12 * octava, d) for g, d in fr] for fr in sec]


def compone_bucle(nombre, raiz, bpm, compases, prog, intensidad, taiko_patron, mano=False,
                  melodia_desde=2, oct_mel=1, lira_patron="arpegio"):
    c = Ctx(nombre)
    negra = 60.0 / bpm
    compas = 4 * negra
    dur = compases * compas
    P = Pista(c, dur, cola=3.0)
    # Drones de quinta cada 2 compases (sobre la raiz del modo, cambian solo en bII).
    for k in range(0, compases, 2):
        g = prog[k % len(prog)]
        base = raiz - 12 + (1 if g % 7 == 1 else 0)
        P.pon(drone(c, hz(base), 2 * compas + 1.0, 1.0), k * compas, 0.07, 0.0, 0.35)
    # Coro por compas (acordes de la progresion).
    for k in range(compases):
        g = prog[k % len(prog)]
        oc = 0 if raiz >= 57 else 1
        notas = acorde(raiz - 12 + 12 * oc, g)
        P.pon(coro(c, notas, compas + 0.6, 1.0, "a" if k % 4 < 2 else "o", 0.4 * compas),
              k * compas, 0.15 * intensidad[k % len(intensidad)], 0.0, 0.7)
    # Lira: arpegio en corcheas o pulsadas en tiempos.
    for k in range(compases):
        g = prog[k % len(prog)]
        notas = acorde(raiz, g)
        orden = [0, 1, 2, 1, 2, 0, 1, 2] if lira_patron == "arpegio" else [0, 2, 1, 2]
        paso = compas / len(orden)
        for j, idx in enumerate(orden):
            m = notas[idx] + (12 if j % 4 == 3 else 0)
            P.pon(lira_nota(c, hz(m), 1.2, 0.55), k * compas + j * paso, 0.24,
                  -0.35 + 0.7 * (j % 2), 0.45)
    # Aulos: melodia frigia (motivo).
    frases = melodia(c, compases - melodia_desde, raiz, oct_mel)
    tpos = melodia_desde * compas
    for fr in frases:
        tf = tpos
        for m, d in fr:
            P.pon(aulos_nota(c, hz(m), d * negra * 0.95), tf, 0.2, 0.15, 0.5)
            tf += d * negra
        tpos += 2 * compas
    # Taiko: patron por compas (pos en negras, fuerza, tono).
    for k in range(compases):
        for pos, fuerza, f0 in taiko_patron:
            P.pon(r_taiko(c, f0, f0 * 0.42, 0.18 + 0.1 * fuerza, 1.0, 0.7), k * compas + pos * negra,
                  0.26 * fuerza * intensidad[k % len(intensidad)], 0.0, 0.2)
        if mano:
            for j in range(8):
                P.pon(tambor_mano(c), k * compas + j * negra * 0.5, 0.08 + 0.05 * (j % 2 == 0),
                      0.4 if j % 2 else -0.4, 0.15)
    return P.mezcla_final(bucle=True)


def m_menu():
    return compone_bucle("menu.ogg", 62, 72.0, 12, [0, 0, 1, 0, 6, 5, 6, 0, 0, 1, 3, 0],
                         [0.7, 0.8, 0.9, 1.0], [(0, 1.0, 120), (2, 0.6, 120)], False, 4, 1, "arpegio")


def m_gate1():
    return compone_bucle("gate1music.ogg", 64, 96.0, 12, [0, 1, 0, 6, 5, 6, 0, 1, 3, 1, 0, 0],
                         [1.0, 0.9, 1.0, 1.1], [(0, 1.0, 140), (1.5, 0.6, 160), (2, 0.9, 140), (3, 0.5, 180), (3.5, 0.5, 180)],
                         True, 2, 1, "arpegio")


def m_gate2():
    return compone_bucle("gate2music.ogg", 59, 120.0, 15, [0, 0, 1, 1, 5, 6, 0, 0, 3, 1, 5, 6, 0, 1, 0],
                         [1.0, 1.1, 1.0, 1.2], [(0, 1.0, 130), (0.75, 0.5, 170), (1.5, 0.7, 150), (2, 1.0, 130),
                                               (2.5, 0.4, 190), (3, 0.7, 150), (3.5, 0.6, 170)],
                         True, 1, 2, "pulsada")


def m_music001():
    return compone_bucle("music_001.ogg", 66, 48 * 60.0 / 35.0, 12, [0, 6, 5, 6, 0, 1, 0, 3, 5, 6, 1, 0],
                         [0.8, 0.9, 1.0, 1.0], [(0, 1.0, 130), (2, 0.7, 150), (3, 0.4, 170)], False, 2, 1, "arpegio")


def m_gamecg():
    """Cinematica 17 s: coro que crece, taikos espaciados y gong final."""
    c = Ctx("gameCG.ogg")
    dur = 17.0
    P = Pista(c, dur, 3.0)
    raiz = 60
    P.pon(drone(c, hz(raiz - 12), dur + 1.0), 0, 0.09, 0, 0.4)
    prog = [(0, 0.0, 4.0), (1, 4.0, 4.0), (6, 8.0, 3.0), (5, 11.0, 2.5), (0, 13.5, 3.5)]
    for k, (g, ini, d) in enumerate(prog):
        P.pon(coro(c, acorde(raiz - 12, g) + [grado_a_midi(raiz, g)], d + 1.0, 1.0, "a", 1.2),
              ini, 0.08 + 0.025 * k, 0, 0.8)
    for k, ti in enumerate([0.0, 4.0, 8.0, 10.0, 11.0, 12.0, 12.5, 13.0, 13.25, 13.5]):
        P.pon(r_taiko(c, 120, 48, 0.35, 1.0, 1.0), ti, 0.25 + 0.03 * k, 0, 0.35)
    for j, g in enumerate([0, 2, 4, 7, 4, 2, 1, 0]):
        P.pon(lira_nota(c, hz(grado_a_midi(raiz, g)), 1.6, 0.5), 4.0 + j * 0.5, 0.15, -0.3 + 0.6 * (j % 2), 0.5)
    for j, (g, ti, d) in enumerate([(7, 8.0, 1.0), (8, 9.0, 0.5), (7, 9.5, 0.5), (5, 10.0, 1.0), (4, 11.0, 2.0), (1, 13.0, 0.5), (0, 13.5, 2.5)]):
        P.pon(aulos_nota(c, hz(grado_a_midi(raiz, g) + 12), d), ti, 0.14, 0.15, 0.5)
    P.pon(r_gong(c, 98.0, 3.5, 0.6), 13.5, 0.4, 0, 0.4)
    return P.mezcla_final(bucle=True)


def m_victoria():
    """Victoria 8 s: frigio dominante (tercera mayor), arpegio de lira, gong."""
    c = Ctx("music_002.ogg")
    dur = 8.0
    P = Pista(c, dur, 0.0)
    raiz = 64
    P.pon(r_taiko(c, 150, 55, 0.3, 1.0, 1.0), 0.0, 0.35, 0, 0.3)
    for k, ti in enumerate([0.0, 0.25, 0.5, 0.75]):
        P.pon(r_taiko(c, 170, 70, 0.2, 1.0, 0.6), 1.0 + ti, 0.18 + 0.03 * k, 0, 0.3)
    mayor = acorde(raiz, 0, 0, mayor=True)
    P.pon(r_cuerno(c, hz(raiz - 12), 3.0, True, 1.0), 2.0, 0.25, 0, 0.4)
    P.pon(coro(c, [m - 12 for m in mayor] + [raiz], 5.5, 1.0, "a", 0.6), 2.0, 0.12, 0, 0.8)
    for j, m in enumerate(mayor + [raiz + 12, mayor[1] + 12, mayor[2] + 12, raiz + 24]):
        P.pon(lira_nota(c, hz(m), 2.0, 0.6), 0.1 + 0.18 * j, 0.2, -0.4 + 0.8 * (j % 2), 0.5)
    P.pon(r_gong(c, hz(raiz - 24), 4.0, 0.5), 2.0, 0.35, 0, 0.4)
    return P.mezcla_final(bucle=False, fade_s=1.5)


def m_derrota():
    """Derrota 7 s: linea frigia descendente grave, coro oscuro, un taiko."""
    c = Ctx("music_003.ogg")
    dur = 7.0
    P = Pista(c, dur, 0.0)
    raiz = 57
    P.pon(drone(c, hz(raiz - 12), dur), 0, 0.08, 0, 0.4)
    P.pon(r_taiko(c, 110, 40, 0.5, 1.0, 1.4), 0.0, 0.4, 0, 0.4)
    for j, g in enumerate([4, 3, 2, 1, 0]):
        P.pon(aulos_nota(c, hz(grado_a_midi(raiz, g)), 1.1), 0.3 + j * 1.0, 0.14, 0.1, 0.5)
    P.pon(coro(c, acorde(raiz - 12, 0), 6.5, 1.0, "o", 1.5), 0.2, 0.12, 0, 0.8)
    P.pon(coro(c, acorde(raiz - 12, 1), 2.5, 1.0, "u", 0.8), 3.0, 0.08, 0, 0.8)
    P.pon(r_gong(c, hz(raiz - 24), 3.0, 0.3), 4.6, 0.25, 0, 0.5)
    return P.mezcla_final(bucle=False, fade_s=2.0)


MUSICA = {
    "menu": ("menu de seleccion (bucle)", "D frigio 72 bpm 12 compases: drone de quinta, coro aah, lira KS, aulos, taiko", m_menu),
    "gate1music": ("escenarios gate1 (bucle)", "E frigio 96 bpm 12 compases: taiko + tof en corcheas, coro, lira, aulos", m_gate1),
    "gate2music": ("escenarios gate2 / volcan (bucle)", "B frigio 120 bpm 15 compases: taiko sincopado, lira pulsada, aulos 2 oct", m_gate2),
    "gameCG": ("cinematica de historia", "C frigio libre 17 s: coro creciente, taikos, lira, aulos, gong", m_gamecg),
    "music_001": ("pieza extra (sin uso hoy)", "F# frigio 82.3 bpm 12 compases: drone, coro, lira, aulos, taiko", m_music001),
    "music_002": ("victoria (una vez)", "E frigio dominante 8 s: taikos, cuerno, coro mayor, arpegio de lira, gong", m_victoria),
    "music_003": ("derrota (una vez)", "A frigio 7 s: aulos descendente, coro oscuro, taiko, gong", m_derrota),
}

# ---------------------------------------------------------------------------
# CODIFICACION / PROCEDENCIA / VERIFICACION
# ---------------------------------------------------------------------------


def codifica_ogg(x, ruta):
    x = np.clip(x, -1.0, 1.0)
    canales = 1 if x.ndim == 1 else x.shape[1]
    pcm = (x * 32767.0).round().astype("<i2").tobytes()
    cmd = ["ffmpeg", "-nostdin", "-v", "error", "-y", "-f", "s16le", "-ar", str(SR), "-ac", str(canales),
           "-i", "pipe:0", "-map_metadata", "-1", "-fflags", "+bitexact", "-flags:a", "+bitexact",
           "-c:a", "libvorbis", "-q:a", "3", str(ruta)]
    subprocess.run(cmd, input=pcm, check=True)


def md5(ruta):
    return hashlib.md5(Path(ruta).read_bytes()).hexdigest()


def todos():
    sfx = _sfx_tabla()
    items = []
    for nombre, (uso, receta, fn) in sorted(sfx.items(), key=lambda kv: int(kv[0][5:])):
        items.append((nombre, uso, "sfx: " + receta, lambda c, fn=fn: termina_sfx(fn(c))))
    for nombre, (uso, receta, fn) in MUSICA.items():
        items.append((nombre, uso, "musica: " + receta, lambda c, fn=fn: fn()))
    return items


def genera(destino, solo=None, habla=True):
    destino.mkdir(parents=True, exist_ok=True)
    filas = []
    for nombre, uso, receta, fn in todos():
        if solo and nombre not in solo:
            continue
        x = fn(Ctx(nombre))
        ruta = destino / f"{nombre}.ogg"
        codifica_ogg(x, ruta)
        dur = (len(x) if x.ndim == 1 else x.shape[0]) / SR
        filas.append((nombre, md5(ruta), dur, uso, receta))
        if habla:
            print(f"{nombre:12s} {dur:6.2f} s  {filas[-1][1]}  {receta}", flush=True)
    return filas


def escribe_procedencia(filas):
    lineas = [
        "PROCEDENCIA del audio del modo BETA (GowApk/assets/audio)",
        "",
        "Todo el audio de esta carpeta es ORIGINAL: sintetizado por codigo con",
        "tools/audio/genera_beta_audio.py (" + VERSION + "), sin muestras externas,",
        "sin transcribir melodias existentes y sin IA de audio. Reemplaza (mismos",
        "nombres) al audio extraido del juego original que habia antes (con copyright).",
        "Codificacion: OGG Vorbis -q:a 3, ffmpeg bitexact, semilla = crc32(nombre).",
        "Comprobar: python3 tools/audio/genera_beta_audio.py --verificar",
        "",
        "nombre.ogg | md5 | duracion | uso | receta",
    ]
    for nombre, h, dur, uso, receta in filas:
        lineas.append(f"{nombre}.ogg | {h} | {dur:.2f} s | {uso} | {receta}")
    (DEST / "PROCEDENCIA.txt").write_text("\n".join(lineas) + "\n", encoding="utf-8")


def lee_procedencia():
    reg = {}
    p = DEST / "PROCEDENCIA.txt"
    if not p.exists():
        return reg
    for ln in p.read_text(encoding="utf-8").splitlines():
        partes = [s.strip() for s in ln.split("|")]
        if len(partes) >= 2 and partes[0].endswith(".ogg") and len(partes[1]) == 32:
            reg[partes[0]] = partes[1]
    return reg


def decodifica(ruta):
    out = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", str(ruta), "-f", "s16le", "-ac", "1",
                          "-ar", str(SR), "pipe:1"], capture_output=True, check=True).stdout
    return np.frombuffer(out, dtype="<i2").astype(np.float64) / 32768.0


def verifica():
    reg = lee_procedencia()
    esperados = {f"{n}.ogg" for n, *_ in todos()}
    fallos = []
    presentes = sorted(p.name for p in DEST.glob("*.ogg"))
    for nombre in presentes:
        if nombre not in esperados:
            fallos.append(f"{nombre}: no lo genera este script (audio de origen desconocido)")
        elif reg.get(nombre) != md5(DEST / nombre):
            fallos.append(f"{nombre}: md5 distinto al registrado en PROCEDENCIA.txt")
    for nombre in sorted(esperados - set(presentes)):
        fallos.append(f"{nombre}: falta")
    avisos = 0
    with tempfile.TemporaryDirectory() as tmp:
        filas = genera(Path(tmp), habla=False)
        for nombre, h, *_ in filas:
            archivo = DEST / f"{nombre}.ogg"
            if not archivo.exists() or h == md5(archivo):
                continue
            # Otro ffmpeg/libvorbis puede dar bytes distintos: entonces se compara la senal.
            a, b = decodifica(archivo), decodifica(Path(tmp) / f"{nombre}.ogg")
            m = min(len(a), len(b))
            corr = float(np.dot(a[:m], b[:m]) / (np.linalg.norm(a[:m]) * np.linalg.norm(b[:m]) + 1e-12))
            if abs(len(a) - len(b)) > SR * 0.05 or corr < 0.98:
                fallos.append(f"{nombre}.ogg: no coincide con lo que genera el script (corr {corr:.3f})")
            else:
                avisos += 1
    for f in fallos:
        print("FALLO", f)
    print(f"verificar: {len(presentes)} ogg en {DEST.relative_to(ROOT)}, fallos: {len(fallos)}"
          + (f", {avisos} identicos en senal pero no en bytes (otro codificador)" if avisos else ""))
    return 0 if not fallos else 1


def informe():
    """Analisis por archivo: duracion, pico, silencio inicial y reparto espectral."""
    print(f"{'archivo':16s} {'dur s':>6s} {'pico dB':>8s} {'ini ms':>7s} {'<300':>5s} {'300-1k2':>7s} {'1k2-2k':>6s} {'2k-6k':>6s} {'>6k':>5s}")
    peor = 0
    for p in sorted(DEST.glob("*.ogg"), key=lambda q: (not q.name.startswith("sound"), len(q.name), q.name)):
        x = decodifica(p)
        dur = len(x) / SR
        pico = 20 * np.log10(np.max(np.abs(x)) + 1e-12)
        lim = np.max(np.abs(x)) * 0.1
        ini = np.argmax(np.abs(x) > lim) / SR * 1000.0
        X = np.abs(np.fft.rfft(x)) ** 2
        fr = np.fft.rfftfreq(len(x), 1 / SR)
        tot = X.sum() + 1e-12
        bandas = [X[(fr >= lo) & (fr < hi)].sum() / tot * 100 for lo, hi in ((0, 300), (300, 1200), (1200, 2000), (2000, 6000), (6000, SR))]
        stub = dur < 0.05
        peor += stub
        print(f"{p.name:16s} {dur:6.2f} {pico:8.1f} {ini:7.1f} " + " ".join(f"{b:5.0f}%" for b in bandas) + ("  STUB" if stub else ""))
    print(f"stubs (<50 ms): {peor}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--verificar", action="store_true")
    ap.add_argument("--informe", action="store_true")
    ap.add_argument("--solo", default="")
    a = ap.parse_args()
    if a.verificar:
        sys.exit(verifica())
    if a.informe:
        informe()
        return
    solo = set(s.strip() for s in a.solo.split(",") if s.strip()) or None
    filas = genera(DEST, solo)
    if solo is None:
        escribe_procedencia(filas)
    else:
        print("(--solo: PROCEDENCIA.txt no se reescribe)")


if __name__ == "__main__":
    main()
