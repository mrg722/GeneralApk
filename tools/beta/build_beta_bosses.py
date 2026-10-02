"""BETA: prepara los jefes Spine 2.1 de nuevosSprites para el juego.

El juego dibuja los Spine en vivo (src/game/beta/Spine21.cpp) desde los archivos
originales, asi que aqui solo se copian (json + atlas + hojas png) a
assets/beta/spine/<Carpeta>/ y se escribe data/beta/bosses/<id>.txt con:
  * spine <carpeta> <archivo> <escala>
  * vida, caja de cuerpo del XML original (anim 51/64/15), sonidos de golpe/muerte
  * clips: nombre del juego -> animacion Spine (y si repite)
  * ataques: clip de aviso, aviso de area de anim 99 (AttackWarning1/2 de
    data/EnemyDate.lua) con su escala de getScale(), alcance AttakFont1/2,
    segundo del golpe = evento Spine attackEffect/skillEffect, efecto y sonido.

Escala y lugar: los de las capturas del juego original (Poseidon gigante sobre
la cubierta, el Titan asomando detras del escenario, tentaculos en el piso).
Uniforme: no deforma.
`tools/beta/spine21.py` es la misma logica en Python (vista previa offline).

Uso: python3 tools/beta/build_beta_bosses.py <carpeta nuevosSprites>
"""
import json
import shutil
import sys
from pathlib import Path

from beta_anim import parse

ROOT = Path(__file__).resolve().parents[2]

BOSSES = [
    # draw = (escala en pantalla, dx, dy desde la posicion del jefe en el carril, capa):
    #   capa 0 = en el piso con los demas (tentaculos), 1 = gigante delante del mapa
    #   (Poseidon sobre la cubierta), 2 = gigante detras del mapa (Titan asomando).
    #   Medidas tomadas de capturas del juego original (1280x720).
    dict(id="jefe_poseidon", name="POSEIDON", dir="Poseidon", file="Poseidon", scale=1.0, xml=64, hp=650,
         draw=(1.5, -100, 147, 1),
         behit="sound59", die="sound60",
         clips={"intro": ("stand", 0), "idle": ("stand", 1), "hit": ("behit", 0), "defeat": ("die4", 0),
                "down": ("lay", 1), "warn1": ("warningAttack", 0), "warn2": ("warningSkill", 0)},
         # (clip, aviso anim99, escala aviso, alcance AttakFont, efecto, sonido al golpear, dano)
         attacks=[("warn1", "a8", 0.8, 280, "fx_poseidon:a0", "sound59", 22),
                  ("warn2", "a9", 0.7, 320, "fx_poseidon:a2", "sound60", 28)]),
    dict(id="jefe_tentaculos", name="MONSTRUO DE TENTACULOS", dir="PoseidonBaby", file="ZS_chushou", scale=1.0, xml=15,
         draw=(1.25, 0, 0, 0),
         hp=600, behit="sound52", die="sound51",
         clips={"intro": ("chuchang", 0), "idle": ("stand", 1), "hit": ("behit", 0), "defeat": ("die5", 0),
                "down": ("lay", 1), "warn1": ("warningAttack", 0), "warn2": ("warningSkill", 0)},
         attacks=[("warn1", "a6", 1.0, 300, "fx_tentaculos:a0", "sound52", 20),
                  ("warn2", "a7", 1.0, 300, "fx_tentaculos:a2", "sound51", 26)]),
    dict(id="jefe_titan", name="TITAN", dir="Titan", file="Titan", scale=1.0, xml=51, hp=700,
         draw=(1.6, 334, 1078, 2),
         behit="sound56", die="sound55",
         clips={"intro": ("stand", 0), "idle": ("stand", 1), "hit": ("behit", 0), "defeat": ("die", 0),
                "down": ("lay", 1), "warn1": ("warningAttack", 0), "warn2": ("warningSkill", 0)},
         attacks=[("warn1", "a10", 1.0, 260, "fx_titan:a1", "sound56", 24),
                  ("warn2", "a11", 1.0, 470, "fx_titan:a3", "sound55", 32)]),
]


