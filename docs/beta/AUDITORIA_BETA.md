# Auditoria BETA (nuevosSprites)

Generado por `tools/beta/audit_beta.py`. Recorre todo el paquete `nuevosSprites.zip` y documenta
cada archivo: ARCHIVO -> TIPO -> PERSONAJE/ESCENARIO -> FUNCION -> ANIMACION -> HABILIDAD -> EFECTO -> SONIDO,
y su estado en el MODO BETA. Nada se escalo ni se redibujo: personajes, efectos y mapas usan las imagenes
originales pieza por pieza (ver `tools/beta/`).

> Nota de propiedad intelectual: el paquete es contenido de un juego de terceros (un derivado de *God of War*).
> Por eso vive aislado en el modo BETA, como prueba, y no se mezcla con los personajes propios del juego.

## Resumen por carpeta

| Carpeta | Archivos | Contenido | Uso en BETA |
|---|---|---|---|
| actor | 706 | imagenes de piezas de personajes, efectos y mecanismos | INTEGRADO (430 imagenes usadas) |
| anim | 113 | animaciones (XML del editor 乐堂) | INTEGRADO personajes y efectos; ver tabla |
| data | 34 | tablas Lua: armas, enemigos, sonidos, niveles, tienda | LEIDO para armas, sonidos, tipos de nivel |
| image | 850 | fondos de parallax, UI, iconos, fuentes | INTEGRADO fondos de parallax (21) |
| map | 17 | tilesets 48x48 de los niveles | INTEGRADO (tilesets compuestos en assets/beta/stages) |
| mapdata | 119 | niveles (capas de tiles + colision) | INTEGRADO 6 niveles planos |
| particle | 45 | particulas cocos2d (.plist) | NO (reemplazado por clima procedural y anims de efecto) |
| plist | 124 | atlas de UI cocos2d | NO (UI de juego movil) |
| refreshEnemy | 39 | oleadas por nivel (Lua) | REFERENCIA para las oleadas por zona |
| script_box | 19 | guiones de cofres | NO |
| script_npc | 124 | guiones de NPC/dialogos | NO |
| sound | 82 | musica y efectos .ogg | INTEGRADO (82 .ogg en assets/beta/audio) |
| spine | 239 | esqueletos Spine 2.1 (jefes y QTE) | PENDIENTE (jefes Titan/Poseidon/Tentaculos) |
| story | 31 | dialogos de la historia | NO (sin modo historia en BETA) |
| uidata | 169 | pantallas cocos (.csb) | NO (BETA dibuja su UI en espanol) |

## Animaciones (anim/N.xml)

