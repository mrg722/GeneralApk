#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generador de la MUSICA ORIGINAL de District Fury (BRIEF F1.12).

QUE ES (y que NO es)
    Toda la musica del juego principal se COMPONE y SINTETIZA aqui, por
    algoritmo, con una semilla fija. Las progresiones de acordes, las lineas
    de bajo, los arpegios y las melodias se eligen con caminatas aleatorias
    (cadenas de Markov sobre grados de la escala, motivos ritmicos sorteados)
    a partir de la semilla de cada tema. NO se transcribe ni se imita ninguna
    melodia existente, NO se usan muestras de audio externas ni IA de audio:
    cada sonido sale de osciladores (sierra, cuadrada, seno, FM), ruido y
    filtros escritos en este archivo. La percusion tipo taiko reutiliza la
    receta de capa_percusion() de tools/audio/genera_intro_ronin.py.

TECNICA
    - Sintesis sustractiva (sierras/cuadradas con polyBLEP + pasa-bajos cuyo
      corte se barre mezclando dos filtros fijos), FM para metales/campanas,
      ruido filtrado para baterias, distorsion tanh para el bajo de combate.
    - Cada tema genera UNA composicion (eventos de notas) y la renderiza en
      pistas ("stems"); cada capa (calma/combate o A/B) es una mezcla distinta
      de las MISMAS pistas: mismo tempo, misma tonalidad, misma longitud, de
      modo que el juego las reproduce a la vez y cruza sus volumenes.
    - Los bucles se renderizan de forma CIRCULAR: la cola de las notas y de
      los efectos (reverb por convolucion circular, eco por rotacion) que
      pasa del ultimo compas se suma al principio. El punto de union cae en
      el limite de compas y se suaviza con un crossfade de 10 ms hacia una
      copia filtrada (se mide el salto de muestra en la union).
    - Normalizacion: sonoridad integrada aprox -16 LUFS (ponderacion K de
      BS.1770 con compuerta) y limitador de pico a -1.5 dBFS antes de
      codificar (el OGG decodificado queda <= -1 dBFS; se verifica).
    - WAV temporal en un directorio temporal -> ffmpeg libvorbis -q:a 3 en
      modo bitexact (md5 identicos entre ejecuciones).

