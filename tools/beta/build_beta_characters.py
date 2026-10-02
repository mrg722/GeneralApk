#!/usr/bin/env python3
"""BETA: convierte los personajes de nuevosSprites al formato del juego.

No re-dibuja ni escala nada: cada cuadro se guarda como la lista de piezas
originales (imagen actor/N.png + recorte + posicion + espejo) y el juego lo
arma al dibujar, igual que el juego original. Se conservan las cajas de golpe
(ax) y de cuerpo (cx) de cada cuadro y los tiempos (1 unidad = 1/30 s).

Salidas:
  assets/beta/actor/N.png              imagenes originales usadas (copia exacta)
  data/beta/characters/<id>.txt        cuadros, piezas, cajas, clips, habilidades, sonidos
Uso: python3 tools/beta/build_beta_characters.py <carpeta nuevosSprites>
"""
import shutil
import sys
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from beta_anim import parse  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
TICK = 1.0 / 30.0

# Sonidos (data/GameMusicFile.lua). Golpes del combo: sound61..65; puño Nemea 73/74;
# Espadas del Caos 75. Recibir dano 36, morir 34, voltereta 70.
HERO_SOUNDS = {"hurt": "sound36", "die": "sound34", "dash": "sound70"}


def heroes():
    # data/HeroAction.lua -> HeroProperty: BasicXML, InterXML1, InterXML2, SkillXML, UltimateSkillXML
    base = [("kratos_espadas", "ESPADAS DEL CAOS", 0, 5, 1, 86, ["sound61", "sound62", "sound63", "sound64"], "sound75"),
            ("kratos_cesto", "CESTUS DE NEMEA", 9, 13, 87, 88, ["sound73", "sound73", "sound74", "sound64"], None),
            ("kratos_rayo", "CADENA DEL RAYO", 17, 32, 105, 106, ["sound61", "sound62", "sound63", "sound64"], "sound3"),
            ("kratos_garras", "GARRAS DE HADES", 34, 36, 107, 108, ["sound61", "sound62", "sound63", "sound64"], "sound4")]
    return [dict(id=i, name=f"GUERRERO ({w})", kind="hero", anims=[b, j, s, u], hp=160, hit=hits, weapon=ws)
            for i, w, b, j, s, u, hits, ws in base]


# Enemigos (anim XML, nombre en espanol, sonidos de golpe/muerte de EnemysoudEffectDate)
ENEMIES = [
    ("esqueleto", "SOLDADO ESQUELETO", 3, 60, "sound67", "sound66"),
    ("excavador", "BESTIA EXCAVADORA", 4, 70, "sound67", "sound66"),
    ("momia_escudo", "MOMIA CON ESCUDO", 10, 80, "sound67", "sound66"),
    ("bestia_elefante", "BESTIA ELEFANTE", 11, 120, "sound68", "sound69"),
    ("ave_sanadora", "AVE SANADORA", 14, 50, "sound67", "sound66"),
    ("centauro", "CENTAURO", 35, 220, "sound39", "sound40"),
    ("centauro_rojo", "CENTAURO ROJO", 110, 300, "sound39", "sound40"),
    ("bruto_cadena", "BRUTO DE LA BOLA", 39, 240, "sound68", "sound69"),
    ("bruto_cadena_rojo", "BRUTO DE LA BOLA ROJO", 111, 320, "sound68", "sound69"),
    ("medusa", "MEDUSA", 45, 200, "sound31", "sound32"),
    ("medusa_roja", "MEDUSA ROJA", 112, 280, "sound31", "sound32"),
]


def find(actions, *keys, exclude=("框", "光效", "QTE", "剧情", "空")):
    """Primera accion cuyo nombre contiene todas las claves (sin las acciones auxiliares)."""
    for a in actions:
        n = a["name"]
        if all(k in n for k in keys) and not any(e in n for e in exclude):
            return a
    return None


def box_action(actions, visual, keys):
    """Accion auxiliar de cajas ("...框") del mismo largo que `visual`."""
    if visual is None: return None
    n = len(visual["seq"])
    cands = [a for a in actions if "框" in a["name"] and len(a["seq"]) == n]
    for a in cands:
        if any(k in a["name"] for k in keys): return a
    return cands[0] if cands else None


