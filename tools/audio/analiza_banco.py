#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Analizador espectral del banco de sonidos de District Fury (BRIEF F1.11).

QUE MIDE (por archivo .wav)
    dur   duracion en ms
    pk    pico en dBFS
    rms   RMS total en dBFS
    st50  RMS maximo en una ventana de 50 ms (paso 5 ms): "volumen de golpe"
    clip  muestras recortadas (|x| >= 0.999)
    lead  silencio inicial en ms (primera muestra con |x| >= -40 dBFS)
    cen   centroide espectral en Hz
    bandas  % de energia por debajo de 120 Hz, 120-500 Hz, 500 Hz-3 kHz y > 3 kHz

QUE EXIGE (sale con codigo 1 si algo falla)
    - formato: mono, 44100 Hz, 16 bit
    - 0 muestras recortadas y pico <= -1 dBFS
    - silencio inicial <= 2 ms
    - regla movil: >= 25 % de la energia entre 500 Hz y 3 kHz (cuerpo audible en
      el altavoz de un telefono). Se aplica a TODOS los archivos analizados.
    - spread de st50 dentro de cada categoria <= 4 dB (categoria = carpeta +
      nombre sin el sufijo _N; p.ej. combat/hit_light)

USO
    python3 tools/audio/analiza_banco.py <carpeta|archivo.wav> [...]
    Opciones: --md  imprime la tabla en Markdown (para el informe)
              --sin-regla-movil  solo informa la regla movil (para comparar
                                 con los WAV viejos; no se usa en la aceptacion)