USO
    python3 -I tools/audio/genera_musica.py            (escribe assets/audio/music/*.ogg y assets/audio/ambient/*.ogg)
    python3 -I tools/audio/genera_musica.py --solo menu,boss
    python3 -I tools/audio/genera_musica.py --informe  (no codifica: solo analiza los OGG existentes)
"""

import argparse
import hashlib
import os
import subprocess
import sys
import tempfile

import numpy as np
from scipy import signal
from scipy.io import wavfile
from scipy.ndimage import minimum_filter1d, uniform_filter1d

SR = 44100
SEMILLA_GLOBAL = 20261103
LUFS_OBJETIVO = -16.0
PICO_RENDER_DB = -1.5        # el OGG decodificado debe quedar <= -1 dBFS
CALIDAD_OGG = "3"
XFADE_UNION_S = 0.010        # crossfade de la union del bucle

RAIZ = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
DIR_MUSICA = os.path.join(RAIZ, "assets", "audio", "music")
DIR_AMBIENTE = os.path.join(RAIZ, "assets", "audio", "ambient")


# ---------------------------------------------------------------------------
# UTILIDADES
# ---------------------------------------------------------------------------

def semilla_de(nombre):
    h = hashlib.md5(nombre.encode("utf-8")).digest()
    return (SEMILLA_GLOBAL ^ int.from_bytes(h[:4], "little")) & 0x7FFFFFFF


def ns(seg):
    return max(1, int(round(seg * SR)))


def midi_hz(m):
    return 440.0 * 2.0 ** ((np.asarray(m, dtype=np.float64) - 69.0) / 12.0)


def sos_lp(corte, orden=2):
    return signal.butter(orden, min(max(corte, 20.0), SR * 0.45) / (SR / 2), btype="low", output="sos")


def sos_hp(corte, orden=2):
    return signal.butter(orden, min(max(corte, 20.0), SR * 0.45) / (SR / 2), btype="high", output="sos")


def sos_bp(bajo, alto, orden=2):
    nyq = SR / 2
    bajo = max(20.0, bajo)
    alto = min(max(alto, bajo * 1.05), nyq * 0.95)
    return signal.butter(orden, [bajo / nyq, alto / nyq], btype="band", output="sos")


def lp(x, c, o=2):
    return signal.sosfilt(sos_lp(c, o), x)


def hp(x, c, o=2):
    return signal.sosfilt(sos_hp(c, o), x)


def bp(x, a, b, o=2):
    return signal.sosfilt(sos_bp(a, b, o), x)


def circ(fn, x, pre_s=1.0):
    """Aplica un filtro IIR de forma circular (bucle): antepone el final de la
    senal para que el estado del filtro en la muestra 0 venga de la vuelta
    anterior, y descarta ese prefijo."""
    k = min(ns(pre_s), x.shape[-1])
    y = fn(np.concatenate([x[..., -k:], x], axis=-1))
    return y[..., k:]


def frec_bucle(f, L):
    """Ajusta f para que entren ciclos enteros en el bucle (sin salto de fase)."""
    ciclos = max(1, round(f * L / SR))
    return ciclos * SR / L


def env_ar(n, ataque, caida, curva=1.0):
    t = np.arange(n) / SR
    sub = np.clip(t / max(ataque, 1e-4), 0, 1)
    return sub * np.exp(-curva * t / max(caida, 1e-4))


def env_gate(n, ataque, sostener_n, release):
    """Ataque lineal, sostiene hasta sostener_n muestras, release lineal."""
    e = np.ones(n)
    na = min(n, ns(ataque))
    e[:na] = np.linspace(0, 1, na, endpoint=False)
    nr = max(1, ns(release))
    fin = min(n, sostener_n)
    if fin < n:
        largo = min(nr, n - fin)
        e[fin:fin + largo] *= np.linspace(1, 0, largo)
        e[fin + largo:] = 0
    return e


def blep_saw_fase(fase, dt):
    y = 2.0 * fase - 1.0
    m = fase < dt
    t = fase[m] / dt[m]
    y[m] -= t + t - t * t - 1.0
    m = fase > 1.0 - dt
    t = (fase[m] - 1.0) / dt[m]
    y[m] -= t * t + t + t + 1.0
    return y


def saw(frec, fase0=0.0):
    frec = np.broadcast_to(np.asarray(frec, dtype=np.float64), np.shape(frec))
    dt = np.clip(frec / SR, 1e-6, 0.45)
    fase = np.mod(fase0 + np.cumsum(dt) - dt[0], 1.0)
    return blep_saw_fase(fase, dt)


def square(frec, fase0=0.0, ancho=0.5):
    frec = np.asarray(frec, dtype=np.float64)
    dt = np.clip(frec / SR, 1e-6, 0.45)
    fase = np.mod(fase0 + np.cumsum(dt) - dt[0], 1.0)
    return 0.5 * (blep_saw_fase(fase, dt) - blep_saw_fase(np.mod(fase + ancho, 1.0), dt))


def seno_fase(frec, fase0=0.0):
    return 2 * np.pi * (fase0 + np.cumsum(np.asarray(frec, dtype=np.float64)) / SR)


def paneo(pan):
    """pan -1..1 -> ganancias (L, R) de potencia constante."""
    a = (pan + 1) * np.pi / 4
    return np.cos(a), np.sin(a)


# ---------------------------------------------------------------------------
# BUS DE MEZCLA CIRCULAR (bucles) O LINEAL (stingers)
# ---------------------------------------------------------------------------

class Bus:
    def __init__(self, n, circular=True):
        self.n = n
        self.circular = circular
        self.x = np.zeros((2, n))

    def suma(self, inicio, sig, pan=0.0, gan=1.0):
        """Suma una senal mono (o (2,k) estereo) a partir de la muestra inicio."""
        if sig.ndim == 1:
            gl, gr = paneo(pan)
            st = np.vstack([sig * gl, sig * gr]) * gan
        else:
            st = sig * gan
        k = st.shape[1]
        inicio = int(inicio)
        if self.circular:
            inicio %= self.n
            pos = 0
            while pos < k:
                trozo = min(k - pos, self.n - inicio)
                self.x[:, inicio:inicio + trozo] += st[:, pos:pos + trozo]
                pos += trozo
                inicio = 0
        else:
            fin = min(self.n, inicio + k)
            if fin > inicio:
                self.x[:, inicio:fin] += st[:, :fin - inicio]


def conv_circular(x, ir):
    """Convolucion circular de x (2,n) mono-sumado con ir (2,m): reverb de bucle."""
    n = x.shape[1]
    mono = x.mean(axis=0)
    X = np.fft.rfft(mono, n)
    out = np.zeros_like(x)
    for c in range(2):
        h = ir[c]
        if len(h) > n:
            h = h[:n]
        H = np.fft.rfft(h, n)
        out[c] = np.fft.irfft(X * H, n)
    return out


def conv_lineal(x, ir):
    mono = x.mean(axis=0)
    out = np.zeros_like(x)
    for c in range(2):
        out[c] = signal.oaconvolve(mono, ir[c])[:x.shape[1]]
    return out


def respuesta_reverb(seg, oscuro, rng, predelay=0.012):
    n = ns(seg)
    t = np.arange(n) / SR
    ir = np.zeros((2, n + ns(predelay)))
    for c in range(2):
        r = rng.standard_normal(n) * np.exp(-6.9 * t / seg)
        r = lp(r, oscuro, 1)
        r = hp(r, 180, 1)
        ir[c, ns(predelay):] = r
    ir /= np.sqrt(np.sum(ir ** 2) / 2 + 1e-12)
    return ir * 0.5


def eco(x, retardo_s, realim, n_ecos, circular, cruzado=True, oscuro=3500.0):
    """Eco estereo (ping-pong si cruzado). Circular -> np.roll (bucle exacto)."""
    d = ns(retardo_s)
    out = np.zeros_like(x)
    cur = x.copy()
    g = 1.0
    for i in range(1, n_ecos + 1):
        g *= realim
        if oscuro:
            cur = circ(lambda v: lp(v, oscuro, 1), cur) if circular else lp(cur, oscuro, 1)
        if cruzado:
            cur = cur[::-1]
        if circular:
            out += np.roll(cur, d * i, axis=1) * g
        else:
            sh = np.zeros_like(cur)
            if d * i < cur.shape[1]:
                sh[:, d * i:] = cur[:, :cur.shape[1] - d * i]
            out += sh * g
    return out


# ---------------------------------------------------------------------------
# INSTRUMENTOS (todos sintetizados)
# ---------------------------------------------------------------------------

_cache = {}


def cacheado(clave, fn):
    if clave not in _cache:
        _cache[clave] = fn()
    return _cache[clave]


def bajo_sierra(midi, dur, brillo=1.0, drive=1.0, caida=0.12):
    def fn():
        n = ns(dur + 0.04)
        f = float(midi_hz(midi))
        x = 0.75 * saw(np.full(n, f)) + 0.45 * np.sin(seno_fase(np.full(n, f)))
        x += 0.25 * saw(np.full(n, f * 1.004), 0.3)
        e = np.exp(-np.arange(n) / SR / caida)
        y = lp(x, 260 * brillo + 2 * f, 2) * (1 - e) + lp(x, 1800 * brillo + 2 * f, 2) * e
        if drive > 1.0:
            y = np.tanh(drive * y) / np.tanh(drive)
            y = lp(y, 3200, 2)
        return y * env_gate(n, 0.003, ns(dur), 0.03)
    return cacheado(("bajo", midi, round(dur, 4), brillo, drive, caida), fn)


def pluck(midi, dur, brillo=1.0, tipo="saw"):
    def fn():
        n = ns(dur + 0.25)
        f = float(midi_hz(midi))
        if tipo == "fm":   # campana FM (puerto / guarida)
            t = np.arange(n) / SR
            ie = 2.2 * np.exp(-t / 0.18)
            x = np.sin(2 * np.pi * f * t + ie * np.sin(2 * np.pi * f * 3.5 * t))
            return x * env_ar(n, 0.002, 0.35, 1.0)
        x = saw(np.full(n, f)) * 0.6 + square(np.full(n, f * 1.003)) * 0.4
        e = np.exp(-np.arange(n) / SR / 0.09)
        y = lp(x, 500 * brillo, 2) * (1 - e) + lp(x, 4200 * brillo, 2) * e
        return y * env_ar(n, 0.002, 0.22, 1.0) * env_gate(n, 0.0, ns(dur + 0.05), 0.2)
    return cacheado(("pluck", midi, round(dur, 4), brillo, tipo), fn)


def bombo(suave=False):
    def fn():
        n = ns(0.45)
        t = np.arange(n) / SR
        f = 52 + (150 if not suave else 105) * np.exp(-t / 0.035)
        x = np.sin(seno_fase(f)) * env_ar(n, 0.0015, 0.32 if not suave else 0.26, 1.6)
        rng = np.random.default_rng(11 if suave else 12)
        click = hp(rng.uniform(-1, 1, n), 2500, 2) * env_ar(n, 0.0003, 0.006, 2.0)
        return np.tanh(1.4 * x) * 0.95 + click * (0.10 if suave else 0.22)
    return cacheado(("bombo", suave), fn)


def caja(variante=0):
    def fn():
        n = ns(0.35)
        t = np.arange(n) / SR
        rng = np.random.default_rng(30 + variante)
        r = bp(rng.uniform(-1, 1, n), 1200, 7500, 2) * env_ar(n, 0.001, 0.16, 1.6)
        tono = np.sin(seno_fase(190 + 60 * np.exp(-t / 0.02))) * env_ar(n, 0.001, 0.06, 1.5)
        return r * 0.8 + tono * 0.55
    return cacheado(("caja", variante), fn)


def charles(abierto=False, variante=0):
    def fn():
        n = ns(0.25 if abierto else 0.07)
        rng = np.random.default_rng(50 + variante + (7 if abierto else 0))
        r = hp(rng.uniform(-1, 1, n), 7000, 2)
        # metalico: suma de cuadradas inarmonicas filtradas
        t = np.arange(n)
        met = sum(np.sign(np.sin(2 * np.pi * fr * t / SR)) for fr in (3150, 4420, 5980, 7340))
        met = hp(met, 6000, 2) * 0.12
        return (r + met) * env_ar(n, 0.0005, 0.12 if abierto else 0.022, 1.5)
    return cacheado(("hat", abierto, variante), fn)


def taiko(f_ini=95, f_fin=55, caida=0.34, variante=0):
    """Receta de capa_percusion() de genera_intro_ronin.py: cuerpo con tono que
    cae + transitorio seco de ruido."""
    def fn():
        n = ns(caida * 3)
        k = np.log(f_fin / f_ini)
        frec = f_ini * np.exp(k * np.linspace(0, 1, n))
        fase = 2 * np.pi * np.cumsum(frec) / SR
        cuerpo = (np.sin(fase) + 0.25 * np.sin(2 * fase)) * env_ar(n, 0.001, caida, 1.4)
        rng = np.random.default_rng(70 + variante)
        click = bp(rng.uniform(-1, 1, n), 1200, 7000, 2) * env_ar(n, 0.0005, 0.02, 3.0) * 0.5
        piel = bp(rng.uniform(-1, 1, n), 150, 900, 2) * env_ar(n, 0.001, caida * 0.4, 1.5) * 0.8
        return (cuerpo + click + piel) * 0.9
    return cacheado(("taiko", f_ini, f_fin, caida, variante), fn)


def metal(f, variante=0):
    """Golpe metalico industrial: FM inarmonica + ruido de banda."""
    def fn():
        n = ns(0.6)
        t = np.arange(n) / SR
        ie = 3.0 * np.exp(-t / 0.08)
        x = np.sin(2 * np.pi * f * t + ie * np.sin(2 * np.pi * f * 1.414 * t))
        x += 0.5 * np.sin(2 * np.pi * f * 2.76 * t + ie * 0.6 * np.sin(2 * np.pi * f * 0.73 * t))
        rng = np.random.default_rng(90 + variante)
        r = bp(rng.uniform(-1, 1, n), f * 2, f * 6, 2) * env_ar(n, 0.0005, 0.04, 1.5)
        return (x * env_ar(n, 0.0008, 0.22, 1.2) + r * 0.6) * 0.6
    return cacheado(("metal", round(f, 2), variante), fn)


def bronces(midis, dur, brillo=1.0):
    """Stab de 'bronces' de sierra: varias sierras desafinadas por nota con
    barrido de filtro rapido."""
    def fn():
        n = ns(dur + 0.2)
        x = np.zeros(n)
        for m in midis:
            f = float(midi_hz(m))
            for det, fz in ((-0.07, 0.0), (0.0, 0.33), (0.08, 0.66)):
                x += saw(np.full(n, f * 2 ** (det / 12)), fz)
        x /= max(1, len(midis) * 3) ** 0.5
        t = np.arange(n) / SR
        e = np.clip(t / 0.025, 0, 1) * np.exp(-t / 0.18)
        y = lp(x, 400 * brillo, 2) * (1 - e) + lp(x, 3500 * brillo, 2) * e
        return y * env_gate(n, 0.006, ns(dur), 0.12)
    return cacheado(("bronces", tuple(midis), round(dur, 4), brillo), fn)


# ---------------------------------------------------------------------------
# COMPOSICION ALGORITMICA
# ---------------------------------------------------------------------------

ESCALAS = {
    "menor": [0, 2, 3, 5, 7, 8, 10],
    "dorica": [0, 2, 3, 5, 7, 9, 10],
    "frigia": [0, 1, 3, 5, 7, 8, 10],
    "armonica": [0, 2, 3, 5, 7, 8, 11],
}

# Transiciones entre grados (0=i, 2=III, 3=iv, 4=v, 5=VI, 6=VII): pesos genericos.
MARKOV = {
    0: {5: 3, 3: 2, 6: 2, 2: 1, 4: 1},
    2: {5: 2, 6: 2, 3: 1, 0: 1},
    3: {0: 2, 4: 2, 6: 1, 5: 1},
    4: {0: 3, 5: 2},
    5: {6: 3, 3: 2, 2: 1, 4: 1},
    6: {0: 3, 2: 2, 5: 1},
    1: {4: 2, 0: 2},
}


class Tema:
    def __init__(self, nombre, bpm, raiz, escala, compases=32, acorde_compases=2):
        self.nombre = nombre
        self.bpm = bpm
        self.raiz = raiz             # midi de la tonica en registro de bajo (octava 2)
        self.escala = ESCALAS[escala]
        self.escala_nombre = escala
        self.compases = compases
        self.paso = 60.0 / bpm / 4.0      # semicorchea
        self.pasos = compases * 16
        self.rng = np.random.default_rng(semilla_de(nombre))
        self.acorde_compases = acorde_compases
        self.L = int(round(self.pasos * self.paso * SR))
        self.progresion = self._progresion()

    def m_paso(self, s):
        return int(round(s * self.paso * SR))

    def grado_midi(self, grado, base):
        o, g = divmod(grado, 7)
        return base + 12 * o + self.escala[g]

    def acorde(self, grado, base, notas=3):
        return [self.grado_midi(grado + 2 * i, base) for i in range(notas)]

    def _frase(self, empieza=0, termina=None, largo=4):
        seq = [empieza]
        while len(seq) < largo:
            opts = MARKOV[seq[-1]]
            if len(seq) == largo - 1 and termina is not None:
                cand = [g for g in termina if g != seq[-1]]
                seq.append(int(self.rng.choice(cand)))
                break
            k = list(opts.keys())
            p = np.array([opts[g] for g in k], dtype=float)
            seq.append(int(self.rng.choice(k, p=p / p.sum())))
        return seq

    def _progresion(self):
        """Lista de grados, uno por bloque de acorde_compases; forma A A B A'."""
        por_frase = 8 // self.acorde_compases
        a = self._frase(0, [4, 6, 3], por_frase)
        b = self._frase(int(self.rng.choice([5, 3, 2])), [4, 6], por_frase)
        a2 = a[:-1] + [int(self.rng.choice([4, 6]))]
        frases = [a, a2, b, a2]
        nfr = self.compases // 8
        out = []
        for i in range(nfr):
            out += frases[i % 4]
        return out

    def grado_en(self, paso):
        bloque = paso // (16 * self.acorde_compases)
        return self.progresion[bloque % len(self.progresion)]

    # --- lineas ------------------------------------------------------------

    def patron_bajo(self, densidad):
        """Plantilla de 16 pasos: (activo, intervalo en semitonos, acento)."""
        pat = []
        for s in range(16):
            if s % 4 == 0:
                act = True
            elif densidad >= 1.0:
                act = True
            else:
                act = self.rng.random() < densidad
            iv = 0
            r = self.rng.random()
            if s % 4 != 0:
                iv = 12 if r < 0.25 else (7 if r < 0.38 else 0)
            pat.append((act, iv, 1.0 if s % 4 == 0 else 0.75))
        return pat

    def melodia(self, frase_inicio, rango_base, densidad=0.6):
        """Melodia de 8 compases: motivo de 2 compases sorteado, repetido sobre
        los acordes y con final variado. Devuelve [(paso, dur_pasos, midi)]."""
        durs = [2, 2, 2, 4, 4, 1, 1, 3, 6, 8]
        pesos = np.array([4, 4, 3, 3, 2, 1, 1, 2, 1, 1], dtype=float)
        if densidad < 0.5:
            pesos = np.array([1, 2, 1, 4, 4, 0.5, 0.5, 2, 3, 3], dtype=float)
        ritmo = []
        t = 0
        while t < 32:
            d = int(self.rng.choice(durs, p=pesos / pesos.sum()))
            d = min(d, 32 - t)
            silencio = self.rng.random() < 0.18 and t > 0
            ritmo.append((t, d, silencio))
            t += d
        contorno = [0]
        for _ in range(len(ritmo) - 1):
            salto = int(self.rng.choice([-2, -1, -1, 1, 1, 2, 3, -3, 0], p=None))
            contorno.append(contorno[-1] + salto)
        notas = []
        for rep in range(4):
            off = frase_inicio + rep * 32
            for (t, d, sil), c in zip(ritmo, contorno):
                if sil:
                    continue
                paso = off + t
                grado_ac = self.grado_en(paso)
                g = grado_ac + c + (1 if rep == 2 else 0)
                if t % 8 == 0:   # tiempos fuertes: nota del acorde
                    opciones = [grado_ac, grado_ac + 2, grado_ac + 4, grado_ac + 7]
                    g = min(opciones, key=lambda o: abs(o - g))
                if rep == 3 and t + d >= 32:
                    g = grado_ac + (0 if self.rng.random() < 0.5 else 2)
                    d = 32 - t
                while g < -2:
                    g += 7
                while g > 11:
                    g -= 7
                notas.append((paso, d, self.grado_midi(g, rango_base)))
            if rep == 3:
                pass
        return notas


# ---------------------------------------------------------------------------
# RENDER DE PISTAS
# ---------------------------------------------------------------------------

def nuevo_bus(tema, circular=True, extra=0):
    return Bus(tema.L + extra, circular)


def pista_pad(tema, circular=True, nivel=0.3, voces=3, oscuro=(320, 2600), periodo_compases=8, base_oct=12):
    b = nuevo_bus(tema, circular)
    bloque = 16 * tema.acorde_compases
    nbl = tema.pasos // bloque
    seg = tema.m_paso(bloque)
    for i in range(nbl):
        g = tema.progresion[i]
        notas = tema.acorde(g, tema.raiz + base_oct, 4 if i % 2 else 3)
        n = seg + ns(0.6)
        for j, m in enumerate(notas):
            f = float(midi_hz(m))
            for v in range(voces):
                det = (v - (voces - 1) / 2) * 0.11
                x = saw(np.full(n, f * 2 ** (det / 12)), (0.17 * v + 0.31 * j) % 1)
                env = env_gate(n, 0.25, seg, 0.5)
                pan = ((v / max(1, voces - 1)) * 2 - 1) * 0.7 if voces > 1 else 0
                b.suma(tema.m_paso(i * bloque), x * env * nivel / np.sqrt(len(notas) * voces), pan)
    # barrido lento de filtro (LFO con periodo divisor del bucle)
    t = np.arange(b.n) / SR
    per = tema.m_paso(16 * periodo_compases) / SR
    mezcla = 0.5 - 0.5 * np.cos(2 * np.pi * t / per)
    for c in range(2):
        a = circ(lambda v: lp(v, oscuro[0], 2), b.x[c])
        z = circ(lambda v: lp(v, oscuro[1], 2), b.x[c])
        b.x[c] = a * (1 - mezcla) + z * mezcla
    return b.x


def pista_bajo(tema, densidad, brillo, drive, circular=True, nivel=0.5, cromatico=False, octava=0):
    b = nuevo_bus(tema, circular)
    pat = tema.patron_bajo(densidad)
    rng = np.random.default_rng(semilla_de(tema.nombre + "bajo"))
    croma = [0, 0, 1, 0, -1, 0, 1, 2, 0, 0, 1, 0, 3, 2, 1, 0]   # base del ostinato
    if cromatico:
        rng.shuffle(croma[4:12])
    dur = tema.paso * 0.82
    for s in range(tema.pasos):
        act, iv, ac = pat[s % 16]
        if not act:
            continue
        g = tema.grado_en(s)
        m = tema.grado_midi(g, tema.raiz + octava)
        while m > tema.raiz + 12 + octava:
            m -= 12
        if cromatico:
            m = tema.raiz + octava + croma[s % 16] + (iv if iv == 12 else 0)
        else:
            m += iv
        if s % 64 >= 60 and rng.random() < 0.5:   # pequeno relleno al final de cada 4 compases
            m += int(rng.choice([7, 10, 12, 3]))
        x = bajo_sierra(m, dur, brillo, drive)
        b.suma(tema.m_paso(s), x * nivel * ac, 0.0)
    return b.x


def pista_arpegio(tema, circular=True, nivel=0.22, tipo="saw", brillo=1.0, cada=1, octava=24, largo_patron=8):
    b = nuevo_bus(tema, circular)
    rng = np.random.default_rng(semilla_de(tema.nombre + "arp"))
    orden = list(rng.permutation(largo_patron) % 6)
    for s in range(0, tema.pasos, cada):
        g = tema.grado_en(s)
        tonos = tema.acorde(g, tema.raiz + octava, 3) + tema.acorde(g, tema.raiz + octava + 12, 3)
        m = tonos[orden[(s // cada) % len(orden)]]
        acento = 1.0 if s % 4 == 0 else 0.7
        x = pluck(m, tema.paso * cada * 0.9, brillo, tipo)
        b.suma(tema.m_paso(s), x * nivel * acento, 0.25 if (s // cada) % 2 else -0.25)
    b.x += eco(b.x, tema.paso * 3, 0.38, 4, circular) * 0.6
    return b.x


def pista_lead(tema, frases, circular=True, nivel=0.24, octava=24, densidad=0.6, portamento=0.045, ancho=0.5):
    """Lead de cuadrada con portamento: un unico oscilador con curva de tono."""
    n = tema.L
    tono = np.full(n, np.nan)
    gate = np.zeros(n)
    for fr in frases:
        for (paso, d, m) in tema.melodia(fr * 128, tema.raiz + octava, densidad):
            a = tema.m_paso(paso)
            z = tema.m_paso(paso + d) - ns(0.02)
            a %= n
            z = a + max(ns(0.03), z - tema.m_paso(paso))
            idx = np.arange(a, z) % n
            tono[idx] = m
            gate[idx] = 1.0
    # mantener el ultimo tono en los silencios (para que el glide salga de ahi)
    valid = ~np.isnan(tono)
    if not valid.any():
        return np.zeros((2, n))
    primero = np.argmax(valid)
    idx = np.where(valid, np.arange(n), 0)
    np.maximum.accumulate(idx, out=idx)
    tono = np.where(valid, tono, tono[idx])
    tono[:primero] = tono[np.where(valid)[0][-1]]
    a_gl = 1 - np.exp(-1 / (portamento * SR))
    # glide en dominio logaritmico, circular: dos pasadas para cerrar el bucle
    log = np.concatenate([tono[-ns(1.0):], tono]) if circular else tono
    sm = signal.lfilter([a_gl], [1, -(1 - a_gl)], log, zi=[log[0] * (1 - a_gl)])[0]
    if circular:
        sm = sm[ns(1.0):]
    f = midi_hz(sm)
    vib = 1 + 0.004 * np.sin(2 * np.pi * 5.2 * np.arange(n) / SR)
    x = square(f * vib, 0.0, ancho) * 0.8 + saw(f * 1.002) * 0.25
    gl = np.concatenate([gate[-ns(1.0):], gate]) if circular else gate
    a_g = 1 - np.exp(-1 / (0.008 * SR))
    env = signal.lfilter([a_g], [1, -(1 - a_g)], gl)
    if circular:
        env = env[ns(1.0):]
    x = circ(lambda v: lp(v, 3200, 2), x * env) * nivel
    st = np.vstack([x, x])
    st += eco(st, tema.paso * 6, 0.35, 3, circular, oscuro=2500) * 0.5
    if not circular:
        return st
    return st


def pista_bateria(tema, patron, circular=True, suave=False, nivel=1.0):
    """patron: dict con listas de pasos (0..15) por pieza y opciones."""
    b = nuevo_bus(tema, circular)
    rng = np.random.default_rng(semilla_de(tema.nombre + "bat" + str(suave)))
    for s in range(tema.pasos):
        st = s % 16
        compas = s // 16
        if st in patron.get("bombo", []):
            b.suma(tema.m_paso(s), bombo(suave), 0.0, 0.6 * nivel)
        if st in patron.get("caja", []):
            b.suma(tema.m_paso(s), caja(compas % 3), 0.05, 0.42 * nivel)
        if st in patron.get("fantasma", []) and rng.random() < 0.5:
            b.suma(tema.m_paso(s), caja(1), 0.05, 0.10 * nivel)
        if patron.get("charles"):
            cada = patron["charles"]
            if st % cada == 0:
                ab = (st in patron.get("abierto", []))
                ac = 1.0 if st % 4 == 2 else 0.6
                b.suma(tema.m_paso(s), charles(ab, s % 4), 0.3, 0.22 * ac * nivel * (1.2 if ab else 1))
        if st in patron.get("taiko", []):
            b.suma(tema.m_paso(s), taiko(variante=s % 3), -0.2 if st % 8 else 0.15, 0.55 * nivel)
        if st in patron.get("taiko_agudo", []):
            b.suma(tema.m_paso(s), taiko(180, 110, 0.18, 1), 0.35, 0.45 * nivel)
        if st in patron.get("metal", []):
            f = [310.0, 415.0, 520.0][compas % 3]
            b.suma(tema.m_paso(s), metal(f, compas % 2), -0.45, 0.3 * nivel)
        # redoble de caja en el ultimo compas de cada 8 (cierra la frase)
        if patron.get("redoble") and compas % 8 == 7 and st >= 12:
            b.suma(tema.m_paso(s), caja(2), -0.1, 0.18 * nivel * (st - 10) / 5)
    return b.x


def pista_bronces(tema, circular=True, nivel=0.3, pasos_stab=(0, 6, 10)):
    b = nuevo_bus(tema, circular)
    for s in range(tema.pasos):
        if s % 16 not in pasos_stab:
            continue
        if (s // 16) % 2 == 1 and s % 16 == 10:
            continue
        g = tema.grado_en(s)
        notas = tema.acorde(g, tema.raiz + 24, 3)
        notas = [notas[0] - 12] + notas
        x = bronces(notas, tema.paso * (2 if s % 16 else 3), 1.0)
        b.suma(tema.m_paso(s), x * nivel, 0.0)
    return b.x


def pista_drone(tema, circular=True, nivel=0.25, maquina=True):
    n = tema.L
    t = np.arange(n) / SR
    f0 = float(midi_hz(tema.raiz))
    fb = lambda f: frec_bucle(f, n)
    x = saw(np.full(n, fb(f0))) + 0.6 * saw(np.full(n, fb(f0 * 1.5 * 1.002)), 0.4) + 0.4 * saw(np.full(n, fb(f0 * 2.003)), 0.7)
    per = tema.m_paso(16 * 4) / SR
    m = 0.5 - 0.5 * np.cos(2 * np.pi * t / per)
    y = circ(lambda v: lp(v, 160, 2), x) * (1 - m) + circ(lambda v: lp(v, 700, 2), x) * m
    out = np.vstack([y, np.roll(y, ns(0.011))]) * nivel * 0.5
    if maquina:   # ruido de maquinaria con pulso de corcheas
        rng = np.random.default_rng(semilla_de(tema.nombre + "maq"))
        r = circ(lambda v: bp(v, 90, 420, 2), rng.uniform(-1, 1, n))
        ciclo = tema.m_paso(2)
        fase = (np.arange(n) % ciclo) / ciclo
        pulso = 0.35 + 0.65 * np.exp(-fase * 6)
        out += np.vstack([r * pulso, np.roll(r * pulso, ns(0.02))]) * nivel * 0.8
    if circular:
        # el drone es continuo: suaviza la union con la cola de la vuelta anterior
        pass
    return out


def pista_viento(tema, circular=True, nivel=0.2):
    n = tema.L
    rng = np.random.default_rng(semilla_de(tema.nombre + "viento"))
    t = np.arange(n) / SR
    per = tema.m_paso(16 * 8) / SR
    out = np.zeros((2, n))
    for c in range(2):
        r = rng.uniform(-1, 1, n)
        m = 0.5 - 0.5 * np.cos(2 * np.pi * (t / per + 0.25 * c))
        a = circ(lambda v: bp(v, 300, 900, 2), r)
        z = circ(lambda v: bp(v, 900, 2600, 2), r)
        out[c] = (a * (1 - m) + z * m) * (0.6 + 0.4 * m)
    return out * nivel


# ---------------------------------------------------------------------------
# MASTER: SONORIDAD, LIMITADOR, UNION DEL BUCLE
# ---------------------------------------------------------------------------

def _biquad_k(sr):
    # Ponderacion K (BS.1770) para sr arbitrario (formulas RBJ: high-shelf + high-pass)
    G, Q, fc = 3.99984385397, 0.7071752369554193, 1681.9744509555319
    A = 10 ** (G / 40)
    w0 = 2 * np.pi * fc / sr
    alfa = np.sin(w0) / (2 * Q)
    cw = np.cos(w0)
    b0 = A * ((A + 1) + (A - 1) * cw + 2 * np.sqrt(A) * alfa)
    b1 = -2 * A * ((A - 1) + (A + 1) * cw)
    b2 = A * ((A + 1) + (A - 1) * cw - 2 * np.sqrt(A) * alfa)
    a0 = (A + 1) - (A - 1) * cw + 2 * np.sqrt(A) * alfa
    a1 = 2 * ((A - 1) - (A + 1) * cw)
    a2 = (A + 1) - (A - 1) * cw - 2 * np.sqrt(A) * alfa
    shelf = np.array([b0, b1, b2, a0, a1, a2]) / a0
    Q2, fc2 = 0.5003270373253953, 38.13547087613982
    w0 = 2 * np.pi * fc2 / sr
    alfa = np.sin(w0) / (2 * Q2)
    cw = np.cos(w0)
    b = np.array([(1 + cw) / 2, -(1 + cw), (1 + cw) / 2])
    a = np.array([1 + alfa, -2 * cw, 1 - alfa])
    hpf = np.concatenate([b, a]) / a[0]
    return np.vstack([shelf, hpf])


def lufs(x):
    sos = _biquad_k(SR)
    y = signal.sosfilt(sos, x, axis=1)
    blk, hop = ns(0.4), ns(0.1)
    if y.shape[1] < blk:
        z = np.mean(y ** 2, axis=1).sum()
        return -0.691 + 10 * np.log10(z + 1e-12)
    cs = np.cumsum(np.concatenate([np.zeros((2, 1)), y ** 2], axis=1), axis=1)
    ini = np.arange(0, y.shape[1] - blk + 1, hop)
    z = ((cs[:, ini + blk] - cs[:, ini]) / blk).sum(axis=0)
    l = -0.691 + 10 * np.log10(z + 1e-12)
    z = z[l > -70]
    if len(z) == 0:
        return -70.0
    rel = -0.691 + 10 * np.log10(z.mean()) - 10
    z2 = z[(-0.691 + 10 * np.log10(z)) > rel]
    return -0.691 + 10 * np.log10(z2.mean())


def limitador(x, techo_db, circular):
    techo = 10 ** (techo_db / 20)
    pico = np.max(np.abs(x), axis=0)
    g = np.minimum(1.0, techo / np.maximum(pico, 1e-9))
    w = ns(0.006)
    modo = "wrap" if circular else "nearest"
    g = minimum_filter1d(g, size=2 * w + 1, mode=modo)
    g = uniform_filter1d(g, size=w, mode=modo)
    return x * g


def master(x, circular, objetivo=LUFS_OBJETIVO):
    x = hp(x, 28, 2) if not circular else x
    for _ in range(4):
        l = lufs(x)
        x = x * 10 ** ((objetivo - l) / 20)
        x = limitador(x, PICO_RENDER_DB, circular)
    x = np.clip(x, -10 ** (PICO_RENDER_DB / 20), 10 ** (PICO_RENDER_DB / 20))
    return x


def une_bucle(x):
    """Crossfade de 10 ms centrado en la union (fin -> principio) hacia una copia
    pasada por un pasa-bajos de fase cero: elimina cualquier salto de muestra."""
    n = x.shape[1]
    k = ns(XFADE_UNION_S) // 2
    ctx = ns(0.05)
    ventana = np.concatenate([x[:, n - ctx:], x[:, :ctx]], axis=1)
    suave = signal.sosfiltfilt(sos_lp(120, 2), ventana, axis=1)
    w = np.zeros(2 * ctx)
    centro = ctx
    rampa = 0.5 - 0.5 * np.cos(np.pi * np.arange(k) / k)
    w[centro - k:centro] = rampa
    w[centro:centro + k] = rampa[::-1]
    mezcla = ventana * (1 - w) + suave * w
    y = x.copy()
    y[:, n - ctx:] = mezcla[:, :ctx]
    y[:, :ctx] = mezcla[:, ctx:]
    return y


def salto_union_db(x):
    d = np.max(np.abs(x[:, 0] - x[:, -1]))
    return 20 * np.log10(d + 1e-12)


# ---------------------------------------------------------------------------
# TEMAS
# ---------------------------------------------------------------------------

PESO_PISTA = {"bajo": 0.6, "bajo_sat": 0.6, "pad": 1.2, "lead": 1.45, "arp": 1.35, "bronces": 1.25,
              "drone": 0.7, "taiko": 0.85}


def mezcla(pistas, ganancias):
    return sum(pistas[k] * g for k, g in ganancias.items() if k in pistas)


def render_tema(nombre, cfg):
    """Devuelve {archivo: senal estereo} para las capas del tema."""
    _cache.clear()
    tema = Tema(nombre, cfg["bpm"], cfg["raiz"], cfg["escala"], cfg.get("compases", 32))
    rng = np.random.default_rng(semilla_de(nombre + "rev"))
    ir = respuesta_reverb(cfg.get("reverb_s", 1.8), cfg.get("reverb_oscuro", 5000), rng)
    p = {}
    for clave, fn in cfg["pistas"].items():
        # Balance pensado para altavoces de telefono (cortan < 200 Hz): menos
        # graves, mas medios melodicos.
        p[clave] = fn(tema) * PESO_PISTA.get(clave, 1.0)
    capas = {}
    for archivo, cap in cfg["capas"].items():
        seco = mezcla(p, cap["seco"])
        envio = mezcla(p, cap.get("reverb", {}))
        x = seco + (conv_circular(envio, ir) if np.ndim(envio) else 0)
        x = circ(lambda v: hp(v, 40, 2), x)
        x = master(x, True, cap.get("lufs", LUFS_OBJETIVO))
        capas[archivo] = une_bucle(x)
    return tema, capas


def cfg_temas():
    T = {}
    # --- MENU: darksynth La menor 96 BPM ------------------------------------
    T["menu"] = dict(
        bpm=96, raiz=45, escala="menor", reverb_s=2.6, reverb_oscuro=4200,
        pistas={
            "pad": lambda t: pista_pad(t, nivel=0.5, voces=3, oscuro=(260, 2200)),
            "arp": lambda t: pista_arpegio(t, nivel=0.2, brillo=0.9),
            "bajo": lambda t: pista_bajo(t, 0.45, 0.8, 1.0, nivel=0.42),
            "bat": lambda t: pista_bateria(t, {"bombo": [0, 10], "caja": [8]}, suave=True),
            "lead": lambda t: pista_lead(t, [1, 3], nivel=0.17, densidad=0.35, portamento=0.06),
        },
        capas={"menu.ogg": dict(seco={"pad": 1, "arp": 1, "bajo": 1, "bat": 0.8, "lead": 1},
                                 reverb={"pad": 0.5, "arp": 0.4, "bat": 0.35, "lead": 0.5})},
    )
    # --- STAGE 1: calles con lluvia, Re menor 120 ---------------------------
    T["stage1"] = dict(
        bpm=120, raiz=38, escala="menor", reverb_s=2.0,
        pistas={
            "pad": lambda t: pista_pad(t, nivel=0.35, oscuro=(300, 1800)),
            "bajo": lambda t: pista_bajo(t, 1.0, 0.7, 1.0, nivel=0.45),
            "bajo_sat": lambda t: pista_bajo(t, 1.0, 1.3, 4.0, nivel=0.42),
            "arp": lambda t: pista_arpegio(t, nivel=0.14, cada=2),
            "lead": lambda t: pista_lead(t, [1, 3], nivel=0.22),
            "bat_c": lambda t: pista_bateria(t, {"bombo": [0, 8, 11], "caja": [4, 12]}, suave=True),
            "bat_k": lambda t: pista_bateria(t, {"bombo": [0, 4, 8, 12, 14], "caja": [4, 12], "fantasma": [7, 15],
                                                 "charles": 1, "abierto": [6, 14], "redoble": True}),
        },
        capas={
            "stage1_calma.ogg": dict(seco={"pad": 1, "bajo": 1, "arp": 1, "lead": 0.8, "bat_c": 0.8},
                                     reverb={"pad": 0.4, "bat_c": 0.6, "lead": 0.4, "arp": 0.3}, lufs=-16.5),
            "stage1_combate.ogg": dict(seco={"pad": 0.8, "bajo_sat": 1, "arp": 1.1, "lead": 1, "bat_k": 1},
                                       reverb={"pad": 0.3, "bat_k": 0.45, "lead": 0.4}, lufs=-15.5),
        },
    )
    # --- STAGE 2: industrial, Do menor 110, drone -----------------------------
    T["stage2"] = dict(
        bpm=110, raiz=36, escala="menor", reverb_s=2.4, reverb_oscuro=3500,
        pistas={
            "drone": lambda t: pista_drone(t, nivel=0.35),
            "pad": lambda t: pista_pad(t, nivel=0.25, voces=2, oscuro=(220, 1300)),
            "bajo": lambda t: pista_bajo(t, 0.6, 0.6, 1.5, nivel=0.5),
            "bajo_sat": lambda t: pista_bajo(t, 0.9, 1.1, 5.0, nivel=0.45),
            "lead": lambda t: pista_lead(t, [1, 3], nivel=0.18, densidad=0.4, ancho=0.3),
            "bat_c": lambda t: pista_bateria(t, {"bombo": [0, 6, 8], "caja": [12], "metal": [10]}, suave=True),
            "bat_k": lambda t: pista_bateria(t, {"bombo": [0, 3, 6, 8, 11], "caja": [4, 12], "metal": [2, 10, 15],
                                                 "charles": 2, "abierto": [14], "redoble": True}),
        },
        capas={
            "stage2_calma.ogg": dict(seco={"drone": 1, "pad": 1, "bajo": 1, "lead": 0.8, "bat_c": 0.9},
                                     reverb={"bat_c": 0.6, "pad": 0.4, "lead": 0.4}, lufs=-16.5),
            "stage2_combate.ogg": dict(seco={"drone": 0.8, "pad": 0.7, "bajo_sat": 1, "lead": 1, "bat_k": 1},
                                       reverb={"bat_k": 0.45, "lead": 0.4}, lufs=-15.5),
        },
    )
    # --- STAGE 3: puerto, Fa# dorico 126, campanas FM ------------------------
    T["stage3"] = dict(
        bpm=126, raiz=42, escala="dorica", reverb_s=2.8, reverb_oscuro=5500,
        pistas={
            "pad": lambda t: pista_pad(t, nivel=0.35, voces=3, oscuro=(380, 2600), periodo_compases=16, base_oct=0),
            "arp": lambda t: pista_arpegio(t, nivel=0.2, tipo="fm", cada=2, octava=24, largo_patron=6),
            "bajo": lambda t: pista_bajo(t, 0.5, 0.8, 1.0, nivel=0.45),
            "bajo_sat": lambda t: pista_bajo(t, 1.0, 1.2, 3.5, nivel=0.42),
            "lead": lambda t: pista_lead(t, [1, 3], nivel=0.2, octava=12, densidad=0.5),
            "bat_c": lambda t: pista_bateria(t, {"bombo": [0, 10], "caja": [4, 12]}, suave=True),
            "bat_k": lambda t: pista_bateria(t, {"bombo": [0, 4, 8, 10, 12], "caja": [4, 12], "fantasma": [9],
                                                 "charles": 2, "abierto": [2, 10], "redoble": True}),
        },
        capas={
            "stage3_calma.ogg": dict(seco={"pad": 1, "arp": 1, "bajo": 1, "lead": 0.8, "bat_c": 0.8},
                                     reverb={"pad": 0.5, "arp": 0.6, "lead": 0.5, "bat_c": 0.5}, lufs=-16.5),
            "stage3_combate.ogg": dict(seco={"pad": 0.8, "arp": 1, "bajo_sat": 1, "lead": 1, "bat_k": 1},
                                       reverb={"pad": 0.4, "arp": 0.4, "bat_k": 0.4, "lead": 0.4}, lufs=-15.5),
        },
    )
    # --- STAGE 4: azotea, Mi menor 132, viento + arpegio brillante ------------
    T["stage4"] = dict(
        bpm=132, raiz=40, escala="menor", reverb_s=2.2,
        pistas={
            "viento": lambda t: pista_viento(t, nivel=0.12),
            "pad": lambda t: pista_pad(t, nivel=0.28, oscuro=(400, 3000)),
            "arp": lambda t: pista_arpegio(t, nivel=0.2, brillo=1.3, octava=24),
            "bajo": lambda t: pista_bajo(t, 0.55, 0.9, 1.0, nivel=0.45),
            "bajo_sat": lambda t: pista_bajo(t, 1.0, 1.4, 4.5, nivel=0.42),
            "lead": lambda t: pista_lead(t, [1, 3], nivel=0.2, densidad=0.7, portamento=0.035),
            "bat_c": lambda t: pista_bateria(t, {"bombo": [0, 8], "caja": [12]}, suave=True),
            "bat_k": lambda t: pista_bateria(t, {"bombo": [0, 4, 8, 12], "caja": [4, 12], "fantasma": [3, 11],
                                                 "charles": 1, "abierto": [2, 6, 10, 14], "redoble": True}),
        },
        capas={
            "stage4_calma.ogg": dict(seco={"viento": 1, "pad": 1, "arp": 0.9, "bajo": 1, "lead": 0.8, "bat_c": 0.8},
                                     reverb={"pad": 0.4, "arp": 0.4, "lead": 0.4, "bat_c": 0.5}, lufs=-16.5),
            "stage4_combate.ogg": dict(seco={"viento": 0.6, "pad": 0.8, "arp": 1, "bajo_sat": 1, "lead": 1, "bat_k": 1},
                                       reverb={"pad": 0.3, "arp": 0.3, "bat_k": 0.4, "lead": 0.4}, lufs=-15.5),
        },
    )
    # --- STAGE 5: guarida, Sol frigio 138, pulso grave -----------------------
    T["stage5"] = dict(
        bpm=138, raiz=43, escala="frigia", reverb_s=2.6, reverb_oscuro=3800,
        pistas={
            "drone": lambda t: pista_drone(t, nivel=0.22, maquina=False),
            "pad": lambda t: pista_pad(t, nivel=0.3, voces=3, oscuro=(240, 1600), base_oct=0),
            "arp": lambda t: pista_arpegio(t, nivel=0.15, tipo="fm", cada=4, octava=36, largo_patron=5),
            "bajo": lambda t: pista_bajo(t, 0.7, 0.7, 2.0, nivel=0.48),
            "bajo_sat": lambda t: pista_bajo(t, 1.0, 1.2, 5.0, nivel=0.42),
            "lead": lambda t: pista_lead(t, [1, 3], nivel=0.2, octava=12, densidad=0.55, ancho=0.25),
            "bat_c": lambda t: pista_bateria(t, {"bombo": [0, 7, 10], "caja": [8], "taiko_agudo": [14]}, suave=True),
            "bat_k": lambda t: pista_bateria(t, {"bombo": [0, 3, 6, 8, 10, 12], "caja": [4, 12], "fantasma": [15],
                                                 "charles": 1, "abierto": [14], "taiko_agudo": [7], "redoble": True}),
        },
        capas={
            "stage5_calma.ogg": dict(seco={"drone": 1, "pad": 1, "arp": 1, "bajo": 1, "lead": 0.8, "bat_c": 0.8},
                                     reverb={"pad": 0.4, "arp": 0.6, "bat_c": 0.5, "lead": 0.4}, lufs=-16.5),
            "stage5_combate.ogg": dict(seco={"drone": 0.7, "pad": 0.8, "arp": 0.9, "bajo_sat": 1, "lead": 1, "bat_k": 1},
                                       reverb={"pad": 0.3, "arp": 0.4, "bat_k": 0.4, "lead": 0.4}, lufs=-15.5),
        },
    )
    # --- JEFE: 140 BPM, Do# frigio, taiko + ostinato cromatico + bronces ------
    T["boss"] = dict(
        bpm=140, raiz=37, escala="frigia", reverb_s=2.2, reverb_oscuro=4500,
        pistas={
            "pad": lambda t: pista_pad(t, nivel=0.25, voces=2, oscuro=(260, 1500)),
            "bajo": lambda t: pista_bajo(t, 1.0, 0.9, 2.5, nivel=0.5, cromatico=True),
            "taiko": lambda t: pista_bateria(t, {"taiko": [0, 6, 8, 11, 14], "taiko_agudo": [3, 12], "bombo": [0, 8]}),
            "bronces": lambda t: pista_bronces(t, nivel=0.3),
            "lead": lambda t: pista_lead(t, [1, 3], nivel=0.2, densidad=0.65, portamento=0.03),
            "bat_b": lambda t: pista_bateria(t, {"bombo": [0, 4, 8, 12], "caja": [4, 12], "charles": 1,
                                                 "abierto": [2, 10], "redoble": True}),
        },
        capas={
            "boss_a.ogg": dict(seco={"pad": 1, "bajo": 1, "taiko": 1, "bronces": 0.45},
                               reverb={"taiko": 0.4, "pad": 0.4, "bronces": 0.4}, lufs=-16.5),
            "boss_b.ogg": dict(seco={"pad": 0.8, "bajo": 1, "taiko": 0.9, "bronces": 1, "lead": 0.8, "bat_b": 0.8},
                               reverb={"taiko": 0.35, "bronces": 0.5, "lead": 0.4, "bat_b": 0.3}, lufs=-15.5),
        },
    )
    # --- VS: 150 BPM, Si menor armonica, corto y energico ---------------------
    T["vs"] = dict(
        bpm=150, raiz=35, escala="armonica", reverb_s=1.6,
        pistas={
            "pad": lambda t: pista_pad(t, nivel=0.25, oscuro=(500, 3200), periodo_compases=4),
            "bajo": lambda t: pista_bajo(t, 1.0, 1.3, 3.5, nivel=0.45, octava=12),
            "arp": lambda t: pista_arpegio(t, nivel=0.16, brillo=1.2, octava=24),
            "lead": lambda t: pista_lead(t, [0, 1, 2, 3], nivel=0.2, densidad=0.75, portamento=0.025),
            "bronces": lambda t: pista_bronces(t, nivel=0.18, pasos_stab=(0, 10)),
            "bat": lambda t: pista_bateria(t, {"bombo": [0, 4, 8, 12], "caja": [4, 12], "fantasma": [7, 13],
                                               "charles": 1, "abierto": [2, 6, 10, 14], "taiko_agudo": [15],
                                               "redoble": True}),
        },
        capas={"vs.ogg": dict(seco={"pad": 1, "bajo": 1, "arp": 1, "lead": 1, "bronces": 1, "bat": 1},
                              reverb={"pad": 0.3, "lead": 0.3, "bat": 0.3, "bronces": 0.4}, lufs=-15.5)},
    )
    for k, v in T.items():
        v.setdefault("compases", 32)
    return T


# ---------------------------------------------------------------------------
# STINGERS (sin bucle)
# ---------------------------------------------------------------------------

def stinger(nombre):
    _cache.clear()
    rng = np.random.default_rng(semilla_de(nombre))
    ir = respuesta_reverb(2.4, 5000, rng)
    total = 4.6
    b = Bus(ns(total), circular=False)
    if nombre == "victoria":
        bpm, raiz = 120, 45              # La: VI - VII - I (mayor) generico
        paso = 60 / bpm / 4
        grados = [(53, 0.0), (55, 0.75)]   # Fa, Sol (raices del VI y VII)
        for m, t0 in grados:
            for k, iv in enumerate((0, 4, 7, 12)):
                x = pluck(m + 12 + iv, paso * 1.5, 1.2)
                b.suma(ns(t0 + k * paso), x * 0.3, (-0.3, 0.3)[k % 2])
            b.suma(ns(t0), bronces([m - 12, m, m + 4, m + 7], 0.5, 1.0) * 0.35)
            b.suma(ns(t0), taiko(variante=1) * 0.7)
        # acorde final de La mayor (picardia) con lead que sube por la escala
        t1 = 1.5
        b.suma(ns(t1), bronces([raiz, raiz + 12, raiz + 16, raiz + 19, raiz + 24], 2.2, 1.2) * 0.45)
        b.suma(ns(t1), taiko(120, 50, 0.6, 2) * 1.0)
        b.suma(ns(t1), bombo() * 0.8)
        n = ns(2.6)
        sube = [raiz + 24 + s for s in (4, 7, 11, 12, 16)]
        orden = list(rng.permutation(4)) + [4]
        for i, k in enumerate(orden):
            m = sube[k] if i < 4 else sube[-1]
            x = pluck(m, 0.4 if i < 4 else 1.6, 1.4)
            b.suma(ns(t1 + 0.12 * i), x * 0.22, 0.0)
        pad = np.zeros(n)
        for iv in (0, 4, 7, 12):
            pad += saw(np.full(n, float(midi_hz(raiz + 12 + iv))), 0.2 * iv % 1)
        pad = lp(pad, 2000, 2) * env_gate(n, 0.05, ns(1.6), 1.0) * 0.12
        b.suma(ns(t1), pad)
    else:  # derrota: descenso en La menor, filtro que se cierra
        raiz = 45
        acordes = [(0, 0.0), (8, 1.0), (5, 2.0)]    # i - bVI - iv
        for iv, t0 in acordes:
            notas = [raiz + iv, raiz + 12 + iv, raiz + 15 + iv if iv != 8 else raiz + 16 + iv, raiz + 19 + iv]
            b.suma(ns(t0), bronces(notas, 0.9, 0.7 - 0.15 * t0) * 0.35)
            b.suma(ns(t0), taiko(80, 42, 0.5, int(t0)) * 0.8)
        n = ns(3.2)
        t = np.arange(n) / SR
        cae = midi_hz(raiz + 24 - 12 * np.clip(t / 2.6, 0, 1) ** 1.5 + rng.uniform(-0.05, 0.05))
        x = square(cae) * env_gate(n, 0.02, ns(2.4), 0.8)
        x = lp(x, 1800, 2) * 0.16
        b.suma(ns(0.3), x, 0.0)
        final = bronces([raiz - 12, raiz, raiz + 3, raiz + 7], 1.6, 0.5)
        b.suma(ns(3.0), final * 0.3)
        b.suma(ns(3.0), taiko(70, 35, 0.8, 3) * 0.9)
    x = b.x + conv_lineal(b.x, ir) * 0.45
    fin = ns(0.8)
    x[:, -fin:] *= np.linspace(1, 0, fin) ** 2
    return master(x, False)


# ---------------------------------------------------------------------------
# AMBIENTES POR STAGE (bucles cortos de 8 s, opcionales para F1.10)
# ---------------------------------------------------------------------------

def ambiente(nombre):
    rng = np.random.default_rng(semilla_de("amb_" + nombre))
    L = ns(8.0)
    t = np.arange(L) / SR
    out = np.zeros((2, L))
    if nombre == "industrial":    # zumbido de nave + golpes lejanos
        for c in range(2):
            r = rng.uniform(-1, 1, L)
            out[c] = bp(r, 60, 260, 2) * (0.7 + 0.3 * np.sin(2 * np.pi * t / 4.0 + c))
            out[c] += 0.15 * np.sin(2 * np.pi * 50 * t) + 0.08 * np.sin(2 * np.pi * 100 * t + c)
        bb = Bus(L, True)
        for k in range(5):
            bb.suma(ns(rng.uniform(0, 8)), metal(float(rng.uniform(180, 300)), k) * 0.25, float(rng.uniform(-0.8, 0.8)))
        out += lp(bb.x, 1800, 2)
    elif nombre == "puerto":      # oleaje (ruido con envolvente lenta) + gaviotas no: solo agua
        for c in range(2):
            r = rng.uniform(-1, 1, L)
            ola = 0.5 - 0.5 * np.cos(2 * np.pi * (t / 4.0 + 0.3 * c))
            out[c] = (lp(r, 500, 2) * 0.8 + bp(r, 800, 3000, 2) * 0.25 * ola) * (0.3 + 0.7 * ola)
    elif nombre == "azotea":      # viento
        for c in range(2):
            r = rng.uniform(-1, 1, L)
            m = 0.5 - 0.5 * np.cos(2 * np.pi * (t / 8.0 + 0.4 * c))
            out[c] = bp(r, 250, 700, 2) * (1 - m) + bp(r, 700, 1800, 2) * m * 0.7
    else:                         # guarida: zumbido grave y goteo
        for c in range(2):
            r = rng.uniform(-1, 1, L)
            out[c] = lp(r, 140, 2) * 1.5 + 0.1 * np.sin(2 * np.pi * 43 * t + c)
        bb = Bus(L, True)
        for k in range(9):
            n = ns(0.25)
            tt = np.arange(n) / SR
            gota = np.sin(2 * np.pi * (900 + 600 * np.exp(-tt / 0.02)) * tt) * env_ar(n, 0.001, 0.05, 1.0)
            bb.suma(ns(rng.uniform(0, 8)), gota * 0.25, float(rng.uniform(-0.7, 0.7)))
        out += bb.x
    # bucle circular: los filtros IIR arrancan en 0 -> aplica la union suave
    x = master(out, True, -28.0)
    return une_bucle_largo(x)


def une_bucle_largo(x, seg=0.25):
    """Para ruido filtrado: crossfade de potencia constante entre el final y el
    principio (sin depender de la continuidad del filtro)."""
    k = ns(seg)
    a = x[:, :k]
    z = x[:, -k:]
    w = np.sin(0.5 * np.pi * np.arange(k) / k)
    y = x[:, k:].copy()
    y[:, -k:] = z * np.cos(0.5 * np.pi * np.arange(k) / k) + a * w
    return une_bucle(y)


# ---------------------------------------------------------------------------
# ESCRITURA Y ANALISIS
# ---------------------------------------------------------------------------

def codifica(x, destino, tmpdir, calidad=CALIDAD_OGG):
    wav = os.path.join(tmpdir, os.path.basename(destino) + ".wav")
    wavfile.write(wav, SR, x.T.astype(np.float32))
    subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", wav, "-map_metadata", "-1",
                    "-fflags", "+bitexact", "-flags:a", "+bitexact",
                    "-c:a", "libvorbis", "-q:a", calidad, destino], check=True)
    os.remove(wav)


def decodifica(ruta):
    r = subprocess.run(["ffmpeg", "-v", "error", "-i", ruta, "-f", "f32le", "-ac", "2", "-ar", str(SR), "-"],
                       check=True, capture_output=True)
    return np.frombuffer(r.stdout, dtype=np.float32).reshape(-1, 2).T.astype(np.float64)


def md5(ruta):
    with open(ruta, "rb") as f:
        return hashlib.md5(f.read()).hexdigest()


NOMBRE_NOTA = ["Do", "Do#", "Re", "Mib", "Mi", "Fa", "Fa#", "Sol", "Lab", "La", "Sib", "Si"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--solo", default="", help="temas separados por coma (menu,stage1..5,boss,vs,stingers,ambiente)")
    args = ap.parse_args()
    solo = set(s for s in args.solo.split(",") if s)
    os.makedirs(DIR_MUSICA, exist_ok=True)
    os.makedirs(DIR_AMBIENTE, exist_ok=True)
    filas = []
    with tempfile.TemporaryDirectory(prefix="df_musica_") as tmp:
        for nombre, cfg in cfg_temas().items():
            if solo and nombre not in solo:
                continue
            tema, capas = render_tema(nombre, cfg)
            tono = NOMBRE_NOTA[cfg["raiz"] % 12] + " " + cfg["escala"]
            for archivo, x in capas.items():
                destino = os.path.join(DIR_MUSICA, archivo)
                codifica(x, destino, tmp)
                filas.append((archivo, cfg["bpm"], tono, tema.compases, x, destino, True))
        if not solo or "stingers" in solo:
            for nombre in ("victoria", "derrota"):
                x = stinger(nombre)
                destino = os.path.join(DIR_MUSICA, nombre + ".ogg")
                codifica(x, destino, tmp)
                filas.append((nombre + ".ogg", 120 if nombre == "victoria" else 0, "La", 0, x, destino, False))
        if not solo or "ambiente" in solo:
            for nombre in ("industrial", "puerto", "azotea", "guarida"):
                x = ambiente(nombre)
                destino = os.path.join(DIR_AMBIENTE, "amb_" + nombre + ".ogg")
                codifica(x, destino, tmp, "1")
                filas.append(("ambient/amb_" + nombre + ".ogg", 0, "-", 0, x, destino, True))
    total = 0
    print("%-22s %4s %-14s %4s %7s %7s %7s %8s %8s %8s %8s  %s" % (
        "archivo", "bpm", "tonalidad", "comp", "dur_s", "LUFS", "pico", "union", "u_ogg", "pico_ogg", "MB", "md5"))
    for archivo, bpm, tono, comp, x, destino, bucle in filas:
        dec = decodifica(destino)
        tam = os.path.getsize(destino)
        total += tam
        u = salto_union_db(x) if bucle else float("nan")
        u2 = salto_union_db(dec) if bucle else float("nan")
        pico = 20 * np.log10(np.max(np.abs(dec)) + 1e-12)
        print("%-22s %4s %-14s %4s %7.2f %7.2f %7.2f %8.1f %8.1f %8.2f %8.3f  %s" % (
            archivo, bpm or "-", tono, comp or "-", x.shape[1] / SR, lufs(x), 20 * np.log10(np.max(np.abs(x))),
            u, u2, pico, tam / 1e6, md5(destino)))
    print("TOTAL: %.3f MB" % (total / 1e6))


if __name__ == "__main__":
    main()
