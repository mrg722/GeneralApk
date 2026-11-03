# Auditoria del motor de combate — District Fury (rama ccr-077ad675-rlrg28)

Auditoria de SOLO LECTURA. No se modifico ningun fuente. Evidencia empirica con un programa de sondeo
propio (`/tmp/claude-0/-home-user/05b0c2b9-43b9-5c8e-82ee-61da3b1aff5a/scratchpad/audit/probe/probe.cpp`,
enlazado contra `build/libdistrict_fury_core.a`) mas `ctest`, `all_modes_check` y `gow_check`
(los tres en verde hoy: ctest 7/7, all_modes `problemas: 0`, gow_check `problemas: 0`).

Convenciones: tiempos en segundos y, entre corchetes, frames a 60 fps. "py/ey" = `position.y` (carril)
del jugador/enemigo; "z" = altura.

---

## 0. Resumen ejecutivo (lo que hay que saber antes de planificar)

1. **El `CombatWorld` NO resuelve el combate cuerpo a cuerpo.** `ResolvePlayerMelee`, `ResolvePlayerMeleeOnBoss`,
   `ResolveEnemyMelee` y `ResolveProjectiles` (CombatWorld.cpp:400-628) no los llama nadie. Cada stage
   (Stage1Combat.cpp, Stage2..5Game.cpp, VSMode.cpp, GowApk/BetaMode.cpp) tiene su propia copia, distinta.
   Consecuencias directas: `OnAttackConnected` nunca se ejecuta en juego real -> **la ventana de combo nunca se
   abre, `comboCount` siempre es 0, el FINISHER por combo es inalcanzable, `rageGain` nunca suma, `comboWindowBonus`
   (mejora de campana) no hace nada, `def.hitstun` no se aplica, `def.launch` solo se aplica en Stage1, `def.hitstop`
   /`def.shake` solo en Stage1.** Cualquier sistema nuevo (juggle, cancel por hit-confirm, contraataque, escalado)
   exige PRIMERO unificar la resolucion en un solo punto.
2. **La Onda de Energia genera ~21 proyectiles por lanzamiento** en todos los stages de Historia (bug de flag
   `energyReleased`, ver B01). En VS no genera ninguno.
3. **La profundidad (carril) no se comprueba**: `def.depth` y `def.range` no se usan en ninguna colision. La caja
   2D mezcla altura de pantalla y carril: un JAB conecta con un Punk desde 99 px "detras" hasta 55 px "delante" en
   carril (el carril entero mide 120). Las cajas no escalan con la profundidad ni con el tamano del personaje.
4. **No hay salto, ni ataques aereos, ni juggle real, ni OTG.** `launch` produce saltitos de 5-30 px de altura;
   la hurtbox del enemigo ignora `z`; un enemigo en el aire puede ser re-golpeado sin limite (solo Stage1).
5. **No hay sistema de direccion+boton** (adelante+J, atras+K, abajo+K). Solo existen: J (cadena P1-P2-P3),
   K (patada), QCF+J (onda), DP+K (= Punch3), L, Espacio, Shift(+J), 1-6.
6. Hay buenos cimientos reutilizables: Input Buffer de 15 frames con cola, `MotionHistory` (QCF/DP), tabla
   `AttackDef`, root motion con curva (`AttackRootMotionProgress`), squash/stretch por fase, estelas, `HitstopClock`,
   cajas por cuadro en personajes BETA, IA de rival que reutiliza `Player`. El atlas de MAX tiene ~196 celdas sin
   usar con salto, ataques aereos, barridos y ganchos (ver 4.2).

---

## 1. Inventario exacto del moveset actual

### 1.1 Tabla de ataques (`src/game/combat/AttackData.cpp:12-93`)

| Id | Nombre | Dano | Startup / Active / Recovery | Total | Caja W x H, centro (fwd, offY) | Knockback | Hitstun | Launch | SP | Rage | Prio | CD | canCombo/canCancel/breaksGuard/heavy/proj | Hitstop / Shake | comboWindow |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Punch1 | GOLPE | 10 | .09/.13/.08 [5/8/5] | .30 [18] | 82x48 (54,-76) | 280 | .22 | 0 | 0 | 7 | 1 | 0 | T/T/F/F/F | .060/.060 | .48 |
| Punch2 | GOLPE 2 | 12 | .08/.13/.09 [5/8/5] | .30 [18] | 88x50 (58,-76) | 320 | .24 | 0 | 0 | 8 | 1 | 0 | T/T/F/F/F | .070/.075 | .46 |
| Punch3 | REMATE | 18 | .11/.15/.16 [7/9/10] | .42 [25] | 96x56 (62,-74) | 470 | .34 | 180 | 0 | 12 | 2 | 0 | F/F/T/T/F | .110/.150 | .30 |
| Kick | PATADA | 16 | .11/.17/.14 [7/10/8] | .42 [25] | 122x58 (66,-64) | 440 | .32 | 140 | 0 | 11 | 2 | .05 | F/F/F/T/F | .105/.140 | .34 |
| EnergyWave | ONDA DE ENERGIA | 20 | .20/.23/.12 [12/14/7] | .55 [33] | 220x76 (122,-72) (no se usa: proyectil) | 500 (proyectil usa 420) | .30 | 0 | 20 | 6 | 3 | .18 | F/F/T/T/T | .100/.130 | .40 |
| DashAttack | EMBESTIDA | 15 | .06/.16/.16 [4/10/10] | .38 [23] | 104x60 (60,-70) | 520 | .30 | 0 | 0 | 10 | 2 | .35 | F/F/T/T/F | .095/.130 | .36 |
| RageAttack | FURIA | 30 | .13/.22/.20 [8/13/12] | .55 [33] | 230x96 (96,-78) | 620 | .42 | 220 | 0 | 0 | 4 | .90 | F/F/T/T/F | .140/.240 | .30 |
| Finisher | FINISHER | 44 | .16/.24/.26 [10/14/16] | .66 [40] | 250x110 (104,-80) | 760 | .55 | 300 | 35 | 0 | 5 | 1.60 | F/F/T/T/F | .180/.300 | .30 |
| Skill | HABILIDAD | 9 | 0/0/0 (lo marca el clip) | clip | 160x110 (80,-70) (o caja del cuadro) | 420 | .30 | 120 | 0 | 6 | 4 | 0 | F/F/T/T/F | .070/.120 | .30 |

Numeros derivados (fisica actual, g = 1500 px/s2, StreetEnemy.cpp:501, damping Hit 0.84/frame StreetEnemy.cpp:520):
- Altura maxima de `launch` = v^2/(2g): Punch3 10.8 px (0.24 s en el aire), Kick 6.5 px (0.19 s), Rage 16 px (0.29 s),
  Finisher 30 px (0.40 s), Skill 4.8 px. **Ninguno es un lanzador real** (un lanzador jugable necesita v ~ 650-750:
  140-190 px, ~0.9-1.0 s).
- Deslizamiento del enemigo en Hit ~ v/60/(1-0.84) = 0.104*v: P1 29 px, P2 33, P3 49, Kick 46, Dash 54, Rage 64, Fin 79.
- `priority`, `canCancel`, `canCombo` (salvo en `OnAttackConnected`, que no se llama), `range`, `depth`,
  `rageGain`, `hitstun` (salvo CombatWorld muerto): **no se leen en ningun camino vivo** (grep: solo CombatWorld.cpp
  y PlayerCombat.cpp:216-222/462-466).

### 1.2 Como se dispara cada cosa

Entrada (teclado `PlayerCombat.cpp:321-340`; tactil `src/ui/TouchControls.cpp:38-51`):
W/S = carril arriba/abajo, A/D = izq/der, B = bloqueo (mantener), Shift = dash, J = golpe, K = patada,
L = onda, Espacio = furia, TAB/PAG = pagina de habilidades, 1-6 = habilidades, Q (VS) = cambiar arma del guerrero,
F9 = debug de animacion. Tactil: GOLPE, PATADA, 1-6, ESPECIAL (= onda directa, `kVirtualSpecialWave`), PAG,
BLOQ, DASH, FURIA. No hay boton tactil de L ni de "gancho" en el layout de combate (existe `kVirtualSpecialRise`
pero no tiene boton). No hay salto.

Flujo (`Player::Update`, Player.cpp:278-412): `PumpInput` lee entrada y empuja J/K al `InputBuffer`
(8 entradas, caducan a los 15 frames, InputBuffer.h). `PollAttackInput` (PlayerCombat.cpp:297-319) convierte
J en `SpecialWave` si `MotionHistory::QuarterCircle` reconoce abajo->(abajo-adelante)->adelante en 15 frames,
y K en `SpecialRise` si `DragonPunch` reconoce adelante->abajo->abajo-adelante.

