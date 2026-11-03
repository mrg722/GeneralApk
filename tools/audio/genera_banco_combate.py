#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generador del banco de sonidos de combate de District Fury (BRIEF F1.11).

PROCEDENCIA
    Todo se SINTETIZA por codigo (numpy + scipy): osciladores, ruido y filtros.
    No se usa ninguna muestra externa, grabacion ni material con copyright.

QUE ESCRIBE
    assets/audio/sfx/combat/*.wav                       banco generico
    assets/audio/sfx/arquetipos/<agil|cuchillas|pesado|electrico>/*.wav
                                                        variantes por arquetipo
    (nombres = contrato con F1.10, ver docs/PLAN_MAESTRO_MOVIMIENTOS.md)

FORMATO Y REGLAS
    mono, 44.1 kHz, 16 bit; semilla fija POR ARCHIVO (crc32 del nombre) ->
    regenerar da archivos identicos byte a byte; paso alto 20 Hz (sin DC);
    fundido de 2 ms al final; ataque inmediato (silencio inicial < 2 ms);
    pico como maximo -3 dBFS (nunca recorta).
    Regla movil: cada sonido lleva >= 25 % de su energia entre 500 Hz y 3 kHz
    (el altavoz de un telefono casi no da nada bajo ~250 Hz). El subgrave es
    un extra. Lo verifica tools/audio/analiza_banco.py.

VOLUMEN
    Cada categoria se normaliza a un st50 objetivo (RMS maximo en 50 ms):
    golpes -10 dBFS (lo mismo que pide F1.10 al cargar), con el pico limitado a
    -3 dBFS. Pasos a -14 dB y telegrafia a -12 dB respecto de los golpes, como
    pide el brief.

USO
    python3 tools/audio/genera_banco_combate.py [carpeta_sfx]
    (por defecto assets/audio/sfx relativo a la raiz del repo)
"""

import os
import sys
import wave
import zlib

import numpy as np
from scipy import signal

SR = 44100
SEMILLA_BASE = 20261103
PICO_MAX_DB = -3.0
MEDIOS_OBJETIVO = 0.30   # 30 %: margen sobre el 25 % que exige la regla movil

# st50 objetivo por familia (dBFS)
ST50_GOLPE = -10.0
ST50 = {
    "step": ST50_GOLPE - 14.0,
    "step_wet": ST50_GOLPE - 14.0,
    "telegraph": ST50_GOLPE - 12.0,
    "ui_select_char": -13.0,
    "ui_deny": -14.0,
    "game_over": -12.0,
    "stage_clear": -12.0,
    "combo_10": -13.0, "combo_20": -13.0, "combo_30": -13.0,
    "pickup_health": -13.0, "pickup_energy": -13.0,
    "energy_charge": -12.0,
}

rng = np.random.default_rng(0)   # se re-siembra por archivo en genera()


# ---------------------------------------------------------------------------
# utilidades
# ---------------------------------------------------------------------------

def ns(seg):
    return max(1, int(round(seg * SR)))


def t_(n):
    return np.arange(n, dtype=np.float64) / SR


def blanco(n):
    return rng.uniform(-1.0, 1.0, n)


def rosa(n):
    """Ruido rosa (1/f) por forma espectral."""
    m = 1
    while m < n:
        m *= 2
    X = np.fft.rfft(rng.standard_normal(m))
    f = np.arange(len(X), dtype=np.float64)
    f[0] = 1.0
    X /= np.sqrt(f)
    X[0] = 0.0
    y = np.fft.irfft(X, m)[:n]
    return y / (np.max(np.abs(y)) + 1e-12)


def marron(n):
    y = np.cumsum(rng.standard_normal(n))
    y = pa(y, 30.0, 2)
    return y / (np.max(np.abs(y)) + 1e-12)


def _sos(tipo, f, orden):
    nyq = SR / 2.0
    if tipo == "band":
        lo = max(20.0, min(f[0], nyq * 0.95))
        hi = max(lo * 1.05, min(f[1], nyq * 0.97))
        return signal.butter(orden, [lo / nyq, hi / nyq], btype="band", output="sos")
    return signal.butter(orden, max(10.0, min(f, nyq * 0.97)) / nyq, btype=tipo, output="sos")


def pb(x, lo, hi, orden=2):
    return signal.sosfilt(_sos("band", (lo, hi), orden), x)


def pbaj(x, fc, orden=2):
    return signal.sosfilt(_sos("low", fc, orden), x)


def pa(x, fc, orden=2):
    return signal.sosfilt(_sos("high", fc, orden), x)


def svf_barrido(x, fc, q):
    """Paso banda de estado variable (TPT) con centro fc[i] por muestra."""
    fc = np.broadcast_to(np.asarray(fc, dtype=np.float64), x.shape)
    g = np.tan(np.pi * np.clip(fc, 20.0, SR * 0.45) / SR)
    k = 1.0 / q
    a1 = 1.0 / (1.0 + g * (g + k))
    y = np.zeros_like(x)
    ic1 = ic2 = 0.0
    for i in range(len(x)):
        gi = g[i]
        v3 = x[i] - ic2
        v1 = a1[i] * ic1 + gi * a1[i] * v3
        v2 = ic2 + gi * v1
        ic1 = 2.0 * v1 - ic1
        ic2 = 2.0 * v2 - ic2
        y[i] = v1
    return y * k   # ganancia unidad en el centro


def barrido(n, f0, f1, dur=None):
    """Frecuencia instantanea exponencial f0 -> f1 en 'dur' s (luego fija)."""
    nd = n if dur is None else min(n, ns(dur))
    f = np.full(n, float(f1))
    f[:nd] = f0 * (f1 / f0) ** np.linspace(0.0, 1.0, nd)
    return f


def fase(f):
    return 2.0 * np.pi * np.cumsum(f) / SR


def seno(f):
    return np.sin(fase(f))


def triangulo(f):
    return signal.sawtooth(fase(f), width=0.5)


def sierra(f):
    return signal.sawtooth(fase(f))


def env_exp(n, tau, ataque=0.0005, retardo=0.0):
    t = t_(n) - retardo
    e = np.where(t >= 0, np.exp(-np.maximum(t, 0) / tau), 0.0)
    na = ns(ataque)
    rampa = np.clip((t * SR) / na, 0.0, 1.0)
    return e * rampa


def env_campana(n, pico=0.4, suelo=0.12):
    """Sube y baja (seno^2) sin empezar en cero (evita silencio inicial)."""
    u = np.linspace(0.0, 1.0, n)
    e = np.where(u < pico, np.sin(0.5 * np.pi * u / pico) ** 2,
                 np.cos(0.5 * np.pi * (u - pico) / (1 - pico)) ** 2)
    return suelo + (1 - suelo) * e * np.linspace(1.0, 0.0, n) ** 0.3


def desplaza(x, seg, n):
    y = np.zeros(n)
    d = ns(seg)
    if d < n:
        m = min(len(x), n - d)
        y[d:d + m] = x[:m]
    return y


def normaliza(x):
    return x / (np.max(np.abs(x)) + 1e-12)


def satura(x, drive):
    return np.tanh(drive * x) / np.tanh(drive)


def granos(n, por_seg, lo, hi, dur_grano=(0.003, 0.008), inicio=0.0):
    y = np.zeros(n)
    tt = inicio
    while True:
        tt += rng.exponential(1.0 / por_seg)
        i = ns(tt)
        if i >= n:
            break
        g = ns(rng.uniform(*dur_grano))
        e = np.hanning(g) * rng.uniform(0.4, 1.0)
        m = min(g, n - i)
        y[i:i + m] += (blanco(g) * e)[:m]
    return pb(y, lo, hi, 2)


def crackle(n, tasa, lo=2000.0, hi=9000.0, tau=0.2):
    """Impulsos de Poisson filtrados (chisporroteo electrico)."""
    imp = (rng.uniform(0, 1, n) < tasa / SR) * rng.uniform(-1, 1, n)
    y = pb(imp, lo, hi, 2)
    y = normaliza(y) * env_exp(n, tau)
    return y


def reverb(x, cola=0.12, mezcla=0.25):
    m = ns(cola)
    ir = blanco(m) * np.exp(-t_(m) / (cola / 5.0))
    ir = pbaj(ir, 6000.0)
    ir /= np.sqrt(np.sum(ir ** 2)) + 1e-12
    w = signal.fftconvolve(x, ir)[:len(x)]
    return x + mezcla * w * (np.max(np.abs(x)) / (np.max(np.abs(w)) + 1e-12))


def fm(fc, fmod, indice, n):
    tt = t_(n)
    return np.sin(2 * np.pi * fc * tt + indice * np.sin(2 * np.pi * fmod * tt))


# ---------------------------------------------------------------------------
# capas de golpe
# ---------------------------------------------------------------------------

def chasquido(n, lo, hi, dur=0.004, nivel=1.0):
    return pb(blanco(n), lo, hi, 2) * env_exp(n, dur / 2.5, 0.0002) * nivel


def cuerpo(n, f0, f1, sweep, tau, tri=0.5):
    f = barrido(n, f0, f1, sweep)
    o = (1 - tri) * seno(f) + tri * triangulo(f)
    return o * env_exp(n, tau)


def carne(n, lo, hi, tau):
    return normaliza(pb(rosa(n), lo, hi, 2)) * env_exp(n, tau, 0.0008)


def golpe_ligero(dur, k=1.0, pitch=1.0, chas=1.0):
    n = ns(dur)
    y = (0.9 * chas * chasquido(n, 2000 * k, 6000 * k)
         + 0.55 * cuerpo(n, 420 * k * pitch, 180 * k * pitch, 0.030, 0.035)
         + 0.95 * carne(n, 300 * k * pitch, 1400 * k, 0.040)
         + 0.35 * pb(cuerpo(n, 1100 * k * pitch, 700 * k * pitch, 0.02, 0.02), 500, 3000))
    return satura(y, 2.5)


def golpe_medio(dur, k=1.0, pitch=1.0, chas=1.0):
    n = ns(dur)
    y = (0.9 * chas * chasquido(n, 1500 * k, 5000 * k, 0.005)
         + 0.55 * cuerpo(n, 420 * k * pitch, 180 * k * pitch, 0.045, 0.045)
         + 1.0 * carne(n, 320 * k * pitch, 1500 * k, 0.060)
         + 0.45 * seno(barrido(n, 160 * pitch, 70 * pitch, 0.06)) * env_exp(n, 0.060)
         + 0.4 * pb(cuerpo(n, 1000 * k * pitch, 600 * k * pitch, 0.03, 0.03), 500, 3000))
    return satura(y, 2.5)


def golpe_pesado(dur, k=1.0, pitch=1.0, chas=1.0, sub=1.0):
    n = ns(dur)
    y = (0.9 * chas * chasquido(n, 800 * k, 8000 * k, 0.007)
         + 0.6 * cuerpo(n, 260 * k * pitch, 60 * k * pitch, 0.080, 0.09, 0.6)
         + 0.4 * sub * seno(barrido(n, 90 * pitch, 40 * pitch, 0.12)) * env_exp(n, 0.12)
         + 1.0 * carne(n, 350 * pitch, 1800 * k, 0.08)
         + 0.8 * granos(n, rng.uniform(30, 60), 1000 * k, 3000 * k) * env_exp(n, 0.10)
         + 0.45 * pb(cuerpo(n, 900 * k * pitch, 500 * k * pitch, 0.05, 0.05), 500, 3000))
    y = reverb(y, 0.12, 0.3)
    return satura(y, 3.0)


def tela(n, tau=0.02):
    return pb(blanco(n), 3000, 7000, 2) * env_exp(n, tau)


def whoosh(n, f0, f1, f2, q, ataque_pico=0.45):
    u = np.linspace(0, 1, n)
    centro = np.where(u < ataque_pico, f0 * (f1 / f0) ** (u / ataque_pico),
                      f1 * (f2 / f1) ** ((u - ataque_pico) / (1 - ataque_pico)))
    y = svf_barrido(rosa(n), centro, q)
    return normaliza(y) * env_campana(n, ataque_pico)


def patada(dur, k=1.0, pitch=1.0, chas=1.0):
    n = ns(dur)
    pre_n = ns(0.040)
    u = np.linspace(0, 1, pre_n)
    pre = normaliza(svf_barrido(rosa(pre_n), 800 * (2500 / 800) ** u, 2.5)) * (0.2 + 0.8 * u ** 1.5)
    imp = golpe_medio(dur - 0.035, k, pitch * 0.75, chas)
    y = np.zeros(n)
    y[:pre_n] += 0.45 * pre
    y += desplaza(imp, 0.035, n)
    y += 0.35 * desplaza(tela(n), 0.035, n)
    return y


def corte(dur, k=1.0):
    n = ns(dur)
    sw_n = ns(0.060)
    u = np.linspace(0, 1, sw_n)
    sw = normaliza(svf_barrido(blanco(sw_n), 6000 * k * (2000 / 6000) ** u, 6.0)) * (0.3 + 0.7 * np.sin(np.pi * u) ** 0.7)
    tt = t_(n)
    ring = np.zeros(n)
    for p, a, tau in ((1900, 1.0, 0.15), (3100, 0.6, 0.12), (4700, 0.35, 0.10), (6200, 0.2, 0.08)):
        ring += a * np.sin(2 * np.pi * p * k * rng.uniform(0.98, 1.02) * tt + rng.uniform(0, 6.28)) * env_exp(n, tau)
    y = np.zeros(n)
    y[:sw_n] += 0.55 * sw
    y += 0.35 * desplaza(ring, 0.03, n)
    y += 0.8 * desplaza(carne(n, 300, 1100, 0.05), 0.03, n)
    y += 0.5 * desplaza(chasquido(n, 1500, 5000), 0.03, n)
    return satura(y, 1.8)


def zap(dur, k=1.0, grave=1.0):
    n = ns(dur)
    f = barrido(n, 1200 * k, 200 * k, min(dur, 0.12))
    fm_mod = rng.uniform(60, 90)
    f = f * (1 + 0.5 * np.sin(2 * np.pi * fm_mod * t_(n)))
    z = sierra(f) * env_exp(n, 0.07, 0.0005)
    z = pbaj(z, 5000)
    y = (0.7 * z + 0.45 * crackle(n, 900, tau=0.2)
         + 0.5 * grave * seno(barrido(n, 150, 50, 0.1)) * env_exp(n, 0.08)
         + 0.4 * chasquido(n, 1500, 6000, 0.004))
    return satura(y, 2.0)


def tok(n, k=1.0, retardo=0.0):
    tt = t_(n)
    e = env_exp(n, 0.015, 0.0003, retardo)
    y = (np.sin(2 * np.pi * 900 * k * tt) + 0.7 * np.sin(2 * np.pi * 1400 * k * tt)) * e
    y += 0.6 * pb(blanco(n), 1000 * k, 3000 * k) * env_exp(n, 0.004, 0.0002, retardo)
    y += 0.3 * np.sin(2 * np.pi * 2200 * k * tt) * env_exp(n, 0.025, 0.0003, retardo)
    return y


def nota(n, f0, tau, forma="seno", retardo=0.0, ataque=0.002):
    tt = t_(n)
    if forma == "seno":
        o = np.sin(2 * np.pi * f0 * tt)
    elif forma == "pulso":
        o = signal.square(2 * np.pi * f0 * tt, duty=0.3) * 0.6
    else:
        o = fm(f0, f0 * 1.41, 2.5, n)
    return o * env_exp(n, tau, ataque, retardo)


# ---------------------------------------------------------------------------
# recetas: nombre -> funcion(variante) -> senal cruda
# ---------------------------------------------------------------------------

def var(i, base=0.08):
    """Factor de frecuencia por variante (+-8 %)."""
    return 1.0 + base * rng.uniform(-1, 1)


def r_hit_light(i, a):
    return golpe_ligero(rng.uniform(0.09, 0.12), var(i) * a["k"], a["pitch"], a["chas"])


def r_hit_medium(i, a):
    return golpe_medio(rng.uniform(0.13, 0.17), var(i) * a["k"], a["pitch"], a["chas"])


def r_hit_heavy(i, a):
    return golpe_pesado(rng.uniform(0.25, 0.32), var(i) * a["k"], a["pitch"], a["chas"], a["sub"])


def r_kick(i, a):
    return patada(rng.uniform(0.16, 0.20), var(i) * a["k"], a["pitch"], a["chas"])


def r_slash(i, a):
    return corte(rng.uniform(0.20, 0.30), var(i, 0.05) * a["k"])


def r_energy(i, a):
    return zap(rng.uniform(0.25, 0.40), var(i) * a["k"])


def r_energy_impact(i, a):
    d = rng.uniform(0.25, 0.35)
    n = ns(d)
    return 0.7 * zap(d, var(i) * 0.9) + 0.6 * golpe_medio(d, var(i))[:n]


def r_energy_shot(i, a):
    d = rng.uniform(0.22, 0.30)
    n = ns(d)
    f = barrido(n, 1800 * var(i), 300, d * 0.8)
    f = f * (1 + 0.3 * np.sin(2 * np.pi * rng.uniform(60, 90) * t_(n)))
    y = pbaj(sierra(f), 4500) * env_exp(n, d / 3, 0.0005) + 0.4 * crackle(n, 600, tau=d / 2)
    y += 0.4 * chasquido(n, 1200, 5000)
    return satura(y, 1.8)


def r_energy_charge(i, a):
    d = 0.6
    n = ns(d)
    u = np.linspace(0, 1, n)
    f = 200 * (900 / 200) ** u
    trem = 1 - (0.2 + 0.6 * u) * (0.5 + 0.5 * np.sin(2 * np.pi * (6 + 18 * u) * t_(n)))
    y = (0.6 * sierra(f) + 0.4 * sierra(f * 1.007)) * trem * (0.35 + 0.65 * u)
    y = pb(y, 300, 5000)
    y += 0.2 * crackle(n, 200 + 1200 * 0.5, tau=1.0) * u
    y[-ns(0.02):] *= np.linspace(1, 0, ns(0.02))
    return satura(y, 1.5)


def r_block(i, a):
    n = ns(rng.uniform(0.07, 0.09))
    return satura(tok(n, var(i, 0.06)), 1.5)


def r_guard_break(i, a):
    n = ns(0.40)
    y = np.zeros(n)
    for j, (r, k) in enumerate(((0.0, 1.0), (0.025, 0.85), (0.055, 0.7))):
        y += (1 - 0.15 * j) * tok(n, k, r)
    y += 0.8 * desplaza(golpe_pesado(0.32), 0.06, n)
    y += 0.4 * desplaza(granos(n, 120, 5000, 12000, (0.002, 0.005)) * env_exp(n, 0.12), 0.06, n)
    return satura(y, 1.5)


def r_player_hurt(i, a):
    n = ns(0.15)
    k = var(i)
    y = golpe_medio(0.15, k * 0.9, 0.85, 0.5)
    thud = pbaj(cuerpo(n, 220 * k, 90 * k, 0.04, 0.05, 0.3) + 0.8 * carne(n, 250, 1100, 0.05), 2000, 4)
    y = 0.6 * pbaj(y, 2000, 2) + 0.9 * thud
    return satura(y, 2.0)


def r_whoosh_light(i, a):
    n = ns(0.12 * var(i, 0.1))
    k = var(i) * a["k"]
    return whoosh(n, 600 * k, 2400 * k, 900 * k, 2.2)


def r_whoosh_heavy(i, a):
    n = ns(0.23 * var(i, 0.1))
    k = var(i) * a["k"] * a["pitch"]
    y = whoosh(n, 600 * k, 2400 * k, 900 * k, 1.0)
    if a["sub"] > 1.0:
        y += 0.25 * seno(barrido(n, 120, 70)) * env_campana(n)
    return y


def r_jump(i, a):
    n = ns(0.11)
    y = whoosh(n, 500, 1500, 1500, 2.0, 0.6)
    y += 0.35 * pb(blanco(n), 2000, 5000) * env_campana(n, 0.3)
    return y


def r_land_light(i, a):
    n = ns(0.12)
    tap = normaliza(pb(blanco(n), 200, 800, 2)) * env_exp(n, 0.015) + 0.5 * cuerpo(n, 500, 250, 0.02, 0.02)
    polvo = pa(blanco(n), 3000) * env_exp(n, 0.06)
    y = tap + 0.3 * polvo + 0.5 * carne(n, 400, 1500, 0.03)
    return satura(y, 1.5)


def r_land_knockdown(i, a):
    n = ns(0.30)
    def golpe(r, g):
        b = cuerpo(n, 180, 60, 0.05, 0.07) + 1.0 * carne(n, 300, 1500, 0.05) + 0.6 * chasquido(n, 1000, 4000, 0.005)
        return g * desplaza(b, r, n)
    y = golpe(0.0, 1.0) + golpe(0.09, 10 ** (-8 / 20.0))
    y += 0.3 * pa(blanco(n), 3000) * env_exp(n, 0.06)
    return satura(y, 2.0)


def r_step(i, a):
    n = ns(0.06)
    k = var(i, 0.1)
    tap = normaliza(pb(blanco(n), 150 * k, 600 * k)) * env_exp(n, 0.012)
    tap += 0.9 * carne(n, 500, 1600, 0.012)
    roce = pb(blanco(n), 2000, 4000) * env_exp(n, 0.02, 0.003)
    return tap + 0.25 * roce


def r_step_wet(i, a):
    n = ns(0.06)
    y = r_step(i, a)
    u = np.linspace(0, 1, n)
    chap = normaliza(svf_barrido(blanco(n), 1000 * 3 ** u, 3.0)) * env_exp(n, 0.025, 0.002)
    return y + 0.6 * chap


def r_ko_enemy(i, a):
    n = ns(0.58)
    y = desplaza(golpe_pesado(0.30, var(i)), 0.0, n)
    y += 0.7 * desplaza(r_land_knockdown(0, a), 0.18, n)
    g = 10 ** (-10 / 20.0)
    f1, f2 = 440 * var(i, 0.03), 330 * var(i, 0.03)
    y += g * (nota(n, f1, 0.12, "fm", 0.20, 0.03) + nota(n, f2, 0.18, "fm", 0.32, 0.03))
    return y


def r_game_over(i, a):
    d = 1.85
    n = ns(d)
    u = np.linspace(0, 1, n)
    caida = 2.0 ** (-u * 1.0)  # baja una octava 220->110
    y = np.zeros(n)
    for semi in (0, 3, 7, 12):
        f = 220 * 2 ** (semi / 12) * caida
        for det in (0.995, 1.0, 1.006):
            y += sierra(f * det) / 3
    env = np.minimum(1, t_(n) / 0.003) * np.exp(-np.maximum(t_(n) - 0.5, 0) / 0.3)
    y = pb(y, 400, 2600, 2) * env
    y += 0.5 * desplaza(pbaj(golpe_pesado(0.4, 0.8), 2500), 0.0, n)
    return y


def r_stage_clear(i, a):
    d = 1.4
    n = ns(d)
    y = np.zeros(n)
    base = 523.25
    for j, semi in enumerate((0, 4, 7, 12)):
        f = base * 2 ** (semi / 12)
        y += 0.6 * nota(n, f, 0.22 if j < 3 else 0.5, "pulso", 0.07 * j, 0.001)
        y += 0.4 * nota(n, f, 0.25 if j < 3 else 0.6, "fm", 0.07 * j, 0.001)
    y += 0.4 * nota(n, base * 2, 0.6, "fm", 0.28, 0.002)
    y = pbaj(y, 5000)
    return reverb(y, 0.3, 0.35)


def r_telegraph(i, a):
    n = ns(0.20)
    tt = t_(n)
    f = 2500 * (1 + 0.015 * np.sin(2 * np.pi * 22 * tt))
    e = np.minimum(1.0, 0.12 + 0.88 * (tt / 0.15)) * np.minimum(1.0, (0.2 - tt) / 0.03)
    y = seno(f) * e + 0.25 * seno(f * 0.5) * e
    return y


def r_boss_hit(i, a):
    d = rng.uniform(0.30, 0.36)
    n = ns(d)
    y = golpe_pesado(d, var(i) * 0.7, 0.7, 1.0, 1.4)
    tt = t_(n)
    y += 0.25 * (np.sin(2 * np.pi * 1300 * tt) + 0.5 * np.sin(2 * np.pi * 2050 * tt)) * env_exp(n, 0.12)
    return satura(y, 1.5)


def r_boss_phase(i, a):
    d = 1.1
    n = ns(d)
    tt = t_(n)
    rug = pb(marron(n), 100, 600) * (0.6 + 0.4 * np.sin(2 * np.pi * 30 * tt))
    rug = normaliza(rug) * np.minimum(1, 0.2 + tt / 0.25) * np.exp(-np.maximum(tt - 0.5, 0) / 0.25)
    rug = normaliza(rug + 0.7 * satura(rug, 4.0))
    y = 0.6 * rug + 0.7 * pb(rug * blanco(n), 600, 2500)
    y += 0.9 * desplaza(golpe_pesado(0.35, 0.8), 0.05, n)
    ring = np.zeros(n)
    for p, amp in ((620, 1.0), (1050, 0.7), (1630, 0.5), (2380, 0.3)):
        ring += amp * np.sin(2 * np.pi * p * tt)
    y += 0.35 * ring * env_exp(n, 0.35, 0.002, 0.05)
    return satura(y, 1.5)


def r_pickup_health(i, a):
    n = ns(0.30)
    y = nota(n, 660, 0.08, "pulso") + nota(n, 990, 0.14, "pulso", 0.09)
    y += 0.5 * (nota(n, 660, 0.08, "seno") + nota(n, 990, 0.14, "seno", 0.09))
    return pbaj(y, 4000)


def r_pickup_energy(i, a):
    n = ns(0.30)
    tt = t_(n)
    f = 700 * (1 + 0.6 * tt / 0.30)
    y = np.sin(fase(f) + 3.0 * np.exp(-tt / 0.1) * np.sin(fase(f * 2.01)))
    y = y * env_exp(n, 0.12, 0.001) + 0.2 * crackle(n, 300, 1500, 6000, 0.15)
    return y


def combo_ping(semitonos):
    def r(i, a):
        n = ns(0.20)
        f = 880 * 2 ** (semitonos / 12)
        y = fm(f, f * 2.0, 2.0, n) * env_exp(n, 0.07, 0.001) + 0.5 * nota(n, f / 2, 0.06)
        return pbaj(y, 4000)
    return r


def r_prop_wood(i, a):
    n = ns(0.18)
    tt = t_(n)
    y = np.zeros(n)
    for p, amp in ((300, 0.6), (437, 1.0), (589, 0.7), (701, 0.5), (1130, 0.4)):
        y += amp * np.sin(2 * np.pi * p * var(i, 0.05) * tt + rng.uniform(0, 6.28)) * env_exp(n, 0.04)
    y += 0.7 * granos(n, 250, 800, 3000, (0.001, 0.004)) * env_exp(n, 0.06)
    y += 0.6 * chasquido(n, 1500, 5000)
    return satura(y, 1.5)


def r_prop_metal(i, a):
    n = ns(0.36)
    tt = t_(n)
    y = np.zeros(n)
    for p, amp in ((1013, 1.0), (1597, 0.8), (2290, 0.6), (3150, 0.4), (4420, 0.25)):
        y += amp * np.sin(2 * np.pi * p * var(i, 0.05) * tt + rng.uniform(0, 6.28)) * env_exp(n, 0.3 * rng.uniform(0.7, 1.0))
    y += 0.6 * chasquido(n, 1500, 7000)
    return satura(y, 1.4)


def r_rage_start(i, a):
    d = 0.75
    n = ns(d)
    tt = t_(n)
    u = np.linspace(0, 1, n)
    sw = pb(sierra(200 * 2 ** (u * 1.5)) + sierra(201.5 * 2 ** (u * 1.5)), 300, 3500)
    sw *= (0.15 + 0.85 * np.minimum(1, u / 0.6)) * np.exp(-np.maximum(tt - 0.5, 0) / 0.08)
    z = desplaza(zap(0.3, 0.6), 0.48, n)
    return satura(0.6 * sw + 0.9 * z, 1.5)


def r_transform(i, a):
    d = 0.8
    n = ns(d)
    tt = t_(n)
    y = 0.8 * desplaza(golpe_pesado(0.3, 1.1), 0.0, n)
    for semi in (0, 7, 12, 19):
        y += 0.25 * fm(880 * 2 ** (semi / 12), 1320, 1.5, n) * env_exp(n, 0.35, 0.002)
    y += 0.4 * crackle(n, 1200, 2000, 9000, 0.3)
    return reverb(satura(y, 1.5), 0.3, 0.35)


def r_ui_select_char(i, a):
    n = ns(0.30)
    tt = t_(n)
    y = 0.8 * desplaza(golpe_medio(0.15, 1.0), 0.0, n)
    for p, amp in ((1240, 1.0), (1870, 0.6), (2730, 0.4)):
        y += 0.3 * amp * np.sin(2 * np.pi * p * tt) * env_exp(n, 0.15, 0.001)
    return satura(y, 1.3)


def r_ui_deny(i, a):
    n = ns(0.26)
    y = nota(n, 330, 0.05, "pulso") + nota(n, 247, 0.07, "pulso", 0.11)
    return pb(y, 250, 3500)


# nombre base -> (receta, cantidad de variantes)
RECETAS = {
    "hit_light": (r_hit_light, 4),
    "hit_medium": (r_hit_medium, 4),
    "hit_heavy": (r_hit_heavy, 3),
    "kick": (r_kick, 3),
    "slash": (r_slash, 3),
    "energy": (r_energy, 3),
    "energy_impact": (r_energy_impact, 3),
    "energy_shot": (r_energy_shot, 2),
    "energy_charge": (r_energy_charge, 1),
    "block": (r_block, 3),
    "guard_break": (r_guard_break, 1),
    "player_hurt": (r_player_hurt, 3),
    "whoosh_light": (r_whoosh_light, 3),
    "whoosh_heavy": (r_whoosh_heavy, 3),
    "jump": (r_jump, 1),
    "land_light": (r_land_light, 1),
    "land_knockdown": (r_land_knockdown, 1),
    "step": (r_step, 4),
    "step_wet": (r_step_wet, 4),
    "ko_enemy": (r_ko_enemy, 2),
    "game_over": (r_game_over, 0),
    "stage_clear": (r_stage_clear, 0),
    "telegraph": (r_telegraph, 1),
    "boss_hit": (r_boss_hit, 2),
    "boss_phase": (r_boss_phase, 1),
    "pickup_health": (r_pickup_health, 0),
    "pickup_energy": (r_pickup_energy, 0),
    "combo_10": (combo_ping(0), 0),
    "combo_20": (combo_ping(1), 0),
    "combo_30": (combo_ping(2), 0),
    "prop_wood": (r_prop_wood, 2),
    "prop_metal": (r_prop_metal, 2),
    "rage_start": (r_rage_start, 0),
    "transform": (r_transform, 0),
    "ui_select_char": (r_ui_select_char, 0),
    "ui_deny": (r_ui_deny, 0),
}
# 0 variantes = archivo unico sin sufijo

NEUTRO = {"k": 1.0, "pitch": 1.0, "chas": 1.0, "sub": 1.0, "crackle": 0.0, "slash": False}

ARQUETIPOS = {
    # agil: +12 % de tono y mas chasquido
    "agil": {"k": 1.12, "pitch": 1.12, "chas": 1.5, "sub": 1.0, "crackle": 0.0, "slash": False},
    # cuchillas: los golpes son cortes (slash)
    "cuchillas": {"k": 1.0, "pitch": 1.0, "chas": 1.0, "sub": 1.0, "crackle": 0.0, "slash": True},
    # pesado: -15 % de tono y mas subgrave
    "pesado": {"k": 0.85, "pitch": 0.85, "chas": 1.0, "sub": 1.6, "crackle": 0.0, "slash": False},
    # electrico: capa de chisporroteo sobre los golpes
    "electrico": {"k": 1.0, "pitch": 1.0, "chas": 1.0, "sub": 1.0, "crackle": 0.5, "slash": False},
}
NOMBRES_ARQUETIPO = ["hit_light", "hit_medium", "hit_heavy", "kick", "whoosh_light", "whoosh_heavy"]


def receta_arquetipo(base, nombre_arq):
    a = ARQUETIPOS[nombre_arq]
    receta, nv = RECETAS[base]
    if a["slash"] and base.startswith("hit_"):
        largo = {"hit_light": (0.12, 0.6), "hit_medium": (0.18, 0.8), "hit_heavy": (0.28, 1.0)}[base]

        def r(i, aa, _base=base, _l=largo):
            n = ns(_l[0])
            y = corte(_l[0], var(i, 0.05) * (1.1 if _base == "hit_light" else 1.0))
            if _base == "hit_heavy":
                y = y + 0.8 * golpe_medio(_l[0], 0.9, 0.8)[:n]
            elif _base == "hit_medium":
                y = y + 0.4 * golpe_ligero(_l[0], 1.0)[:n]
            return y
        return r, nv
    if a["crackle"] > 0 and (base.startswith("hit_") or base == "kick"):
        def r(i, aa, _rec=receta):
            y = _rec(i, aa)
            return y + aa["crackle"] * crackle(len(y), 1500, 2000, 9000, len(y) / SR / 2.5) + \
                0.3 * aa["crackle"] * pb(sierra(barrido(len(y), 1100, 250, 0.08)), 400, 4000) * env_exp(len(y), 0.05)
        return r, nv
    return receta, nv


# ---------------------------------------------------------------------------
# pipeline comun
# ---------------------------------------------------------------------------

def st50(x):
    w = ns(0.05)
    if len(x) <= w:
        return np.sqrt(np.mean(x * x))
    c = np.concatenate([[0.0], np.cumsum(x * x)])
    idx = np.arange(0, len(x) - w + 1, ns(0.005))
    return np.sqrt(np.max((c[idx + w] - c[idx]) / w))


def fraccion_medios(x):
    """Fraccion de energia entre 500 Hz y 3 kHz (misma medida que analiza_banco)."""
    X = np.abs(np.fft.rfft(x)) ** 2
    f = np.fft.rfftfreq(len(x), 1.0 / SR)
    return float(np.sum(X[(f >= 500) & (f < 3000)]) / (np.sum(X) + 1e-20))


def realza_medios(x, objetivo=MEDIOS_OBJETIVO, drive=4.0):
    """
    "Realzador de graves" psicoacustico (como un bass enhancer): satura la
    senal con tanh para que el grave genere armonicos y suma SOLO la parte
    500 Hz-3 kHz de esos armonicos, en paralelo, hasta llegar al objetivo de
    energia en medios. El grave original queda intacto (en un altavoz grande
    se sigue oyendo), y en el movil el oido "reconstruye" el fundamental a
    partir de los armonicos. Si ya cumple, no toca nada. Devuelve (x, ganancia).
    """
    if fraccion_medios(x) >= objetivo:
        return x, 0.0
    pk = np.max(np.abs(x)) + 1e-12
    h = pb(np.tanh(drive * x / pk) * pk, 500, 3000, 2)
    h = pb(h, 500, 3000, 2)
    lo, hi = 0.0, 0.25
    while fraccion_medios(x + hi * h) < objetivo and hi < 64:
        hi *= 2
    for _ in range(24):
        m = 0.5 * (lo + hi)
        if fraccion_medios(x + m * h) < objetivo:
            lo = m
        else:
            hi = m
    return x + hi * h, hi


def recorta_cola(x, umbral_db=-45.0):
    """Quita la cola por debajo de umbral_db relativo al pico (ahorra bytes)."""
    u = 10 ** (umbral_db / 20.0) * np.max(np.abs(x))
    idx = np.nonzero(np.abs(x) >= u)[0]
    fin = min(len(x), (idx[-1] if len(idx) else len(x)) + ns(0.005))
    return x[:fin]


def termina(x, base):
    x = np.asarray(x, dtype=np.float64)
    x = signal.sosfilt(_sos("high", 20.0, 2), x)
    x, _g = realza_medios(x)
    x = recorta_cola(x)
    # el paso alto no debe dejar silencio inicial: recorta lo que este bajo -50 dB
    umbral = 10 ** (-50 / 20.0) * np.max(np.abs(x))
    primero = int(np.argmax(np.abs(x) >= umbral))
    x = x[primero:]
    obj = 10 ** (ST50.get(base, ST50_GOLPE) / 20.0)
    g = obj / (st50(x) + 1e-12)
    pico_max = 10 ** (PICO_MAX_DB / 20.0)
    g = min(g, pico_max / (np.max(np.abs(x)) + 1e-12))
    x = x * g
    k = ns(0.002)
    x[-k:] *= np.linspace(1.0, 0.0, k)
    return x


def escribe(ruta, x):
    os.makedirs(os.path.dirname(ruta), exist_ok=True)
    q = np.clip(np.round(x * 32767.0), -32767, 32767).astype("<i2")
    with wave.open(ruta, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(q.tobytes())


def genera(salida, rel, base, receta, i, arq):
    global rng
    semilla = (SEMILLA_BASE ^ zlib.crc32(rel.encode("utf-8"))) & 0xFFFFFFFF
    rng = np.random.default_rng(semilla)
    x = termina(receta(i, arq), base)
    escribe(os.path.join(salida, rel), x)
    return rel


def nombres(base, nv):
    return [base] if nv == 0 else ["%s_%d" % (base, j) for j in range(1, nv + 1)]


def main(argv):
    raiz = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    salida = argv[0] if argv else os.path.join(raiz, "assets", "audio", "sfx")
    hechos = []
    for base, (receta, nv) in RECETAS.items():
        for j, nom in enumerate(nombres(base, nv)):
            hechos.append(genera(salida, "combat/%s.wav" % nom, base, receta, j, NEUTRO))
    for arq in ARQUETIPOS:
        for base in NOMBRES_ARQUETIPO:
            receta, nv = receta_arquetipo(base, arq)
            for j, nom in enumerate(nombres(base, nv)):
                hechos.append(genera(salida, "arquetipos/%s/%s.wav" % (arq, nom), base, receta, j, ARQUETIPOS[arq]))
    print("escritos %d archivos en %s" % (len(hechos), salida))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