class Builder:
    def __init__(self, ns, cid):
        self.ns = Path(ns)
        self.cid = cid
        self.images = []          # actor ids, en orden
        self.img_index = {}
        self.frames = []          # (pieces, body, attack)
        self.frame_key = {}
        self.clips = {}           # name -> (loop, [(frame, dur)], sounds[(i, snd)])
        self.abilities = []
        self.sounds = {}
        self.sizes = {}

    def image(self, actor_id):
        if actor_id not in self.img_index:
            self.img_index[actor_id] = len(self.images)
            self.images.append(actor_id)
            p = self.ns / "actor" / f"{actor_id}.png"
            self.sizes[actor_id] = Image.open(p).size if p.exists() else (0, 0)
        return self.img_index[actor_id]

    def frame(self, anim, fid, extra_attack=None):
        fr = anim["frames"][fid]
        pieces = []
        for s in fr["sprites"]:
            mid = s.get("module_id", -1)
            if mid < 0 or mid >= len(anim["modules"]): continue
            m = anim["modules"][mid]
            if m["imageId"] >= len(anim["images"]) or m["w"] <= 0 or m["h"] <= 0: continue
            aid = anim["images"][m["imageId"]]["id"]
            gi = self.image(aid)
            iw, ih = self.sizes[aid]
            # recorte dentro de la imagen (los modulos a veces se salen un pixel)
            sx0, sy0 = max(0, m["x"]), max(0, m["y"])
            sx1, sy1 = min(iw, m["x"] + m["w"]), min(ih, m["y"] + m["h"])
            if sx1 <= sx0 or sy1 <= sy0: continue
            flip = s.get("flip", 0) & 3
            fx, fy = flip & 1, (flip >> 1) & 1
            # desplazamiento de la parte recortada dentro de la pieza (respetando el espejo)
            dx = (m["x"] + m["w"] - sx1) if fx else (sx0 - m["x"])
            dy = (m["y"] + m["h"] - sy1) if fy else (sy0 - m["y"])
            pieces.append((gi, sx0, sy0, sx1 - sx0, sy1 - sy0, s["x"] + dx, s["y"] + dy, flip))
        attack = fr["attack"] if any(fr["attack"]) else (extra_attack or [0, 0, 0, 0])
        key = (tuple(pieces), tuple(fr["body"]), tuple(attack))
        if key not in self.frame_key:
            self.frame_key[key] = len(self.frames)
            self.frames.append((pieces, fr["body"], attack))
        return self.frame_key[key]

    def clip(self, name, anim, actions, loop=False, sounds=None, box_keys=("红框", "攻击框")):
        """Concatena acciones (preparacion + ejecucion + fin) en un clip."""
        seq, snd = [], []
        actions = [a for a in actions if a is not None]
        if not actions: return False
        for a in actions:
            boxes = box_action(anim["actions"], a, box_keys) if not any(any(anim["frames"][s["frameid"]]["attack"]) for s in a["seq"] if s["frameid"] < len(anim["frames"])) else None
            for k, s in enumerate(a["seq"]):
                if s["frameid"] >= len(anim["frames"]): continue
                extra = None
                if boxes and k < len(boxes["seq"]) and boxes["seq"][k]["frameid"] < len(anim["frames"]):
                    bx = anim["frames"][boxes["seq"][k]["frameid"]]["attack"]
                    extra = bx if any(bx) else None
                seq.append((self.frame(anim, s["frameid"], extra), max(1, s["duration"]) * TICK))
        if not seq: return False
        for idx, snd_name in (sounds or []):
            i = idx if idx >= 0 else len(seq) + idx
            if 0 <= i < len(seq): snd.append((i, snd_name))
        self.clips[name] = (loop, seq, snd)
        return True

    def alias(self, name, src):
        if src in self.clips and name not in self.clips: self.clips[name] = self.clips[src]

    def first_hit_sound(self, name, snd):
        """Sonido en el primer cuadro con caja de golpe del clip."""
        if name not in self.clips or not snd: return
        loop, seq, sounds = self.clips[name]
        for i, (f, _) in enumerate(seq):
            if any(self.frames[f][2]):
                self.clips[name] = (loop, seq, sounds + [(i, snd)])
                return

    def write(self, name, hp, kind):
        out = ROOT / "data/beta/characters" / f"{self.cid}.txt"
        out.parent.mkdir(parents=True, exist_ok=True)
        L = [f"# BETA: {name} (generado por tools/beta/build_beta_characters.py)",
             f"character {self.cid} {kind} {hp}", f"name {name}"]
        for i, aid in enumerate(self.images): L.append(f"image {i} {aid}")
        for i, (pieces, body, attack) in enumerate(self.frames):
            L.append(f"frame {i} {len(pieces)} {' '.join(map(str, body))} {' '.join(map(str, attack))}")
            for p in pieces: L.append("piece " + " ".join(map(str, p)))
        for cname, (loop, seq, snd) in self.clips.items():
            L.append(f"clip {cname} {1 if loop else 0} {len(seq)} " + " ".join(f"{f}:{d:.4f}" for f, d in seq))
            for i, s in snd: L.append(f"clipsound {cname} {i} {s}")
        for clip, label in self.abilities: L.append(f"ability {clip} {label}")
        for ev, s in self.sounds.items(): L.append(f"sound {ev} {s}")
        out.write_text("\n".join(L) + "\n", encoding="utf-8")
        return out