| Accion | Input | Resultado | Donde |
|---|---|---|---|
| Golpe | J (Idle/Walk/Recovery) | Punch1; si `comboWindow>0`: siguiente de `NextPunchInChain` (P1->P2->P3->P1) | PlayerCombat.cpp:379-381 |
| Cadena por cancel | J/K en ventana de cancel (desde `startup+active`, es decir solo en recovery) | `ResolveCancel`: P1+J->P2, P2+J->P3, P3+J->nada, P1/P2/P3+K->Kick, Kick+J->P1, Kick+K->nada | PlayerCommon.h:51-65, PlayerCombat.cpp:349-355 |
| Patada | K | Kick; Finisher si `isRageMode && comboCount>=4 && sp>=35 && CD listo` (inalcanzable, ver B03) | PlayerCombat.cpp:374-378 |
| Onda | QCF+J, boton ESPECIAL, L, habilidad 1 | EnergyWave (20 SP; sin SP el QCF sale como golpe) | PlayerCombat.cpp:305-311, 272-276, 384-388 |
| "Gancho" | DP+K | **Punch3** (el mismo REMATE de la cadena) | PlayerCombat.cpp:372-373 |
| Special cancel | QCF+J sobre cualquier normal; DP+K sobre P1/P2/Kick | EnergyWave / Punch3 | PlayerCommon.h:55-56 |
| Embestida | J durante el dash (0.12 s, 700 px/s = 84 px, invuln 0.20 s, CD 0.38 s) | DashAttack | Player.cpp:363-378, PlayerCombat.cpp:256-266 |
| Furia | Espacio con rage lleno -> Starting 0.45 s -> Active 9 s (drena ~11/s) | Espacio en Active -> RageAttack | PlayerCombat.cpp:278-291, Player.cpp:213-253 |
| Habilidades (nuestros) | 1..5 | 1=EnergyWave, 2=Punch3, 3=RageAttack, 4=DashAttack, 5=Finisher; sin coste de SP, CD 15 s, superarmadura | PlayerCombat.cpp:135-143 |
| Habilidades (KF) | 1..5 + PAG | clips `abN`/`skill1-5` (todas las habilidades del personaje de 5 en 5) | PlayerCombat.cpp:129-133 |
| Transformar | 6 | 12 s: dano x1.4, velocidad x1.15, aura | PlayerCombat.cpp:119-128 |
| Bloqueo | B mantenido | reduce 78 %, absorbe con escudo, empuje 55, GuardBreak 0.42 s si escudo=0 | Player.cpp:442-454 |

Multiplicadores de dano (PlayerCombat.cpp:435-444): upgrades x transform 1.4 x rage 1.35 x buff 1.25;
KF normales x0.6. Los stages suman ademas +5/+6/+8/+9 planos en rage (doble bonus, distinto por stage).
`CharacterVisual::damageMultiplier/speedMultiplier` estan declarados pero **no se usan** (grep).