"""

import glob
import os
import re
import sys
import wave

import numpy as np

SR_ESPERADO = 44100
BANDAS = [(0, 120), (120, 500), (500, 3000), (3000, 1e9)]
MIN_MEDIOS = 25.0      # % minimo en 500-3k
MAX_PICO_DB = -1.0
MAX_LEAD_MS = 2.0
MAX_SPREAD_DB = 4.0
UMBRAL_LEAD = 10 ** (-40 / 20.0)


def db(x):
    return 20.0 * np.log10(max(float(x), 1e-12))


def lee_wav(ruta):
    with wave.open(ruta, "rb") as w:
        nch, sr, sw, n = w.getnchannels(), w.getframerate(), w.getsampwidth(), w.getnframes()
        datos = w.readframes(n)
    if sw != 2:
        return None, nch, sr, sw
    x = np.frombuffer(datos, dtype="<i2").astype(np.float64) / 32768.0
    if nch > 1:
        x = x.reshape(-1, nch).mean(axis=1)
    return x, nch, sr, sw


def analiza(ruta):
    x, nch, sr, sw = lee_wav(ruta)
    r = {"ruta": ruta, "nch": nch, "sr": sr, "sw": sw, "errores": []}
    if x is None or len(x) == 0:
        r["errores"].append("formato no legible (16 bit requerido)")
        return r
    n = len(x)
    r["dur"] = 1000.0 * n / sr
    pico = np.max(np.abs(x))
    r["pk"] = db(pico)
    r["rms"] = db(np.sqrt(np.mean(x * x)))
    win, hop = int(0.05 * sr), int(0.005 * sr)
    if n <= win:
        st = np.sqrt(np.mean(x * x))
    else:
        c = np.concatenate([[0.0], np.cumsum(x * x)])
        idx = np.arange(0, n - win + 1, hop)
        st = np.sqrt(np.max((c[idx + win] - c[idx]) / win))
    r["st50"] = db(st)
    r["clip"] = int(np.sum(np.abs(x) >= 0.999))
    sobre = np.nonzero(np.abs(x) >= UMBRAL_LEAD)[0]
    r["lead"] = 1000.0 * (sobre[0] if len(sobre) else n) / sr
    X = np.abs(np.fft.rfft(x)) ** 2
    f = np.fft.rfftfreq(n, 1.0 / sr)
    tot = max(np.sum(X), 1e-20)
    r["cen"] = float(np.sum(f * X) / tot)
    r["bandas"] = [100.0 * np.sum(X[(f >= a) & (f < b)]) / tot for a, b in BANDAS]
    return r


def categoria(ruta, raices):
    base = os.path.splitext(os.path.basename(ruta))[0]
    base = re.sub(r"_\d+$", "", base)
    carpeta = os.path.dirname(os.path.abspath(ruta))
    rel = os.path.basename(carpeta)
    padre = os.path.basename(os.path.dirname(carpeta))
    if padre == "arquetipos":
        rel = "arquetipos/" + rel
    return rel + "/" + base


def main(argv):
    md = "--md" in argv
    regla_movil = "--sin-regla-movil" not in argv
    entradas = [a for a in argv if not a.startswith("--")]
    if not entradas:
        print(__doc__)
        return 2
    archivos = []
    for e in entradas:
        if os.path.isdir(e):
            archivos += sorted(glob.glob(os.path.join(e, "**", "*.wav"), recursive=True))
        elif e.lower().endswith(".wav"):
            archivos.append(e)
    if not archivos:
        print("sin archivos .wav")
        return 1

    filas, fallos, cats, total_bytes = [], 0, {}, 0
    for ruta in archivos:
        r = analiza(ruta)
        total_bytes += os.path.getsize(ruta)
        err = r["errores"]
        if "dur" in r:
            if r["nch"] != 1:
                err.append("no es mono (%d canales)" % r["nch"])
            if r["sr"] != SR_ESPERADO:
                err.append("sr %d" % r["sr"])
            if r["clip"] > 0:
                err.append("recorta %d muestras" % r["clip"])
            if r["pk"] > MAX_PICO_DB:
                err.append("pico %.2f dBFS > %.1f" % (r["pk"], MAX_PICO_DB))
            if r["lead"] > MAX_LEAD_MS:
                err.append("silencio inicial %.2f ms" % r["lead"])
            if regla_movil and r["bandas"][2] < MIN_MEDIOS:
                err.append("regla movil: %.1f %% en 0.5-3k < %.0f %%" % (r["bandas"][2], MIN_MEDIOS))
            cats.setdefault(categoria(ruta, entradas), []).append(r["st50"])
        if err:
            fallos += 1
        filas.append(r)

    if md:
        print("| Archivo | dur | pk | rms | st50 | clip | lead | cen | <120 / 120-500 / 0.5-3k / >3k | estado |")
        print("|---|---|---|---|---|---|---|---|---|---|")
    else:
        print("%-46s %6s %6s %6s %6s %4s %5s %6s  %s" % (
            "archivo", "dur", "pk", "rms", "st50", "clip", "lead", "cen", "<120/120-500/0.5-3k/>3k"))
    for r in filas:
        nombre = os.path.relpath(r["ruta"])
        nombre = nombre.replace("assets/audio/sfx/", "")
        if "dur" not in r:
            print(nombre, "ERROR", "; ".join(r["errores"]))
            continue
        b = r["bandas"]
        estado = "OK" if not r["errores"] else "FALLA: " + "; ".join(r["errores"])
        if md:
            print("| %s | %.0f | %.1f | %.1f | %.1f | %d | %.2f | %.1fk | %.0f / %.0f / **%.0f** / %.0f | %s |" % (
                nombre, r["dur"], r["pk"], r["rms"], r["st50"], r["clip"], r["lead"], r["cen"] / 1000,
                b[0], b[1], b[2], b[3], estado))
        else:
            print("%-46s %6.0f %6.1f %6.1f %6.1f %4d %5.2f %5.1fk  %3.0f/%3.0f/%3.0f/%3.0f  %s" % (
                nombre, r["dur"], r["pk"], r["rms"], r["st50"], r["clip"], r["lead"], r["cen"] / 1000,
                b[0], b[1], b[2], b[3], estado))

    print()
    print("spread de st50 por categoria (objetivo <= %.0f dB):" % MAX_SPREAD_DB)
    peor = 0.0
    for c in sorted(cats):
        v = cats[c]
        sp = max(v) - min(v)
        peor = max(peor, sp)
        marca = "" if sp <= MAX_SPREAD_DB else "  <-- FALLA"
        if sp > MAX_SPREAD_DB:
            fallos += 1
        print("  %-40s n=%2d  st50 %6.1f..%6.1f  spread %4.1f dB%s" % (c, len(v), min(v), max(v), sp, marca))
    medios = [r["bandas"][2] for r in filas if "dur" in r]
    print()
    print("archivos: %d  tamano total: %.2f MB  peor spread: %.1f dB  minimo 0.5-3k: %.1f %%" % (
        len(filas), total_bytes / 1048576.0, peor, min(medios) if medios else 0.0))
    print("fallos: %d" % fallos)
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
