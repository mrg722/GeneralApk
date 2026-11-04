#!/usr/bin/env python3
"""Escribe los juegos de clips de MAX (skin 6) en data/sprite_manifest.json.

Uso (desde la raiz del repo): python3 tools/max/aplicar_manifiesto.py [--comprobar]

SOLO toca los juegos "max" y "max_white" (antes "rayder_kf_white", que nunca
cargaba porque SpriteManifest busca el juego con el id del atlas) y deja el resto
del archivo byte a byte igual (json.dumps(indent=2) reproduce el archivo).
No lee ni escribe ningun PNG.

Escala coherente: el atlas mezcla tres sets dibujados a distinto tamano. La
escala por cuadro ("scales") sale del set de cada celda, medida a ojo y con
tools/max/medir_celdas.py comparando cabeza y torso con el reposo (celda 0):
  - 0-152  chaqueta (reposo, caminar, golpes 1-2, aire, agachado, energia): 1.00
  - 206-244 set pequeno (transformacion, victoria): 1.15
  - 245-285 set grande "camiseta + cinta azul" (punch3, kick, energy, dash...): 0.64
  - 286-303 set del abrigo (giros con rayo, derrota con sangre): 1.05
La decision de ropa (chaqueta o camiseta) queda abierta para el usuario.
PROHIBIDO 153-203 (otro personaje) y la celda 142 de max_white (texto). Unica
excepcion: el clip "projectile" (176-180), que ya existia y son solo orbes de
energia (sin personaje; lo usa el dibujo de la onda).
"""
import json
import sys

MANIFEST = "data/sprite_manifest.json"
FORBIDDEN = range(153, 204)
WHITE_CELLS = 14 * 18   # 252


def set_scale(i, white=False):
    if white:
        return 1.15 if 206 <= i <= 251 else 1.0
    if 245 <= i <= 285:
        return 0.64
    if 206 <= i <= 244:
        return 1.15
    if 286 <= i <= 303:
        return 1.05
    return 1.0


def F(n):
    return round(n / 60.0, 4)


# Clips nuevos o corregidos del juego "max": nombre -> (cuadros, duraciones, loop).
# Las duraciones de los ataques siguen S/A/R de su movimiento (Moves_Max.cpp): el
# cuadro de impacto cae dentro de la fase activa.
NEW_MAX = {
    # ---- estados ----
    "knockdown": ([102, 103, 104], [0.08, 0.10, 0.16], False),
    "defeat": ([299, 300, 301], [0.12, 0.16, 0.45], False),
    "getup": ([109, 108, 107, 110, 111], [0.08, 0.08, 0.08, 0.08, 0.08], False),
    "jump": ([21, 22, 23, 24], [0.05, 0.07, 0.08, 0.14], False),
    "jump_rise": ([21, 22, 23, 24], [0.05, 0.07, 0.08, 0.14], False),
    "jump_fall": ([27], [0.2], False),
    # ---- cadena J ----
    "punch_hook": ([122, 123, 124, 135], [0.06, 0.06, 0.09, 0.17], False),
    "media_luna": ([113, 53, 58], [F(9), F(6), F(16)], False),
    # ---- cadena K ----
    "kick_front": ([52, 118, 54], [F(7), F(5), F(10)], False),
    "kick2": ([51, 57, 56], [F(8), F(6), F(12)], False),
    "kick3": ([52, 53, 58], [F(11), F(6), F(18)], False),
    # ---- normales de comando ----
    "kick_launcher": ([56, 60, 77], [F(8), F(5), F(16)], False),
    "fwd_punch": ([121, 45, 46], [F(12), F(5), F(14)], False),
    "fwd_kick": ([27, 30, 33], [F(10), F(6), F(14)], False),
    "back_elbow": ([54, 55, 56], [F(6), F(4), F(12)], False),
    "spin_kick": ([52, 86, 77], [F(9), F(8), F(16)], False),
    "sweep": ([71, 75, 76], [F(7), F(6), F(16)], False),
    "uppercut": ([61, 63, 60], [F(6), F(3), F(21)], False),
    "stomp": ([22, 39, 40], [F(10), F(4), F(14)], False),
    "dash_kick": ([72, 74, 73], [F(4), F(10), F(14)], False),
    # ---- aereos ----
    "air_punch": ([35, 36], [F(4), F(14)], False),
    "air_punch2": ([37, 34], [F(4), F(14)], False),
    "air_kick": ([28, 30], [F(5), F(18)], False),
    "dive_kick": ([32, 33], [F(6), F(30)], False),
    "air_spike": ([23, 38], [F(8), F(19)], False),
    # ---- especiales y firmas ----
    "rising_uppercut": ([129, 130, 131, 132, 133], [F(3), F(5), F(7), F(10), F(12)], False),
    "hurricane": ([30, 31, 32, 34], [F(8), F(15), F(15), F(16)], False),
    "drill_kick": ([31, 86, 33], [F(10), F(14), F(18)], False),
    "signature1": ([126, 139, 140, 127], [F(12), F(10), F(12), F(14)], False),
    "signature2": ([130, 131, 132, 134], [F(14), F(10), F(12), F(16)], False),
    "signature3": ([143, 144, 145, 141], [F(16), F(8), F(10), F(18)], False),
    # ---- Fase 2 (agarres; los activa F2.1). NUNCA 181-188. ----
    "grab": ([114], [0.2], False),
    "grab_knee": ([54], [0.2], False),
    "throw_forward": ([47, 50], [0.1, 0.2], False),
    "throw_back": ([67], [0.25], False),
    "guard_counter": ([88, 46], [0.08, 0.2], False),
    "wakeup_attack": ([111, 75], [0.1, 0.2], False),
}

