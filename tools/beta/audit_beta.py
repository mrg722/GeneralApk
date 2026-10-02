"""Auditoria completa de nuevosSprites -> docs/beta/AUDITORIA_BETA.md

Recorre TODAS las carpetas del paquete y deja, para cada archivo o grupo de
archivos: TIPO -> PERSONAJE/ESCENARIO -> FUNCION -> ANIMACION -> HABILIDAD ->
EFECTO -> SONIDO, si quedo integrado en el modo BETA y por que no (ambiguos,
UI de tienda, mecanismos de nivel...). Traduce al espanol los nombres chinos.

Uso: python3 tools/beta/audit_beta.py <carpeta nuevosSprites>
"""
import re
import sys
from collections import defaultdict
from pathlib import Path

from beta_anim import parse, read_text

ROOT = Path(__file__).resolve().parents[2]

# anim id -> (nombre en espanol, tipo, dueno/uso, estado en BETA)
ANIM = {
    0: ("Heroe arma 1 (Espadas del Caos) - basico", "personaje", "GUERRERO (ESPADAS DEL CAOS)", "INTEGRADO kratos_espadas"),
    1: ("Heroe habilidad 1-1 (Espadas)", "habilidad", "GUERRERO (ESPADAS DEL CAOS)", "INTEGRADO ab0..ab2"),
    2: ("Mecanismo activable (palanca)", "mecanismo", "nivel", "NO: mecanismo de nivel de plataformas"),
    3: ("Soldado esqueleto", "enemigo", "SOLDADO ESQUELETO", "INTEGRADO esqueleto"),
    4: ("Bestia excavadora", "enemigo", "BESTIA EXCAVADORA", "INTEGRADO excavador"),
    5: ("Heroe arma 1 - intermedio 1 (salto, voltereta)", "personaje", "GUERRERO (ESPADAS DEL CAOS)", "PARCIAL: salto integrado; abrir cofres/puertas, cuerdas y paredes son de niveles de plataformas"),
    6: ("Portal", "mecanismo", "nivel", "NO: transicion entre niveles"),
    7: ("Heroe arma 1 - intermedio 2", "personaje", "GUERRERO (ESPADAS DEL CAOS)", "NO: puente de tronco, patear puertas, columpio (mecanismos de niveles de plataformas)"),
    8: ("Mecanismo activable (invertido)", "mecanismo", "nivel", "NO"),
    9: ("Heroe arma 2 (Cestus de Nemea) - basico", "personaje", "GUERRERO (CESTUS DE NEMEA)", "INTEGRADO kratos_cesto"),
    10: ("Momia con escudo", "enemigo", "MOMIA CON ESCUDO", "INTEGRADO momia_escudo"),
    11: ("Bestia elefante", "enemigo", "BESTIA ELEFANTE", "INTEGRADO bestia_elefante"),
    12: ("Capa frontal del mapa (oclusion)", "escenario", "niveles", "NO: capa decorativa de algunos niveles"),
    13: ("Heroe arma 2 - intermedio 1", "personaje", "GUERRERO (CESTUS DE NEMEA)", "PARCIAL: salto integrado; abrir cofres/puertas, cuerdas y paredes son de niveles de plataformas"),
    14: ("Ave sanadora", "enemigo", "AVE SANADORA", "INTEGRADO ave_sanadora"),
    15: ("Monstruo de tentaculos (jefe, solo cajas)", "jefe", "TENTACULOS (spine PoseidonBaby)", "PENDIENTE: arte en Spine"),
    16: ("Heroe arma 2 - intermedio 2", "personaje", "GUERRERO (CESTUS DE NEMEA)", "NO: puente de tronco, patear puertas, columpio (mecanismos de niveles de plataformas)"),
    17: ("Heroe arma 3 (Cadena del Rayo) - basico", "personaje", "GUERRERO (CADENA DEL RAYO)", "INTEGRADO kratos_rayo"),
    18: ("Mecanismo: puerta de hierro", "mecanismo", "nivel", "NO"),
    19: ("Mecanismo: caja que se rompe", "mecanismo", "nivel", "NO"),
    20: ("Mecanismo: tubo de acero", "mecanismo", "nivel", "NO"),
    21: ("Mecanismo: fuego en el suelo", "mecanismo", "nivel", "NO"),
    22: ("Mecanismo: pinchos del suelo", "mecanismo", "nivel", "NO"),
    23: ("Mecanismo: puente de tronco", "mecanismo", "nivel", "NO"),
    24: ("Mecanismo: roca", "mecanismo", "nivel", "NO"),
    25: ("Mecanismo: plataforma flotante", "mecanismo", "nivel", "NO"),
    26: ("Mecanismo: cuerda para deslizarse", "mecanismo", "nivel", "NO"),
    27: ("Mecanismo: caja verde (frente)", "mecanismo", "nivel", "NO"),
    28: ("Mecanismo: caja verde (lado)", "mecanismo", "nivel", "NO"),
    29: ("Mecanismo: columpio", "mecanismo", "nivel", "NO"),
    30: ("Mecanismo: palanca", "mecanismo", "nivel", "NO"),
    31: ("Mecanismo: puente levadizo", "mecanismo", "nivel", "NO"),
    32: ("Heroe arma 3 - intermedio 1", "personaje", "GUERRERO (CADENA DEL RAYO)", "PARCIAL: salto integrado; abrir cofres/puertas, cuerdas y paredes son de niveles de plataformas"),
    33: ("Heroe arma 3 - intermedio 2", "personaje", "GUERRERO (CADENA DEL RAYO)", "NO: puente de tronco, patear puertas, columpio (mecanismos de niveles de plataformas)"),
    34: ("Heroe arma 4 (Garras de Hades) - basico", "personaje", "GUERRERO (GARRAS DE HADES)", "INTEGRADO kratos_garras"),
    35: ("Centauro", "enemigo", "CENTAURO", "INTEGRADO centauro"),
    36: ("Heroe arma 4 - intermedio 1", "personaje", "GUERRERO (GARRAS DE HADES)", "PARCIAL: salto integrado; abrir cofres/puertas, cuerdas y paredes son de niveles de plataformas"),
    37: ("Animacion de monedas", "efecto/UI", "recompensas", "NO: no hay economia en BETA"),
    38: ("Escalon del jefe", "escenario", "arena de jefe", "NO"),
    39: ("Bruto de la bola y cadena", "enemigo", "BRUTO DE LA BOLA", "INTEGRADO bruto_cadena"),
    40: ("Heroe arma 4 - intermedio 2", "personaje", "GUERRERO (GARRAS DE HADES)", "NO: puente de tronco, patear puertas, columpio (mecanismos de niveles de plataformas)"),
    41: ("Animacion del interior del barco 1", "escenario", "barco (interior)", "NO: nivel vertical no usado"),
    42: ("Mecanismo: caja roja (frente)", "mecanismo", "nivel", "NO"),
    43: ("Mecanismo: caja roja (lado)", "mecanismo", "nivel", "NO"),
    44: ("Jefe Titan - efectos de luz", "efecto", "TITAN", "PENDIENTE con el jefe"),
    45: ("Medusa", "enemigo", "MEDUSA", "INTEGRADO medusa"),
    46: ("Efecto de muerte de enemigo", "efecto", "enemigos", "INTEGRADO fx_humo_enemigo"),
    47: ("Mecanismo: caja azul (frente)", "mecanismo", "nivel", "NO"),
    48: ("Mecanismo: caja azul (lado)", "mecanismo", "nivel", "NO"),
    49: ("Efectos del volcan (lava, fuego, meteoros)", "efecto", "VOLCAN", "INTEGRADO fx_volcan (+ ceniza procedural)"),
    50: ("UI registro diario", "UI", "tienda/menus", "NO: UI de juego movil"),
    51: ("Jefe Titan (solo cajas y tiempos)", "jefe", "TITAN (spine Titan)", "PENDIENTE: arte en Spine"),
    52: ("Luz de curacion de enemigo", "efecto", "AVE SANADORA", "INTEGRADO fx_curacion"),
    53: ("Efecto nieve (exterior)", "efecto", "MONTANA NEVADA", "INTEGRADO fx_nieve (+ nieve procedural)"),
    54: ("Efecto nieve (interior)", "efecto", "CUEVA HELADA", "INTEGRADO fx_nieve_cueva"),
    55: ("Primer plano de la montana", "escenario", "nieve", "NO"),
    56: ("Dano por caida al vacio", "efecto", "niveles", "NO: no hay fosos en los niveles planos"),
    57: ("UI logo", "UI", "menu", "NO"),
    58: ("(vacio)", "vacio", "-", "AMBIGUO: archivo sin contenido util"),
    59: ("UI ciudad principal", "UI", "menu", "NO"),
    60: ("UI pantalla de nivel superado", "UI", "menu", "NO (BETA dibuja su propia pantalla en espanol)"),
    61: ("Efectos exteriores del barco (lluvia, gotas, rayos, tornado, olas)", "efecto", "CUBIERTA DEL BARCO", "INTEGRADO fx_barco (+ lluvia y rayos procedurales)"),
    62: ("Efectos interiores del barco", "efecto", "barco (interior)", "NO"),
    63: ("Efectos de la ciudad principal", "efecto", "menu", "NO"),
    64: ("Jefe Poseidon (solo cajas y tiempos)", "jefe", "POSEIDON (spine Poseidon)", "PENDIENTE: arte en Spine"),
    65: ("Jefe Poseidon - efectos de luz", "efecto", "POSEIDON", "PENDIENTE con el jefe"),
    66: ("Antorcha (montana)", "efecto", "nieve", "INTEGRADO fx_antorcha"),
    67: ("Alma roja", "efecto", "recompensa", "INTEGRADO fx_alma_roja (al vencer enemigos)"),
    68: ("Efecto de muerte (humo)", "efecto", "heroe/enemigos", "INTEGRADO fx_humo_muerte"),
    69: ("Efecto de escena 2", "efecto", "niveles", "AMBIGUO: un solo cuadro sin uso claro"),
    70: ("Alma verde", "efecto", "recompensa", "INTEGRADO fx_alma_verde"),
    71: ("Alma azul", "efecto", "recompensa", "INTEGRADO fx_alma_azul"),
    72: ("Barco destruido", "escenario", "barco", "NO (decorado)"),
    73: ("Duende en la pared", "escenario", "niveles", "NO"),
    74: ("Objetos sobre el barco", "escenario", "barco", "NO (decorado)"),
    75: ("UI boton generico", "UI", "menus", "NO"),
    76: ("Hombre de la ciudad (rojo)", "NPC", "ciudad", "NO: NPC decorativo"),
    77: ("Duende tutorial", "UI", "tutorial", "NO"),
    78: ("UI efectos de la pantalla de habilidades", "UI", "menus", "NO"),
    79: ("Monstruo de tentaculos - efectos", "efecto", "TENTACULOS", "PENDIENTE con el jefe"),
    80: ("Efecto de subir de nivel", "efecto", "UI", "NO"),
    81: ("UI pociones roja y verde", "UI", "tienda", "NO"),
    82: ("Chispa de impacto", "efecto", "golpes", "INTEGRADO fx_impacto"),
    83: ("Hombre de la ciudad (morado)", "NPC", "ciudad", "NO"),
    84: ("Hombre de la ciudad (azul)", "NPC", "ciudad", "NO"),
    85: ("Hombre de la ciudad (dorado)", "NPC", "ciudad", "NO"),
    86: ("Heroe habilidad 1-2 (Espadas, definitiva)", "habilidad", "GUERRERO (ESPADAS DEL CAOS)", "INTEGRADO ab3/ab4"),
    87: ("Heroe habilidad 2-1 (Cestus)", "habilidad", "GUERRERO (CESTUS DE NEMEA)", "INTEGRADO"),
    88: ("Heroe habilidad 2-2 (Cestus, definitiva)", "habilidad", "GUERRERO (CESTUS DE NEMEA)", "INTEGRADO"),
    89: ("NPC escolta", "NPC", "mision escolta", "NO: modo de mision no incluido"),
    90: ("NPC ataque con tiempo", "NPC", "mision", "NO"),
    91: ("NPC guardian", "NPC", "mision defensa", "NO"),
    92: ("Siete iconos pequenos de UI", "UI", "menus", "NO"),
    93: ("Objetos de cofre", "UI", "recompensas", "NO"),
    94: ("(vacio)", "vacio", "-", "AMBIGUO"), 95: ("(vacio)", "vacio", "-", "AMBIGUO"),
    96: ("(vacio)", "vacio", "-", "AMBIGUO"), 97: ("(vacio)", "vacio", "-", "AMBIGUO"),
    98: ("(vacio)", "vacio", "-", "AMBIGUO"),
    99: ("Rangos de ataque (Medusa, Centauro, Bruto, Tentaculos, Poseidon, Titan)", "datos", "enemigos/jefes",
         "REFERENCIA: cajas de golpe auxiliares (las de los enemigos ya vienen en sus cuadros)"),
    100: ("UI cuerno", "UI", "menus", "NO"),
    101: ("Chispa de impacto dorada", "efecto", "golpes", "INTEGRADO fx_impacto_oro (furia)"),
    102: ("Chispa de impacto morada", "efecto", "golpes", "INTEGRADO fx_impacto_morado (golpes enemigos)"),
    103: ("Chispa de impacto azul", "efecto", "golpes", "INTEGRADO fx_impacto_azul"),
    104: ("UI numeros saltarines", "UI", "danio", "NO"),
    105: ("Heroe habilidad 3-1 (Cadena del Rayo)", "habilidad", "GUERRERO (CADENA DEL RAYO)", "INTEGRADO"),
    106: ("Heroe habilidad 3-2 (Cadena del Rayo, definitiva)", "habilidad", "GUERRERO (CADENA DEL RAYO)", "INTEGRADO"),
    107: ("Heroe habilidad 4-1 (Garras de Hades)", "habilidad", "GUERRERO (GARRAS DE HADES)", "INTEGRADO"),
    108: ("Heroe habilidad 4-2 (Garras de Hades, definitiva)", "habilidad", "GUERRERO (GARRAS DE HADES)", "INTEGRADO"),
    109: ("NPC escolta (modificado)", "NPC", "mision", "NO"),
    110: ("Centauro rojo (variante elite)", "enemigo", "CENTAURO ROJO", "INTEGRADO centauro_rojo"),
    111: ("Bruto de la bola rojo", "enemigo", "BRUTO DE LA BOLA ROJO", "INTEGRADO bruto_cadena_rojo"),
    112: ("Medusa roja", "enemigo", "MEDUSA ROJA", "INTEGRADO medusa_roja"),
}