### 1.3 Recepcion de dano del jugador (`Player::TakeDamage`, Player.cpp:436-498)
- Sin direccion del atacante. Golpe normal: `Hit` 0.13 s + invuln 0.28 s, **sin knockback** (sondeo #12: dx=0).
- `damage>=28` -> Airborne (vx 260 hacia atras de SU facing, vz 380 = 48 px) -> Knockdown 0.85 s -> getup con 0.55 s invuln.
- Superarmadura total (no cancela, flash 0.16 s, empujon 10 px) si `activeSkill>=0` o el ataque es
  Skill/EnergyWave/RageAttack/Finisher.
- Mientras `Hit` con `stateTimer>0`, Knockdown o Airborne: ignora golpes.

### 1.4 Enemigos (`StreetEnemy.cpp`, perfiles `EnemyBrain.cpp:72-142`)
- Un solo ataque por tipo: `attackDamage`, activo entre 34 % y 74 % de `attackDuration` (StreetEnemy.cpp:619-629);
  caja W = 0.68*range, H = 52, centro (0.52*range, -66) fija (StreetEnemy.cpp:638-658), sin escala por tamano.
- Especiales: Charge (520 px/s), ChemicalCloud (hazard), Flank, GuardCounter, HeavySlam (220x78), Frenzy (3 golpes).
- Hurtbox = bodyWidth x bodyHeight sobre `position.y`, **ignora z** (StreetEnemy.cpp:631-636).
- TakeDamage: stun fijo 0.30 s (hitstun real de la tabla no llega), armor ignora golpes bajo umbral en Attack/Special,
  `launch>0` o z>0 -> Airborne; al caer -> Hit 0.45 s (no hay estado "en el suelo", no hay OTG ni invuln al levantarse).
- `attackDepth` solo lo usa la IA para decidir atacar (StreetEnemy.cpp:397).

### 1.5 Bosses y VS
- `Boss` (VS) caja: `facingRight ? pos.x : pos.x - boxForward - boxWidth` (Boss.cpp:167) -> asimetrica (a la derecha
  ignora `boxForward`, a la izquierda lo suma).
- VS jugador vs rival (VSMode.cpp:331-342): sin hitstop, sin knockback, sin `OnAttackConnected`, sin prioridad;
  ambos golpes del mismo frame entran (trade siempre). VS vs bosses/enemigos: sin knockback de launch, sin hitstun.
- BETA (GowApk/src/BetaMode.cpp:435-457): unico sitio con chequeo de carril (|dy|<=42); el heroe **no marca
  `hasHit`** (golpea cada frame activo, limitado solo por la invuln de 0.28 s del objetivo).

### 1.6 Tipos de personaje y como se comporta su ataque hoy
| Tipo | Personajes (skin) | Dibujo | Tiempo del golpe | Caja de golpe | Root motion | Squash/estelas |
|---|---|---|---|---|---|---|
| A1 atlas manifiesto sin perfil | Rayden 0 | `animator` manifiesto "rayden" (24 cuadros) | `AttackDef` | fija de `AttackDef` | lineal 14-46 px (PlayerDraw.cpp:139-158) | generico sin(pi t) |
| A2 atlas manifiesto con perfil | Brakk 2 (`brakk_v2`), MAX 6 (`max`, transform `max_white`) | manifiesto; clip escalado a la duracion del `AttackDef` (PlayerCombat.cpp:171) | `AttackDef` | fija | `AttackRootMotionProgress` (ease-in/ease-out) x `profile.rootMotion` | anticipacion/impacto/recoil por fase |
| B poses sueltas | Clon antiguo 1, Grinder 3, Titan-X 4, Titan-X Mejorado 5 | `CharacterPose` por `AttackType` (CharacterVisual.cpp:116-187); `animator` interno = atlas de Rayden invisible | `AttackDef` (skin 1 con poses por fase, PlayerDraw.cpp:243-255; 3-5 una pose fija por tipo) | fija | lineal | **ninguno** (DrawSpriteUniform/DrawBossPose sin escala ni giro; sin estelas) |
| C KF por clip | KF HEROE..BUFONA 7-19, RAYDER CLON BETA, RAYDER CLON, RONIN | clip de la APK (atk1-4, special, super, abN) | clip completo; activo 12 %-92 %; re-golpe cada 0.30 s (0.26 en Skill), dano x0.6 | extension del dibujo del cuadro (frente>=60 px, detras 30 o **400 px** en Skill), alto completo | ninguna (el arte trae sus pasos) | no (arte propio) |
| D BETA por piezas | GUERRERO x4, RONIN-KRATOS x4, bestias, medusas... | piezas + caja de ataque/cuerpo del cuadro original | cuadros con `hitbox` | **caja del cuadro** (y hurtbox del cuadro, con z) | ninguna | no |

---

## 2. Bugs y defectos concretos

Severidad: [C] critico (rompe el juego/balance), [A] alto, [M] medio, [B] bajo.

**B01 [C] La onda de energia dispara ~21 proyectiles por uso.**
`Player.cpp:384-386` vuelve a poner `energyReleased=true` en cada frame con `attackElapsed>=startup`; cada stage
lo consume y lo pone a false (`stage1/Stage1Combat.cpp:180-184`, `stage1/Stage1Boss.cpp:193-195`,
`Stage2Game.cpp:234-238`, `Stage3Game.cpp:350-352`, `Stage4Game.cpp:247-249`, `Stage5Game.cpp:219-221`).
Sondeo #1: **21 spawns** en un lanzamiento (todo active+recovery). Con 23 de dano cada uno (Stage1) un
enemigo en la trayectoria recibe ~480. Aplica tambien a la habilidad 1 de nuestros personajes. Arreglo: flag
`projectileSpawned` separado que solo `BeginAttack` resetea, o que el Player exponga un evento de un solo disparo
(`ConsumeProjectileEvent()`).

**B02 [C] La resolucion de combate esta duplicada y la del CombatWorld es codigo muerto.**
CombatWorld.cpp:400-628 sin llamadas (solo `ResolveHazards`). Las copias divergen: objetivo "mas cercano"
(Stage2/4) vs "primero de la lista" (Stage1/3/5); launch solo en Stage1 (Stage1Combat.cpp:112) y 0 en el resto
(Stage2Game.cpp:174, Stage3:285, Stage4:183, Stage5:182, VSMode.cpp:294); hitstop fijo .08/.085 en Stage2-5;
nadie llama `ApplyHitstun` ni `OnAttackConnected`.

**B03 [A] La ventana de combo nunca se abre en juego real -> Finisher inalcanzable, rageGain muerto.**
`OnAttackConnected` (PlayerCombat.cpp:215-223) solo lo llama CombatWorld (muerto). Por eso: rama
`comboWindow>0 ? NextPunchInChain` (PlayerCombat.cpp:380, 387) nunca se toma; `comboCount>=4` del Finisher
(375-378) nunca se cumple; `AddRage(def.rageGain)` nunca ocurre (la rabia solo sube al recibir/bloquear golpes o
por pickup); la mejora `comboWindowBonus` (Campaign.cpp:46) no tiene efecto. Ademas, aunque se arreglase, el camino
de cancel (`ResolveCancel`) nunca puede dar Finisher (sondeo #9: K en cancel de P1 con rage y combo 5 -> PATADA).

**B04 [A] Al expirar `comboWindow` en mitad de un ataque, `currentAttack` pasa a `None`.**
Player.cpp:306-311 se ejecuta antes de la FSM y pone `currentAttack=AttackId::None` aunque `state==Attack`.
Escenario: Kick conecta (comboWindow = max(.., .18)) -> expira a los .18 s con la patada aun activa (total .42).
Sondeo #2: 16 frames en Attack con `currentAttack==None`: la hitbox desaparece, `GetAttack(None)` da todo 0,
`InCancelWindow` se abre al instante, el root motion y el squash usan la fila vacia, `heavy` se pierde en
`EndAttack`. Latente hoy (B03 impide que se abra), se activara en cuanto se arregle B03. Arreglo: no tocar
`currentAttack` si `state==Attack`; separar "ultimo ataque para la cadena" (`chainAttack`) de "ataque en curso".

**B05 [A] La profundidad no se valida; `def.depth`/`def.range` no se usan.**
`CombatBox` es 2D en (x, y de pantalla) (Types.h:21-38). La caja del jugador se coloca en `position.y + boxCenterOffsetY`
y la hurtbox del enemigo en `[ey-bodyHeight, ey]`. El solape en Y mezcla altura y carril. Sondeo #3 (Punk):

| Ataque | Carril que aun conecta (ey-py) | def.depth |
|---|---|---|
| Punch1 | [-99, +55] | 42 |
| Kick | [-92, +72] | 44 |
| RageAttack | [-125, +77] | 62 |
| Finisher | [-134, +82] | 64 |
| Ataque del Punk al jugador | [-71, +84] | attackDepth 38 (solo IA) |
| KF clip / Skill (caja de alto completo) | todo el carril (120 px) | - |

Asimetrico (se pega mucho mas "hacia el fondo"), y depende de la altura del cuerpo del objetivo: los altos
(Mutant 122) son mas faciles de golpear desde otro carril. El jugador percibe golpes que "conectan sin tocar" y
otros que fallan por pocos px en X (el alcance real del JAB es dx [-13, +121] aunque `range=115`).
Solo BetaMode.cpp:439 filtra carril (42 px).

**B06 [A] Las cajas no siguen al sprite.**
(a) No escalan con `DepthScaleFor` (0.86-1.12) aunque el sprite si (PlayerDraw.cpp:174-180): al fondo la caja es
14 % mayor que el dibujo, delante 12 % menor. (b) No escalan con el personaje: Titan-X Mejorado mide 235 px y su
JAB esta a la altura de la cadera (y-76) con 82x48; la hurtbox del jugador es 48x105 fija para todos
(Player.cpp:433). (c) El tiempo activo no coincide con el cuadro de impacto: con el clip escalado uniformemente,
el `impactFrameIndex` del perfil cae fuera de la ventana activa (MAX punch1: impacto .046-.104 s vs activo .09-.22;
MAX rage_attack: impacto .033-.073 vs activo .13-.35; Brakk energy .338-.55 vs .20-.43). `AttackIsImpactFrame`
solo se usa en debug. (d) KF: la caja del cuadro usa `width-pivotX` (incluye efectos) y en Skill llega **400 px
hacia atras** (PlayerCombat.cpp:485), alto completo -> golpea a espaldas y en cualquier carril. (e) El enemigo:
caja fija en y-66 para todos los tamanos (StreetEnemy.cpp:655-657). (f) Boss VS asimetrico (Boss.cpp:167).

**B07 [A] No hay juggle real; el que existe (solo Stage1) es infinito y "fantasma".**
StreetEnemy.cpp:689-691: cualquier golpe con el enemigo en el aire le da `vz = max(launch,160)`, sin limite,
sin escalado de gravedad, de hitstun ni de dano. Sondeo #6: 7 golpes ligeros seguidos mantienen al enemigo en el
aire. La hurtbox ignora z (StreetEnemy.cpp:634): se le golpea "en el suelo" aunque se dibuje arriba; un ataque
aereo real fallaria. `velocity = knockback` en el aire (686) lo saca volando 80+ px por golpe. Al caer pasa a Hit
0.45 s (no hay Down/OTG/wakeup).

**B08 [M] Un ataque solo golpea a UN enemigo.**
`hasHit` es por ataque y cada resolucion hace `break` tras el primer objetivo (Stage1Combat.cpp:125 etc.).
Sondeo #7: FURIA (230 px, "barrido amplio") con 3 enemigos dentro -> 1 golpeado. Ningun ataque de area funciona
como tal. Arreglo: lista `hitTargets` por activacion (ids ya golpeados) en lugar de un bool.

**B09 [M] `velocity.x` del jugador no se integra en Idle/Walk/Block/Attack/Recovery y se acumula.**
`TakeDamage` en bloqueo suma 55 a `velocity.x` (Player.cpp:448) y Stage3Game.cpp:328 suma 150 al chocar contra el
boss en guardia, pero solo Hit/Airborne/Knockdown integran velocity. Sondeo #4: 5 bloqueos -> `velocity.x=-275`
que no decae; 30 frames despues sigue en -275; al recibir el siguiente golpe el jugador **resbala 19 px con ese
residuo**. Ni el empuje de bloqueo ni el retroceso al pegar a un bloqueador se ven. El comentario de
Player.cpp:474-476 ya lo reconoce para Attack. Arreglo: integrar `velocity.x` con amortiguacion exponencial en
todos los estados terrestres (y limpiarla al cambiar de estado), o un `pushback` dedicado.

**B10 [M] El knockback/derribo del jugador ignora de donde viene el golpe.**
`TakeDamage(int)` no recibe direccion; Player.cpp:487 lo lanza siempre hacia atras de SU facing. Golpeado por la
espalda, vuela hacia el atacante (sondeo #5). Igual en el rival de VS. Arreglo: `TakeDamage(const HitInfo&)` con
direccion, knockback, launch, hitstun, flags (counter, guardBreak, otg).

**B11 [M] Amortiguaciones dependientes del frame rate.**
`velocity *= 0.84f` (StreetEnemy.cpp:520), `0.95` (504), `0.80` (Player.cpp:355), `0.94` (324), `0.86` (333):
se aplican por frame, no por dt. A 30 fps (Android) el empuje se duplica aprox.; a 120 fps se reduce a la mitad.
Arreglo: `v *= std::pow(k, dt*60)` o `v *= exp(-lambda*dt)`.

**B12 [M] La cadena pierde el 4o golpe y el input de K/J tras REMATE se descarta.**
`ResolveCancel(Punch3, Punch)=None` y `Kick+K=None`: lo pulsado durante P3/Kick queda en el buffer y caduca
(15 frames) antes de que el ataque (25 frames) termine. Sondeo #8: J en f0,10,20,30 -> P1(f0) P2(f14) P3(f27),
el J de f30 se pierde. Sondeo #10: tras P1>P2>P3 en vacio, un J mas termina en Idle. No hay "J J J J".

**B13 [M] La ventana de cancel es solo recovery y no distingue golpe/fallo.**
`InCancelWindow` = `attackElapsed >= startup+active` (PlayerCombat.cpp:354): en P1 ahorra 5 frames; no hay
cancel durante el activo tras conectar (hit-confirm), ni restricciones de "solo en hit" para specials. En clip-driven
(KF) la ventana es `max(startup+active, 38 % del clip)`. `canCancel` del AttackDef no se lee.

**B14 [M] Comandos direccionales chocan con el movimiento de carril.**
"Abajo" de QCF/DP es S = bajar de carril (PlayerCombat.cpp:303). El jugador se mueve de carril al marcar el
comando (HandleInput mueve antes de consumir el buffer), y caminar abajo-adelante + J dispara una onda sin querer
si coincide la secuencia. Ademas `MotionHistory` guarda direcciones absolutas (+1 derecha), correcto para QCF
"adelante" solo porque se compara con `last.h`; un QCB (atras) no existe.

**B15 [M] El buffer envejece durante el hitstop.**
Los stages llaman `PumpInput` antes de `hitstop.Consume` (Stage1StoryGame.cpp:311-312, VSMode.cpp:237-240):
`inputBuffer.Tick` avanza frames congelados. Con hitstop .18 s (11 frames) de un Finisher, un J pulsado justo
antes caduca (15). Arreglo: no avanzar el reloj del buffer mientras haya hitstop (`Tick(0)`), o caducidad en
"frames de simulacion".

**B16 [M] Trades sin prioridad, sin counter-hit, sin clash.**
Jugador y enemigo se resuelven por separado en el mismo frame (Stage*::HandlePlayerHits + HandleEnemyHits);
`priority` no se usa. Un normal del jugador que recibe un golpe pasa a Hit y se cancela (sondeo #11) aunque haya
conectado ese mismo frame.

**B17 [M] El jugador no tiene hitstun proporcional ni knockback normal.**
Hit fijo 0.13 s y 0 px de retroceso (sondeo #12), invuln 0.28 s posterior. Los enemigos no pueden "comboear" al
jugador ni empujarlo; todo golpe enemigo se siente igual. Mutant (29 dano) derriba SIEMPRE porque el umbral es
`damage>=28` (Player.cpp:485), no una propiedad del ataque.

**B18 [M] Fallback de clip "idle" como ataque.**
`ClipNameFor` devuelve "idle" para ids no mapeados (PlayerCommon.h:77) y KfReference.cpp:581-582 rellena clips
faltantes (`punch3`, `kick`, `dash_attack`...) con el clip `idle`: el personaje "ataca" quieto y la caja (derivada
del dibujo idle) golpea. Cualquier AttackId nuevo sin tocar `ClipNameFor/ClipFor/PhaseFor/LegacyType` reproducira
idle, tendra fase `Special` (no cancelable, nombre "ATTACK_SPECIAL") y `AttackType::None`.

**B19 [B] Superarmadura demasiado amplia.**
TakeDamage (Player.cpp:471-482) da armadura total a EnergyWave/RageAttack/Finisher/Skill y a TODO especial KF
(su `currentAttack` es Skill), incluso en recovery. Tambien a la habilidad 2 (= Punch3). No hay armadura por
tramo (solo startup) ni limite de golpes absorbidos.

**B20 [B] Bloqueo omnidireccional, sin blockstun, sin parry.**
`IsBlocking()` no mira el lado del atacante (Player.cpp:418); se sale del bloqueo y se ataca el mismo frame
(PlayerCombat.cpp:229-236 + 270); no hay "just defend".

**B21 [B] No se puede girar dentro de una cadena.**
El facing solo cambia en HandleInput (Idle/Walk) o por `dir` de un especial; en Recovery no se procesa
movimiento. Un enemigo detras no se puede castigar con la cadena (comun en SoR/FF: los golpes "auto-giran" al
pulsar atras+J).

**B22 [B] Estados inconsistentes: `PlayerState::Airborne` significa "derribado en el aire".**
No existe un estado de salto propio; reutilizar Airborne para saltar romperia `IsKnockedDown`, `CanAct`,
`SetState` (limpia buffer al entrar), `TakeDamage` (ignora golpes) y `CharacterPose` (Airborne->Knockdown).

**B23 [B] `GetAttackHitbox` ignora `position.z` (salvo BETA) y la caja del jugador KF ignora z.**
Irrelevante hoy (no se ataca en el aire), bloqueante para ataques aereos.

**B24 [B] Codigo muerto / confuso.** `src/game/Enemy.cpp` no esta en CMakeLists (no compila); `HitstopClock`
(Stage1) convive con floats `hitstop` sueltos (Stage2-5, VS, CombatWorld); `AttackIsImpactFrame` y
`CurrentAttackAnimationProfile` (con `static` mutable, PlayerCombat.cpp:457) solo para debug.

---

## 3. Que falta y propuesta concreta de diseno

### 3.0 Prerrequisito: un solo resolvedor de golpes (bloquea todo lo demas)

Crear `src/game/combat/HitResolver.h/.cpp` (o reactivar y ampliar `CombatWorld::ResolvePlayerMelee`) con:

```cpp
struct HitInfo {            // lo que viaja del atacante al objetivo
  int damage; float dirX;   // +1/-1 desde el atacante hacia el objetivo
  float knockback, launch, hitstun, blockstun, hitstop, shake;
  int juggleCost; bool otg, breaksGuard, heavy, counter, wallBounce, groundBounce, unblockable;
  AttackId source; int hitIndex;   // para escalado/visual
};
struct Hurt3 { float x0, x1, laneY, halfDepth, z0, z1; };      // volumen de cuerpo
struct Hit3  { /* capsula o caja 3D, ver 3.4 */ };
HitReport ResolvePlayerVsEnemies(Player&, std::vector<StreetEnemy>&, CombatWorld&);
void ResolveEnemiesVsPlayer(std::vector<StreetEnemy>&, Player&, CombatWorld&, int maxAttackers);
void ResolvePlayerVsPlayer(Player& a, Player& b, CombatWorld&);   // VS rival, BETA
bool ResolvePlayerVsBoss(Player&, BossTarget&, CombatWorld&);
```
Reglas: (1) un `hitTargets` por activacion (multi-objetivo, re-golpe por `multiHitInterval`); (2) prioridad y trade:
si ambos conectan el mismo frame gana `priority` mayor; empate = trade con hitstop compartido; (3) counter-hit si el
objetivo esta en startup; (4) llama `OnAttackConnected` y aplica hitstun/launch/juggle; (5) un unico evento de
proyectil. Sustituir las copias en Stage1Combat.cpp:104-148, Stage2Game.cpp:157-200, Stage3Game.cpp:278-320,
Stage4Game.cpp:166-205, Stage5Game.cpp:176-205, VSMode.cpp:250-342, y opcional BetaMode.cpp:435.
Los ajustes por stage (+5 de rage, color de chispa, score) pasan a parametros (`StageHitStyle`).
**Arte nuevo: no.** Riesgo: equilibrio de los bots (seccion 5).

### 3.1 Modelo de datos ampliado (AttackDef v2 / MoveDef)

Anadir a `AttackDef` (AttackData.h) sin quitar campos (los stages y tests los leen):

| Campo | Uso |
|---|---|
| `int startupF, activeF, recoveryF` (o mantener segundos) | autoridad en frames, mas legible |
| `float cancelFrom, cancelTo` (fraccion o frames) + `CancelMask onHit, onWhiff` | ventanas por ataque (normal/command/special/super/jump/dash) |
| `float blockstun, pushback, selfPushOnBlock` | bloqueo con peso |
| `int juggleCost; bool otg, launcher, groundBounce, wallBounce, knockdown, low, overhead, hitsBehind` | juggle y propiedades |
| `float armorFrom, armorTo; int armorHits` | superarmadura por tramo (reemplaza B19) |
| `LimbArc arc` | caja matematica (3.4) |
| `RootCurve move` (dx, dz, easing, t0, t1) + `float laneMagnet` | avance con curva (3.5) |
| `int hitCount; float multiHitInterval` | multigolpe controlado |
| `const char* clip; const char* clipFallback; PoseFx procedural` | integracion por tipo de personaje (seccion 4) |
| `float hitstopScale, shakeScale` | 3.6 |

Agregar los nuevos `AttackId` **al final, despues de `Skill` y antes de `Count`** (los arrays se dimensionan
con `Count` y los valores historicos no cambian). Actualizar SIEMPRE juntos: `kAttacks`, `ClipNameFor`, `ClipFor`,
`PhaseFor`, `LegacyType` (PlayerCommon.h), `GetAttackAnimationProfile` (AttackAnimation.h) y
`CharacterPose` (CharacterVisual.cpp). Recomendado: mover la tabla de cancel a datos (`CancelRule { from, cmd,
dirMod, onHit, to }`) en vez del `switch` de `ResolveCancel`.

### 3.2 Moveset propuesto

Leyenda input: `>` = adelante (hacia donde mira), `<` = atras, `v` = abajo (S) **sin componente horizontal**,
`J/K` = boton, `[salto]` = boton nuevo (ver 3.3). Frames a 60 fps (S/A/R). Caja: ver 3.4 (pivote en px desde los
pies del personaje "estandar" de 112 px; se escala por `bodyScale = alturaSprite/112 * DepthScaleFor(y)`).
Dano antes de multiplicadores. Kb = knockback px/s; Lch = launch px/s (altura = Lch^2/3000).

**Normales de puno (cadena J):**
| Id | Input | S/A/R | Dano | Kb | Hitstun | Lch | Caja (arco) | Notas |
|---|---|---|---|---|---|---|---|---|
| Punch1 JAB | J | 5/4/8 (hoy 5/8/5) | 8 | 220 | .30 | 0 | hombro (8,-84), L 36->74, theta -8->0, r 14 | cancel desde A+1 en hit, desde R en whiff |
| Punch2 RECTO | J J | 5/4/9 | 10 | 260 | .32 | 0 | (8,-86) L 40->82, theta -4->2, r 15 | |
| Punch3 GANCHO | J J J | 7/5/11 | 13 | 300 | .34 | 0 | (4,-88) L 60->70, theta barre -55->+20 (arco horizontal), r 18 | pasa a ser el 3er golpe; hoy es REMATE |
| PunchFinal REMATE (nuevo id) | J J J J | 9/6/16 | 20 | 520 | .45 | 260 | (6,-92) L 50->92, theta +20->-15 (de arriba abajo), r 22 | knockdown, breaksGuard, hitstop x1.3 |

Si se quiere conservar el test `TestSpecialCommands` (DP+K == Punch3) sin tocarlo, mantener `Punch3` = REMATE y
crear `PunchHook` como 3er golpe; la cadena queda J J J(hook) J(Punch3). Ver seccion 5.

**Patadas (cadena K y ramas):**
| Id | Input | S/A/R | Dano | Kb | Hitstun | Lch | Caja | Notas |
|---|---|---|---|---|---|---|---|---|
| Kick (existente) PATADA FRONTAL | K | 7/5/12 | 14 | 360 | .32 | 0 | cadera (0,-58) L 50->110, theta -20->-5, r 18 | |
| Kick2 MEDIA LUNA | K K | 8/6/12 | 16 | 400 | .36 | 0 | (0,-62) L 100, theta -70->+15 (barrido horizontal), r 20 | |
| Kick3 TALON GIRATORIO | K K K | 11/6/18 | 22 | 600 | .40 | 300 | (0,-78) L 105, theta +60->-10, r 22 | knockdown, wallBounce |
| KickLauncher PATADA ALTA | J J K | 8/5/16 | 14 | 80 | .55 | **720** (172 px, ~0.96 s) | (0,-70) L 95, theta -40->-110 (de frente hacia arriba), r 18 | launcher; jump-cancel en hit |
| FwdPunch PUNO DE AVANCE | > J | 12/5/14 | 16 | 420 | .36 | 0 | (8,-86) L 60->96, r 18 | root motion +70 px easeOutCubic; overhead (no lo para bloqueo bajo) |
| BackElbow CODAZO | < J | 6/4/12 | 12 | 380 | .34 | 0 | caja DETRAS: (-6,-84) L 30->62, theta 180 | golpea a espaldas sin girar (Final Fight) |
| SpinKick PATADA GIRATORIA | < K | 9/8/16 | 16 | 420 | .34 | 0 | dos arcos: frente y detras (theta 0->360 en A) | golpea ambos lados (multi-objetivo) |
| Sweep BARRIDA | v K | 7/6/16 | 12 | 120 | .5 | 0 | (0,-10) L 40->110, theta -2, r 14 (bajo) | low, knockdown "trip" (pasa a Down), OTG-able despues |
| Uppercut GANCHO ASCENDENTE | v J | 6/6/18 | 14 | 60 | .6 | **680** | (6,-70) L 50->95, theta -30->-95, r 18 | launcher; armadura frames 1-6; +10 px de z propio (hop sin(pi u)) |
| Stomp PISOTON | v J sobre enemigo en Down | 10/4/14 | 10 | 0 | - | 0 | (24,-6) caja 60x30 | OTG, 1 por derribo |

**Dash:** DashAttack (J en dash, existente); `DashKick` DESLIZAMIENTO (K en dash): 4/10/14, 13 dano, low,
knockdown, root motion +120 px easeOutQuad; `BackDash` (< + Shift): 0.20 s invuln 0.12, -110 px.

**Aire (requiere estado Jump, 3.3):**
| Id | Input | S/A/R | Dano | Kb | Lch | Caja | Notas |
|---|---|---|---|---|---|---|---|
| AirPunch | J en salto | 4/6/8 | 9 | 160 | 220 (re-elevacion en juggle) | (10,-80) L 70, theta -20->+10 | cancelable en AirPunch2/AirKick |
| AirPunch2 | J J en salto | 4/6/8 | 10 | 180 | 220 | idem L 78 | |
| AirKick PATADA VOLADORA | K en salto | 5/12/6 | 14 | 380 | 0 | (0,-50) L 95, theta +25 (diagonal abajo) | knockdown si el objetivo esta en el suelo |
| DiveKick PATADA PICADA | v K en salto | 6/hasta tocar suelo/10 | 14 | 300 | -500 (hacia abajo) | caja en el pie, trayectoria lineal 45 deg | groundBounce si el objetivo esta en el aire |
| AirSpike MARTILLO | > J en salto (o 3er golpe aereo) | 8/5/14 | 18 | 200 | -700 | (8,-70) L 80, theta -60->+60 | groundBounce (rebote bajo 300) -> OTG |

**Agarre/lanzamiento (procedural, sin arte nuevo; 3.9):** `Grab` (caminar contra un enemigo en Idle/Hit/Stun a
<34 px del borde de su hurtbox, mismo carril |dy|<16, mantener > 8 frames, o J+K juntos cerca);
`GrabKnee` (J en agarre, x3, 6 dano, 0.32 s c/u); `ThrowForward` (> K o K, 22 dano, arco parabolico 160 px,
knockdown, el lanzado golpea a otros: proyectil-cuerpo); `ThrowBack` (< K, 24 dano, suplex por encima del hombro).

**Especiales (comando):**
| Id | Input | S/A/R | Dano | SP | Propiedades |
|---|---|---|---|---|---|
| EnergyWave (existente) | QCF+J / L / ESPECIAL | 12/proyectil/14 | 20 | 20 | proyectil (un solo spawn, B01) |
| RisingUppercut SHORYU (nuevo; o mantener Punch3 por test) | DP+K | 3/12/22 | 16 x2 | 0 | invuln frames 1-5, launcher 760, root dz hop 90 px |
| Hurricane HURACAN | QCB+K (nuevo `QuarterCircleBack`) | 8/5 golpes cada 6f/16 | 6 x5 | 15 | se desplaza +160 px, multi-objetivo, hitsBehind |
| DrillKick TALADRO | QCF+K | 10/14/18 | 8 x3 | 15 | avance +140 px easeInOutSine, ultimo golpe knockdown |
| EX (cualquier especial) | J+K a la vez en el comando | -2 startup | x1.4 | +25 | armadura 1 golpe, flash |

**Supers / defensa:**
- RageAttack (Espacio en Active, existente) y Finisher: reasignar Finisher a "QCF QCF + J en Furia" o
  "Espacio + K en Furia" (no depender de `comboCount`), con camara/flash y zoom-pause 0.25 s.
- `JustDefend`: B pulsado <=6 frames antes del impacto -> 0 chip, +10 escudo, blockstun 0, flash azul.
- `GuardCounter` CONTRA: J+K (o > J) durante blockstun, 25 SP o 30 escudo, 4/6/18, 12 dano, kb 500, invuln 1-8.
- `TechRoll`: pulsar Shift al tocar suelo tras Airborne -> rodar 90 px por el carril, sin knockdown.
- `WakeupAttack`: J al final del getup -> barrido de levantarse (8 dano, invuln 1-6).

### 3.3 Rutas de combo y tabla de cancelacion

Jerarquia: Normal < CommandNormal < Special < Super. Regla general: en hit o bloqueo se puede cancelar desde
`startup+1` hasta fin de recovery; en whiff solo desde recovery (frame advantage negativo).

| Desde | En hit/bloqueo puede ir a | En whiff |
|---|---|---|
| Punch1 | Punch2, Kick, FwdPunch, Uppercut, Sweep, especiales, Grab | Punch2 |
| Punch2 | Punch3(Hook), KickLauncher (J J K), Kick, especiales | Punch3 |
| Punch3/Hook | PunchFinal, Kick2, especiales | PunchFinal |
| Kick | Kick2, Punch1 (K J -> reinicia), especiales | Kick2 |
| Kick2 | Kick3, especiales | Kick3 |
| KickLauncher / Uppercut | **JumpCancel** (pulsar salto o J/K en los 10 frames tras el hit -> salto persecucion automatico hacia el enemigo), especiales | - |
| AirPunch | AirPunch2, AirKick, AirSpike | AirPunch2 |
| AirPunch2 | AirKick, AirSpike | - |
| Cualquier normal | EnergyWave, RisingUppercut, Hurricane, DrillKick | igual pero solo en recovery |
| Especial | Super (RageAttack/Finisher) si Furia activa | - |
| DashAttack | Kick2, Uppercut | - |

Rutas objetivo (para probar en tests):
- `J J J J` = JAB, RECTO, GANCHO, REMATE (4 golpes, ~58 dano, derriba).
- `J J K` = lanzador -> `[salto auto] J J K` (aereos) -> `> J` MARTILLO -> rebote -> `v J` PISOTON (OTG).
- `K K K` = 3 patadas, derriba con rebote en pared.
- `J J QCF+J` = cadena con onda (special cancel).
- `v K` (barrida) -> `v J` pisoton OTG.
- `DP+K` (shoryu) -> jump cancel -> `K` en el aire.
- `< J` (codazo) en multitudes; `< K` giratoria cuando rodean.
- `Dash J` -> `K K`.
- Bloqueo -> JustDefend -> CONTRA.

Resolucion de modificadores direccionales (sustituye a la logica de `TryStartBufferedAttack`): guardar en cada
`InputBuffer::Entry` la direccion **relativa** al facing en el momento de pulsar (`relX = moveX*facingDir`,
`down = moveY>0.5 && |moveX|<0.5`, `up`) y resolver: comando especial (MotionHistory) > modificador direccional >
cadena. `<` + J ejecuta BackElbow SIN girar; si se quiere "girar y pegar", usar `<` + J + J (el segundo gira).
Mientras se esta en Attack/Recovery el movimiento de carril no se aplica, asi que leer `v` al pulsar no mueve
al jugador. Para que caminar abajo-adelante no dispare QCF: exigir que el primer paso del QCF sea `down` sin
horizontal mantenido >=2 frames (MotionHistory ya guarda frames; anadir duracion minima) y anadir `QuarterCircleBack`.

### 3.4 Cajas de golpe calculadas matematicamente

Objetivo: la caja sale de la geometria del golpe (hombro/cadera + miembro que rota y se estira), sigue al
sprite (escala, profundidad, z) y no "atraviesa" enemigos en golpes rapidos.

**Espacio de colision 3D (arregla B05/B07/B23).** Representar todo en (X, Lane, H) con H = altura sobre el
suelo (positiva hacia arriba):
- Hurtbox enemigo: `X in [ex - w/2, ex + w/2]`, `Lane in [ey - d, ey + d]` (d = 18 px), `H in [ez, ez + bodyHeight*s]`.
- Hurtbox jugador igual con su tamano (derivado de la altura de su sprite, no 48x105 fijos); Knockdown/Down:
  `H in [0, 36]`, ancho 76.
- Test: solape en X y H con la forma del golpe **y** `|ey - py| <= attack.depth + d` (usa por fin `def.depth`).
- La conversion a `CombatBox` de pantalla (x, y - H) queda solo para dibujar el debug.

**Arco del miembro (LimbArc).** Por ataque: pivote `P = (px, ph)` (hombro o cadera, en px del personaje estandar),
longitud `L(u)`, angulo `theta(u)` y radio `r` de la punta. Con `u = clamp((t - startup)/active, 0, 1)` y
`e(u)` una curva de easing (por defecto easeOutCubic `1-(1-u)^3`, el golpe sale rapido y frena):

```
theta(u) = theta0 + (theta1 - theta0) * e(u)          // grados, 0 = horizontal al frente, + = arriba
L(u)     = L0 + (L1 - L0) * e(u)
s        = bodyScale = (spriteHeight/112) * DepthScaleFor(y)   // sigue al sprite
tip(u)   = Pw + s * (facing * L(u) * cos(theta), L(u) * sin(theta))
Pw       = (x + facing * s * px, z + s * ph)              // en (X, H)
```
Forma activa = capsula (segmento `[Pw + s*0.35*L*dir, tip]`, radio `s*r`): el antebrazo/espinilla tambien golpea,
no solo la punta. Para golpes de barrido (hook, media luna, spin) theta recorre un arco grande.

**Barrido entre frames (anti-tunneling).** Cada frame con hitbox activa se guarda `tipPrev`. La forma probada es
la capsula `[tipPrev, tip]` (union convexa de las dos posiciones) ademas del miembro actual. Con un JAB que
recorre 38 px en 4 frames (~10 px/frame) no importa a 60 fps, pero a 30 fps (Android) o con el arco del gancho
(~70 px en 5 frames) evita que la punta salte por encima de un enemigo estrecho (Urban Ninja 52 px). Para
proyectiles igual: segmento `[pos_prev, pos]` con radio.

Test capsula vs caja (X,H) — exacto y barato:
```
// distancia^2 entre segmento AB y rectangulo R (en el plano X-H)
// 1) si A o B dentro de R -> hit. 2) si el segmento corta algun lado -> hit.
// 3) si no, d2 = min(distPuntoSegmento^2 de las 4 esquinas al AB, distPuntoRect^2(A), distPuntoRect^2(B))
// hit si d2 <= radio^2  (y ademas el test de carril)
```
Poner todo en `src/game/combat/CombatMath.h` (header puro, sin raylib salvo Vector2) para testearlo en un
`tests/CombatMathTests.cpp` nuevo: punta del arco, barrido, simetria izq/der, escalado.

**Punto de impacto.** El punto de contacto (para chispa FX y numeros) = punto del segmento mas cercano al centro
de la hurtbox. Hoy la chispa se pone a `min(b.width,90)` (PlayerDraw.cpp:115), no donde pega.

**Derivar los parametros del arte (opcional, recomendado para atlas A2).** Herramienta offline
`tools/derive_hitboxes.py` (no en runtime): para cada cuadro de los clips de ataque del manifiesto, medir el
alfa (como ya hace `BuildSpriteFrames` para enemigos, StreetEnemy.cpp:52-117), localizar el pixel opaco mas
adelantado por bandas de altura y escribir en el manifiesto `"hit": {"frame": i, "tip": [x,h], "r": n}` por clip.
En runtime el arco se ajusta a esa punta medida (`L1, theta1` = polares de la punta respecto al pivote) y el
cuadro de impacto pasa a ser el que define `startup` (alinea animacion y caja: B06c). Los BETA ya traen la caja
por cuadro: mantenerla tal cual.

**Profundidad asistida ("lane magnet").** Durante el startup, desplazar el carril hacia el objetivo mas cercano
que este dentro de `depth*1.6`: `dy = clamp(ey - py, -m, m) * e(u_startup)` con `m = 14..20` px. Hace que el
`depth` estricto no se sienta injusto (SoR4 lo hace).

### 3.5 Root motion con curvas, inercia y altura

Hoy: perfiles con curva fija ease-in .30 / ease-out (AttackAnimation.h:267) solo para skins 2 y 6; el resto lineal
(PlayerDraw.cpp:139-158). Propuesta `RootCurve { float dx, dz; float t0, t1; Ease ease; bool cancelOnHit; }`:

```
u   = clamp((t - t0)/(t1 - t0), 0, 1)
x(t) = x_start + facing * dx * ease(u)            // incremento por frame = diferencia, como hoy
z(t) = dz * sin(pi * u)                           // hop del gancho/shoryu
Eases: easeOutCubic (golpes), easeInOutSine (taladro), easeOutBack(1.2) (embestida con rebote),
       bezier cubica (p1,p2) para casos a medida: B(u)=3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3
```
- Al conectar: frenar el avance (`cancelOnHit`, multiplicar el resto por 0.3) y aplicar `selfPushOnBlock` al
  bloquear, integrando `velocity.x` con `v *= exp(-12*dt)` en Attack (B09).
- Clamp contra bordes de arena ya existe; anadir "no atravesar" al objetivo: no avanzar si la distancia a la
  hurtbox objetivo < 24 px.
- Mantener `AttackRootMotionProgress` como `Ease::Profile` para no romper `AttackAnimationTimingTests`.
- Movimiento base mas fluido (no es combate pero se pidio): aceleracion/frenado en Walk
  (`v += (vObjetivo - v) * (1 - exp(-14*dt))`), giro con `turn` clip (MAX/Rayder lo tienen), run con doble toque.

### 3.6 Hitstop, shake y feedback escalados

```
hitstop = clamp(0.035 + 0.0035*damage, 0.035, 0.20) * (counter ? 1.5 : 1) * (lastHitOfRoute ? 1.35 : 1)
          * (guarded ? 0.6 : 1) * def.hitstopScale
victimShake(t) = A * sin(2*pi*28*t) * exp(-t/0.06),  A = 2 + 0.15*damage px (solo sprite de la victima)
cameraShake    = amplitud 1.5 + 0.22*damage px, decae exp(-t/0.12), direccion = dirX del golpe (no aleatoria)
killSlowMo     = ultimo enemigo de la oleada o finisher: timeScale 0.35 durante 0.35 s
```
- Hitstop asimetrico opcional: el atacante sale 1-2 frames antes que la victima (sensacion de "peso").
- Unificar en `HitstopClock` (todos los stages) y no avanzar el buffer durante el congelamiento (B15).
- Chispas: usar el punto de contacto (3.4); FX distintos por tipo (low/high/launcher/counter). El atlas
  `assets/fx/efectos_azules.png` ya tiene impactos (8-13), estela de dash (42-45), escudo (16-18).
- Numeros de dano y contador de hits con escalado visible (ya existe en BetaMode, portar).

### 3.7 Juggle, OTG y estados del enemigo

Nuevos estados en `StreetEnemyState` (anadir AL FINAL del enum, el comentario del header pide conservar orden):
`Launched` (o reutilizar Airborne), `Down` (en el suelo), `WakeUp`, `Grabbed`, `Thrown`, `WallBounce`.

```
juggleBudget = 6 (por lanzamiento; Brute/Mutant/Armored 4)
al golpear en el aire: if (juggleUsed + cost > budget) -> no levanta: cae (Down) y el golpe solo hace 30 %
gEff   = 1500 * (1 + 0.12 * hitsInAir)                 // cada golpe aereo cae mas rapido
vz     = max(def.launch, 220) * pow(0.9, hitsInAir)    // relanzamiento decreciente
hitstun= def.hitstun * pow(0.92, comboHits)            // degradacion
dano   = base * max(0.35, 1 - 0.08 * max(0, comboHits - 3))   // escalado de combo
Down   : 0.55 s; golpeable solo por ataques otg (1 vez por derribo); WakeUp 0.35 s invulnerable
groundBounce: si llega al suelo con flag, vz = 300 una vez; wallBounce: al tocar limite de arena, vx = -0.5vx, vz=260
```
- La hurtbox en aire sube con z (3.4). La IA no decide durante Launched/Down/WakeUp.
- El jugador tambien debe poder ser lanzado/derribado por propiedades del ataque enemigo (no por `damage>=28`).
- Archivos: StreetEnemy.h/.cpp (estados, TakeDamage(HitInfo)), EnemyBrain.h (juggleBudget/weight por perfil),
  Player.cpp (TakeDamage(HitInfo), estado Down propio para tech/wakeup).
- Arte: enemigos solo tienen 12 cuadros (idle 0-3, attack 4-7, hit 8-9, death 10-11, StreetEnemy.cpp:33): Launched
  = cuadro 8/9 rotado procedimentalmente (angulo = atan2(vz, vx)*0.6), Down = cuadro 10/11 (death) sin desvanecer
  y aplastado `sy=0.55`, WakeUp = reproducir 11->10->idle. **Sin arte nuevo** (procedural). Los bots de referencia
  KF tienen `air`, `knockdown`, `getup`.

### 3.8 Salto (necesario para el aire)

- `PlayerState::Jump` NUEVO al final del enum (no reutilizar `Airborne`, B22). `CanAct`, `IsKnockedDown`,
  `SetState`, `UpdateTransformForm`, `CharacterPose` deben tratarlo.
- Fisica: `vz0 = 820` (apex 224 px a g=1500, 1.09 s) o salto mas corto `vz0=700` (163 px, 0.93 s) con gravedad
  de caida 1.25x para que se sienta menos flotante; control aereo `vx` = 60 % de la velocidad de caminar; el carril
  no cambia en el aire. Aterrizaje 4 frames de recovery (landing clip de MAX existe: frames 39-42).
- Input: tecla nueva (sugerido `KEY_I` o `KEY_U`; `KEY_SPACE` es Furia) y boton tactil nuevo `kVirtualJump`
  (InputMap.h, TouchControls.cpp). Alternativa sin boton: J+K a la vez = salto (choca con EX: elegir uno).
- Ataques aereos con `AttackPhase::Air*`; al tocar suelo durante un ataque aereo -> landing recovery.

### 3.9 Agarre y lanzamiento (viable con el arte existente)
- Jugador: pose de agarre = primer cuadro de `punch2`/`block` (A1/A2) o pose "ready"/"basic" (B); rodillazo =
  `kick` cuadro 1 con hop de 6 px; lanzamiento = `punch3` + rotacion. MAX tiene cuadros de agarre/proyeccion
  plausibles en 112-124 y 181-188 (verificar visualmente).
- Enemigo agarrado: cuadro `hit` (8) anclado a la mano del jugador (`hand = Pw + s*(facing*30, -10)`);
  lanzado: parabola `x(t)=x0+facing*vx*t, z(t)=vz*t - g t^2/2`, rotacion `angle = 540 deg * u`, al aterrizar
  Down y dano de impacto; su cuerpo es un "proyectil" que golpea a otros enemigos (8 dano, knockdown).
- Escape del agarre: enemigos con armor (Brute/Mutant/Armored) no son agarrables o escapan a 0.6 s.
- Archivos: Player.h/cpp (estado Grab o AttackPhase::Grab), StreetEnemy (Grabbed/Thrown), HitResolver.

### 3.10 Contraataque y defensa
- Counter-hit: si el objetivo esta en `startup` (enemigo: `telegraphing` o `attackElapsed < 0.34*duration`):
  dano x1.25, hitstun +0.12 s, hitstop x1.5, texto "CONTRA". Tambien para el jugador recibiendo.
- Bloqueo direccional: bloquea solo si el atacante esta delante (`(ax - px) * facing > -10`).
- Blockstun por ataque (`def.blockstun`, enemigos 0.18-0.30) y pushback; JustDefend y GuardCounter (3.2).
- Enemigos: `GuardCounter` ya existe en la IA; usar el mismo HitInfo.

### 3.11 Enemigos: datos de ataque propios
- `EnemyAttackDef` por perfil (startup/active/recovery en vez de 34 %-74 % fijo, caja por arco escalada con
  `visualScale`, knockback, launch, propiedades low/high/unblockable) en EnemyBrain.cpp.
- Variedad sin arte nuevo: 2-3 ataques por tipo reutilizando los cuadros 4-7 con distintas velocidades y
  root motion (lunge, golpe bajo con `sy` 0.85, golpe alto).

---

## 4. Integracion con los 4 tipos de personaje

Principio: **el moveset es comun y vive en datos; cada tipo declara que puede mostrar**. Anadir a `CharacterVisual`
(o a un `MoveCapabilities` derivado) una mascara de movimientos soportados + tabla de mapeo de clips. Si un
movimiento no tiene arte, se usa `clipFallback` + transformacion procedural, o se desactiva para ese tipo
(el input cae al movimiento base). Nunca reproducir `idle` como ataque (B18): `PlayNamed` debe fallar si el clip
"real" no existe (en KF marcar los rellenados en KfReference.cpp:581-582 como `synthetic`).

### 4.1 Tipo A — atlas de manifiesto (Rayden 0; MAX 6 / MAX blanco; Brakk 2 con `brakk_v2`)
- Nuevos clips en `data/sprite_manifest.json` con nombres canonicos: `punch_hook`, `punch_final`, `kick2`, `kick3`,
  `kick_launcher`, `uppercut`, `sweep`, `back_elbow`, `spin_kick`, `fwd_punch`, `jump`, `jump_fall`, `landing`,
  `air_punch`, `air_kick`, `dive_kick`, `air_spike`, `grab`, `grab_knee`, `throw`, `stomp`, `dash_kick`,
  `rising_uppercut`, `hurricane`, `drill_kick`, `guard_counter`, `tech_roll`.
- **MAX: hay arte sin usar** (196 celdas; rangos libres 10-13, 21-38, 51-87, 90-91, 98-101, 112-125, 128-140, 144-175,
  181-205, 218-242, 286-307). Inspeccion visual (hojas en
  `/tmp/claude-0/-home-user/05b0c2b9-43b9-5c8e-82ee-61da3b1aff5a/scratchpad/audit/max_a.png` .. `max_d.png`):
  22-24 despegue/salto, 25-27 patadas en el aire, 28-38 giros/patadas aereas con estela azul (huracan / aire),
  51-60 golpes con estela y arcos (gancho ascendente 53/58/60), 61-76 ataques bajos agachado y barridos (72-75),
  77 y 86 arcos grandes (media luna / giratoria), 125-153 cargas y haces de energia, 286-307 estelas grandes y
  super. 154-225 parecen OTRO personaje (pelo largo, ropa negra) mezclado en el atlas: no usar sin confirmar.
  Con esto MAX puede tener todo 3.2 sin arte nuevo (solo manifiesto + timings).
- **Brakk v2**: ~40 celdas libres (18-22, 32-35, 47-57, 67-76, 81-84, 103-107); clips `chain/smash/charge/fury`
  sirven para patadas, barrida (smash bajo) y agarre (chain_throw). Salto: no hay; usar `charge` + hop procedural
  o desactivar el salto para Brakk (personaje pesado: tiene sentido).
- **Rayden 0 (24 cuadros)**: poco arte. Movimientos nuevos = cuadro base + `PoseFx` procedural (rotacion,
  desplazamiento, escala por fase): barrida = `kick`(10) con `sy 0.70`, `angle -12`, `at.y +18`; uppercut = `punch3`(11)
  con `angle -18` y hop; aire = `airborne`(18) con rotacion; agarre = `punch2`(9). Reutilizar el squash por fase de
  PlayerDraw.cpp:305-313 generalizado a `PoseFx { sx(u), sy(u), angle(u), dx(u), dy(u) }` por movimiento.
- Ajustar `AttackAnimationProfile` (AttackAnimation.h) para que el `impactFrameIndex` defina el comienzo del
  activo (o escalar el clip por tramos: startup->cuadros previos al impacto, active->impacto, recovery->resto) en
  vez del escalado uniforme `ScaleCurrentClipToDuration` (Animator.h:74-79).

### 4.2 Tipo B — poses sueltas (Clon antiguo 1, Grinder 3, Titan-X 4, Titan-X Mejorado 5)
- `CharacterPose` (CharacterVisual.cpp:116-187) decide por `AttackType` (Punch/Kick/Energy) y `rage`; hay que
  pasarle `AttackId` + fase (u) y una tabla `pose por (AttackId, fase)` por personaje. Poses disponibles por set:
  clon {ready, punch, kick, release_orb, dash, hurt, death, idle1-4}; Grinder {saw, slam, ram, overdrive, hurt...};
  Titan-X {punch1, uppercut, slam, shoot_orb, lean, recoil...}; Titan-X M. {charge_orb, slam, shoot_orb, rage_aura,
  cannon_aim, dash...}. Ej: Uppercut -> Titan-X `uppercut`; Sweep -> `slam` + `sy 0.8`; Aire -> `lean` rotado.
- **El dibujo de poses no tiene squash/giro ni estelas** (PlayerDraw.cpp:259-262): anadir `angle`, `sx/sy`
  a `DrawSpriteUniform`/`DrawBossPose` (rendering/BossSprite) y generar ghosts de la textura de pose. Sin eso los
  movimientos nuevos de estos personajes se veran "pegados".
- Su `animator` es el atlas de Rayden invisible (Player.cpp:167-170): la caja/tiempos salen del `AttackDef`, y el
  fin del ataque por `animator.isFinished` depende de clips de Rayden. Para ids nuevos sin clip en Rayden,
  forzar fin por `stateTimer` (ya lo hace) y no depender de `isFinished`.
- Escala de cajas: usar `cv.targetHeight` (170/235) o `cv.scale` para `bodyScale` (3.4). Recomendado desactivar
  salto/agarre para Grinder/Titan-X Mejorado (pesados) via mascara.

### 4.3 Tipo C — KF por clip (heroes/enemigos KF, RAYDER CLON BETA y RAYDER CLON con `kfMoves`, RONIN)
- Su golpe y su caja los marca el clip (PlayerCombat.cpp:178-185, 424-430, 480-489). No mezclar sus clips con
  `AttackDef` de tiempos. Mapear el moveset nuevo a SU vocabulario: `atk1-atk4` (cadena J/K), `special` (onda/
  finisher), `super` (rage), `air` (ataque aereo si existe), `dash` (dash attack), `abN` (habilidades). Para KF
  enemigos (plantilla ~30 acciones) muchos movimientos no existen: mascara de capacidades por roster.
- Corregir la caja derivada del dibujo: limitar `back` tambien en Skill (no 400 px), limitar alto a la banda del
  miembro (no `top+10` completo) y aplicar el filtro de carril de 3.4. Quitar el x0.6/re-golpe cada 0.30 s en favor
  de `hitCount/multiHitInterval` por clip (tabla en KfReference: duracion del clip / golpes).
- Los combos de juggle y agarre: habilitarlos solo si el clip `air`/`knockdown` existe; si no, el lanzador lanza
  pero el KF no tiene ataque aereo (sigue con cadena en el suelo cuando el enemigo cae: OTG).

### 4.4 Tipo D — BETA por piezas (GUERRERO x4, RONIN-KRATOS x4, bestias/medusas; GowApk BetaMode)
- Caja y hurtbox del cuadro son autoritativas (Player.cpp:424-429, PlayerCombat.cpp:473-479) y ya incluyen z.
  Integrarlos en el `HitResolver` con su propia forma (rectangulo del cuadro) + filtro de carril (42 hoy).
- BetaMode tiene su resolvedor y su balance; si se migra, conservar que el heroe "pega en cada cuadro con caja"
  (hoy por no marcar `hasHit`) con un `multiHitInterval` equivalente (~0.28 s, la invuln del objetivo), o el
  tiempo de las oleadas de `gow_check` subira (ver 5).
- Sus clips traen sonidos por cuadro y FX propios; no aplicar PoseFx ni estelas.

### 4.5 Resumen de arte
| Propuesta | A MAX | A Brakk | A Rayden | B poses | C KF | D BETA |
|---|---|---|---|---|---|---|
| Cadena J x4, K x3, ramas direccionales | arte existente | arte existente | procedural | procedural limitado | clips atk1-4 (parcial) | clips propios |
| Salto + aereos | arte existente (21-38) | no (desactivar) | procedural (airborne) | procedural (lean) / desactivar | `air` si existe | si su set lo trae |
| Barrida / uppercut / codazo / giratoria | existente (51-77) | parcial | procedural | procedural | parcial | - |
| Agarre / lanzamiento | procedural + 112-124? | chain_throw | procedural | procedural | no | no |
| Especiales nuevos (huracan, taladro, shoryu) | existente (28-38, 53-60, 125-153) | fury/charge | procedural + FX azules | FX | sus `abN` | sus habilidades |
| Juggle/OTG/Down en enemigos | procedural sobre cuadros 8-11 de enemigos | | | | | |

---

## 5. Riesgos: tests y chequeos que pueden romperse

| Test / chequeo | Que asume (y se rompe si...) | Como evitarlo |
|---|---|---|
| `tests/InputBufferTests.cpp` `TestChainWithCancel` | P1 se cancela en P2 exactamente tras `Advance(14)` (startup+active = 0.22 s) y P2+K -> Kick en 20 frames | Si se cambian los tiempos de Punch1/2 (p.ej. a 5/4/8), actualizar los frames del test en el mismo PR; mantener `ResolveCancel(P2,Kick)=Kick` o la ruta J J K = KickLauncher rompera la aserción `attackPhase == Kick` (mapear `PhaseFor(KickLauncher)=Kick` o actualizar test) |
| `TestSpecialCommands` | DP+K -> `currentAttack == Punch3`; QCF+J -> EnergyWave; cancel P1->onda; sin SP el QCF sale como Punch1 | Si DP+K pasa a `RisingUppercut`, cambiar la aserción; alternativa: mantener Punch3 como REMATE y crear `PunchHook` |
| `TestRecoveryBeforeIdle`, `TestExpiredInputDoesNotFire`, `TestHitClearsBuffer` | Recovery obligatoria tras atacar; buffer de 15 frames; un golpe limpia el buffer | No quitar Recovery; si se congela el buffer en hitstop (B15) no afecta a estos tests |
| `tests/CombatSmokeTests.cpp` | `TakeDamage(20)` -> hp 80 y rage>=15 sin knockdown; bloqueo absorbe en escudo; `TakeDamage(100)` con escudo 20 -> GuardBreak; P1 activo a 0.15 s; Rayden Punch3 avanza >1 px en 5x0.02 s; enemigo Brute pierde exactamente 20 con `TakeDamage(20,{0,0,0})` y muere con 10000 | Mantener la sobrecarga `TakeDamage(int)` (y `TakeDamage(int, Vector3D)` en enemigo) como envoltorios del nuevo `TakeDamage(HitInfo)`; P1 debe seguir activo a 0.15 s (con 5/4/8 el activo es .083-.150: borde exacto, usar `<=`); el derribo no debe depender de dano 20 |
| `tests/AttackAnimationTimingTests.cpp` | Para skins 2 y 6 y los 8 ids historicos: clip no nulo, `AttackRootMotionProgress(total)<=1`, `AttackPhaseLabel(startup)=="ACTIVE"`; `UsesAttackAnimationProfile(0/1)==false` | No cambiar la firma de esas funciones; los ids nuevos pueden tener perfil propio |
| `tests/SpriteManifestTests.cpp` | Atlas rayden 128x128, 24 cuadros, tiempos exactos de `punch1` (16 -> 8), `hit_low`=17, `airborne`=18, `getup`=20 | Anadir clips nuevos esta bien; NO tocar los existentes de "rayden"; validar que los indices nuevos existen en la rejilla (ApplyProfile falla si un indice sale del atlas: rompe la carga de todo el personaje) |
| `tests/TouchControlsTests.cpp` | J por toque produce Pressed/Down; en combate no se tocan flechas | Un boton de salto nuevo no debe solapar el de GOLPE (1170,615 r62) ni moverlo |
| `tests/Stage1PlaythroughTest.cpp` (ctest) | Bot J J J K cada 9 frames con |dy|<32, rango 140; modo invulnerable debe limpiar el nivel en <900 s (hoy 342.7 s), 4 escenarios, >=12 bloqueos, >=8 GO | Arreglar B01 quita DPS de la onda (el bot la usa contra el boss): medir; un filtro de carril estricto <32 no afecta (el bot alinea a 14). Si se cambia la ruta J J J K (p.ej. K tras P3 = Kick2 en vez de Kick) el bot sigue pegando, pero vigilar tiempo |
| `tests/AllModesCheck.cpp` (`all_modes_check`, no ctest) | Cada personaje VS carga y se dibuja; rivales IA {7,8,18,19,6,21,2} deben usar >=1 habilidad, golpear al jugador Y recibir dano en 25 s (semilla fija); stages 30-40 s sin caidas | El rival usa `RivalAI` (J J J K con gap .16 s y habilidades): si el nuevo resolvedor de VS anade knockback/hitstop o cambia trades, re-ejecutar; mantener que los golpes entre jugadores conecten con el filtro de carril (la IA alinea a 14 px) |
| `GowApk/src/GowCheck.cpp` (`gow_check`) | Oleadas BETA ganadas en <240 s (hoy la peor 213.1 s: margen 27 s), jefes ganados, duelos, VS guerrero/ronin hiere y es herido | No reducir el DPS del heroe BETA (mantener su cadencia multi-golpe si se migra BetaMode al resolvedor); cualquier cambio de dano/invuln global a `Player::TakeDamage` (0.28 s) afecta aqui |
| `skills_visual_check`, `stage1_visual_check`, `kf_bot_visual_check`, `vs_boss_visual_check`, `roster_sheets` | Capturas (no asertan logica) | Revisar visualmente; dejan PNG en la raiz: borrar |
| Guardado de campana (`Campaign.cpp:75-84`) | Serializa `PlayerUpgrades` por orden | No reordenar campos de `PlayerUpgrades`; anadir al final con valor por defecto |
| Consumidores de `AttackType` | Stages usan `attackType==Energy` (excluir melee), `==Kick` (chispa pesada), `comboStep>=2` | `LegacyType` para ids nuevos: patadas -> Kick, puños -> Punch, proyectiles -> Energy; mantener `comboStep` |
| Enum `PlayerState` / `StreetEnemyState` | Comentarios piden no reordenar; hay `switch` sin default en SetState/UpdateTransformForm | Anadir al final y cubrir los `switch` (el compilador avisa con -Wswitch) |

Otras recomendaciones de proceso:
- Anadir tests nuevos en ctest antes de tocar el comportamiento: `CombatMathTests` (arco, barrido, carril),
  `MovesetTests` (rutas J J J J, J J K + jump cancel, K K K, v K, < J, QCF/DP/QCB, finisher), `JuggleTests`
  (budget, gravedad, OTG una vez), y un test que cuente **1 proyectil por onda** (regresion de B01).
- Probar con dt de 1/30 (Android) ademas de 1/60 (B11).
- Los checks visuales (`gow_check`, `all_modes_check`) dejan PNG en la raiz del repo y `gow_check` crea
  `district_fury_beta.dat` (en .gitignore). En esta auditoria se borraron ambos tras ejecutarlos.

### Orden sugerido para repartir entre implementadores (minimiza conflictos de archivos)
1. **Base (1 persona, primero):** HitResolver unico + HitInfo + `TakeDamage(HitInfo)` con envoltorios; B01, B03,
   B04, B08, B09, B10, B11, B15. Archivos: combat/CombatWorld|HitResolver, Player.cpp, StreetEnemy.cpp, todos los
   Stage*/VSMode (solo la parte de hits). Tests de regresion.
2. **Datos y cancel (paralelo tras 1):** AttackDef v2, ids nuevos, tabla de cancel, modificadores direccionales,
   QCB. Archivos: AttackData.*, PlayerCommon.h, PlayerCombat.cpp, InputBuffer.h, tests/InputBufferTests.
3. **Matematica de cajas:** CombatMath.h, GetAttackHitbox/GetHurtbox 3D, lane magnet, herramienta derive_hitboxes.
   Archivos: nuevos + PlayerCombat.cpp:468-494, Player.cpp:422-434, StreetEnemy.cpp:631-658.
4. **Aire/juggle/OTG/agarre:** PlayerState::Jump, estados de enemigo, fisica. Archivos: Player.*, StreetEnemy.*,
   EnemyBrain.*, InputMap/TouchControls (boton salto).
5. **Sensacion:** RootCurve, PoseFx, hitstop/shake escalados, FX por contacto, poses de tipo B con giro.
   Archivos: PlayerDraw.cpp, AttackAnimation.h, rendering/BossSprite, CombatWorld (feedback).
6. **Contenido por personaje:** clips nuevos en sprite_manifest.json (MAX, Brakk), mapeos KF/BETA, mascaras de
   capacidad, IA (`RivalAI` usa las rutas nuevas; enemigos con EnemyAttackDef).