| Archivo | Nombre original | Espanol | Tipo | Personaje/Escenario | Acciones | Estado BETA |
|---|---|---|---|---|---|---|
| 0.xml | [0]主角1-1 | Heroe arma 1 (Espadas del Caos) - basico | personaje | GUERRERO (ESPADAS DEL CAOS) | 33 | INTEGRADO kratos_espadas |
| 1.xml | [1]主角技能1-1 | Heroe habilidad 1-1 (Espadas) | habilidad | GUERRERO (ESPADAS DEL CAOS) | 8 | INTEGRADO ab0..ab2 |
| 2.xml | [2]触发机关 | Mecanismo activable (palanca) | mecanismo | nivel | 26 | NO: mecanismo de nivel de plataformas |
| 3.xml | [3]DD--骷髅兵 | Soldado esqueleto | enemigo | SOLDADO ESQUELETO | 36 | INTEGRADO esqueleto |
| 4.xml | [4]DD--钻地怪 | Bestia excavadora | enemigo | BESTIA EXCAVADORA | 36 | INTEGRADO excavador |
| 5.xml | [5]主角1-2 | Heroe arma 1 - intermedio 1 (salto, voltereta) | personaje | GUERRERO (ESPADAS DEL CAOS) | 49 | PARCIAL: salto integrado; abrir cofres/puertas, cuerdas y paredes son de niveles de plataformas |
| 6.xml | [6]传送门 | Portal | mecanismo | nivel | 3 | NO: transicion entre niveles |
| 7.xml | [7]主角1-3 | Heroe arma 1 - intermedio 2 | personaje | GUERRERO (ESPADAS DEL CAOS) | 45 | NO: puente de tronco, patear puertas, columpio (mecanismos de niveles de plataformas) |
| 8.xml | [8]触发机关--反向 | Mecanismo activable (invertido) | mecanismo | nivel | 26 | NO |
| 9.xml | [9]主角2-1 | Heroe arma 2 (Cestus de Nemea) - basico | personaje | GUERRERO (CESTUS DE NEMEA) | 32 | INTEGRADO kratos_cesto |
| 10.xml | [10]DD--带盾干尸兵 | Momia con escudo | enemigo | MOMIA CON ESCUDO | 33 | INTEGRADO momia_escudo |
| 11.xml | [11]DD--象怪 | Bestia elefante | enemigo | BESTIA ELEFANTE | 34 | INTEGRADO bestia_elefante |
| 12.xml | [12]地图前层遮挡 | Capa frontal del mapa (oclusion) | escenario | niveles | 8 | NO: capa decorativa de algunos niveles |
| 13.xml | [13]主角2-2 | Heroe arma 2 - intermedio 1 | personaje | GUERRERO (CESTUS DE NEMEA) | 49 | PARCIAL: salto integrado; abrir cofres/puertas, cuerdas y paredes son de niveles de plataformas |
| 14.xml | [14]DD--加血鸟 | Ave sanadora | enemigo | AVE SANADORA | 34 | INTEGRADO ave_sanadora |
| 15.xml | [15]BB--触手怪 | Monstruo de tentaculos (jefe, solo cajas) | jefe | TENTACULOS (spine PoseidonBaby) | 43 | PENDIENTE: arte en Spine |
| 16.xml | [16]主角2-3 | Heroe arma 2 - intermedio 2 | personaje | GUERRERO (CESTUS DE NEMEA) | 45 | NO: puente de tronco, patear puertas, columpio (mecanismos de niveles de plataformas) |
| 17.xml | [17]主角3-1 | Heroe arma 3 (Cadena del Rayo) - basico | personaje | GUERRERO (CADENA DEL RAYO) | 32 | INTEGRADO kratos_rayo |
| 18.xml | [18]机关-铁门 | Mecanismo: puerta de hierro | mecanismo | nivel | 4 | NO |
| 19.xml | [19]机关--箱碎裂 | Mecanismo: caja que se rompe | mecanismo | nivel | 5 | NO |
| 20.xml | [20]机关--钢管 | Mecanismo: tubo de acero | mecanismo | nivel | 2 | NO |
| 21.xml | [21]机关--地面火焰 | Mecanismo: fuego en el suelo | mecanismo | nivel | 2 | NO |
| 22.xml | [22]机关--地刺 | Mecanismo: pinchos del suelo | mecanismo | nivel | 2 | NO |
| 23.xml | [23]机关--独木桥 | Mecanismo: puente de tronco | mecanismo | nivel | 3 | NO |
| 24.xml | [24]机关--岩石 | Mecanismo: roca | mecanismo | nivel | 2 | NO |
| 25.xml | [25]机关--浮台 | Mecanismo: plataforma flotante | mecanismo | nivel | 16 | NO |
| 26.xml | [26]机关--下滑绳 | Mecanismo: cuerda para deslizarse | mecanismo | nivel | 6 | NO |
| 27.xml | [27]机关--正面绿箱 | Mecanismo: caja verde (frente) | mecanismo | nivel | 5 | NO |
| 28.xml | [28]机关--侧面绿箱 | Mecanismo: caja verde (lado) | mecanismo | nivel | 5 | NO |
| 29.xml | [29]机关--荡秋千 | Mecanismo: columpio | mecanismo | nivel | 4 | NO |
| 30.xml | [30]机关--拉机关 | Mecanismo: palanca | mecanismo | nivel | 2 | NO |
| 31.xml | [31]机关--吊桥 | Mecanismo: puente levadizo | mecanismo | nivel | 3 | NO |
| 32.xml | [32]主角3-2 | Heroe arma 3 - intermedio 1 | personaje | GUERRERO (CADENA DEL RAYO) | 49 | PARCIAL: salto integrado; abrir cofres/puertas, cuerdas y paredes son de niveles de plataformas |
| 33.xml | [33]主角3-3 | Heroe arma 3 - intermedio 2 | personaje | GUERRERO (CADENA DEL RAYO) | 45 | NO: puente de tronco, patear puertas, columpio (mecanismos de niveles de plataformas) |
| 34.xml | [34]主角4-1 | Heroe arma 4 (Garras de Hades) - basico | personaje | GUERRERO (GARRAS DE HADES) | 32 | INTEGRADO kratos_garras |
| 35.xml | [35]JJ--人马 | Centauro | enemigo | CENTAURO | 43 | INTEGRADO centauro |
| 36.xml | [36]主角4-2 | Heroe arma 4 - intermedio 1 | personaje | GUERRERO (GARRAS DE HADES) | 49 | PARCIAL: salto integrado; abrir cofres/puertas, cuerdas y paredes son de niveles de plataformas |
| 37.xml | [37]金币动画 | Animacion de monedas | efecto/UI | recompensas | 11 | NO: no hay economia en BETA |
| 38.xml | [38]BOSS台阶 | Escalon del jefe | escenario | arena de jefe | 1 | NO |
| 39.xml | [39]JJ--链球怪 | Bruto de la bola y cadena | enemigo | BRUTO DE LA BOLA | 43 | INTEGRADO bruto_cadena |
| 40.xml | [40]主角4-3 | Heroe arma 4 - intermedio 2 | personaje | GUERRERO (GARRAS DE HADES) | 45 | NO: puente de tronco, patear puertas, columpio (mecanismos de niveles de plataformas) |
| 41.xml | [41]船内动画1 | Animacion del interior del barco 1 | escenario | barco (interior) | 4 | NO: nivel vertical no usado |
| 42.xml | [42]机关--正面红箱 | Mecanismo: caja roja (frente) | mecanismo | nivel | 5 | NO |
| 43.xml | [43]机关--侧面红箱 | Mecanismo: caja roja (lado) | mecanismo | nivel | 5 | NO |
| 44.xml | [44]BOSS--泰坦光效 | Jefe Titan - efectos de luz | efecto | TITAN | 4 | PENDIENTE con el jefe |
| 45.xml | [45]JJ--美杜莎 | Medusa | enemigo | MEDUSA | 43 | INTEGRADO medusa |
| 46.xml | [46]敌人死亡效果 | Efecto de muerte de enemigo | efecto | enemigos | 1 | INTEGRADO fx_humo_enemigo |
| 47.xml | [47]机关--正面蓝箱 | Mecanismo: caja azul (frente) | mecanismo | nivel | 5 | NO |
| 48.xml | [48]机关--侧面蓝箱 | Mecanismo: caja azul (lado) | mecanismo | nivel | 5 | NO |
| 49.xml | [49]火山内场景特效 | Efectos del volcan (lava, fuego, meteoros) | efecto | VOLCAN | 9 | INTEGRADO fx_volcan (+ ceniza procedural) |
| 50.xml | [50]UI_签到 | UI registro diario | UI | tienda/menus | 1 | NO: UI de juego movil |
| 51.xml | [51]B0SS--泰坦 | Jefe Titan (solo cajas y tiempos) | jefe | TITAN (spine Titan) | 43 | PENDIENTE: arte en Spine |
| 52.xml | [52]敌人加血光效 | Luz de curacion de enemigo | efecto | AVE SANADORA | 2 | INTEGRADO fx_curacion |
| 53.xml | [53]雪山外场景特效 | Efecto nieve (exterior) | efecto | MONTANA NEVADA | 1 | INTEGRADO fx_nieve (+ nieve procedural) |
| 54.xml | [54]雪山内场景特效 | Efecto nieve (interior) | efecto | CUEVA HELADA | 1 | INTEGRADO fx_nieve_cueva |
| 55.xml | [55]雪山前景 | Primer plano de la montana | escenario | nieve | 2 | NO |
| 56.xml | [56]掉坑死亡伤害 | Dano por caida al vacio | efecto | niveles | 1 | NO: no hay fosos en los niveles planos |
| 57.xml | [57]UI_LOGO | UI logo | UI | menu | 1 | NO |
| 58.xml | [58]无 | (vacio) | vacio | - | 1 | AMBIGUO: archivo sin contenido util |
| 59.xml | [59]UI_主城 | UI ciudad principal | UI | menu | 1 | NO |
| 60.xml | [60]UI_通过界面 | UI pantalla de nivel superado | UI | menu | 5 | NO (BETA dibuja su propia pantalla en espanol) |
| 61.xml | [61]场景船外特效 | Efectos exteriores del barco (lluvia, gotas, rayos, tornado, olas) | efecto | CUBIERTA DEL BARCO | 17 | INTEGRADO fx_barco (+ lluvia y rayos procedurales) |
| 62.xml | [62]场景船内特效 | Efectos interiores del barco | efecto | barco (interior) | 8 | NO |
| 63.xml | [63]主城特效 | Efectos de la ciudad principal | efecto | menu | 15 | NO |
| 64.xml | [64]B00S--波塞冬 | Jefe Poseidon (solo cajas y tiempos) | jefe | POSEIDON (spine Poseidon) | 43 | PENDIENTE: arte en Spine |
| 65.xml | [65]B00S--波塞冬光效 | Jefe Poseidon - efectos de luz | efecto | POSEIDON | 5 | PENDIENTE con el jefe |
| 66.xml | [66]雪山--火炬 | Antorcha (montana) | efecto | nieve | 1 | INTEGRADO fx_antorcha |
| 67.xml | [67]红魄 | Alma roja | efecto | recompensa | 14 | INTEGRADO fx_alma_roja (al vencer enemigos) |
| 68.xml | [68]死亡特效 | Efecto de muerte (humo) | efecto | heroe/enemigos | 1 | INTEGRADO fx_humo_muerte |
| 69.xml | [69]场景特效2 | Efecto de escena 2 | efecto | niveles | 1 | AMBIGUO: un solo cuadro sin uso claro |
| 70.xml | [70]绿魂 | Alma verde | efecto | recompensa | 14 | INTEGRADO fx_alma_verde |
| 71.xml | [71]蓝魂 | Alma azul | efecto | recompensa | 14 | INTEGRADO fx_alma_azul |
| 72.xml | [72]破船 | Barco destruido | escenario | barco | 1 | NO (decorado) |
| 73.xml | [73]插墙精灵 | Duende en la pared | escenario | niveles | 2 | NO |
| 74.xml | [74]船上地物 | Objetos sobre el barco | escenario | barco | 5 | NO (decorado) |
| 75.xml | [75]UI_通用按钮 | UI boton generico | UI | menus | 1 | NO |
| 76.xml | [76]主城男_红 | Hombre de la ciudad (rojo) | NPC | ciudad | 4 | NO: NPC decorativo |
| 77.xml | [77]教学精灵 | Duende tutorial | UI | tutorial | 1 | NO |
| 78.xml | [78]UI技能界面特效 | UI efectos de la pantalla de habilidades | UI | menus | 4 | NO |
| 79.xml | [79]触手怪--光效 | Monstruo de tentaculos - efectos | efecto | TENTACULOS | 4 | PENDIENTE con el jefe |
| 80.xml | [80]升级光效 | Efecto de subir de nivel | efecto | UI | 2 | NO |
| 81.xml | [81]UI_药瓶、红、绿 | UI pociones roja y verde | UI | tienda | 4 | NO |
| 82.xml | [82]打击光效 | Chispa de impacto | efecto | golpes | 6 | INTEGRADO fx_impacto |
| 83.xml | [83]主城男_紫 | Hombre de la ciudad (morado) | NPC | ciudad | 4 | NO |
| 84.xml | [84]主城男_蓝 | Hombre de la ciudad (azul) | NPC | ciudad | 4 | NO |
| 85.xml | [85]主城男_金 | Hombre de la ciudad (dorado) | NPC | ciudad | 4 | NO |
| 86.xml | [86]主角技能1-2 | Heroe habilidad 1-2 (Espadas, definitiva) | habilidad | GUERRERO (ESPADAS DEL CAOS) | 5 | INTEGRADO ab3/ab4 |
| 87.xml | [87]主角技能2-1 | Heroe habilidad 2-1 (Cestus) | habilidad | GUERRERO (CESTUS DE NEMEA) | 8 | INTEGRADO |
| 88.xml | [88]主角技能2-2 | Heroe habilidad 2-2 (Cestus, definitiva) | habilidad | GUERRERO (CESTUS DE NEMEA) | 5 | INTEGRADO |
| 89.xml | [89]NPC--护送 | NPC escolta | NPC | mision escolta | 36 | NO: modo de mision no incluido |
| 90.xml | [90]NPC--限时攻击 | NPC ataque con tiempo | NPC | mision | 33 | NO |
| 91.xml | [91]NPC--守护 | NPC guardian | NPC | mision defensa | 33 | NO |
| 92.xml | [92]七个小UI | Siete iconos pequenos de UI | UI | menus | 7 | NO |
| 93.xml | [93]宝箱道具 | Objetos de cofre | UI | recompensas | 10 | NO |
| 94.xml | [94]无 | (vacio) | vacio | - | 1 | AMBIGUO |
| 95.xml | [95]无 | (vacio) | vacio | - | 1 | AMBIGUO |
| 96.xml | [96]无 | (vacio) | vacio | - | 1 | AMBIGUO |
| 97.xml | [97]无 | (vacio) | vacio | - | 1 | AMBIGUO |
| 98.xml | [98]无 | (vacio) | vacio | - | 1 | AMBIGUO |
| 99.xml | [99]攻击范围合集 | Rangos de ataque (Medusa, Centauro, Bruto, Tentaculos, Poseidon, Titan) | datos | enemigos/jefes | 14 | REFERENCIA: cajas de golpe auxiliares (las de los enemigos ya vienen en sus cuadros) |
| 100.xml | [100]UI-号角 | UI cuerno | UI | menus | 1 | NO |
| 101.xml | [101]打击光效-金 | Chispa de impacto dorada | efecto | golpes | 6 | INTEGRADO fx_impacto_oro (furia) |
| 102.xml | [102]打击光效-紫 | Chispa de impacto morada | efecto | golpes | 6 | INTEGRADO fx_impacto_morado (golpes enemigos) |
| 103.xml | [103]打击光效-蓝 | Chispa de impacto azul | efecto | golpes | 6 | INTEGRADO fx_impacto_azul |
| 104.xml | [104]UI--跳跃数字 | UI numeros saltarines | UI | danio | 1 | NO |
| 105.xml | [105]主角技能3-1 | Heroe habilidad 3-1 (Cadena del Rayo) | habilidad | GUERRERO (CADENA DEL RAYO) | 8 | INTEGRADO |
| 106.xml | [106]主角技能3-2 | Heroe habilidad 3-2 (Cadena del Rayo, definitiva) | habilidad | GUERRERO (CADENA DEL RAYO) | 5 | INTEGRADO |
| 107.xml | [107]主角技能4-1 | Heroe habilidad 4-1 (Garras de Hades) | habilidad | GUERRERO (GARRAS DE HADES) | 8 | INTEGRADO |
| 108.xml | [108]主角技能4-2 | Heroe habilidad 4-2 (Garras de Hades, definitiva) | habilidad | GUERRERO (GARRAS DE HADES) | 5 | INTEGRADO |
| 109.xml | [109]NPC--护送修改 | NPC escolta (modificado) | NPC | mision | 1 | NO |
| 110.xml | [110]JJ--人马红 | Centauro rojo (variante elite) | enemigo | CENTAURO ROJO | 43 | INTEGRADO centauro_rojo |
| 111.xml | [111]JJ--链球怪红 | Bruto de la bola rojo | enemigo | BRUTO DE LA BOLA ROJO | 43 | INTEGRADO bruto_cadena_rojo |
| 112.xml | [112]JJ--美杜莎红 | Medusa roja | enemigo | MEDUSA ROJA | 43 | INTEGRADO medusa_roja |

