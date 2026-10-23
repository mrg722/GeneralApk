#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generador de efectos de sonido para el personaje RONIN (District Fury).

QUE HACE
    Sintetiza por codigo (numpy + scipy) los .wav de punetazos, patadas y la
    habilidad de espada de Ronin, y los escribe en assets/audio/sfx/ronin/ en
    WAV estereo 44100 Hz / 16 bits.

    Misma tecnica que tools/audio/genera_sonidos_rayder.py (ver
    docs/SONIDO_RAYDER.md): tres capas por sonido (transitorio + cuerpo +
    cola de reverberacion), estereo "ancho" con microretardo y cola distinta
    por canal, semilla fija para reproducibilidad. Lo que cambia es el
    CARACTER de cada capa: Ronin es un artista marcial a manos limpias (sin
    el toque "electrico" de Rayder, golpes mas secos y rapidos, con un
    chasquido de tela de gi en vez de la capa de tela generica) mas una
    habilidad propia de espada (desenvaine, tajo de energia, impacto).

SENSACION "ENVOLVENTE" (ojo, honestidad)
    Igual que en el generador de Rayder: esto es ESTEREO ancho (microretardo
    + cola decorrelada por canal), NO es audio 3D con HRTF.

REPRODUCIBLE
    Semilla fija (SEMILLA). Misma semilla = mismos archivos, siempre.

USO
    python3 -I tools/audio/genera_sonidos_ronin.py <carpeta_de_salida>

    Ejemplo:
    python3 -I tools/audio/genera_sonidos_ronin.py assets/audio/sfx/ronin
