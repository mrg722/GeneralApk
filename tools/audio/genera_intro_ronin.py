#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generador del jingle de ARRANQUE del juego (District Fury).

QUE ES (y que NO es)
    Esto sintetiza por codigo, con osciladores y filtros matematicos, una
    pieza ORIGINAL de ~4 segundos pensada para sonar al abrir el juego.
    NO transcribe ninguna melodia existente, NO samplea ni copia audio de
    ningun juego o pelicula real, y NO fue generada con una IA de audio.
    Es composicion nueva: se buscó capturar la ENERGIA de un jingle de
    arranque de juego de pelea dramatico (bronces graves que suben,
    percusion tipo taiko, un barrido de "carga de poder" y un impacto
    final brillante con cola larga) sin citar ninguna obra en concreto.
    Ver docs/INTRO_ARRANQUE.md para el detalle de esta aclaracion.

TECNICA (misma familia que tools/audio/genera_sonidos_rayder.py)
    - Capas por bloque: percusion sintetica (toms/taiko = ruido filtrado +
      cuerpo con tono cayendo), un "acorde de bronces" sintetico hecho con
      varios osciladores en relacion de quinta/octava que suben de tono,
      un barrido de energia tipo carga_poder() (tono ascendente + tremolo
      que se acelera + chispas de ruido agudo, igual que en
      genera_sonidos_rayder.py), y un impacto final con reverberacion por
      convolucion (ruido que decae, pasa-bajo + pasa-alto) para la cola.
    - Reproducible: semilla fija (SEMILLA). Mismo resultado siempre.

ESTRUCTURA (0.0 - ~4.2 s)
    (a) 0.00-0.30 s : silencio/respiracion tensa (ruido grave muy bajo).
    (b) 0.30-1.00 s : golpe de entrada (acorde grave + percusion) y un
                      stinger brillante encima.
    (c) 1.00-2.60 s : barrido ascendente de energia ("carga de poder").
    (d) 2.60-4.20 s : impacto final grande + cola de reverberacion larga
                      (1.0-1.5 s) que se apaga.

USO
    python3 -I tools/audio/genera_intro_ronin.py assets/audio/sfx
    (escribe assets/audio/sfx/intro_arranque.wav)