## Personajes integrados: accion original -> clip -> habilidad -> sonido

### AVE SANADORA (`ave_sanadora`)
- Clips (30): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, special, punch2, punch3, atk1, atk2, atk3, atk4, energy, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; special = TECNICA
- Sonidos de evento: hurt=sound67, die=sound66

### BESTIA ELEFANTE (`bestia_elefante`)
- Clips (30): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, special, punch2, punch3, atk1, atk2, atk3, atk4, energy, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; special = TECNICA
- Sonidos de evento: hurt=sound68, die=sound69

### BRUTO DE LA BOLA (`bruto_cadena`)
- Clips (31): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, energy, projectile, punch2, punch3, atk1, atk2, atk3, atk4, special, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; energy = ATAQUE A DISTANCIA
- Sonidos de evento: hurt=sound68, die=sound69

### BRUTO DE LA BOLA ROJO (`bruto_cadena_rojo`)
- Clips (31): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, energy, projectile, punch2, punch3, atk1, atk2, atk3, atk4, special, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; energy = ATAQUE A DISTANCIA
- Sonidos de evento: hurt=sound68, die=sound69

### CENTAURO (`centauro`)
- Clips (30): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, special, punch2, punch3, atk1, atk2, atk3, atk4, energy, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; special = TECNICA
- Sonidos de evento: hurt=sound39, die=sound40

### CENTAURO ROJO (`centauro_rojo`)
- Clips (30): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, special, punch2, punch3, atk1, atk2, atk3, atk4, energy, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; special = TECNICA
- Sonidos de evento: hurt=sound39, die=sound40

### SOLDADO ESQUELETO (`esqueleto`)
- Clips (31): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, energy, projectile, punch2, punch3, atk1, atk2, atk3, atk4, special, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; energy = ATAQUE A DISTANCIA
- Sonidos de evento: hurt=sound67, die=sound66

### BESTIA EXCAVADORA (`excavador`)
- Clips (31): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, energy, projectile, punch2, punch3, atk1, atk2, atk3, atk4, special, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; energy = ATAQUE A DISTANCIA
- Sonidos de evento: hurt=sound67, die=sound66

### ALMA AZUL (`fx_alma_azul`)
- Clips (15): a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, idle

### ALMA ROJA (`fx_alma_roja`)
- Clips (15): a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, idle

### ALMA VERDE (`fx_alma_verde`)
- Clips (15): a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, idle

### ANTORCHA (`fx_antorcha`)
- Clips (2): a0, idle

### CLIMA DEL BARCO (LLUVIA/RAYOS/OLAS) (`fx_barco`)
- Clips (18): a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, idle

### LUZ DE CURACION (`fx_curacion`)
- Clips (3): a0, a1, idle

### HUMO DE ENEMIGO VENCIDO (`fx_humo_enemigo`)
- Clips (2): a0, idle

### HUMO DE MUERTE (`fx_humo_muerte`)
- Clips (2): a0, idle

### CHISPA DE IMPACTO (`fx_impacto`)
- Clips (7): a0, a1, a2, a3, a4, a5, idle

### CHISPA DE IMPACTO AZUL (`fx_impacto_azul`)
- Clips (7): a0, a1, a2, a3, a4, a5, idle

### CHISPA DE IMPACTO MORADA (`fx_impacto_morado`)
- Clips (7): a0, a1, a2, a3, a4, a5, idle

### CHISPA DE IMPACTO DORADA (`fx_impacto_oro`)
- Clips (7): a0, a1, a2, a3, a4, a5, idle

### NEVADA FUERTE (`fx_nieve`)
- Clips (2): a0, idle

### NEVADA LIGERA (`fx_nieve_cueva`)
- Clips (2): a0, idle

### LAVA Y FUEGO DEL VOLCAN (`fx_volcan`)
- Clips (10): a0, a1, a2, a3, a4, a5, a6, a7, a8, idle

### GUERRERO (CESTUS DE NEMEA) (`kratos_cesto`)
- Clips (35): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, block, victory, punch1, punch2, punch3, kick, atk1, atk2, atk3, atk4, dash_attack, finisher, ab0, ab1, ab2, ab3, ab4, energy, special, super, rage_attack, jump
- Habilidades: ab0 = HABILIDAD A; ab1 = HABILIDAD B; ab2 = HABILIDAD C; ab3 = COMBO DE HABILIDAD 1; ab4 = COMBO DE HABILIDAD 2
- Sonidos por cuadro: dash[0]=sound70, hit[0]=sound36, hit_high[0]=sound36, hit_low[0]=sound36, recovery[0]=sound36, defeat[0]=sound34, punch1[4]=sound73, punch1[4]=sound73, punch3[1]=sound74, punch3[1]=sound74, kick[4]=sound64, kick[4]=sound64, atk1[4]=sound73, atk1[4]=sound73, atk3[1]=sound74, atk3[1]=sound74, atk4[4]=sound64, atk4[4]=sound64, dash_attack[1]=sound74, dash_attack[1]=sound74, finisher[4]=sound64, finisher[4]=sound64, ab0[5]=sound65, ab0[7]=sound38, ab1[0]=sound65, ab1[44]=sound64, ab1[42]=sound64, ab1[38]=sound64, ab1[30]=sound64, ab1[26]=sound64, ab1[22]=sound64, ab1[18]=sound64, ab1[26]=sound38, ab2[0]=sound37, ab2[53]=sound65, ab2[67]=sound38, ab2[59]=sound38, ab2[45]=sound38, ab2[39]=sound38, ab2[34]=sound38, ab2[28]=sound38, ab2[21]=sound38, ab3[0]=sound65, ab4[0]=sound37, energy[5]=sound65, energy[7]=sound38, special[0]=sound65, special[44]=sound64, special[42]=sound64, special[38]=sound64, special[30]=sound64, special[26]=sound64, special[22]=sound64, special[18]=sound64, special[26]=sound38, super[0]=sound37, super[53]=sound65, super[67]=sound38, super[59]=sound38, super[45]=sound38, super[39]=sound38, super[34]=sound38, super[28]=sound38, super[21]=sound38, rage_attack[0]=sound37, rage_attack[53]=sound65, rage_attack[67]=sound38, rage_attack[59]=sound38, rage_attack[45]=sound38, rage_attack[39]=sound38, rage_attack[34]=sound38, rage_attack[28]=sound38, rage_attack[21]=sound38, jump[0]=sound70
- Sonidos de evento: hurt=sound36, die=sound34, dash=sound70