def build(ns, b):
    ns = Path(ns)
    src = ns / "spine" / b["dir"]
    dst = ROOT / "assets/beta/spine" / b["dir"]
    dst.mkdir(parents=True, exist_ok=True)
    for f in src.iterdir():
        if f.suffix in (".json", ".atlas", ".png"): shutil.copyfile(f, dst / f.name)
    data = json.loads((src / f"{b['file']}.json").read_text(encoding="utf-8"))
    xml = parse(ns / "anim" / f"{b['xml']}.xml")
    body = xml["frames"][xml["actions"][0]["seq"][0]["frameid"]]["body"]
    L = [f"# BETA jefe {b['name']} (generado por tools/beta/build_beta_bosses.py)",
         f"boss {b['id']}", f"name {b['name']}", f"spine assets/beta/spine/{b['dir']} {b['file']} {b['scale']}",
         f"hp {b['hp']}", "draw " + " ".join(map(str, b["draw"])), f"body {' '.join(map(str, body))}", f"sound hurt {b['behit']}", f"sound die {b['die']}"]
    for clip, (anim, loop) in b["clips"].items():
        assert anim in data["animations"], (b["id"], anim)
        L.append(f"clip {clip} {anim} {loop}")
    for clip, warn, wscale, reach, fx, snd, dmg in b["attacks"]:
        anim = b["clips"][clip][0]
        evs = [e["time"] for e in data["animations"][anim].get("events", []) if "Effect" in e["name"]]
        hit = ",".join(str(t) for t in evs) if evs else "0.7"
        L.append(f"attack {clip} {warn} {wscale} {reach} {hit} {fx} {snd} {dmg}")
    out = ROOT / "data/beta/bosses" / f"{b['id']}.txt"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(L) + "\n", encoding="utf-8")
    print(b["id"], "->", out.relative_to(ROOT))


if __name__ == "__main__":
    for b in BOSSES: build(sys.argv[1], b)


# ------------------------------------------------------------------ QTE
# Cinematicas de remate (spine/*QTE). Cada archivo trae uno o dos "fenjingdonghua"
# (vinetas). La vineta con evento chuanniu*/jiaoxue*/QTEchuxian muestra el boton;
# la otra animacion de ese archivo (numero mas alto) es la rama de fallo. Las
# demas se encadenan en orden numerico. data/EnemyDate.lua: elites y jefes con
# spineName; data/QteControl.lua: se activa con <=20% de vida, exito -10% de vida
# (aqui: remate), fallo +10%.
QTES = [("medusa", "medsuaQTE"), ("centauro", "RenMaQTE"), ("bruto", "ChainfattyQTE"),
        ("tentaculos", "PoseidonBabyQTE"), ("poseidon", "PoseidonQTE")]
PROMPTS = ("chuanniu", "jiaoxue", "QTEchuxian")


def num(name):
    import re
    m = re.search(r"(\d+)$", name)
    return int(m.group(1)) if m else 0


def build_qte(ns, key, folder):
    src = Path(ns) / "spine" / folder
    dst = ROOT / "assets/beta/spine/qte" / folder
    dst.mkdir(parents=True, exist_ok=True)
    for f in src.iterdir():
        if f.suffix in (".json", ".atlas", ".png"): shutil.copyfile(f, dst / f.name)
    main, fails = [], {}
    for jf in sorted(src.glob("*.json")):
        anims = json.loads(jf.read_text(encoding="utf-8"))["animations"]
        prompt = {a: next((e["time"] for e in v.get("events", []) if e["name"].startswith(PROMPTS)), -1)
                  for a, v in anims.items()}
        with_prompt = [a for a in anims if prompt[a] >= 0]
        for a in anims:
            if with_prompt and a not in with_prompt and num(a) == max(num(x) for x in anims) and len(anims) > 1:
                fails[with_prompt[0]] = a
            else:
                main.append((num(a), jf.stem, a, prompt[a]))
    L = [f"qte {key}", f"dir assets/beta/spine/qte/{folder}"]
    for _, file, a, p in sorted(main):
        L.append(f"seg {file} {a} {p} {fails.get(a, '-')}")
    out = ROOT / "data/beta/qte" / f"{key}.txt"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(L) + "\n", encoding="utf-8")
    print("qte", key, [(x[2], x[3]) for x in sorted(main)], "fallos", fails)


if __name__ == "__main__":
    for key, folder in QTES: build_qte(sys.argv[1], key, folder)