GLOSSARY = [
    ("主角", "heroe / protagonista"), ("技能", "habilidad / tecnica"), ("待战 / 待机", "guardia / reposo"),
    ("跑 / 移动", "correr / moverse"), ("前翻", "voltereta hacia adelante (esquiva)"), ("跳 / 蓄力准备跳", "salto / preparar salto"),
    ("攻击一段..四段", "ataque golpe 1..4 del combo"), ("连击", "combo"), ("被打", "recibir golpe"),
    ("击飞 / 被打起飞", "lanzado por el aire"), ("倒地", "caido en el suelo"), ("起身", "levantarse"),
    ("死亡", "muerte"), ("出场", "entrada en escena"), ("休息", "descanso (pose de victoria)"),
    ("准备 / 中 / 结束", "preparacion / en curso / final"), ("近身攻击", "ataque cuerpo a cuerpo"),
    ("远程攻击", "ataque a distancia"), ("飞镖", "proyectil"), ("框 / 红框 / 攻击框", "caja / caja roja / caja de ataque"),
    ("光效", "efecto de luz"), ("打击光效", "chispa de impacto"), ("机关", "mecanismo"), ("骷髅兵", "soldado esqueleto"),
    ("钻地怪", "bestia excavadora"), ("带盾干尸兵", "momia con escudo"), ("象怪", "bestia elefante"),
    ("加血鸟", "ave sanadora"), ("人马 / 半人马", "centauro"), ("链球怪", "bruto de la bola y cadena"),
    ("美杜莎 / 蛇发女妖", "Medusa / gorgona"), ("触手怪", "monstruo de tentaculos"), ("泰坦", "Titan"),
    ("波塞冬", "Poseidon"), ("精英", "elite"), ("红 (名)", "rojo / variante mutada"), ("混沌刃 / 混沌之刃", "Espadas del Caos"),
    ("拳套 / 尼米亚拳套", "Cestus de Nemea"), ("电光链 / 宙斯电光链", "Cadena del Rayo (de Zeus)"),
    ("钩爪 / 哈迪斯钩爪", "Garras de Hades"), ("雪山外 / 雪山内", "montana nevada exterior / interior (cueva)"),
    ("火山", "volcan"), ("战船 / 船外 / 船内", "barco de guerra / exterior / interior"), ("下雨 / 闪电", "lluvia / relampago"),
    ("碰撞", "colision"), ("红魂 / 绿魂 / 蓝魂", "alma roja / verde / azul"), ("主城", "ciudad principal"),
    ("千层塔", "torre de mil pisos (modo desafio)"), ("护送 / 守护 / 限时攻击", "escolta / defensa / ataque con tiempo"),
]