### GUERRERO (ESPADAS DEL CAOS) (`kratos_espadas`)
- Clips (35): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, block, victory, punch1, punch2, punch3, kick, atk1, atk2, atk3, atk4, dash_attack, finisher, ab0, ab1, ab2, ab3, ab4, energy, special, super, rage_attack, jump
- Habilidades: ab0 = HABILIDAD A; ab1 = HABILIDAD B; ab2 = HABILIDAD C; ab3 = COMBO DE HABILIDAD 1; ab4 = COMBO DE HABILIDAD 2
- Sonidos por cuadro: dash[0]=sound70, hit[0]=sound36, hit_high[0]=sound36, hit_low[0]=sound36, recovery[0]=sound36, defeat[0]=sound34, punch1[2]=sound75, punch1[2]=sound61, punch2[2]=sound75, punch2[2]=sound62, punch3[2]=sound75, punch3[2]=sound63, kick[5]=sound75, kick[5]=sound64, atk1[2]=sound75, atk1[2]=sound61, atk2[2]=sound75, atk2[2]=sound62, atk3[2]=sound75, atk3[2]=sound63, atk4[5]=sound75, atk4[5]=sound64, dash_attack[2]=sound75, dash_attack[2]=sound63, finisher[5]=sound75, finisher[5]=sound64, ab0[5]=sound65, ab0[7]=sound38, ab1[0]=sound65, ab1[44]=sound64, ab1[42]=sound64, ab1[38]=sound64, ab1[30]=sound64, ab1[26]=sound64, ab1[22]=sound64, ab1[18]=sound64, ab1[26]=sound38, ab2[0]=sound37, ab2[54]=sound65, ab2[68]=sound38, ab2[60]=sound38, ab2[46]=sound38, ab2[40]=sound38, ab2[35]=sound38, ab2[29]=sound38, ab2[22]=sound38, ab3[0]=sound65, ab4[0]=sound37, energy[5]=sound65, energy[7]=sound38, special[0]=sound65, special[44]=sound64, special[42]=sound64, special[38]=sound64, special[30]=sound64, special[26]=sound64, special[22]=sound64, special[18]=sound64, special[26]=sound38, super[0]=sound37, super[54]=sound65, super[68]=sound38, super[60]=sound38, super[46]=sound38, super[40]=sound38, super[35]=sound38, super[29]=sound38, super[22]=sound38, rage_attack[0]=sound37, rage_attack[54]=sound65, rage_attack[68]=sound38, rage_attack[60]=sound38, rage_attack[46]=sound38, rage_attack[40]=sound38, rage_attack[35]=sound38, rage_attack[29]=sound38, rage_attack[22]=sound38, jump[0]=sound70
- Sonidos de evento: hurt=sound36, die=sound34, dash=sound70

### GUERRERO (GARRAS DE HADES) (`kratos_garras`)
- Clips (35): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, block, victory, punch1, punch2, punch3, kick, atk1, atk2, atk3, atk4, dash_attack, finisher, ab0, ab1, ab2, ab3, ab4, energy, special, super, rage_attack, jump
- Habilidades: ab0 = HABILIDAD A; ab1 = HABILIDAD B; ab2 = HABILIDAD C; ab3 = COMBO DE HABILIDAD 1; ab4 = COMBO DE HABILIDAD 2
- Sonidos por cuadro: dash[0]=sound70, hit[0]=sound36, hit_high[0]=sound36, hit_low[0]=sound36, recovery[0]=sound36, defeat[0]=sound34, punch1[2]=sound4, punch1[2]=sound61, punch2[2]=sound4, punch2[2]=sound62, punch3[2]=sound4, punch3[2]=sound63, kick[5]=sound4, kick[5]=sound64, atk1[2]=sound4, atk1[2]=sound61, atk2[2]=sound4, atk2[2]=sound62, atk3[2]=sound4, atk3[2]=sound63, atk4[5]=sound4, atk4[5]=sound64, dash_attack[2]=sound4, dash_attack[2]=sound63, finisher[5]=sound4, finisher[5]=sound64, ab0[5]=sound65, ab0[7]=sound38, ab1[0]=sound65, ab1[44]=sound64, ab1[42]=sound64, ab1[38]=sound64, ab1[30]=sound64, ab1[26]=sound64, ab1[22]=sound64, ab1[18]=sound64, ab1[26]=sound38, ab2[0]=sound37, ab2[54]=sound65, ab2[68]=sound38, ab2[60]=sound38, ab2[46]=sound38, ab2[40]=sound38, ab2[35]=sound38, ab2[29]=sound38, ab2[22]=sound38, ab3[0]=sound65, ab4[0]=sound37, energy[5]=sound65, energy[7]=sound38, special[0]=sound65, special[44]=sound64, special[42]=sound64, special[38]=sound64, special[30]=sound64, special[26]=sound64, special[22]=sound64, special[18]=sound64, special[26]=sound38, super[0]=sound37, super[54]=sound65, super[68]=sound38, super[60]=sound38, super[46]=sound38, super[40]=sound38, super[35]=sound38, super[29]=sound38, super[22]=sound38, rage_attack[0]=sound37, rage_attack[54]=sound65, rage_attack[68]=sound38, rage_attack[60]=sound38, rage_attack[46]=sound38, rage_attack[40]=sound38, rage_attack[35]=sound38, rage_attack[29]=sound38, rage_attack[22]=sound38, jump[0]=sound70
- Sonidos de evento: hurt=sound36, die=sound34, dash=sound70

### GUERRERO (CADENA DEL RAYO) (`kratos_rayo`)
- Clips (35): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, block, victory, punch1, punch2, punch3, kick, atk1, atk2, atk3, atk4, dash_attack, finisher, ab0, ab1, ab2, ab3, ab4, energy, special, super, rage_attack, jump
- Habilidades: ab0 = HABILIDAD A; ab1 = HABILIDAD B; ab2 = HABILIDAD C; ab3 = COMBO DE HABILIDAD 1; ab4 = COMBO DE HABILIDAD 2
- Sonidos por cuadro: dash[0]=sound70, hit[0]=sound36, hit_high[0]=sound36, hit_low[0]=sound36, recovery[0]=sound36, defeat[0]=sound34, punch1[2]=sound3, punch1[2]=sound61, punch2[2]=sound3, punch2[2]=sound62, punch3[2]=sound3, punch3[2]=sound63, kick[5]=sound3, kick[5]=sound64, atk1[2]=sound3, atk1[2]=sound61, atk2[2]=sound3, atk2[2]=sound62, atk3[2]=sound3, atk3[2]=sound63, atk4[5]=sound3, atk4[5]=sound64, dash_attack[2]=sound3, dash_attack[2]=sound63, finisher[5]=sound3, finisher[5]=sound64, ab0[5]=sound65, ab0[7]=sound38, ab1[0]=sound65, ab1[44]=sound64, ab1[42]=sound64, ab1[38]=sound64, ab1[30]=sound64, ab1[26]=sound64, ab1[22]=sound64, ab1[18]=sound64, ab1[26]=sound38, ab2[0]=sound37, ab2[54]=sound65, ab2[68]=sound38, ab2[60]=sound38, ab2[46]=sound38, ab2[40]=sound38, ab2[35]=sound38, ab2[29]=sound38, ab2[22]=sound38, ab3[0]=sound65, ab4[0]=sound37, energy[5]=sound65, energy[7]=sound38, special[0]=sound65, special[44]=sound64, special[42]=sound64, special[38]=sound64, special[30]=sound64, special[26]=sound64, special[22]=sound64, special[18]=sound64, special[26]=sound38, super[0]=sound37, super[54]=sound65, super[68]=sound38, super[60]=sound38, super[46]=sound38, super[40]=sound38, super[35]=sound38, super[29]=sound38, super[22]=sound38, rage_attack[0]=sound37, rage_attack[54]=sound65, rage_attack[68]=sound38, rage_attack[60]=sound38, rage_attack[46]=sound38, rage_attack[40]=sound38, rage_attack[35]=sound38, rage_attack[29]=sound38, rage_attack[22]=sound38, jump[0]=sound70
- Sonidos de evento: hurt=sound36, die=sound34, dash=sound70