def build_hero(ns, h):
    b = Builder(ns, h["id"])
    base, inter, skill, ult = (parse(Path(ns) / "anim" / f"{i}.xml") for i in h["anims"])
    A = base["actions"]
    b.clip("idle", base, [find(A, "待战")], loop=True)
    b.clip("walk", base, [find(A, "跑")], loop=True)
    b.alias("run", "walk")
    b.clip("dash", base, [find(A, "前翻")], sounds=[(0, "sound70")])
    b.clip("hit", base, [find(A, "被打")], sounds=[(0, "sound36")])
    for n in ("hit_high", "hit_low", "recovery"): b.alias(n, "hit")
    b.clip("airborne", base, [find(A, "击飞")])
    b.alias("air", "airborne")
    b.clip("knockdown", base, [find(A, "倒地")])
    b.clip("getup", base, [find(A, "起身")])
    b.clip("defeat", base, [find(A, "死亡1"), find(A, "死亡2")], sounds=[(0, "sound34")])
    b.clip("block", base, [find(A, "普功结束站立等待")], loop=True)
    b.clip("victory", base, [find(A, "休息")], loop=True)
    combo = [("punch1", "攻击一段"), ("punch2", "攻击二段"), ("punch3", "攻击三段"), ("kick", "攻击四段")]
    for (clip, key), snd in zip(combo, h["hit"]):
        b.clip(clip, base, [find(A, key)])
        b.first_hit_sound(clip, h["weapon"] or snd)
        b.first_hit_sound(clip, snd)
    for i, (clip, _) in enumerate(combo): b.alias(f"atk{i + 1}", clip)
    b.alias("dash_attack", "punch3")
    b.alias("finisher", "kick")
    # Habilidades (精灵号 SkillXML / UltimateSkillXML). Sonidos segun GameMusicFile.lua.
    S, U = skill["actions"], ult["actions"]
    b.clip("ab0", skill, [find(S, "①技能准备"), find(S, "①技能中"), find(S, "①技能结束")],
           sounds=[(5, "sound65"), (-15, "sound38")])
    b.clip("ab1", skill, [find(S, "②技能准备"), find(S, "②技能中"), find(S, "②技能结束")],
           sounds=[(0, "sound65")] + [(-k, "sound64") for k in (2, 4, 8, 16, 20, 24, 28)] + [(-20, "sound38")])
    b.clip("ab2", ult, [find(U, "③技能准备"), find(U, "③技能中"), find(U, "③技能结束")],
           sounds=[(0, "sound37"), (-22, "sound65")] + [(-k, "sound38") for k in (8, 16, 30, 36, 41, 47, 54)])
    b.clip("ab3", skill, [find(S, "技能连击用1")], sounds=[(0, "sound65")])
    b.clip("ab4", ult, [find(U, "技能连击用2")], sounds=[(0, "sound37")])
    b.abilities = [("ab0", "HABILIDAD A"), ("ab1", "HABILIDAD B"), ("ab2", "HABILIDAD C"),
                   ("ab3", "COMBO DE HABILIDAD 1"), ("ab4", "COMBO DE HABILIDAD 2")]
    b.alias("energy", "ab0"); b.alias("special", "ab1")
    for n in ("super", "rage_attack"): b.alias(n, "ab2")
    # Saltos (InterXML1): estado en el aire
    J = inter["actions"]
    if find(J, "跳跃一段-中"): b.clip("jump", inter, [find(J, "跳跃一段-中"), find(J, "跳跃一段-后")], sounds=[(0, "sound70")])
    b.sounds = dict(HERO_SOUNDS)
    return b.write(h["name"], h["hp"], "hero")