"""

import os
import sys
import wave

import numpy as np
from scipy import signal

# ---------------------------------------------------------------------------
# AJUSTES GLOBALES
# ---------------------------------------------------------------------------

SR = 44100          # frecuencia de muestreo (igual que el resto del juego)
SEMILLA = 20261023  # semilla fija -> resultado reproducible (distinta de Rayder)
PICO_OBJETIVO_DB = -3.0   # margen de seguridad: nada llega a 0 dBFS

rng = np.random.default_rng(SEMILLA)


# ---------------------------------------------------------------------------
# UTILIDADES BASICAS (idénticas al generador de Rayder)
# ---------------------------------------------------------------------------

def n_muestras(segundos):
    """Cuantas muestras ocupan N segundos."""
    return max(1, int(round(segundos * SR)))


def tiempo(n):
    """Vector de tiempo en segundos, de longitud n."""
    return np.arange(n, dtype=np.float64) / SR


def ruido(n):
    """Ruido blanco entre -1 y 1."""
    return rng.uniform(-1.0, 1.0, n)


def pasa_banda(x, bajo, alto, orden=4):
    """Deja pasar solo las frecuencias entre 'bajo' y 'alto' Hz."""
    nyq = SR / 2.0
    bajo = max(20.0, min(bajo, nyq * 0.98))
    alto = max(bajo * 1.05, min(alto, nyq * 0.98))
    sos = signal.butter(orden, [bajo / nyq, alto / nyq], btype="band", output="sos")
    return signal.sosfilt(sos, x)


def pasa_bajo(x, corte, orden=4):
    """Quita los agudos por encima de 'corte' Hz."""
    nyq = SR / 2.0
    sos = signal.butter(orden, min(corte, nyq * 0.98) / nyq, btype="low", output="sos")
    return signal.sosfilt(sos, x)


def pasa_alto(x, corte, orden=4):
    """Quita los graves por debajo de 'corte' Hz (evita el retumbe sucio)."""
    nyq = SR / 2.0
    sos = signal.butter(orden, max(20.0, min(corte, nyq * 0.98)) / nyq,
                        btype="high", output="sos")
    return signal.sosfilt(sos, x)


# ---------------------------------------------------------------------------
# ENVOLVENTES
# ---------------------------------------------------------------------------

def env_percusiva(n, ataque=0.002, caida=0.08, curva=2.0):
    """Envolvente de golpe: sube casi instantaneo y cae exponencial."""
    t = tiempo(n)
    na = max(1, n_muestras(ataque))
    subida = np.clip(np.arange(n) / na, 0.0, 1.0)
    bajada = np.exp(-curva * t / max(caida, 1e-4))
    return subida * bajada


def env_ad(n, ataque, caida, forma=1.0):
    """Envolvente ataque/caida lineal-exponencial, para cuerpos sostenidos."""
    na = min(n, max(1, n_muestras(ataque)))
    env = np.zeros(n)
    env[:na] = np.linspace(0.0, 1.0, na)
    resto = n - na
    if resto > 0:
        t = np.arange(resto) / SR
        env[na:] = np.exp(-forma * t / max(caida, 1e-4))
    return env


def env_rampa(n, inicio=0.0, fin=1.0, curva=2.0):
    """Rampa con curvatura (para el tajo de energia, que crece rapido)."""
    x = np.linspace(0.0, 1.0, n) ** curva
    return inicio + (fin - inicio) * x


def suaviza_bordes(x, ms=3.0):
    """Mete un fundido muy corto al inicio y al final: evita chasquidos."""
    n = len(x)
    k = min(n // 2, n_muestras(ms / 1000.0))
    if k <= 1:
        return x
    y = x.copy()
    y[:k] *= np.linspace(0.0, 1.0, k)
    y[-k:] *= np.linspace(1.0, 0.0, k)
    return y


# ---------------------------------------------------------------------------
# CAPA 1 : TRANSITORIO
# ---------------------------------------------------------------------------

def capa_transitorio(n, bajo=1200.0, alto=9000.0, caida=0.012, nivel=1.0):
    """Chasquido corto: ruido filtrado en agudos con caida muy rapida."""
    x = pasa_banda(ruido(n), bajo, alto)
    return x * env_percusiva(n, ataque=0.0005, caida=caida, curva=3.0) * nivel


# ---------------------------------------------------------------------------
# CAPA 2 : CUERPO
# ---------------------------------------------------------------------------

def capa_cuerpo(n, f_ini, f_fin, caida=0.09, nivel=1.0, forma="seno", armonico=0.0):
    """Oscilador cuyo tono baja de f_ini a f_fin (barrido descendente)."""
    t = tiempo(n)
    k = np.log(max(f_fin, 1.0) / max(f_ini, 1.0))
    frec = f_ini * np.exp(k * np.linspace(0.0, 1.0, n))
    fase = 2.0 * np.pi * np.cumsum(frec) / SR

    if forma == "seno":
        onda = np.sin(fase)
    elif forma == "triangulo":
        onda = signal.sawtooth(fase, width=0.5)
    elif forma == "sierra":
        onda = signal.sawtooth(fase)
    elif forma == "cuadrada":
        onda = signal.square(fase, duty=0.45)
    else:
        onda = np.sin(fase)

    if armonico > 0.0:
        onda = onda + armonico * np.sin(2.73 * fase)

    return onda * env_percusiva(n, ataque=0.001, caida=caida, curva=1.6) * nivel * 0.7


def capa_zumbido(n, f_ini, f_fin, ancho=0.35, nivel=1.0):
    """Ruido con el filtro barriendo de grave a agudo (o al reves)."""
    x = ruido(n)
    bloques = 48
    paso = max(1, n // bloques)
    salida = np.zeros(n)
    centros = np.geomspace(max(f_ini, 60.0), max(f_fin, 60.0), bloques)
    for i, fc in enumerate(centros):
        a = i * paso
        b = n if i == bloques - 1 else min(n, a + paso)
        if b <= a:
            continue
        a2 = max(0, a - paso)
        trozo = pasa_banda(x[a2:b], fc * (1.0 - ancho), fc * (1.0 + ancho), orden=2)
        salida[a:b] = trozo[a - a2:]
    return salida * nivel


def capa_metal(n, parciales, caida=0.25, nivel=1.0):
    """
    Resonancia metalica (campana/hoja de espada): suma de senos en
    frecuencias no armonicas (parciales) que decaen cada uno a su ritmo.
    Es lo que distingue el acero de Ronin del tono "organico" de los
    punetazos: un metal vibra en varias frecuencias no relacionadas.
    'parciales' es una lista de (frecuencia_hz, nivel_relativo, caida_seg).
    """
    t = tiempo(n)
    out = np.zeros(n)
    for freq, niv, cae in parciales:
        out += niv * np.sin(2.0 * np.pi * freq * t) * np.exp(-t / max(cae, 1e-4))
    return out * nivel


def capa_gi(n, caida=0.02, nivel=0.4):
    """
    Chasquido de tela de gi: ruido de banda media-alta muy corto, como el
    tiron de la manga/cinturon en un golpe de artes marciales. Sustituye a
    la capa de "tela" generica de Rayder por algo mas ligado al personaje.
    """
    x = pasa_banda(ruido(n), 900, 4200)
    return x * env_percusiva(n, ataque=0.0008, caida=caida, curva=2.6) * nivel


# ---------------------------------------------------------------------------
# CAPA 3 : COLA (reverberacion corta)
# ---------------------------------------------------------------------------

def impulso_reverb(duracion=0.14, predelay=0.006, brillo=5000.0):
    """Respuesta al impulso sintetica: ruido que decae exponencial."""
    n = n_muestras(duracion)
    t = tiempo(n)
    ir = ruido(n) * np.exp(-7.0 * t / max(duracion, 1e-4))
    ir = pasa_bajo(ir, brillo)
    ir = pasa_alto(ir, 180.0)
    hueco = np.zeros(n_muestras(predelay))
    ir = np.concatenate([hueco, ir])
    return ir / (np.max(np.abs(ir)) + 1e-9)


def anade_cola(x, mezcla=0.22, duracion=0.14, brillo=5000.0, predelay=0.006):
    """Suma al sonido seco una version reverberada (cola corta)."""
    if mezcla <= 0.0:
        return x
    ir = impulso_reverb(duracion, predelay, brillo)
    humedo = signal.fftconvolve(x, ir)[:len(x)]
    humedo /= (np.max(np.abs(humedo)) + 1e-9)
    seco_pico = np.max(np.abs(x)) + 1e-9
    return x + humedo * seco_pico * mezcla


# ---------------------------------------------------------------------------
# ESTEREO Y ESCRITURA
# ---------------------------------------------------------------------------

def a_estereo(x, ancho=0.35, retardo_ms=0.9, cola_mezcla=0.18, cola_dur=0.14,
              cola_brillo=5000.0):
    """Convierte el sonido mono en estereo "ancho" (ver genera_sonidos_rayder.py)."""
    izq = x.copy()
    der = np.roll(x, n_muestras(retardo_ms / 1000.0))
    der[:n_muestras(retardo_ms / 1000.0)] = 0.0

    medio = 0.5 * (izq + der)
    lado = 0.5 * (izq - der)
    izq = medio + ancho * lado
    der = medio - ancho * lado

    if cola_mezcla > 0.0:
        izq = anade_cola(izq, cola_mezcla, cola_dur, cola_brillo, 0.005)
        der = anade_cola(der, cola_mezcla, cola_dur * 1.12, cola_brillo * 0.92, 0.008)

    return np.stack([izq, der], axis=1)


def compacta(x, empuje=1.0):
    """Compacta el rango dinamico con una saturacion suave (tanh)."""
    if empuje <= 1.0:
        return x
    pico = np.max(np.abs(x)) + 1e-9
    return np.tanh(empuje * x / pico) / np.tanh(empuje)


def normaliza(est, pico_db=PICO_OBJETIVO_DB):
    """Ajusta el volumen para que el pico quede en 'pico_db' (p.ej. -3 dBFS)."""
    pico = np.max(np.abs(est))
    if pico < 1e-9:
        return est
    objetivo = 10.0 ** (pico_db / 20.0)
    return est * (objetivo / pico)


def guarda_wav(ruta, est):
    """Escribe WAV estereo 44100 Hz 16 bits."""
    est = np.clip(est, -1.0, 1.0)
    datos = (est * 32767.0).astype("<i2")
    with wave.open(ruta, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(datos.tobytes())


def termina(nombre, mono, carpeta, ancho=0.35, cola=0.18, cola_dur=0.14,
            cola_brillo=5000.0, retardo_ms=0.9, empuje=1.0):
    """Pasos finales comunes: compactar, suavizar, estereo, normalizar y guardar."""
    mono = compacta(mono, empuje)
    mono = suaviza_bordes(mono, ms=2.5)
    est = a_estereo(mono, ancho=ancho, retardo_ms=retardo_ms,
                    cola_mezcla=cola, cola_dur=cola_dur, cola_brillo=cola_brillo)
    est[:, 0] = suaviza_bordes(est[:, 0], 2.0)
    est[:, 1] = suaviza_bordes(est[:, 1], 2.0)
    est = normaliza(est)
    ruta = os.path.join(carpeta, nombre + ".wav")
    guarda_wav(ruta, est)
    return ruta


def variacion(base, porcentaje):
    """Variacion aleatoria controlada alrededor de 'base' (+/- porcentaje)."""
    return base * (1.0 + rng.uniform(-porcentaje, porcentaje))


# ---------------------------------------------------------------------------
# RECETAS: un sonido por funcion
# ---------------------------------------------------------------------------

def golpe(indice):
    """
    Punetazo del combo de Ronin. Mas seco y rapido que el de Rayder: sin
    armonico "electrico", caida mas corta, transitorio mas arriba en
    frecuencia (nudillo contra carne, no guante), y una capa de gi en vez
    de tela generica. (~0.09-0.10 s, Rayder anda en ~0.13-0.15 s).
    """
    base = [240.0, 268.0, 218.0][indice]
    dur = variacion([0.090, 0.097, 0.100][indice], 0.05)
    n = n_muestras(dur)

    t_click = capa_transitorio(n, bajo=variacion(2400, 0.15), alto=variacion(11000, 0.1),
                               caida=variacion(0.007, 0.2), nivel=0.9)
    cuerpo = capa_cuerpo(n, f_ini=variacion(base, 0.06), f_fin=variacion(base * 0.46, 0.08),
                         caida=variacion(0.028, 0.12), nivel=0.85, armonico=0.0)
    gi = capa_gi(n, caida=variacion(0.016, 0.2), nivel=0.35)

    return t_click + cuerpo + gi, dur


def patada(indice):
    """
    Patada de Ronin. indice 0 = patada normal (barrida, baja), indice 1 =
    patada voladora del remate (mas aire, mas alcance, algo mas larga).
    """
    if indice == 0:
        base, dur = 150.0, variacion(0.145, 0.05)
        aire_nivel, aire_ancho = 0.45, 0.42
    else:
        base, dur = 132.0, variacion(0.205, 0.05)
        aire_nivel, aire_ancho = 0.75, 0.55   # mas vuelo = mas aire cortando

    n = n_muestras(dur)
    aire = capa_zumbido(n, 900 if indice == 0 else 1400, 3000 if indice == 0 else 4200,
                        ancho=aire_ancho, nivel=aire_nivel)
    aire *= env_ad(n, 0.004, 0.045 if indice == 0 else 0.090, forma=2.2)

    t_click = capa_transitorio(n, bajo=variacion(1000, 0.15), alto=variacion(7000, 0.1),
                               caida=variacion(0.013, 0.2), nivel=0.85)
    cuerpo = capa_cuerpo(n, f_ini=variacion(base, 0.06), f_fin=variacion(base * 0.40, 0.08),
                         caida=variacion(0.055, 0.12), nivel=1.05,
                         forma="triangulo", armonico=0.12)

    return aire + t_click + cuerpo, dur


def impacto_fuerte():
    """Remate del combo a manos limpias (~0.26 s): mas peso que un golpe suelto."""
    dur = 0.26
    n = n_muestras(dur)

    t_click = capa_transitorio(n, bajo=800, alto=9500, caida=0.016, nivel=1.0)
    cuerpo = capa_cuerpo(n, 168.0, 54.0, caida=0.075, nivel=1.15, forma="triangulo", armonico=0.15)
    sub = capa_cuerpo(n, 90.0, 38.0, caida=0.100, nivel=0.8, forma="seno")
    gi = capa_gi(n, caida=0.03, nivel=0.5)

    return t_click + cuerpo + sub + gi, dur


def espada_desenvaine():
    """
    Sacar la espada al empezar la habilidad especial (~0.30 s).
    Capas: friccion metal-funda (ruido barriendo agudo) + resonancia
    metalica corta de la hoja al salir.
    """
    dur = 0.30
    n = n_muestras(dur)

    friccion = capa_zumbido(n, 2200, 7000, ancho=0.4, nivel=0.9)
    friccion *= env_ad(n, 0.006, 0.150, forma=1.8)
    clic_salida = capa_transitorio(n, bajo=2000, alto=12000, caida=0.010, nivel=0.7)
    metal = capa_metal(n, [(3100.0, 0.5, 0.10), (4700.0, 0.35, 0.08), (6600.0, 0.22, 0.06)],
                       nivel=0.8)
    metal *= env_percusiva(n, 0.001, 0.14, 1.8)

    return friccion + clic_salida + metal, dur


def espada_tajo():
    """
    El corte de energia de la habilidad (~0.32 s).
    Capas: silbido de hoja (barrido de aire) + un filo de energia tonal que
    sube y cae rapido (la "carga" del tajo, mas corta que la de Rayder
    porque es un corte, no una acumulacion).
    """
    dur = 0.32
    n = n_muestras(dur)
    mitad = n // 2

    sube = capa_zumbido(mitad, 700, 4200, ancho=0.5, nivel=1.0)
    baja = capa_zumbido(n - mitad, 4200, 900, ancho=0.5, nivel=0.9)
    silbido = np.concatenate([sube, baja])
    silbido *= np.sin(np.linspace(0.0, np.pi, n)) ** 1.3

    filo = capa_cuerpo(n, 260.0, 980.0, caida=0.18, nivel=0.5, forma="sierra")
    filo = pasa_alto(filo, 500.0)

    return silbido * 0.85 + filo, dur


def espada_impacto():
    """
    Cuando el tajo conecta (~0.34 s).
    Capas: estallido + cuerpo grave + resonancia metalica (en vez del
    "electrico" por modulacion en anillo que usa Rayder) + cola larga.
    """
    dur = 0.34
    n = n_muestras(dur)

    estallido = pasa_banda(ruido(n), 500, 9500) * env_percusiva(n, 0.0008, 0.045, 2.3)
    cuerpo = capa_cuerpo(n, 175.0, 50.0, caida=0.095, nivel=1.0, forma="triangulo", armonico=0.18)
    metal = capa_metal(n, [(2600.0, 0.45, 0.14), (3900.0, 0.3, 0.10), (5400.0, 0.2, 0.07)],
                       nivel=0.9)
    metal *= env_percusiva(n, 0.001, 0.12, 1.6)

    return estallido + cuerpo + metal, dur


def salto():
    """Impulso al saltar (~0.11 s): empuje de aire corto, sin impacto."""
    dur = 0.11
    n = n_muestras(dur)

    empuje = capa_cuerpo(n, 230.0, 110.0, caida=0.040, nivel=0.5, forma="seno")
    aire = capa_zumbido(n, 1800, 4800, ancho=0.45, nivel=0.8)
    aire *= env_ad(n, 0.003, 0.050, forma=2.0)

    return aire + empuje, dur


def aterrizaje():
    """Caer al suelo tras un salto o remate (~0.16 s): golpe sordo y corto."""
    dur = 0.16
    n = n_muestras(dur)

    t_click = capa_transitorio(n, bajo=400, alto=5500, caida=0.018, nivel=0.9)
    cuerpo = capa_cuerpo(n, 95.0, 40.0, caida=0.075, nivel=1.0, forma="seno")
    polvo = pasa_banda(ruido(n), 600, 3200) * env_percusiva(n, 0.004, 0.055, 1.8) * 0.4

    return t_click + cuerpo + polvo, dur


# ---------------------------------------------------------------------------
# PROGRAMA PRINCIPAL
# ---------------------------------------------------------------------------

# Cada entrada: nombre de archivo -> (funcion, ancho estereo, mezcla de cola,
#                                     duracion de la cola, brillo de la cola,
#                                     empuje de compactado)
# El 'empuje' se ajusto para que el RMS final quede en el mismo rango que el
# resto del juego y que el paquete de Rayder (-10 a -15 dB RMS aprox).
RECETAS = [
    ("golpe_1",          lambda: golpe(0),       0.26, 0.11, 0.075, 6500.0, 6.5),
    ("golpe_2",          lambda: golpe(1),       0.30, 0.11, 0.075, 7000.0, 7.0),
    ("golpe_3",          lambda: golpe(2),       0.24, 0.12, 0.080, 6000.0, 6.5),
    ("patada_1",         lambda: patada(0),      0.30, 0.15, 0.11,  4800.0, 6.0),
    ("patada_2",         lambda: patada(1),      0.40, 0.17, 0.14,  4600.0, 6.0),
    ("impacto_fuerte",   impacto_fuerte,         0.40, 0.24, 0.18,  4200.0, 6.5),
    ("espada_desenvaine", espada_desenvaine,     0.42, 0.18, 0.16,  6500.0, 4.5),
    ("espada_tajo",      espada_tajo,            0.46, 0.18, 0.15,  6500.0, 3.0),
    ("espada_impacto",   espada_impacto,         0.44, 0.28, 0.22,  4800.0, 5.5),
    ("salto",            salto,                  0.34, 0.10, 0.07,  6000.0, 3.0),
    ("aterrizaje",       aterrizaje,             0.32, 0.14, 0.10,  4000.0, 3.5),
]


def main():
    if len(sys.argv) < 2:
        print("Uso: python3 -I genera_sonidos_ronin.py <carpeta_de_salida>")
        return 1
    carpeta = os.path.abspath(sys.argv[1])
    os.makedirs(carpeta, exist_ok=True)

    print("Generando sonidos de RONIN en:", carpeta)
    print("Semilla:", SEMILLA, "| pico objetivo:", PICO_OBJETIVO_DB, "dBFS")
    for nombre, receta, ancho, cola, cola_dur, brillo, empuje in RECETAS:
        mono, dur = receta()
        ruta = termina(nombre, mono, carpeta, ancho=ancho, cola=cola,
                       cola_dur=cola_dur, cola_brillo=brillo, empuje=empuje)
        print("  OK  %-18s %.3f s  -> %s" % (nombre, dur, os.path.basename(ruta)))
    print("Listo: %d archivos." % len(RECETAS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