### MEDUSA (`medusa`)
- Clips (30): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, punch2, punch3, atk1, atk2, atk3, atk4, special, energy, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2
- Sonidos de evento: hurt=sound31, die=sound32

### MEDUSA ROJA (`medusa_roja`)
- Clips (30): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, punch2, punch3, atk1, atk2, atk3, atk4, special, energy, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2
- Sonidos de evento: hurt=sound31, die=sound32

### MOMIA CON ESCUDO (`momia_escudo`)
- Clips (31): idle, walk, run, dash, hit, hit_high, hit_low, recovery, airborne, air, knockdown, getup, defeat, victory, intro, punch1, kick, energy, projectile, punch2, punch3, atk1, atk2, atk3, atk4, special, dash_attack, rage_attack, finisher, super, block
- Habilidades: punch1 = ATAQUE; kick = ATAQUE 2; energy = ATAQUE A DISTANCIA
- Sonidos de evento: hurt=sound67, die=sound66

## Imagenes de piezas (actor/N.png)

706 imagenes; 430 usadas por BETA (copiadas tal cual a `assets/beta/actor`).

| Dueno (anim) | Imagenes | Usadas |
|---|---|---|
| 0, 9, 17, 34: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 4 (Garras de Hades) - basico | 8 (0..224) | 8 |
| 9: Heroe arma 2 (Cestus de Nemea) - basico | 32 (1..440) | 31 |
| 31: Mecanismo: puente levadizo | 4 (2..7) | 0 |
| 17: Heroe arma 3 (Cadena del Rayo) - basico | 22 (5..409) | 22 |
| 0: Heroe arma 1 (Espadas del Caos) - basico | 10 (6..220) | 10 |
| 60: UI pantalla de nivel superado | 6 (8..641) | 0 |
| 41: Animacion del interior del barco 1 | 5 (9..70) | 0 |
| 49: Efectos del volcan (lava, fuego, meteoros) | 6 (11..513) | 6 |
| 27, 28, 42, 43, 47, 48: Mecanismo: caja verde (frente); Mecanismo: caja verde (lado); Mecanismo: caja roja (frente); Mecanismo: caja roja (lado); Mecanismo: caja azul (frente); Mecanismo: caja azul (lado) | 1 (12..12) | 0 |
| 66: Antorcha (montana) | 1 (14..14) | 1 |
| 103: Chispa de impacto azul | 3 (15..17) | 3 |
| 4: Bestia excavadora | 10 (18..685) | 10 |
| 55: Primer plano de la montana | 1 (20..20) | 0 |
| 102: Chispa de impacto morada | 2 (21..22) | 2 |
| 56, 81, 93: Dano por caida al vacio; UI pociones roja y verde; Objetos de cofre | 1 (23..23) | 0 |
| 108: Heroe habilidad 4-2 (Garras de Hades, definitiva) | 1 (24..24) | 1 |
| 87: Heroe habilidad 2-1 (Cestus) | 3 (25..636) | 2 |
| 67, 70, 71: Alma roja; Alma verde; Alma azul | 1 (27..27) | 1 |
| 82: Chispa de impacto | 2 (28..29) | 2 |
| 25: Mecanismo: plataforma flotante | 1 (30..30) | 0 |
| 91: NPC guardian | 7 (31..501) | 0 |
| 2: Mecanismo activable (palanca) | 1 (33..33) | 0 |
| 42: Mecanismo: caja roja (frente) | 7 (34..149) | 0 |
| 86, 88, 106, 108: Heroe habilidad 1-2 (Espadas, definitiva); Heroe habilidad 2-2 (Cestus, definitiva); Heroe habilidad 3-2 (Cadena del Rayo, definitiva); Heroe habilidad 4-2 (Garras de Hades, definitiva) | 62 (37..580) | 62 |
| 0, 1, 5, 7, 76, 86: Heroe arma 1 (Espadas del Caos) - basico; Heroe habilidad 1-1 (Espadas); Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 1 - intermedio 2; Hombre de la ciudad (rojo); Heroe habilidad 1-2 (Espadas, definitiva) | 1 (40..40) | 1 |
| 36: Heroe arma 4 - intermedio 1 | 7 (41..158) | 0 |
| 12: Capa frontal del mapa (oclusion) | 1 (42..42) | 0 |
| 65: Jefe Poseidon - efectos de luz | 14 (47..99) | 0 |
| 34: Heroe arma 4 (Garras de Hades) - basico | 22 (50..476) | 22 |
| 8: Mecanismo activable (invertido) | 1 (54..54) | 0 |
| 15: Monstruo de tentaculos (jefe, solo cajas) | 1 (59..59) | 0 |
| 34, 36, 83: Heroe arma 4 (Garras de Hades) - basico; Heroe arma 4 - intermedio 1; Hombre de la ciudad (morado) | 1 (62..62) | 0 |
| 80: Efecto de subir de nivel | 3 (63..65) | 0 |
| 34, 83: Heroe arma 4 (Garras de Hades) - basico; Hombre de la ciudad (morado) | 1 (67..67) | 1 |
| 37, 81, 93: Animacion de monedas; UI pociones roja y verde; Objetos de cofre | 1 (69..69) | 0 |
| 35, 110: Centauro; Centauro rojo (variante elite) | 50 (71..655) | 38 |
| 53: Efecto nieve (exterior) | 4 (100..103) | 4 |
| 40: Heroe arma 4 - intermedio 2 | 5 (104..479) | 0 |
| 90: NPC ataque con tiempo | 7 (105..485) | 0 |
| 44: Jefe Titan - efectos de luz | 9 (107..469) | 0 |
| 6: Portal | 2 (123..124) | 0 |
| 45, 112: Medusa; Medusa roja | 28 (125..702) | 23 |
| 99: Rangos de ataque (Medusa, Centauro, Bruto, Tentaculos, Poseidon, Titan) | 4 (129..311) | 0 |
| 36, 40: Heroe arma 4 - intermedio 1; Heroe arma 4 - intermedio 2 | 2 (131..514) | 1 |
| 43: Mecanismo: caja roja (lado) | 6 (140..145) | 0 |
| 26: Mecanismo: cuerda para deslizarse | 1 (150..150) | 0 |
| 30: Mecanismo: palanca | 1 (152..152) | 0 |
| 29: Mecanismo: columpio | 1 (153..153) | 0 |
| 62: Efectos interiores del barco | 7 (154..598) | 0 |
| 34, 36, 40, 83, 107, 108: Heroe arma 4 (Garras de Hades) - basico; Heroe arma 4 - intermedio 1; Heroe arma 4 - intermedio 2; Hombre de la ciudad (morado); Heroe habilidad 4-1 (Garras de Hades); Heroe habilidad 4-2 (Garras de Hades, definitiva) | 1 (156..156) | 1 |
| 90, 91: NPC ataque con tiempo; NPC guardian | 5 (157..504) | 0 |
| sin anim: SIN DUENO (AMBIGUO) | 6 (159..682) | 0 |
| 0, 1, 5, 7, 9, 13, 16, 17, 32, 33, 34, 36, 40, 62, 69, 76, 83, 84, 85, 86, 87, 88, 92, 94, 95, 96, 97, 98, 105, 106, 107, 108: Heroe arma 1 (Espadas del Caos) - basico; Heroe habilidad 1-1 (Espadas); Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 1 - intermedio 2; Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2; Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 3 - intermedio 1; Heroe arma 3 - intermedio 2; Heroe arma 4 (Garras de Hades) - basico; Heroe arma 4 - intermedio 1; Heroe arma 4 - intermedio 2; Efectos interiores del barco; Efecto de escena 2; Hombre de la ciudad (rojo); Hombre de la ciudad (morado); Hombre de la ciudad (azul); Hombre de la ciudad (dorado); Heroe habilidad 1-2 (Espadas, definitiva); Heroe habilidad 2-1 (Cestus); Heroe habilidad 2-2 (Cestus, definitiva); Siete iconos pequenos de UI; (vacio); (vacio); (vacio); (vacio); (vacio); Heroe habilidad 3-1 (Cadena del Rayo); Heroe habilidad 3-2 (Cadena del Rayo, definitiva); Heroe habilidad 4-1 (Garras de Hades); Heroe habilidad 4-2 (Garras de Hades, definitiva) | 1 (160..160) | 1 |
| 0, 7, 9, 16, 17, 33, 34, 40, 76, 83, 84, 85: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 1 - intermedio 2; Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 2; Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 3 - intermedio 2; Heroe arma 4 (Garras de Hades) - basico; Heroe arma 4 - intermedio 2; Hombre de la ciudad (rojo); Hombre de la ciudad (morado); Hombre de la ciudad (azul); Hombre de la ciudad (dorado) | 1 (161..161) | 1 |
| 5, 7, 13, 16, 32, 33, 36, 40: Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 1 - intermedio 2; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2; Heroe arma 3 - intermedio 1; Heroe arma 3 - intermedio 2; Heroe arma 4 - intermedio 1; Heroe arma 4 - intermedio 2 | 3 (163..534) | 0 |
| 0, 9: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 2 (Cestus de Nemea) - basico | 2 (164..232) | 2 |
| 0, 7, 9, 16, 76, 85: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 1 - intermedio 2; Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 2; Hombre de la ciudad (rojo); Hombre de la ciudad (dorado) | 1 (165..165) | 1 |
| 39, 111: Bruto de la bola y cadena; Bruto de la bola rojo | 42 (168..377) | 40 |
| 61: Efectos exteriores del barco (lluvia, gotas, rayos, tornado, olas) | 14 (170..264) | 14 |
| 19, 44: Mecanismo: caja que se rompe; Jefe Titan - efectos de luz | 5 (175..471) | 0 |
| 46: Efecto de muerte de enemigo | 2 (179..180) | 2 |
| 68, 90, 91: Efecto de muerte (humo); NPC ataque con tiempo; NPC guardian | 5 (181..185) | 5 |
| 68: Efecto de muerte (humo) | 1 (186..186) | 1 |
| 78: UI efectos de la pantalla de habilidades | 5 (187..487) | 0 |
| 22: Mecanismo: pinchos del suelo | 1 (197..197) | 0 |
| 0, 17, 34: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 4 (Garras de Hades) - basico | 5 (200..206) | 5 |
| 0, 17: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 3 (Cadena del Rayo) - basico | 1 (204..204) | 1 |
| 0, 34: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 4 (Garras de Hades) - basico | 2 (208..213) | 2 |
| 0, 5, 9, 13, 16, 76, 85: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2; Hombre de la ciudad (rojo); Hombre de la ciudad (dorado) | 1 (218..218) | 0 |
| 7, 16, 33, 40: Heroe arma 1 - intermedio 2; Heroe arma 2 - intermedio 2; Heroe arma 3 - intermedio 2; Heroe arma 4 - intermedio 2 | 4 (219..659) | 0 |
| 105, 106: Heroe habilidad 3-1 (Cadena del Rayo); Heroe habilidad 3-2 (Cadena del Rayo, definitiva) | 1 (221..221) | 1 |
| 5, 13, 32, 33, 36: Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 2 - intermedio 1; Heroe arma 3 - intermedio 1; Heroe arma 3 - intermedio 2; Heroe arma 4 - intermedio 1 | 1 (225..225) | 1 |
| 0, 5, 7, 9, 13, 16, 17, 32, 33, 34, 36, 40: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 1 - intermedio 2; Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2; Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 3 - intermedio 1; Heroe arma 3 - intermedio 2; Heroe arma 4 (Garras de Hades) - basico; Heroe arma 4 - intermedio 1; Heroe arma 4 - intermedio 2 | 2 (226..265) | 0 |
| 5, 13, 32, 36: Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 2 - intermedio 1; Heroe arma 3 - intermedio 1; Heroe arma 4 - intermedio 1 | 9 (227..624) | 0 |
| 0, 5, 7, 9, 13, 16: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 1 - intermedio 2; Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2 | 1 (233..233) | 0 |
| 7, 16: Heroe arma 1 - intermedio 2; Heroe arma 2 - intermedio 2 | 4 (234..309) | 0 |
| 0, 5, 9, 13, 17, 32, 34, 36, 76, 83, 84, 85: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 3 - intermedio 1; Heroe arma 4 (Garras de Hades) - basico; Heroe arma 4 - intermedio 1; Hombre de la ciudad (rojo); Hombre de la ciudad (morado); Hombre de la ciudad (azul); Hombre de la ciudad (dorado) | 1 (237..237) | 0 |
| 74: Objetos sobre el barco | 1 (238..238) | 0 |
| 18: Mecanismo: puerta de hierro | 1 (239..239) | 0 |
| 3: Soldado esqueleto | 4 (240..243) | 4 |
| 3, 10: Soldado esqueleto; Momia con escudo | 2 (244..331) | 2 |
| 1, 87, 105, 106, 107, 108: Heroe habilidad 1-1 (Espadas); Heroe habilidad 2-1 (Cestus); Heroe habilidad 3-1 (Cadena del Rayo); Heroe habilidad 3-2 (Cadena del Rayo, definitiva); Heroe habilidad 4-1 (Garras de Hades); Heroe habilidad 4-2 (Garras de Hades, definitiva) | 2 (245..246) | 2 |
| 1, 87, 105, 107: Heroe habilidad 1-1 (Espadas); Heroe habilidad 2-1 (Cestus); Heroe habilidad 3-1 (Cadena del Rayo); Heroe habilidad 4-1 (Garras de Hades) | 35 (247..693) | 35 |
| 1, 87, 107: Heroe habilidad 1-1 (Espadas); Heroe habilidad 2-1 (Cestus); Heroe habilidad 4-1 (Garras de Hades) | 1 (248..248) | 1 |
| 1, 86, 87, 88, 105, 107: Heroe habilidad 1-1 (Espadas); Heroe habilidad 1-2 (Espadas, definitiva); Heroe habilidad 2-1 (Cestus); Heroe habilidad 2-2 (Cestus, definitiva); Heroe habilidad 3-1 (Cadena del Rayo); Heroe habilidad 4-1 (Garras de Hades) | 1 (251..251) | 1 |
| 1, 86, 87, 88, 105, 106, 107, 108: Heroe habilidad 1-1 (Espadas); Heroe habilidad 1-2 (Espadas, definitiva); Heroe habilidad 2-1 (Cestus); Heroe habilidad 2-2 (Cestus, definitiva); Heroe habilidad 3-1 (Cadena del Rayo); Heroe habilidad 3-2 (Cadena del Rayo, definitiva); Heroe habilidad 4-1 (Garras de Hades); Heroe habilidad 4-2 (Garras de Hades, definitiva) | 1 (252..252) | 1 |
| 1: Heroe habilidad 1-1 (Espadas) | 2 (262..625) | 2 |
| 35, 39, 45, 110, 111, 112: Centauro; Bruto de la bola y cadena; Medusa; Centauro rojo (variante elite); Bruto de la bola rojo; Medusa roja | 1 (263..263) | 1 |
| 5, 7, 13, 16: Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 1 - intermedio 2; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2 | 2 (267..272) | 1 |
| 5, 13: Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 2 - intermedio 1 | 5 (270..393) | 0 |
| 21: Mecanismo: fuego en el suelo | 2 (284..596) | 0 |
| 33: Heroe arma 3 - intermedio 2 | 4 (300..458) | 0 |
| 32: Heroe arma 3 - intermedio 1 | 7 (302..697) | 0 |
| 0, 5, 7, 13, 16: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 1 - intermedio 1 (salto, voltereta); Heroe arma 1 - intermedio 2; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2 | 1 (308..308) | 0 |
| 16: Heroe arma 2 - intermedio 2 | 2 (312..403) | 0 |
| 23: Mecanismo: puente de tronco | 1 (330..330) | 0 |
| 10: Momia con escudo | 4 (362..365) | 4 |
| 107, 108: Heroe habilidad 4-1 (Garras de Hades); Heroe habilidad 4-2 (Garras de Hades, definitiva) | 1 (366..366) | 1 |
| 0, 9, 15, 17, 19, 34, 41, 44, 49, 50, 54, 57, 58, 59, 60, 61, 63, 72, 75, 77, 79, 92, 94, 95, 96, 97, 98, 100, 104, 109: Heroe arma 1 (Espadas del Caos) - basico; Heroe arma 2 (Cestus de Nemea) - basico; Monstruo de tentaculos (jefe, solo cajas); Heroe arma 3 (Cadena del Rayo) - basico; Mecanismo: caja que se rompe; Heroe arma 4 (Garras de Hades) - basico; Animacion del interior del barco 1; Jefe Titan - efectos de luz; Efectos del volcan (lava, fuego, meteoros); UI registro diario; Efecto nieve (interior); UI logo; (vacio); UI ciudad principal; UI pantalla de nivel superado; Efectos exteriores del barco (lluvia, gotas, rayos, tornado, olas); Efectos de la ciudad principal; Barco destruido; UI boton generico; Duende tutorial; Monstruo de tentaculos - efectos; Siete iconos pequenos de UI; (vacio); (vacio); (vacio); (vacio); (vacio); UI cuerno; UI numeros saltarines; NPC escolta (modificado) | 1 (367..367) | 1 |
| 38: Escalon del jefe | 1 (368..368) | 0 |
| 20: Mecanismo: tubo de acero | 1 (378..378) | 0 |
| 14: Ave sanadora | 12 (390..705) | 12 |
| 24: Mecanismo: roca | 1 (391..391) | 0 |
| 9, 13, 85, 88: Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Hombre de la ciudad (dorado); Heroe habilidad 2-2 (Cestus, definitiva) | 1 (392..392) | 1 |
| 86, 87, 88, 106, 108: Heroe habilidad 1-2 (Espadas, definitiva); Heroe habilidad 2-1 (Cestus); Heroe habilidad 2-2 (Cestus, definitiva); Heroe habilidad 3-2 (Cadena del Rayo, definitiva); Heroe habilidad 4-2 (Garras de Hades, definitiva) | 1 (395..395) | 1 |
| 17, 32, 33: Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 3 - intermedio 1; Heroe arma 3 - intermedio 2 | 2 (404..406) | 0 |
| 32, 33: Heroe arma 3 - intermedio 1; Heroe arma 3 - intermedio 2 | 2 (405..584) | 1 |
| 9, 13, 85: Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Hombre de la ciudad (dorado) | 1 (410..410) | 0 |
| 9, 13, 16: Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2 | 2 (411..412) | 0 |
| 13, 16: Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2 | 1 (413..413) | 1 |
| 9, 13, 16, 85, 87, 88: Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 1; Heroe arma 2 - intermedio 2; Hombre de la ciudad (dorado); Heroe habilidad 2-1 (Cestus); Heroe habilidad 2-2 (Cestus, definitiva) | 2 (414..495) | 2 |
| 79: Monstruo de tentaculos - efectos | 15 (416..464) | 0 |
| 17, 32, 33, 84, 105, 106: Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 3 - intermedio 1; Heroe arma 3 - intermedio 2; Hombre de la ciudad (azul); Heroe habilidad 3-1 (Cadena del Rayo); Heroe habilidad 3-2 (Cadena del Rayo, definitiva) | 1 (455..455) | 1 |
| 17, 84: Heroe arma 3 (Cadena del Rayo) - basico; Hombre de la ciudad (azul) | 1 (456..456) | 1 |
| 34, 40: Heroe arma 4 (Garras de Hades) - basico; Heroe arma 4 - intermedio 2 | 1 (477..477) | 0 |
| 34, 36, 40: Heroe arma 4 (Garras de Hades) - basico; Heroe arma 4 - intermedio 1; Heroe arma 4 - intermedio 2 | 1 (478..478) | 0 |
| 106: Heroe habilidad 3-2 (Cadena del Rayo, definitiva) | 1 (486..486) | 1 |
| 89: NPC escolta | 7 (488..494) | 0 |
| 9, 16, 85: Heroe arma 2 (Cestus de Nemea) - basico; Heroe arma 2 - intermedio 2; Hombre de la ciudad (dorado) | 1 (496..496) | 1 |
| 63: Efectos de la ciudad principal | 7 (516..522) | 0 |
| 86, 88, 106: Heroe habilidad 1-2 (Espadas, definitiva); Heroe habilidad 2-2 (Cestus, definitiva); Heroe habilidad 3-2 (Cadena del Rayo, definitiva) | 1 (561..561) | 1 |
| 101: Chispa de impacto dorada | 1 (582..582) | 1 |
| 92: Siete iconos pequenos de UI | 1 (586..586) | 0 |
| 18, 19, 20, 23, 24, 26, 27, 28, 29, 30, 42, 43, 47, 48, 51, 64, 73: Mecanismo: puerta de hierro; Mecanismo: caja que se rompe; Mecanismo: tubo de acero; Mecanismo: puente de tronco; Mecanismo: roca; Mecanismo: cuerda para deslizarse; Mecanismo: caja verde (frente); Mecanismo: caja verde (lado); Mecanismo: columpio; Mecanismo: palanca; Mecanismo: caja roja (frente); Mecanismo: caja roja (lado); Mecanismo: caja azul (frente); Mecanismo: caja azul (lado); Jefe Titan (solo cajas y tiempos); Jefe Poseidon (solo cajas y tiempos); Duende en la pared | 1 (597..597) | 0 |
| 27: Mecanismo: caja verde (frente) | 7 (600..606) | 0 |
| 28: Mecanismo: caja verde (lado) | 6 (607..612) | 0 |
| 105: Heroe habilidad 3-1 (Cadena del Rayo) | 1 (626..626) | 1 |
| 11: Bestia elefante | 16 (629..675) | 16 |
| 47: Mecanismo: caja azul (frente) | 7 (642..648) | 0 |
| 48: Mecanismo: caja azul (lado) | 6 (649..654) | 0 |
| 19: Mecanismo: caja que se rompe | 6 (665..670) | 0 |
| 107: Heroe habilidad 4-1 (Garras de Hades) | 1 (671..671) | 1 |
| 52: Luz de curacion de enemigo | 3 (689..695) | 3 |
| 17, 32, 84: Heroe arma 3 (Cadena del Rayo) - basico; Heroe arma 3 - intermedio 1; Hombre de la ciudad (azul) | 1 (696..696) | 0 |
| 93: Objetos de cofre | 1 (698..698) | 0 |