def build_enemy(ns, cid, name, anim_id, hp, behit, die):
    b = Builder(ns, cid)
    a = parse(Path(ns) / "anim" / f"{anim_id}.xml")
    A = a["actions"]
    b.clip("idle", a, [find(A, "待战") or find(A, "待机")], loop=True)
    b.clip("walk", a, [find(A, "移动")], loop=True)
    b.alias("run", "walk")
    b.alias("dash", "walk")
    b.clip("hit", a, [find(A, "被打")])
    for n in ("hit_high", "hit_low", "recovery"): b.alias(n, "hit")
    b.clip("airborne", a, [find(A, "击飞") or find(A, "被打起飞")])
    b.alias("air", "airborne")
    b.clip("knockdown", a, [find(A, "倒地")])
    b.clip("getup", a, [find(A, "起身")])
    b.clip("defeat", a, [find(A, "死亡1"), find(A, "死亡2"), find(A, "死亡3")])
    if "defeat" not in b.clips: b.alias("defeat", "knockdown")
    b.clip("victory", a, [find(A, "休息")], loop=True)
    b.clip("intro", a, [find(A, "出场")])
    # Ataque cuerpo a cuerpo: preparacion + ejecucion + fin (cajas desde "攻击框" si el dibujo no las trae)
    melee = [find(A, "近身攻击准备") or find(A, "攻击准备"), find(A, "攻击中"), find(A, "攻击结束")]
    b.clip("punch1", a, melee)
    second = [find(A, "攻击准备2"), find(A, "攻击中2"), find(A, "攻击结束2")]
    if not b.clip("kick", a, second): b.alias("kick", "punch1")
    skill = [find(A, "技能准备"), find(A, "技能中"), find(A, "技能结束")]
    ranged = [find(A, "远程准备"), find(A, "远程攻击中"), find(A, "远程攻击结束")]
    has_skill = b.clip("special", a, skill, box_keys=("技能攻击框", "攻击框"))
    has_ranged = b.clip("energy", a, ranged, box_keys=("远程攻击框", "攻击框"))
    if find(A, "飞镖"): b.clip("projectile", a, [find(A, "飞镖")], loop=True)
    b.alias("punch2", "punch1"); b.alias("punch3", "kick")
    for i, n in enumerate(("punch1", "punch2", "punch3", "kick")): b.alias(f"atk{i + 1}", n)
    if not has_skill: b.alias("special", "kick")
    if not has_ranged: b.alias("energy", "special")
    for n in ("dash_attack", "rage_attack", "finisher", "super"): b.alias(n, "special")
    b.alias("block", "idle")
    b.abilities = [("punch1", "ATAQUE"), ("kick", "ATAQUE 2")]
    if has_skill: b.abilities.append(("special", "TECNICA"))
    if has_ranged: b.abilities.append(("energy", "ATAQUE A DISTANCIA"))
    b.sounds = {"hurt": behit, "die": die}
    return b.write(name, hp, "enemy")


# Efectos (anim XML): cada accion queda como clip a0..aN (a0 tambien es "idle").
FX = [
    ("fx_impacto", "CHISPA DE IMPACTO", 82), ("fx_impacto_oro", "CHISPA DE IMPACTO DORADA", 101),
    ("fx_impacto_morado", "CHISPA DE IMPACTO MORADA", 102), ("fx_impacto_azul", "CHISPA DE IMPACTO AZUL", 103),
    ("fx_humo_muerte", "HUMO DE MUERTE", 68), ("fx_humo_enemigo", "HUMO DE ENEMIGO VENCIDO", 46),
    ("fx_curacion", "LUZ DE CURACION", 52), ("fx_barco", "CLIMA DEL BARCO (LLUVIA/RAYOS/OLAS)", 61),
    ("fx_nieve", "NEVADA FUERTE", 53), ("fx_nieve_cueva", "NEVADA LIGERA", 54),
    ("fx_volcan", "LAVA Y FUEGO DEL VOLCAN", 49), ("fx_antorcha", "ANTORCHA", 66),
    ("fx_alma_roja", "ALMA ROJA", 67), ("fx_alma_verde", "ALMA VERDE", 70), ("fx_alma_azul", "ALMA AZUL", 71),
]


def build_fx(ns, cid, name, anim_id):
    b = Builder(ns, cid)
    a = parse(Path(ns) / "anim" / f"{anim_id}.xml")
    for i, act in enumerate(a["actions"]):
        b.clip(f"a{i}", a, [act])
    first = next(iter(b.clips), None)
    if first: b.alias("idle", first)
    return b.write(name, 0, "fx")


def copy_images(ns):
    used = set()
    for f in (ROOT / "data/beta/characters").glob("*.txt"):
        for line in f.read_text(encoding="utf-8").splitlines():
            if line.startswith("image "): used.add(int(line.split()[2]))
    dst = ROOT / "assets/beta/actor"
    dst.mkdir(parents=True, exist_ok=True)
    for aid in sorted(used):
        shutil.copyfile(Path(ns) / "actor" / f"{aid}.png", dst / f"{aid}.png")
    return len(used)


def main():
    ns = sys.argv[1] if len(sys.argv) > 1 else "nuevosSprites"
    for h in heroes():
        print("heroe", build_hero(ns, h).name)
    for cid, name, aid, hp, behit, die in ENEMIES:
        print("enemigo", build_enemy(ns, cid, name, aid, hp, behit, die).name)
    for cid, name, aid in FX:
        print("efecto", build_fx(ns, cid, name, aid).name)
    print("imagenes copiadas:", copy_images(ns))


if __name__ == "__main__":
    main()
