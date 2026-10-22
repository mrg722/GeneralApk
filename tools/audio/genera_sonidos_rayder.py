#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generador de efectos de sonido para el personaje RAYDER (District Fury).

QUE HACE
    Sintetiza por codigo (numpy + scipy) los .wav de golpes, patadas y poderes
    de Rayder, y los escribe en assets/audio/sfx/rayder/ en WAV estereo
    44100 Hz / 16 bits.

COMO ESTA ARMADO CADA SONIDO
    Se construye por capas, igual que en diseno de sonido real:
      1) TRANSITORIO : el "clac" inicial. Ruido muy corto y filtrado alto.
                       Es lo que da la sensacion de impacto seco.
      2) CUERPO      : la parte con tono. Un oscilador con la frecuencia
                       cayendo (de agudo a grave) = sensacion de peso.
      3) COLA        : una reverberacion corta (convolucion con ruido que
                       decae). Da el "aire" alrededor del golpe.
    Las tres capas se suman, se les da anchura estereo y se normalizan.

SENSACION "ENVOLVENTE" (ojo, honestidad)
    Lo que se hace aqui es ESTEREO: las capas de cada canal estan
    ligeramente decorreladas (microretardo y cola distinta por lado), asi
    el sonido se siente ancho y no pegado al centro. NO es audio 3D con
    HRTF. La posicion real en pantalla se consigue luego en el juego con
    SetSoundPan() / SetSoundVolume() de raylib.

REPRODUCIBLE
    Hay una SEMILLA fija. Con la misma semilla salen exactamente los
    mismos archivos. Cambia SEMILLA (o los parametros de cada receta) y
    vuelve a ejecutar para obtener otra tanda.

USO
    python3 -I tools/audio/genera_sonidos_rayder.py <carpeta_de_salida>

    Ejemplo:
    python3 -I tools/audio/genera_sonidos_rayder.py assets/audio/sfx/rayder
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
SEMILLA = 20261021  # semilla fija -> resultado reproducible
PICO_OBJETIVO_DB = -3.0   # margen de seguridad: nada llega a 0 dBFS

rng = np.random.default_rng(SEMILLA)


# ---------------------------------------------------------------------------
# UTILIDADES BASICAS
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
# ENVOLVENTES (la "forma" del volumen en el tiempo)
# ---------------------------------------------------------------------------

def env_percusiva(n, ataque=0.002, caida=0.08, curva=2.0):
    """
    Envolvente de golpe: sube casi instantaneo y cae exponencial.
    'curva' alto = cae mas rapido al principio (mas seco).
    """
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
    """Rampa con curvatura (para cargas de energia que van subiendo)."""
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
# CAPA 1 : TRANSITORIO (el golpe seco del inicio)
# ---------------------------------------------------------------------------

def capa_transitorio(n, bajo=1200.0, alto=9000.0, caida=0.012, nivel=1.0):
    """
    Chasquido corto: ruido filtrado en agudos con caida muy rapida.
    Es la capa que hace que el golpe "suene a impacto" y no a zumbido.
    """
    x = pasa_banda(ruido(n), bajo, alto)
    return x * env_percusiva(n, ataque=0.0005, caida=caida, curva=3.0) * nivel


# ---------------------------------------------------------------------------
# CAPA 2 : CUERPO (la parte con tono / el peso del golpe)
# ---------------------------------------------------------------------------