## Sonidos y musica (sound/*.ogg)

| Archivo | Evento original (es) | Uso en BETA |
|---|---|---|
| sound1.ogg | abrir ventana; salir a pelear | no usado |
| sound2.ogg | cerrar ventana | volver |
| sound3.ogg | Cadena del Rayo (bucle) | kratos_rayo |
| sound4.ogg | Garras de Hades (bucle) | kratos_garras |
| sound5.ogg | Cestus de Nemea (bucle) | no usado |
| sound6.ogg | pocion | no usado |
| sound7.ogg | moneda | no usado |
| sound8.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound9.ogg | fragmento | no usado |
| sound10.ogg | abrir cofre | no usado |
| sound11.ogg | mejora de habilidad; aviso tutorial | no usado |
| sound12.ogg | carga de mejora | no usado |
| sound13.ogg | cambiar habilidad | mover cursor |
| sound14.ogg | cambiar arma | cambiar personaje |
| sound15.ogg | estrella de arma | no usado |
| sound16.ogg | mejora de arma ok | no usado |
| sound17.ogg | mejora de arma fallida | no usado |
| sound18.ogg | equipar arma | no usado |
| sound19.ogg | desbloquear arma | no usado |
| sound20.ogg | subir de nivel | no usado |
| sound21.ogg | comprar | no usado |
| sound22.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound23.ogg | nivel nuevo | no usado |
| sound24.ogg | aviso de confirmacion | confirmar / iniciar pelea |
| sound25.ogg | aviso de tipo de nivel | no usado |
| sound26.ogg | cofre del nivel | no usado |
| sound27.ogg | cuenta 3-2-1 | no usado |
| sound28.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound29.ogg | pausa | no usado |
| sound30.ogg | cambio de torre | no usado |
| sound31.ogg | medusa: recibe golpe | medusa, medusa_roja |
| sound32.ogg | medusa: muere | medusa, medusa_roja |
| sound33.ogg | embestida (provisional) | no usado |
| sound34.ogg | muerte del heroe | kratos_cesto, kratos_espadas, kratos_garras, kratos_rayo |
| sound35.ogg | revivir | no usado |
| sound36.ogg | heroe herido | kratos_cesto, kratos_espadas, kratos_garras, kratos_rayo |
| sound37.ogg | 技能3正数第1帧 | kratos_cesto, kratos_espadas, kratos_garras, kratos_rayo |
| sound38.ogg | alma roja; 技能1倒数第15帧; 技能2倒数第20帧; 技能3倒数第8,30 | kratos_cesto, kratos_espadas, kratos_garras, kratos_rayo |
| sound39.ogg | centauro: recibe golpe | centauro, centauro_rojo |
| sound40.ogg | centauro: muere | centauro, centauro_rojo |
| sound41.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound42.ogg | NPC escolta: golpe | no usado |
| sound43.ogg | NPC escolta: muere | no usado |
| sound44.ogg | aviso de mecanismo | aparece oleada |
| sound45.ogg | portal | no usado |
| sound46.ogg | puerta de mecanismo | no usado |
| sound47.ogg | Atenea/Zeus NPC | no usado |
| sound48.ogg | victoria | victoria |
| sound49.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound50.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound51.ogg | tentaculos: muere | no usado |
| sound52.ogg | tentaculos: recibe golpe | no usado |
| sound53.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound54.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound55.ogg | Titan: muere | no usado |
| sound56.ogg | Titan: recibe golpe | no usado |
| sound57.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound58.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound59.ogg | Poseidon: recibe golpe | no usado |
| sound60.ogg | Poseidon: muere | no usado |
| sound61.ogg | golpe 1 del combo | kratos_espadas, kratos_garras, kratos_rayo |
| sound62.ogg | golpe 2 | kratos_espadas, kratos_garras, kratos_rayo |
| sound63.ogg | golpe 3; 技能2倒数第2帧 | kratos_espadas, kratos_garras, kratos_rayo |
| sound64.ogg | golpe 4; 技能2倒数第2,4 | kratos_cesto, kratos_espadas, kratos_garras, kratos_rayo |
| sound65.ogg | golpe 5; 技能1正数第1帧; 技能2正数第1帧; 技能3倒数第22帧 | kratos_cesto, kratos_espadas, kratos_garras, kratos_rayo |
| sound66.ogg | esqueleto/momia/excavador: muere | ave_sanadora, esqueleto, excavador, momia_escudo |
| sound67.ogg | esqueleto/momia/excavador: recibe golpe | ave_sanadora, esqueleto, excavador, momia_escudo |
| sound68.ogg | elefante/bruto: recibe golpe | bestia_elefante, bruto_cadena, bruto_cadena_rojo |
| sound69.ogg | elefante/bruto: muere | bestia_elefante, bruto_cadena, bruto_cadena_rojo |
| sound70.ogg | embestida / voltereta; preparar salto | kratos_cesto, kratos_espadas, kratos_garras, kratos_rayo |
| sound71.ogg | AMBIGUO: sin referencia en GameMusicFile.lua | no usado |
| sound72.ogg | tirar palanca | no usado |
| sound73.ogg | Cestus golpe 1 | kratos_cesto |
| sound74.ogg | Cestus golpe 3 | kratos_cesto |
| sound75.ogg | Espadas del Caos | kratos_espadas |
| menu.ogg | menu principal (usado en la seleccion BETA) | musica |
| gameCG.ogg | cinematica inicial (no usado) | no usado |
| music_001.ogg | ciudad / menus internos (no usado) | no usado |
| gate1music.ogg | niveles barco/montana (BETA) | musica |
| gate2music.ogg | niveles volcan (BETA) | musica |
| music_002.ogg | victoria (BETA) | musica |
| music_003.ogg | derrota (BETA) | musica |