"""

import os
import sys
import wave

import numpy as np
from scipy import signal

# ---------------------------------------------------------------------------
# AJUSTES GLOBALES
# ---------------------------------------------------------------------------

SR = 44100
SEMILLA = 20261023  # semilla fija -> resultado reproducible
PICO_OBJETIVO_DB = -1.0  # mas fuerte que los sfx normales: suena una sola vez

rng = np.random.default_rng(SEMILLA)


# ---------------------------------------------------------------------------
# UTILIDADES BASICAS (mismo estilo que genera_sonidos_rayder.py)
# ---------------------------------------------------------------------------

def n_muestras(segundos):
    return max(1, int(round(segundos * SR)))


def tiempo(n, offset=0.0):
    return np.arange(n, dtype=np.float64) / SR + offset


def ruido(n):
    return rng.uniform(-1.0, 1.0, n)


def pasa_banda(x, bajo, alto, orden=4):
    nyq = SR / 2.0
    bajo = max(20.0, min(bajo, nyq * 0.98))
    alto = max(bajo * 1.05, min(alto, nyq * 0.98))
    sos = signal.butter(orden, [bajo / nyq, alto / nyq], btype="band", output="sos")
    return signal.sosfilt(sos, x)


def pasa_bajo(x, corte, orden=4):
    nyq = SR / 2.0
    sos = signal.butter(orden, min(corte, nyq * 0.98) / nyq, btype="low", output="sos")
    return signal.sosfilt(sos, x)


def pasa_alto(x, corte, orden=4):
    nyq = SR / 2.0
    sos = signal.butter(orden, max(20.0, min(corte, nyq * 0.98)) / nyq,
                        btype="high", output="sos")
    return signal.sosfilt(sos, x)


def env_percusiva(n, ataque=0.002, caida=0.08, curva=2.0):
    t = tiempo(n)
    na = max(1, n_muestras(ataque))
    subida = np.clip(np.arange(n) / na, 0.0, 1.0)
    bajada = np.exp(-curva * t / max(caida, 1e-4))
    return subida * bajada


def env_rampa(n, inicio=0.0, fin=1.0, curva=2.0):
    x = np.linspace(0.0, 1.0, n) ** curva
    return inicio + (fin - inicio) * x


def suaviza_bordes(x, ms=3.0):
    n = len(x)
    k = min(n // 2, n_muestras(ms / 1000.0))
    if k <= 1:
        return x
    y = x.copy()
    y[:k] *= np.linspace(0.0, 1.0, k)
    y[-k:] *= np.linspace(1.0, 0.0, k)
    return y


def pad(x, antes, despues):
    """Agrega silencio antes y despues (en muestras)."""
    return np.concatenate([np.zeros(antes), x, np.zeros(despues)])


# ---------------------------------------------------------------------------
# CAPAS
# ---------------------------------------------------------------------------

def capa_percusion(n, f_ini, f_fin, caida=0.12, nivel=1.0):
    """Tom/taiko sintetico: cuerpo con tono cayendo + transitorio seco."""
    t = tiempo(n)
    k = np.log(max(f_fin, 1.0) / max(f_ini, 1.0))
    frec = f_ini * np.exp(k * np.linspace(0.0, 1.0, n))
    fase = 2.0 * np.pi * np.cumsum(frec) / SR
    cuerpo = np.sin(fase) + 0.25 * np.sin(2.0 * fase)
    cuerpo *= env_percusiva(n, ataque=0.001, caida=caida, curva=1.4)
    click = pasa_banda(ruido(n), 1200, 7000) * env_percusiva(n, 0.0005, 0.02, 3.0) * 0.5
    return (cuerpo * nivel + click) * 0.9


def acorde_bronces(n, f_base, caida=0.5, nivel=1.0):
    """
    'Acorde de bronces' sintetico: varios osciladores de sierra en relacion
    de octava/quinta que suben de tono juntos. No reproduce ninguna melodia
    existente: es un acorde generico (fundamental + octava + quinta) usado
    como bloque de energia, igual que en furia() de genera_sonidos_rayder.py.
    """
    salida = np.zeros(n)
    for mult, peso in ((1.0, 1.0), (1.5, 0.65), (2.0, 0.45)):
        f_ini = f_base * mult * 0.75
        f_fin = f_base * mult
        frec = np.geomspace(max(f_ini, 1.0), max(f_fin, 1.0), n)
        fase = 2.0 * np.pi * np.cumsum(frec) / SR
        salida += peso * signal.sawtooth(fase, width=0.5)
    salida = pasa_bajo(salida, 3200)
    salida *= env_percusiva(n, ataque=0.01, caida=caida, curva=1.1)
    return salida * nivel


def carga_de_poder(n, f_ini=140.0, f_fin=900.0, nivel=1.0):
    """
    Barrido ascendente de "carga de energia": tono subiendo + tremolo que
    se acelera + chispas de ruido agudo. Misma tecnica que carga_poder() en
    genera_sonidos_rayder.py (composicion propia de este proyecto).
    """
    frec = np.geomspace(f_ini, f_fin, n)
    fase = 2.0 * np.pi * np.cumsum(frec) / SR
    tono = np.sin(fase) + 0.35 * np.sin(2.0 * fase) + 0.18 * np.sin(3.01 * fase)

    vel = np.geomspace(6.0, 48.0, n)
    tremolo = 0.70 + 0.30 * np.sin(2.0 * np.pi * np.cumsum(vel) / SR)

    chispas = pasa_banda(ruido(n), 2500, 12000) * env_rampa(n, 0.0, 0.6, curva=3.0)

    env = env_rampa(n, 0.03, 1.0, curva=1.6)
    return (tono * 0.6 * tremolo + chispas) * env * nivel


def impulso_reverb(duracion=1.2, predelay=0.01, brillo=4200.0):
    """Cola de reverberacion larga: ruido que decae, filtrado (misma receta
    que impulso_reverb() en genera_sonidos_rayder.py, con mas duracion)."""
    n = n_muestras(duracion)
    t = tiempo(n)
    ir = ruido(n) * np.exp(-4.0 * t / max(duracion, 1e-4))
    ir = pasa_bajo(ir, brillo)
    ir = pasa_alto(ir, 120.0)
    hueco = np.zeros(n_muestras(predelay))
    ir = np.concatenate([hueco, ir])
    return ir / (np.max(np.abs(ir)) + 1e-9)


def anade_cola(x, mezcla=0.5, duracion=1.2, brillo=4200.0, predelay=0.01):
    if mezcla <= 0.0:
        return x
    ir = impulso_reverb(duracion, predelay, brillo)
    humedo = signal.fftconvolve(x, ir)[:len(x)]
    humedo /= (np.max(np.abs(humedo)) + 1e-9)
    seco_pico = np.max(np.abs(x)) + 1e-9
    return x + humedo * seco_pico * mezcla


def impacto_final(n, nivel=1.0):
    """Impacto grande: transitorio ancho + cuerpo doble grave + brillo
    metalico encima (modulacion en anillo, igual tecnica que impacto_poder()
    en genera_sonidos_rayder.py)."""
    t = tiempo(n)
    click = pasa_banda(ruido(n), 500, 9000) * env_percusiva(n, 0.0005, 0.05, 2.2)
    k1 = np.log(60.0 / 170.0)
    frec1 = 170.0 * np.exp(k1 * np.linspace(0.0, 1.0, n))
    fase1 = 2.0 * np.pi * np.cumsum(frec1) / SR
    cuerpo = np.sin(fase1) + 0.3 * np.sin(2.0 * fase1)
    cuerpo *= env_percusiva(n, 0.001, 0.14, 1.3)
    sub = np.sin(2.0 * np.pi * 45.0 * t) * env_percusiva(n, 0.001, 0.18, 1.2)
    metalico = np.sin(2 * np.pi * 900.0 * t) * np.sin(2 * np.pi * 240.0 * t)
    metalico *= env_percusiva(n, 0.002, 0.08, 1.8) * 0.5
    return (click + cuerpo + sub * 0.8 + metalico) * nivel


# ---------------------------------------------------------------------------
# ESTEREO Y ESCRITURA
# ---------------------------------------------------------------------------

def a_estereo(x, ancho=0.4, retardo_ms=1.1):
    """Ancho estereo simple por microretardo + M/S (NO es 3D/HRTF, ver nota
    en genera_sonidos_rayder.py: mismo principio)."""
    izq = x.copy()
    r = n_muestras(retardo_ms / 1000.0)
    der = np.roll(x, r)
    der[:r] = 0.0
    medio = 0.5 * (izq + der)
    lado = 0.5 * (izq - der)
    izq = medio + ancho * lado
    der = medio - ancho * lado
    return np.stack([izq, der], axis=1)


def normaliza(est, pico_db=PICO_OBJETIVO_DB):
    pico = np.max(np.abs(est))
    if pico < 1e-9:
        return est
    objetivo = 10.0 ** (pico_db / 20.0)
    return est * (objetivo / pico)


def compacta(x, empuje=2.0):
    """Saturacion suave (tanh) para controlar el pico sin aplanar el golpe."""
    if empuje <= 1.0:
        return x
    pico = np.max(np.abs(x)) + 1e-9
    return np.tanh(empuje * x / pico) / np.tanh(empuje)


def guarda_wav(ruta, est):
    est = np.clip(est, -1.0, 1.0)
    datos = (est * 32767.0).astype("<i2")
    with wave.open(ruta, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(datos.tobytes())


# ---------------------------------------------------------------------------
# COMPOSICION
# ---------------------------------------------------------------------------

def compone():
    # --- (a) 0.00-0.30 s: respiracion tensa (silencio casi total, ruido grave bajo) ---
    dur_a = 0.30
    n_a = n_muestras(dur_a)
    respiracion = pasa_banda(ruido(n_a), 50, 300) * 0.02
    respiracion *= env_rampa(n_a, 0.0, 1.0, curva=1.0)

    # --- (b) 0.30-1.00 s: golpe de entrada (acorde grave + percusion) + stinger ---
    dur_b = 0.70
    n_b = n_muestras(dur_b)
    acorde_b = acorde_bronces(n_b, f_base=110.0, caida=0.55, nivel=0.9)
    perc_b = capa_percusion(n_b, 180.0, 55.0, caida=0.30, nivel=0.9)
    # Stinger brillante: un parcial agudo corto encima del golpe de entrada.
    stinger = (pasa_banda(ruido(n_b), 3000, 9000) * env_percusiva(n_b, 0.001, 0.05, 2.5) * 0.35
               + np.sin(2.0 * np.pi * 1760.0 * tiempo(n_b)) * env_percusiva(n_b, 0.001, 0.08, 2.0) * 0.25)
    bloque_b = acorde_b + perc_b + stinger

    # Dos golpes de percusion tipo taiko repartidos en el bloque (patron rítmico simple).
    n_tom = n_muestras(0.18)
    tom = capa_percusion(n_tom, 140.0, 48.0, caida=0.16, nivel=0.8)
    pos_tom = n_muestras(0.45)
    if pos_tom + n_tom <= n_b:
        bloque_b[pos_tom:pos_tom + n_tom] += tom

    # --- (c) 1.00-2.60 s: barrido ascendente de energia (carga de poder) ---
    dur_c = 1.60
    n_c = n_muestras(dur_c)
    bloque_c = carga_de_poder(n_c, f_ini=130.0, f_fin=950.0, nivel=0.85)
    # Percusion acelerando debajo del barrido (taiko rapido tipo "redoble").
    n_redoble = n_muestras(0.10)
    pasos = np.linspace(0.0, dur_c - 0.12, 10) ** 1.3 / (dur_c - 0.12) * (dur_c - 0.12)
    for ts in pasos:
        ini = n_muestras(ts)
        if ini + n_redoble <= n_c:
            golpe = capa_percusion(n_redoble, 220.0, 90.0, caida=0.06, nivel=0.5)
            bloque_c[ini:ini + n_redoble] += golpe

    # --- (d) 2.60-4.20 s: impacto final + cola larga ---
    dur_d_seco = 0.45
    n_d = n_muestras(dur_d_seco)
    impacto = impacto_final(n_d, nivel=1.0)
    acorde_final = acorde_bronces(n_d, f_base=220.0, caida=0.35, nivel=0.6)
    bloque_d_seco = impacto + acorde_final
    # Cola de reverberacion larga (1.0-1.5 s) que se apaga.
    dur_cola = 1.3
    n_cola = n_muestras(dur_cola)
    bloque_d = pad(bloque_d_seco, 0, max(0, n_cola - n_d))
    bloque_d = anade_cola(bloque_d, mezcla=0.55, duracion=dur_cola, brillo=4000.0, predelay=0.012)
    cierre = np.clip(np.linspace(1.3, 0.0, len(bloque_d)), 0.0, 1.0) ** 0.6
    bloque_d *= cierre

    total = np.concatenate([respiracion, bloque_b, bloque_c, bloque_d])
    total = suaviza_bordes(total, ms=4.0)
    return total


def main():
    if len(sys.argv) < 2:
        print("Uso: python3 -I genera_intro_ronin.py <carpeta_de_salida>")
        return 1
    carpeta = os.path.abspath(sys.argv[1])
    os.makedirs(carpeta, exist_ok=True)

    print("Generando intro de arranque en:", carpeta)
    print("Semilla:", SEMILLA, "| pico objetivo:", PICO_OBJETIVO_DB, "dBFS")

    mono = compone()
    mono = compacta(mono, empuje=2.5)
    est = a_estereo(mono, ancho=0.4, retardo_ms=1.1)
    est[:, 0] = suaviza_bordes(est[:, 0], 3.0)
    est[:, 1] = suaviza_bordes(est[:, 1], 3.0)
    est = normaliza(est)

    dur = len(mono) / SR
    pico = np.max(np.abs(est))
    rms = np.sqrt(np.mean(est ** 2))
    pico_db = 20.0 * np.log10(pico + 1e-12)
    rms_db = 20.0 * np.log10(rms + 1e-12)

    ruta = os.path.join(carpeta, "intro_arranque.wav")
    guarda_wav(ruta, est)
    print("  OK  intro_arranque  %.3f s  pico=%.2f dBFS  rms=%.2f dBFS -> %s"
          % (dur, pico_db, rms_db, ruta))
    print("Listo.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