SOUND_ES = {
    "打开窗口": "abrir ventana", "关闭窗口": "cerrar ventana", "宙斯电光链音效": "Cadena del Rayo (bucle)",
    "哈迪斯钩爪音效": "Garras de Hades (bucle)", "尼米亚拳套音效": "Cestus de Nemea (bucle)", "药剂音": "pocion",
    "金币音": "moneda", "红魂音": "alma roja", "碎片音": "fragmento", "打开宝箱": "abrir cofre",
    "技能升级成功": "mejora de habilidad", "升级技能蓄力": "carga de mejora", "切换技能": "cambiar habilidad",
    "切换武器": "cambiar arma", "武器升星": "estrella de arma", "武器升级成功": "mejora de arma ok",
    "武器升级失败": "mejora de arma fallida", "装备武器音": "equipar arma", "解锁武器音": "desbloquear arma",
    "角色升级": "subir de nivel", "购买": "comprar", "出战": "salir a pelear", "新关卡开启": "nivel nuevo",
    "确认提示出现": "aviso de confirmacion", "关卡类型提示": "aviso de tipo de nivel", "三二一读秒": "cuenta 3-2-1",
    "游戏暂停": "pausa", "千层塔切换": "cambio de torre", "角色前窜(待换)": "embestida (provisional)",
    "角色死亡": "muerte del heroe", "角色复活": "revivir", "角色受伤": "heroe herido", "教学提示": "aviso tutorial",
    "机关提示": "aviso de mecanismo", "传送门": "portal", "机关开门": "puerta de mecanismo",
    "战斗胜利间接音": "victoria", "第一次攻击": "golpe 1 del combo", "第二次攻击": "golpe 2", "第三次攻击": "golpe 3",
    "第四次攻击": "golpe 4", "第五次攻击": "golpe 5", "关卡宝箱开启": "cofre del nivel",
    "前窜动作": "embestida / voltereta", "蓄力准备跳": "preparar salto", "拉机关": "tirar palanca",
    "拳套攻击第一下": "Cestus golpe 1", "拳套攻击第三下": "Cestus golpe 3", "混沌之刃音效": "Espadas del Caos",
}
MUSIC = {"menu.ogg": "menu principal (usado en la seleccion BETA)", "gate1music.ogg": "niveles barco/montana (BETA)",
         "gate2music.ogg": "niveles volcan (BETA)", "music_001.ogg": "ciudad / menus internos (no usado)",
         "music_002.ogg": "victoria (BETA)", "music_003.ogg": "derrota (BETA)", "gameCG.ogg": "cinematica inicial (no usado)"}