## Niveles (mapdata) y escenarios

Tipos segun `data/GameSeting.lua`: BoatGate = barco exterior con lluvia, BoatInGate = interior del barco,
SnowOutGate = montana exterior, SnowInGate = cueva helada, FireOutGateBackRound = volcan.

| Escenario BETA | Nivel | Tamano | Franja caminable (px del mapa) | Fondos | Musica | Clima |
|---|---|---|---|---|---|---|
| ARENA DEL VOLCAN | mapdata/142.XML | 1440 528 | 336 432 0 1392 | bg/Huoshanbeijing.png, bg/HuoShanZhongJing_02.png, bg/HuoShanZhongJing_01.png | gate2music.ogg | ceniza |
| CUBIERTA DEL BARCO | mapdata/101.XML | 4800 624 | 432 528 384 4608 | bg/Sky.png, bg/Sea.png, bg/Boat_04.png, bg/Boat_03.png, bg/Boat_02.png, bg/Boat_01.png | gate1music.ogg | lluvia |
| CUEVA HELADA | mapdata/282.XML | 1440 576 | 384 480 0 1440 | bg/Xueshanneibeijing.png, bg/Xueshanneizhongjing_2.png, bg/Xueshanneizhongjing_1.png, bg/Xueshanneizhongjing_4.png | gate1music.ogg | nieve |
| MONTANA NEVADA | mapdata/201.XML | 3168 576 | 384 480 48 3168 | bg/Xueshanwaibeijing.png, bg/Xueshanwaizhongjing_4.png, bg/Xueshanwaizhongjing_5.png, bg/Xueshanwaizhongjing_1.png, bg/Xueshanwaizhongjing_3.png | gate1music.ogg | nieve |
| PASO DE LA MONTANA | mapdata/351.XML | 5520 576 | 384 480 240 4512 | bg/Xueshanwaibeijing.png, bg/Xueshanwaizhongjing_4.png, bg/Xueshanwaizhongjing_5.png, bg/Xueshanwaizhongjing_1.png, bg/Xueshanwaizhongjing_3.png | gate1music.ogg | nieve |
| CAMINO DEL VOLCAN | mapdata/301.XML | 4800 576 | 384 480 0 4800 | bg/Huoshanbeijing.png, bg/HuoShanZhongJing_02.png, bg/HuoShanZhongJing_01.png | gate2music.ogg | ceniza |