def capa_cuerpo(n, f_ini, f_fin, caida=0.09, nivel=1.0, forma="seno", armonico=0.0):
    """
    Oscilador cuyo tono baja de f_ini a f_fin (barrido descendente).
    Esa caida de tono es lo que el oido lee como "masa" o "fuerza".
    'armonico' agrega un parcial agudo para darle garra.
    """
    t = tiempo(n)
    # Barrido exponencial de frecuencia: integramos la frecuencia para la fase.
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
    """
    Ruido con el filtro barriendo de grave a agudo (o al reves).
    Sirve para silbidos de brazo y para el desplazamiento de la esquiva.
    """
    x = ruido(n)
    # Se hace por bloques: mover el filtro muestra a muestra seria lentisimo.
    bloques = 48
    paso = max(1, n // bloques)
    salida = np.zeros(n)
    centros = np.geomspace(max(f_ini, 60.0), max(f_fin, 60.0), bloques)
    for i, fc in enumerate(centros):
        a = i * paso
        b = n if i == bloques - 1 else min(n, a + paso)
        if b <= a:
            continue
        # Se filtra un trozo algo mayor para que no se oigan los cortes.
        a2 = max(0, a - paso)
        trozo = pasa_banda(x[a2:b], fc * (1.0 - ancho), fc * (1.0 + ancho), orden=2)
        salida[a:b] = trozo[a - a2:]
    return salida * nivel


# ---------------------------------------------------------------------------
# CAPA 3 : COLA (reverberacion corta, el "aire" del golpe)
# ---------------------------------------------------------------------------

def impulso_reverb(duracion=0.14, predelay=0.006, brillo=5000.0):
    """
    Respuesta al impulso sintetica: ruido que decae exponencial.
    Convolucionar un sonido con esto = ponerlo dentro de una sala pequena.
    """
    n = n_muestras(duracion)
    t = tiempo(n)
    ir = ruido(n) * np.exp(-7.0 * t / max(duracion, 1e-4))
    ir = pasa_bajo(ir, brillo)          # la sala se come los agudos
    ir = pasa_alto(ir, 180.0)           # y tampoco deja retumbe grave
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
    """
    Convierte el sonido mono en estereo "ancho".

    Truco habitual (y honesto): cada canal recibe el mismo material seco,
    pero una cola de reverberacion DISTINTA y un microretardo de menos de
    1 ms en un lado. El oido lo interpreta como amplitud espacial.
    Esto NO es 3D ni HRTF: es anchura estereo.
    """
    izq = x.copy()
    der = np.roll(x, n_muestras(retardo_ms / 1000.0))
    der[:n_muestras(retardo_ms / 1000.0)] = 0.0

    # Mezcla M/S: 'ancho' controla cuanto se separan los canales.
    medio = 0.5 * (izq + der)
    lado = 0.5 * (izq - der)
    izq = medio + ancho * lado
    der = medio - ancho * lado

    # Cola distinta por canal (decorrelada) = sensacion envolvente.
    if cola_mezcla > 0.0:
        izq = anade_cola(izq, cola_mezcla, cola_dur, cola_brillo, 0.005)
        der = anade_cola(der, cola_mezcla, cola_dur * 1.12, cola_brillo * 0.92, 0.008)

    return np.stack([izq, der], axis=1)


def compacta(x, empuje=1.0):
    """
    Compacta el rango dinamico con una saturacion suave (tanh).

    Por que: los golpes sintetizados tienen un pico enorme y muy corto, asi
    que al normalizar a -3 dBFS quedan MAS BAJOS de lo que suena el resto
    del juego (los .wav que ya existen en assets/audio/sfx/ andan en torno
    a -12 dB RMS). El tanh redondea ese pico y sube el cuerpo, de modo que
    el sonido "pesa" lo mismo que los antiguos sin pasarse de 0 dBFS.
    'empuje' = cuanta compactacion; 1.0 es casi nada, 6.0 es bastante.
    """
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
    Punetazo del combo. Seco y corto (~0.14 s).
    Capas: transitorio agudo + cuerpo grave que cae + cola muy corta.
    La variante 'indice' (0,1,2) cambia tono y duracion un poco para que
    al encadenar el combo no suene tres veces lo mismo.
    """
    # Cada variante parte de un tono base distinto.
    base = [205.0, 230.0, 185.0][indice]
    dur = variacion([0.135, 0.145, 0.150][indice], 0.05)
    n = n_muestras(dur)

    t_click = capa_transitorio(n, bajo=variacion(1800, 0.15), alto=variacion(9500, 0.1),
                               caida=variacion(0.010, 0.2), nivel=0.85)
    cuerpo = capa_cuerpo(n, f_ini=variacion(base, 0.06), f_fin=variacion(base * 0.42, 0.08),
                         caida=variacion(0.045, 0.12), nivel=1.0, armonico=0.28)
    # Un golpecito de piel/tela para que no suene sintetico.
    tela = pasa_banda(ruido(n), 350, 2400) * env_percusiva(n, 0.001, 0.025, 2.4) * 0.45

    return t_click + cuerpo + tela, dur


def patada(indice):
    """
    Patada. Mas grave y un pelin mas larga que el punetazo (~0.17 s).
    Capas: silbido corto de la pierna + transitorio + cuerpo muy grave.
    """
    base = [128.0, 142.0, 116.0][indice]
    dur = variacion([0.165, 0.175, 0.185][indice], 0.05)
    n = n_muestras(dur)

    # Arranque: el aire de la pierna justo antes del impacto.
    aire = capa_zumbido(n, 700, 2600, ancho=0.45, nivel=0.5)
    aire *= env_ad(n, 0.004, 0.030, forma=2.5)

    t_click = capa_transitorio(n, bajo=variacion(900, 0.15), alto=variacion(6500, 0.1),
                               caida=variacion(0.014, 0.2), nivel=0.8)
    cuerpo = capa_cuerpo(n, f_ini=variacion(base, 0.06), f_fin=variacion(base * 0.38, 0.08),
                         caida=variacion(0.060, 0.12), nivel=1.1,
                         forma="triangulo", armonico=0.2)

    return aire + t_click + cuerpo, dur


def impacto_fuerte():
    """
    Ultimo golpe del combo. Mas peso, mas cola (~0.30 s).
    Capas: transitorio ancho + cuerpo doble (grave + subgrave) + crujido + cola larga.
    """
    dur = 0.30
    n = n_muestras(dur)

    t_click = capa_transitorio(n, bajo=700, alto=8000, caida=0.020, nivel=1.0)
    cuerpo = capa_cuerpo(n, 150.0, 48.0, caida=0.090, nivel=1.2, forma="triangulo", armonico=0.3)
    sub = capa_cuerpo(n, 80.0, 34.0, caida=0.130, nivel=0.9, forma="seno")
    crujido = pasa_banda(ruido(n), 1200, 5000) * env_percusiva(n, 0.001, 0.045, 2.0) * 0.5

    return t_click + cuerpo + sub + crujido, dur


def silbido():
    """
    El brazo cortando el aire, SIN impacto (~0.20 s).
    Capa unica trabajada: ruido con el filtro barriendo hacia arriba y
    luego hacia abajo (efecto doppler barato), sin transitorio seco.
    """
    dur = 0.20
    n = n_muestras(dur)
    mitad = n // 2

    sube = capa_zumbido(mitad, 500, 3200, ancho=0.5, nivel=1.0)
    baja = capa_zumbido(n - mitad, 3200, 700, ancho=0.5, nivel=1.0)
    x = np.concatenate([sube, baja])
    # Envolvente en campana: entra y sale suave, nunca golpea.
    x *= np.sin(np.linspace(0.0, np.pi, n)) ** 1.4
    return x * 0.9, dur


def carga_poder():
    """
    Subida de energia antes de lanzar el poder (~0.85 s).
    Capas: tono que SUBE de frecuencia + ruido electrico filtrado que
    se abre + un temblor (tremolo) que se acelera.
    """
    dur = 0.85
    n = n_muestras(dur)
    t = tiempo(n)

    # Tono ascendente (al reves que un golpe: aqui la energia se acumula).
    frec = np.geomspace(120.0, 780.0, n)
    fase = 2.0 * np.pi * np.cumsum(frec) / SR
    tono = (np.sin(fase) + 0.35 * np.sin(2.0 * fase) + 0.18 * np.sin(3.01 * fase))

    # Tremolo que se acelera: da sensacion de carga a punto de reventar.
    vel = np.geomspace(7.0, 42.0, n)
    tremolo = 0.72 + 0.28 * np.sin(2.0 * np.pi * np.cumsum(vel) / SR)

    # Chispas electricas: ruido agudo que va apareciendo.
    chispas = pasa_banda(ruido(n), 2500, 11000) * env_rampa(n, 0.0, 0.55, curva=3.0)

    env = env_rampa(n, 0.02, 1.0, curva=1.8)
    # Cierre rapido al final para que enlace con el disparo.
    env *= np.clip(np.linspace(1.3, 0.0, n), 0.0, 1.0) ** 0.35

    return (tono * 0.55 * tremolo + chispas) * env, dur


def disparo_poder():
    """
    Lanzar el proyectil (~0.28 s).
    Capas: chasquido de salida + cuerpo descendente rapido + estela de ruido
    que se va hacia los agudos (la bola alejandose).
    """
    dur = 0.28
    n = n_muestras(dur)

    salida = capa_transitorio(n, bajo=1500, alto=11000, caida=0.008, nivel=0.9)
    cuerpo = capa_cuerpo(n, 900.0, 160.0, caida=0.060, nivel=1.0, forma="sierra", armonico=0.0)
    cuerpo = pasa_bajo(cuerpo, 6000)
    estela = capa_zumbido(n, 1200, 5200, ancho=0.4, nivel=0.65)
    estela *= env_ad(n, 0.006, 0.110, forma=1.4)

    return salida + cuerpo * 0.8 + estela, dur


def impacto_poder():
    """
    El proyectil pega en el enemigo (~0.40 s).
    Capas: estallido de ruido + cuerpo grave + componente electrico
    (modulacion en anillo) + cola mas larga que un golpe normal.
    """
    dur = 0.40
    n = n_muestras(dur)
    t = tiempo(n)

    estallido = pasa_banda(ruido(n), 400, 9000) * env_percusiva(n, 0.0008, 0.055, 2.2)
    cuerpo = capa_cuerpo(n, 190.0, 45.0, caida=0.110, nivel=1.1, forma="triangulo", armonico=0.25)
    # Modulacion en anillo: dos senos multiplicados = timbre metalico/electrico.
    electrico = (np.sin(2 * np.pi * 620 * t) * np.sin(2 * np.pi * 173 * t))
    electrico *= env_percusiva(n, 0.002, 0.090, 1.5) * 0.45

    return estallido + cuerpo + electrico, dur


def furia():
    """
    Activar la FURIA (~0.95 s).
    Capas: golpe grave de arranque + acorde grave que sube + rugido de
    ruido filtrado + cola larga. Es el sonido mas largo del paquete.
    """
    dur = 0.95
    n = n_muestras(dur)
    t = tiempo(n)

    # Arranque: un impacto que abre el efecto.
    arranque = (capa_transitorio(n, 300, 7000, caida=0.030, nivel=0.9)
                + capa_cuerpo(n, 180.0, 55.0, caida=0.120, nivel=1.0, forma="triangulo"))

    # Acorde grave ascendente (quinta): suena "poderoso" sin ser musical.
    acorde = np.zeros(n)
    for mult, nivel in ((1.0, 1.0), (1.5, 0.6), (2.0, 0.4)):
        frec = np.geomspace(70.0 * mult, 185.0 * mult, n)
        acorde += nivel * signal.sawtooth(2 * np.pi * np.cumsum(frec) / SR, width=0.5)
    acorde = pasa_bajo(acorde, 3500) * env_rampa(n, 0.05, 1.0, curva=1.5) * 0.45

    # Rugido: ruido grave con el filtro abriendose.
    rugido = capa_zumbido(n, 150, 1800, ancho=0.6, nivel=0.5)
    rugido *= env_rampa(n, 0.0, 1.0, curva=2.2)

    # Cierre del final para que no quede cortado.
    cierre = np.clip(np.linspace(1.25, 0.0, n), 0.0, 1.0) ** 0.5

    return (arranque + acorde + rugido) * cierre, dur


def esquiva():
    """
    Desplazamiento rapido / dash (~0.24 s).
    Capas: golpe de aire corto + barrido de ruido hacia abajo + un toque
    de tono grave para dar empuje. Sin impacto duro.
    """
    dur = 0.24
    n = n_muestras(dur)

    empuje = capa_cuerpo(n, 320.0, 70.0, caida=0.050, nivel=0.55, forma="seno")
    aire = capa_zumbido(n, 2800, 600, ancho=0.55, nivel=1.0)
    aire *= env_ad(n, 0.008, 0.075, forma=1.6)
    roce = pasa_banda(ruido(n), 3000, 9000) * env_percusiva(n, 0.004, 0.030, 2.0) * 0.3

    return aire + empuje + roce, dur


# ---------------------------------------------------------------------------
# PROGRAMA PRINCIPAL
# ---------------------------------------------------------------------------

# Cada entrada: nombre de archivo -> (funcion, ancho estereo, mezcla de cola,
#                                     duracion de la cola, brillo de la cola,
#                                     empuje de compactado)
# El 'empuje' se ajusto comparando el RMS resultante con el de los .wav que
# ya estaban en assets/audio/sfx/ (los golpes del juego rondan -12 dB RMS).
RECETAS = [
    ("golpe_1",        lambda: golpe(0),   0.30, 0.14, 0.10, 5500.0, 6.0),
    ("golpe_2",        lambda: golpe(1),   0.34, 0.14, 0.10, 6000.0, 7.0),
    ("golpe_3",        lambda: golpe(2),   0.28, 0.15, 0.11, 5000.0, 6.5),
    ("patada_1",       lambda: patada(0),  0.32, 0.16, 0.12, 4800.0, 5.5),
    ("patada_2",       lambda: patada(1),  0.36, 0.16, 0.12, 5200.0, 6.0),
    ("patada_3",       lambda: patada(2),  0.30, 0.17, 0.13, 4500.0, 6.5),
    ("impacto_fuerte", impacto_fuerte,     0.42, 0.26, 0.20, 4200.0, 7.0),
    ("silbido",        silbido,            0.50, 0.12, 0.09, 7000.0, 3.5),
    ("carga_poder",    carga_poder,        0.45, 0.18, 0.16, 6000.0, 3.0),
    ("disparo_poder",  disparo_poder,      0.44, 0.20, 0.15, 6500.0, 6.0),
    ("impacto_poder",  impacto_poder,      0.46, 0.30, 0.24, 4800.0, 7.0),
    ("furia",          furia,              0.48, 0.28, 0.26, 4000.0, 4.0),
    ("esquiva",        esquiva,            0.46, 0.14, 0.11, 6500.0, 4.5),
]


def main():
    if len(sys.argv) < 2:
        print("Uso: python3 -I genera_sonidos_rayder.py <carpeta_de_salida>")
        return 1
    carpeta = os.path.abspath(sys.argv[1])
    os.makedirs(carpeta, exist_ok=True)

    print("Generando sonidos de RAYDER en:", carpeta)
    print("Semilla:", SEMILLA, "| pico objetivo:", PICO_OBJETIVO_DB, "dBFS")
    for nombre, receta, ancho, cola, cola_dur, brillo, empuje in RECETAS:
        mono, dur = receta()
        ruta = termina(nombre, mono, carpeta, ancho=ancho, cola=cola,
                       cola_dur=cola_dur, cola_brillo=brillo, empuje=empuje)
        print("  OK  %-16s %.3f s  -> %s" % (nombre, dur, os.path.basename(ruta)))
    print("Listo: %d archivos." % len(RECETAS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