ENEMY_SND = {"sound67": "esqueleto/momia/excavador: recibe golpe", "sound66": "esqueleto/momia/excavador: muere",
             "sound68": "elefante/bruto: recibe golpe", "sound69": "elefante/bruto: muere", "sound39": "centauro: recibe golpe",
             "sound40": "centauro: muere", "sound31": "medusa: recibe golpe", "sound32": "medusa: muere",
             "sound52": "tentaculos: recibe golpe", "sound51": "tentaculos: muere", "sound56": "Titan: recibe golpe",
             "sound55": "Titan: muere", "sound59": "Poseidon: recibe golpe", "sound60": "Poseidon: muere",
             "sound42": "NPC escolta: golpe", "sound43": "NPC escolta: muere", "sound47": "Atenea/Zeus NPC"}


def main(ns):
    ns = Path(ns)
    out = ROOT / "docs/beta/AUDITORIA_BETA.md"
    out.parent.mkdir(parents=True, exist_ok=True)
    L = ["# Auditoria BETA (nuevosSprites)", "",
         "Generado por `tools/beta/audit_beta.py`. Recorre todo el paquete `nuevosSprites.zip` y documenta",
         "cada archivo: ARCHIVO -> TIPO -> PERSONAJE/ESCENARIO -> FUNCION -> ANIMACION -> HABILIDAD -> EFECTO -> SONIDO,",
         "y su estado en el MODO BETA. Nada se escalo ni se redibujo: personajes, efectos y mapas usan las imagenes",
         "originales pieza por pieza (ver `tools/beta/`).", "",
         "> Nota de propiedad intelectual: el paquete es contenido de un juego de terceros (un derivado de *God of War*).",
         "> Por eso vive aislado en el modo BETA, como prueba, y no se mezcla con los personajes propios del juego.", "",
         "## Resumen por carpeta", "", "| Carpeta | Archivos | Contenido | Uso en BETA |", "|---|---|---|---|"]
    usage = {
        "actor": "imagenes de piezas de personajes, efectos y mecanismos", "anim": "animaciones (XML del editor 乐堂)",
        "data": "tablas Lua: armas, enemigos, sonidos, niveles, tienda", "image": "fondos de parallax, UI, iconos, fuentes",
        "map": "tilesets 48x48 de los niveles", "mapdata": "niveles (capas de tiles + colision)",
        "particle": "particulas cocos2d (.plist)", "plist": "atlas de UI cocos2d", "refreshEnemy": "oleadas por nivel (Lua)",
        "script_box": "guiones de cofres", "script_npc": "guiones de NPC/dialogos", "sound": "musica y efectos .ogg",
        "spine": "esqueletos Spine 2.1 (jefes y QTE)", "story": "dialogos de la historia", "uidata": "pantallas cocos (.csb)"}
    state = {"actor": "INTEGRADO (430 imagenes usadas)", "anim": "INTEGRADO personajes y efectos; ver tabla",
             "data": "LEIDO para armas, sonidos, tipos de nivel", "image": "INTEGRADO fondos de parallax (21)",
             "map": "INTEGRADO (tilesets compuestos en assets/beta/stages)", "mapdata": "INTEGRADO 6 niveles planos",
             "particle": "NO (reemplazado por clima procedural y anims de efecto)", "plist": "NO (UI de juego movil)",
             "refreshEnemy": "REFERENCIA para las oleadas por zona", "script_box": "NO", "script_npc": "NO",
             "sound": "INTEGRADO (82 .ogg en assets/beta/audio)", "spine": "PENDIENTE (jefes Titan/Poseidon/Tentaculos)",
             "story": "NO (sin modo historia en BETA)", "uidata": "NO (BETA dibuja su UI en espanol)"}
    for d in sorted(p for p in ns.iterdir() if p.is_dir()):
        n = sum(1 for f in d.rglob("*") if f.is_file())
        L.append(f"| {d.name} | {n} | {usage.get(d.name, '?')} | {state.get(d.name, '?')} |")

    # Animaciones
    L += ["", "## Animaciones (anim/N.xml)", "",
          "| Archivo | Nombre original | Espanol | Tipo | Personaje/Escenario | Acciones | Estado BETA |", "|---|---|---|---|---|---|---|"]
    owners = defaultdict(set)
    for f in sorted((ns / "anim").glob("*.xml"), key=lambda p: int(p.stem)):
        i = int(f.stem)
        try:
            a = parse(f)
        except Exception as e:  # noqa: BLE001
            L.append(f"| {f.name} | ERROR | {e} | | | | AMBIGUO |")
            continue
        es, kind, owner, st = ANIM.get(i, ("?", "?", "?", "AMBIGUO"))
        for img in a["images"]: owners[img["id"]].add(i)
        L.append(f"| {f.name} | {a['name']} | {es} | {kind} | {owner} | {len(a['actions'])} | {st} |")

    # Acciones de personajes integrados -> clips
    L += ["", "## Personajes integrados: accion original -> clip -> habilidad -> sonido", ""]
    for f in sorted((ROOT / "data/beta/characters").glob("*.txt")):
        t = f.read_text(encoding="utf-8").splitlines()
        name = next((x[5:] for x in t if x.startswith("name ")), f.stem)
        clips = [x.split()[1] for x in t if x.startswith("clip ")]
        abil = [x.split(None, 2)[1:] for x in t if x.startswith("ability ")]
        snds = [x.split()[1:] for x in t if x.startswith("clipsound ")]
        ev = [x.split()[1:] for x in t if x.startswith("sound ")]
        L.append(f"### {name} (`{f.stem}`)")
        L.append(f"- Clips ({len(clips)}): {', '.join(clips)}")
        if abil: L.append("- Habilidades: " + "; ".join(f"{c} = {lab}" for c, lab in abil))
        if snds: L.append("- Sonidos por cuadro: " + ", ".join(f"{c}[{i}]={s}" for c, i, s in snds))
        if ev: L.append("- Sonidos de evento: " + ", ".join(f"{e}={s}" for e, s in ev))
        L.append("")

    # Imagenes actor
    used = {int(p.stem) for p in (ROOT / "assets/beta/actor").glob("*.png")}
    actors = sorted(int(p.stem) for p in (ns / "actor").glob("*.png") if p.stem.isdigit())
    L += ["## Imagenes de piezas (actor/N.png)", "",
          f"{len(actors)} imagenes; {len(used)} usadas por BETA (copiadas tal cual a `assets/beta/actor`).", "",
          "| Dueno (anim) | Imagenes | Usadas |", "|---|---|---|"]
    by_owner = defaultdict(list)
    for aid in actors:
        key = ", ".join(str(x) for x in sorted(owners.get(aid, []))) or "sin anim"
        by_owner[key].append(aid)
    for key, ids in sorted(by_owner.items(), key=lambda kv: kv[1][0]):
        names = "; ".join(ANIM.get(int(k), ("?",))[0] for k in key.split(", ") if k.isdigit()) or "SIN DUENO (AMBIGUO)"
        L.append(f"| {key}: {names} | {len(ids)} ({ids[0]}..{ids[-1]}) | {sum(1 for i in ids if i in used)} |")

    # Sonidos
    lua = read_text(ns / "data/GameMusicFile.lua")
    snd_names = defaultdict(list)
    for m in re.finditer(r'name = "([^"]*)".*?fileName = "([^"]*)"', lua):
        snd_names[m.group(2)].append(SOUND_ES.get(m.group(1).strip(), m.group(1)))
    L += ["", "## Sonidos y musica (sound/*.ogg)", "", "| Archivo | Evento original (es) | Uso en BETA |", "|---|---|---|"]
    beta_use = defaultdict(set)
    for f in (ROOT / "data/beta/characters").glob("*.txt"):
        for x in f.read_text(encoding="utf-8").splitlines():
            if x.startswith("clipsound ") or x.startswith("sound "):
                beta_use[x.split()[-1]].add(f.stem)
    for s, why in (("sound24", "confirmar / iniciar pelea"), ("sound13", "mover cursor"), ("sound14", "cambiar personaje"),
                   ("sound2", "volver"), ("sound44", "aparece oleada"), ("sound48", "victoria")):
        beta_use[s].add(why)
    for f in sorted((ns / "sound").glob("*.ogg"), key=lambda p: (not p.stem.startswith("sound"), int(re.sub(r"\D", "", p.stem) or 0))):
        k = f.stem
        orig = "; ".join(snd_names.get(k, [])) or ENEMY_SND.get(k, "") or MUSIC.get(f.name, "")
        if k in ENEMY_SND and ENEMY_SND[k] not in orig: orig = (orig + "; " if orig else "") + ENEMY_SND[k]
        use = ", ".join(sorted(beta_use.get(k, []))) or ("musica" if f.name in MUSIC and "BETA" in MUSIC[f.name] else "")
        if not orig: orig = "AMBIGUO: sin referencia en GameMusicFile.lua"
        L.append(f"| {f.name} | {orig} | {use or 'no usado'} |")

    # Niveles
    L += ["", "## Niveles (mapdata) y escenarios", "",
          "Tipos segun `data/GameSeting.lua`: BoatGate = barco exterior con lluvia, BoatInGate = interior del barco,",
          "SnowOutGate = montana exterior, SnowInGate = cueva helada, FireOutGateBackRound = volcan.", "",
          "| Escenario BETA | Nivel | Tamano | Franja caminable (px del mapa) | Fondos | Musica | Clima |", "|---|---|---|---|---|---|---|"]
    for f in sorted((ROOT / "data/beta/stages").glob("*.txt")):
        kv = defaultdict(list)
        for x in f.read_text(encoding="utf-8").splitlines():
            p = x.split(None, 1)
            if len(p) == 2: kv[p[0]].append(p[1])
        L.append(f"| {kv['name'][0]} | {kv['source'][0]} | {kv['size'][0]} | {kv['walk'][0]} | "
                 f"{', '.join(v.split()[0] for v in kv['layer'])} | {kv['music'][0]} | {kv['weather'][0]} |")
    L += ["", "Niveles no usados: los de 22 filas (interior del barco 111/112/191/252, montana 212/241) son de",
          "plataformas verticales con mecanismos; los demas repiten los mismos tilesets que los 6 elegidos.", "",
          "## Spine (jefes y QTE) - PENDIENTE", "",
          "- `spine/Titan`, `spine/Poseidon`, `spine/PoseidonBaby` (= monstruo de tentaculos, anim 15): Spine 2.1.27 con",
          "  mallas y mallas con huesos. Sus cajas y tiempos estan en anim 51/64/15 (cuadros con imagen de relleno 597).",
          "  Para integrarlos sin deformar hay que hornear las animaciones Spine a cuadros (pendiente).",
          "- `ChainfattyQTE`, `medsuaQTE`, `RenMaQTE`, `PoseidonQTE`, `PoseidonBabyQTE`: cinematicas de remate (QTE): INTEGRADAS (data/beta/qte, boton a tiempo).",
          "- `Menu/zhujue.json`: heroe animado del menu original.", "",
          "## Archivos ambiguos", "",
          "- anim 58, 94-98: archivos marcados 无 (vacio) con una imagen de relleno.",
          "- anim 69 (efecto de escena 2) y 12 (capa frontal): un solo cuadro sin nivel claro que los use.",
          "- actor sin anim que los referencie: ver la fila 'sin anim' en la tabla de imagenes.",
          "- sound sin entrada en GameMusicFile.lua: ver 'AMBIGUO' en la tabla de sonidos.", "",
          "## Glosario chino -> espanol", "", "| Chino | Espanol |", "|---|---|"]
    L += [f"| {zh} | {es} |" for zh, es in GLOSSARY]
    out.write_text("\n".join(L) + "\n", encoding="utf-8")
    print("escrito", out, len(L), "lineas")


if __name__ == "__main__":
    main(sys.argv[1])