Niveles no usados: los de 22 filas (interior del barco 111/112/191/252, montana 212/241) son de
plataformas verticales con mecanismos; los demas repiten los mismos tilesets que los 6 elegidos.

## Spine (jefes y QTE) - PENDIENTE

- `spine/Titan`, `spine/Poseidon`, `spine/PoseidonBaby` (= monstruo de tentaculos, anim 15): Spine 2.1.27 con
  mallas y mallas con huesos. Sus cajas y tiempos estan en anim 51/64/15 (cuadros con imagen de relleno 597).
  Para integrarlos sin deformar hay que hornear las animaciones Spine a cuadros (pendiente).
- `ChainfattyQTE`, `medsuaQTE`, `RenMaQTE`, `PoseidonQTE`, `PoseidonBabyQTE`: cinematicas de remate (QTE).
- `Menu/zhujue.json`: heroe animado del menu original.

## Archivos ambiguos

- anim 58, 94-98: archivos marcados 无 (vacio) con una imagen de relleno.
- anim 69 (efecto de escena 2) y 12 (capa frontal): un solo cuadro sin nivel claro que los use.
- actor sin anim que los referencie: ver la fila 'sin anim' en la tabla de imagenes.
- sound sin entrada en GameMusicFile.lua: ver 'AMBIGUO' en la tabla de sonidos.

## Glosario chino -> espanol

| Chino | Espanol |
|---|---|
| 主角 | heroe / protagonista |
| 技能 | habilidad / tecnica |
| 待战 / 待机 | guardia / reposo |
| 跑 / 移动 | correr / moverse |
| 前翻 | voltereta hacia adelante (esquiva) |
| 跳 / 蓄力准备跳 | salto / preparar salto |
| 攻击一段..四段 | ataque golpe 1..4 del combo |
| 连击 | combo |
| 被打 | recibir golpe |
| 击飞 / 被打起飞 | lanzado por el aire |
| 倒地 | caido en el suelo |
| 起身 | levantarse |
| 死亡 | muerte |
| 出场 | entrada en escena |
| 休息 | descanso (pose de victoria) |
| 准备 / 中 / 结束 | preparacion / en curso / final |
| 近身攻击 | ataque cuerpo a cuerpo |
| 远程攻击 | ataque a distancia |
| 飞镖 | proyectil |
| 框 / 红框 / 攻击框 | caja / caja roja / caja de ataque |
| 光效 | efecto de luz |
| 打击光效 | chispa de impacto |
| 机关 | mecanismo |
| 骷髅兵 | soldado esqueleto |
| 钻地怪 | bestia excavadora |
| 带盾干尸兵 | momia con escudo |
| 象怪 | bestia elefante |
| 加血鸟 | ave sanadora |
| 人马 / 半人马 | centauro |
| 链球怪 | bruto de la bola y cadena |
| 美杜莎 / 蛇发女妖 | Medusa / gorgona |
| 触手怪 | monstruo de tentaculos |
| 泰坦 | Titan |
| 波塞冬 | Poseidon |
| 精英 | elite |
| 红 (名) | rojo / variante mutada |
| 混沌刃 / 混沌之刃 | Espadas del Caos |
| 拳套 / 尼米亚拳套 | Cestus de Nemea |
| 电光链 / 宙斯电光链 | Cadena del Rayo (de Zeus) |
| 钩爪 / 哈迪斯钩爪 | Garras de Hades |
| 雪山外 / 雪山内 | montana nevada exterior / interior (cueva) |
| 火山 | volcan |
| 战船 / 船外 / 船内 | barco de guerra / exterior / interior |
| 下雨 / 闪电 | lluvia / relampago |
| 碰撞 | colision |
| 红魂 / 绿魂 / 蓝魂 | alma roja / verde / azul |
| 主城 | ciudad principal |
| 千层塔 | torre de mil pisos (modo desafio) |
| 护送 / 守护 / 限时攻击 | escolta / defensa / ataque con tiempo |