# Forma blanca: mismos cuadros cuando la celda existe en max_white (< 148: la
# parte alta de esa hoja es otra disposicion) y no es 142; los demas caen al
# atlas base con tinte (lo hace el motor F0.3).
NEW_WHITE_OVERRIDE = {
    "defeat": ([102, 103, 104, 105], [0.1, 0.12, 0.14, 0.4], False),
    "victory": ([143, 144], [0.25, 0.6], False),
    "turn": ([0, 1], [0.06, 0.06], False),
    "landing": ([39, 40, 41, 42], [0.05, 0.05, 0.05, 0.05], False),
}


def clip(frames, durs, loop, white=False):
    assert len(frames) == len(durs)
    for f in frames:
        assert f not in FORBIDDEN, f
        if white:
            assert f != 142 and f < WHITE_CELLS, f
    c = {"frames": list(frames), "durations": [round(d, 4) for d in durs], "loop": loop}
    sc = [set_scale(f, white) for f in frames]
    if any(abs(s - 1.0) > 1e-6 for s in sc):
        c["scales"] = sc
    return c


def build(m):
    clips = m["clips"]
    base = clips["max"]
    # 1) escala coherente en los clips que ya existian
    for name, c in base.items():
        sc = [set_scale(f) for f in c["frames"]]
        c.pop("scales", None)
        if any(abs(s - 1.0) > 1e-6 for s in sc):
            c["scales"] = sc
    # 2) clips nuevos / corregidos
    for name, (fr, du, lp) in NEW_MAX.items():
        base[name] = clip(fr, du, lp)

    # 3) forma blanca: renombrar el juego conservando su posicion en el archivo
    old = clips.get("rayder_kf_white")
    white = clips.get("max_white")
    if old is not None:
        rebuilt = {}
        for k, v in clips.items():
            rebuilt["max_white" if k == "rayder_kf_white" else k] = v
        m["clips"] = clips = rebuilt
        white = clips["max_white"]
    for name, c in white.items():
        c["frames"] = [f for f in c["frames"]]
        sc = [set_scale(f, True) for f in c["frames"]]
        c.pop("scales", None)
        if any(abs(s - 1.0) > 1e-6 for s in sc):
            c["scales"] = sc
    for name, (fr, du, lp) in NEW_MAX.items():
        if name in NEW_WHITE_OVERRIDE:
            continue
        if all(f < 148 and f != 142 for f in fr):
            white[name] = clip(fr, du, lp, True)
    for name, (fr, du, lp) in NEW_WHITE_OVERRIDE.items():
        white[name] = clip(fr, du, lp, True)
    return m


def check(m):
    errs = []
    for set_name, white in (("max", False), ("max_white", True)):
        s = m["clips"].get(set_name)
        if s is None:
            errs.append("falta el juego " + set_name)
            continue
        for n, c in s.items():
            for f in c["frames"]:
                if f in FORBIDDEN and n != "projectile":
                    errs.append("%s.%s usa %d (prohibido 153-203)" % (set_name, n, f))
                if white and (f == 142 or f >= WHITE_CELLS):
                    errs.append("%s.%s usa %d" % (set_name, n, f))
    if "rayder_kf_white" in m["clips"]:
        errs.append("sigue el juego rayder_kf_white")
    return errs


def main():
    text = open(MANIFEST).read()
    m = json.loads(text)
    if "--comprobar" in sys.argv:
        errs = check(m)
        print("\n".join(errs) if errs else "manifiesto de MAX: ok")
        return 1 if errs else 0
    m = build(m)
    errs = check(m)
    if errs:
        print("\n".join(errs))
        return 1
    with open(MANIFEST, "w") as f:
        f.write(json.dumps(m, indent=2, ensure_ascii=True) + "\n")
    print("escrito", MANIFEST, "max:", len(m["clips"]["max"]), "clips, max_white:", len(m["clips"]["max_white"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
