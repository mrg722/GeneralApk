# PLAN MAESTRO — District Fury: movimientos, combate, personajes y audio

Rama base: `ccr-077ad675-rlrg28` (commit de partida `1567205`). Autor: agente pensador (solo planifica; no toca el repo).
Insumos: `docs/auditoria_movimientos/{combate,personajes,audio}.md`
+ lectura propia del codigo (ver seccion 2, correcciones).

Pedido del usuario (resumido): mas movimientos para TODOS los personajes, arreglar Titan-X (camina de espaldas, le faltan
golpes), golpes "posicionados matematicamente", combos/patadas/poderes en serie, mas fluidez, auditar todo, trabajo
ENORME (no movimientos basicos) y una super mejora de audio.

Indice
1. Resumen ejecutivo (por impacto en el jugador)
2. Correcciones a las auditorias (lo que estaba mal o incompleto)
3. Que SI se hace, que NO (descartes con motivo) y decisiones abiertas para el usuario
4. Arquitectura objetivo y propiedad de archivos
5. Fases, dependencias y orden de integracion
6. Vocabulario comun (ids, inputs, formulas, presets) — referencia
7. BRIEFS (uno por implementador, autocontenidos)
8. Re-auditoria

---

## 1. Resumen ejecutivo (orden de impacto para el jugador)

1. **El combate esta roto por dentro, aunque "funcione".** Cada modo (Stage1..5, VS, BETA) tiene su propio resolvedor de
   golpes y el `CombatWorld` que deberia unificarlos es codigo muerto. Consecuencia directa: la ventana de combo nunca se
   abre (el contador siempre es 0, el FINISHER es inalcanzable, la furia no se gana pegando), la Onda de Energia dispara
   ~21 proyectiles por uso, los golpes conectan a otro carril (hasta 99 px "detras"), un ataque de area golpea a un solo
   enemigo, el jugador no retrocede al recibir golpes y el retroceso del bloqueo se acumula y lo hace resbalar despues.
   Todo lo que el usuario pide (mas golpes, combos, juggle, lanzadores) se construye encima de esto: **va primero**.
2. **TITAN-X camina, dashea, patea y dispara de espaldas** (ficha `facesRightByDefault=false` con arte que mira a la
   derecha), su golpe es media pose sin cabeza (dos PNG mitades), al recibir golpe muestra la pose de disparo, nunca cae y
   cambia de tamano en cada pose. TITAN-X MEJORADO igual y su derrota sale 2.25x. Los JEFES Titan-X tambien salen al reves.
3. **El moveset es diminuto e igual para los 42**: J,J,J + K + onda + dash. No hay salto, aereos, lanzador, barrida,
   codazo, giratoria, cadenas de patadas, ni combos de 4-6 golpes. Las habilidades 1-5 de "nuestros" personajes repiten los
   golpes normales. Los de poses sueltas (Clon, Grinder, Titan-X x2) no tienen NADA procedural: patinan congelados.
4. **Audio**: los golpes genericos son siseo sin cuerpo (no se oyen en movil), los de MAX/RONIN son graves sin chasquido,
   y **casi todo evento de combate es mudo en todos los modos** (muerte de enemigo, golpe enemigo de menos de 28, golpe al
   jefe, disparo de onda). No hay musica en el juego principal; la musica y SFX del modo BETA son del juego original
   (copyright: viola una regla del usuario). Sin mezcla (cada sonido corta al anterior), sin variacion, sin buses.
5. **MAX**: su forma blanca (240 cuadros) nunca se carga, sus clips mezclan dos escalas, el derribo empieza con cuadros de
   ataque; tiene ~190 cuadros sin usar (saltos, aereos, barridas, ganchos, rayos) = la mayor mina de movimientos.
6. **KF y BETA**: clips rellenados en silencio con `idle` o con 1 cuadro invisible (especiales de enemigos KF), "SOMBRAS" es
   la animacion de levantarse, doble caida de los heroes KF, `recovery == hit` en todos los BETA (gesto de dolor tras cada
   golpe propio), bestias BETA con un solo ataque para todo, KF HEROINA TRANSFORMADA sin animacion de golpe ni derrota.
7. **Enemigos de calle**: un solo ataque por tipo, juggle infinito y "fantasma" (hurtbox ignora la altura), sin estado en
   el suelo/levantarse, sin telegrafia sonora.

Meta cuantitativa al terminar todas las fases (se mide en la re-auditoria):
- Atlas A (Rayden 0, MAX 6, Brakk 2) y poses B (1, 3, 4, 5): >= 28 movimientos distintos por personaje (excepto pesados sin
  salto: >= 22), cadena J de 4-6 golpes, cadena K de 3-4, lanzador + aereos (si salta), 3 poderes propios + 5 habilidades
  que NO repiten normales.
- KF heroes (7, 8, 18-21): >= 22; KF enemigos (9-17): >= 12; GUERRERO/RONIN-KRATOS/RONIN: >= 18; bestias BETA: >= 9.
- 0 movimientos que reproduzcan `idle` como ataque. 0 personajes que caminen de espaldas.
- Todo evento de combate con sonido propio, dueno unico; musica original por modo; 0 archivos de audio con copyright en
  rutas que el juego cargue.

---

## 2. Correcciones a las auditorias (verificado en el codigo)

1. **audio.md §2.2 y §2.5 ("sonido doble por CombatWorld") es incorrecto.** `CombatWorld::ResolvePlayerMelee`,
   `ResolvePlayerMeleeOnBoss`, `ResolveEnemyMelee`, `ResolveProjectiles`, `ResolveDestructibles`, `ResolvePickups`,
   `SpawnProjectile` y `SpawnPlayerEnergyWave` **no los llama nadie** (grep: los stages solo llaman `combatWorld.Update` y
   `ResolveHazards`). Las 13 llamadas `AudioSystem::Play` de `CombatWorld.cpp` son codigo muerto. La realidad es PEOR: en
   TODOS los modos (Stage1 incluido) son mudos la muerte del enemigo, el golpe enemigo de menos de 28 de dano, el golpe al
   jefe, el disparo/impacto de la onda y los objetos. Lo unico que suena en combate: impacto del jugador
   (`PlayerDraw.cpp` UpdateMotionFeel), whoosh/especiales (`PlayerCombat.cpp`), `Block`/`Hit`/`HeavyHit` en
   `Player::TakeDamage` (solo bloqueo, golpe durante habilidad, dano>=28), rotura de guardia enemiga (`StreetEnemy.cpp:262`)
   y la pausa del Stage1. La regla "dueno unico por evento" sigue siendo la correcta para lo que viene.
2. **personajes.md bloque 4 (regenerar `data/kratos/ronin.txt` con `tools/ronin/build_ronin.py`) y bloque 5 (regenerar
   `kratos_*.txt`) violan la regla md5 del usuario.** Descartado: toda mejora de RONIN / GUERRERO / RONIN-KRATOS / bestias se
   hace **derivando clips en memoria al cargar, solo en District Fury** (`src/game/beta/BetaDerive*.cpp`), sin escribir
   ningun `.txt`. Ya existe el verificador `tools/ronin_kratos/check_kratos_md5.py <ref>` (415 archivos, hoy "distintos: 0").
   Igual para audio.md P4.3 (quitar `clipsound` duplicados en los `.txt`): se deduplica en runtime en `BetaMode`.
3. **MAX blanco (`max_white`)**: la causa exacta no es "el atlas no declara clips" en abstracto: el juego de clips sigue
   llamandose `rayder_kf_white` (no se renombro en el commit 550c16e) y el atlas `max_white` busca el juego `max_white`
   (`SpriteManifest.cpp:94`: `clipSet = "clips" o el id`). Arreglo: renombrar el juego a `max_white` o anadir
   `"clips": "rayder_kf_white"` al atlas. Sus indices llegan a 241 (< 252 celdas de la rejilla 14x18): cargan. La celda 142
   trae texto incrustado: no usarla en ningun clip. Las celdas 245-285 de `max` (agregadas en 550c16e) NO existen en
   `max_white`: los movimientos que las usan deben caer al atlas base con tinte blanco durante la transformacion.
4. **`CMakeLists.txt` lista los .cpp a mano** (`district_fury_core`): cualquier archivo nuevo obligaria a todos los
   implementadores a editarlo (choque seguro en paralelo). Fase 0.1 lo cambia a globs por carpeta.
5. **`BetaMode` (GowApk) usa el mismo `Player` y `KfReference`** (`BetaMode.cpp:161 ApplyCharacter`, `RefFor`): todo cambio
   en `Player.cpp`, `PlayerCombat.cpp` o `KfReference.cpp` afecta a `gow_check` (margen actual 27 s en la peor oleada). Las
   derivaciones de clips BETA se activan solo en District Fury (flag que BetaMode apaga).
6. combate.md cita lineas como `AttackAnimation.h:267`: ese archivo tiene 14 lineas muy largas; las referencias de linea
   de los informes son aproximadas. Los hechos (perfiles solo skins 2 y 6, curva .30) son correctos.
7. combate.md propone retocar Punch1/Punch2 a 5/4/8: **descartado** (rompe `TestChainWithCancel` y el borde de
   `CombatSmokeTests` "P1 activo a 0.15 s" por poca ganancia). Se conservan los tiempos de los 9 ataques historicos.
8. Confirmados por lectura directa: B01 (Player.cpp:384 vuelve a poner `energyReleased`), B02/B03 (OnAttackConnected solo lo
   llama CombatWorld muerto), Titan-X `facesRightByDefault=false` y `Boss.cpp`/`Stage3Game.cpp:567`/`Stage4Game.cpp:447`
   con el mismo supuesto, `DrawBossPose` reescala cada pose (BossSprite.h), `CharacterPose` decide solo por `AttackType`.

---

## 3. Que SI, que NO, y decisiones abiertas

### 3.1 SI (entra en este plan)
- Resolvedor unico de golpes con `HitInfo` + colision 3D (X, carril, altura) + arcos de miembro + barrido anti-tunel.
- Arreglo de B01, B03, B04, B05, B06 (a-f), B07 (con juggle real), B08, B09, B10, B11, B12, B13, B14, B15, B16, B17, B18, B19,
  B21, B22, B23. B20 (bloqueo direccional/parry) y agarres en Fase 2.
- 31 AttackIds nuevos genericos + cadenas por personaje de hasta 6 golpes + 3 poderes "Signature" propios por personaje.
- Salto (estado propio), aereos, lanzador con persecucion, OTG, rebotes, victoria, carrera con doble toque, back-dash,
  aceleracion/frenado al caminar, giro con clip `turn`.
- Motor visual procedural: PoseFx por fotogramas clave con easing, presets matematicos, estelas con textura, FX
  superpuestos, escala uniforme y orientacion POR POSE para personajes de poses sueltas, "procedural ligero" para KF/BETA.
- Contenido por personaje para los 42 (bloques en paralelo).
- Audio: motor de mezcla (pool de voces, prioridad, antirrafaga, variacion, buses, ducking, pan, limitador), director de
  eventos, banco sintetizado nuevo, musica original adaptativa, BETA sin copyright.
- IA de rivales usando las rutas nuevas, HUD de combo y lista de movimientos en pausa.

### 3.2 NO (descartado) y por que
| Propuesta | Motivo |
|---|---|
| Regenerar `data/kratos/*.txt` (Kratos, RONIN, RONIN-KRATOS) | Regla md5 del usuario. Se deriva en memoria. |
| Editar PNG de `assets/characters/max/` o de `rayden_*.png` | Regla del usuario. MAX solo cambia listas de cuadros y escalas en el manifiesto. |
| Editar PNG de `assets/bosses/titanx*`, `rayder_clone` (restos de recorte) | Innecesario: se recorta con `source rect` al dibujar y se unen las dos mitades del puno en runtime. Cero riesgo para los jefes. |
| Movimientos EX (J+K en el comando) | Choca con agarre/contra, ilegible en tactil, poca ganancia. |
| Retocar tiempos de Punch1/Punch2 | Tests y poca ganancia (ver 2.7). |
| Migrar `BetaMode` al resolvedor unico | Riesgo directo sobre `gow_check` (margen 27 s) y su balance propio. Se re-evalua en la re-auditoria. |
| Arte nuevo (caminar real de Titan-X, Grinder en HD, victoria dibujada) | Fuera de alcance: se resuelve con transformaciones procedurales sobre arte existente. Queda listado como "pedido de arte" para el usuario. |
| Celdas 153-203 del atlas de MAX | Son OTRO personaje (encapuchado, chaqueta negra) mezclado en el atlas. |
| Celda 142 de `max_white` | Trae texto incrustado. |
| Sintesis de musica en tiempo real | CPU en Android. Se pre-renderiza a OGG. |
| Boton "arriba" como modificador | W es cambio de carril. Se usa boton de SALTO nuevo. |
| Salto para pesados (Brakk 2, Grinder 3, Titan-X Mejorado 5, bestias BETA) | Coherencia de personaje; Titan-X (4) tiene salto pesado corto. |
| Reproducir los `clipsound` de Kratos/RK en el juego principal | Son SFX del juego original (copyright). Se usan arquetipos sintetizados por arma. |
| Herramienta offline generica de cajas desde el alfa para todos | Solo para MAX (mucho arte nuevo); el resto usa arcos parametricos. |

### 3.3 Decisiones abiertas para el usuario (los implementadores aplican el valor por defecto y lo reportan)
1. **Diseno de MAX**: hoy mezcla el set "chaqueta" (reposo/caminar) con el set "camiseta + cinta azul" (sus hojas v3). Por
   defecto: se conservan ambos y se iguala la ESCALA del cuerpo; no se elige uno.
2. **`assets/kratos/audio/*.ogg`** (SFX del juego original, huerfanos: nadie los carga, pero `build_apk.sh` los mete en el
   APK). Por defecto: no se borran; el brief F1.13 deja la lista y el comando de borrado para que el usuario lo apruebe.
3. **`apk_reference/king_fighter_iii/bin/music/`** (MIDI/WAV con copyright, no se cargan). Igual que el punto 2.
4. Tecla de salto: por defecto `I` (teclado) y boton tactil "SALTO".

---

## 4. Arquitectura objetivo y propiedad de archivos

Principio: **motor comun en datos, contenido por personaje en archivos propios**. Despues de Fase 0 el motor queda
"congelado": los bloques de Fase 1 solo escriben datos/tablas en sus archivos. Si un bloque necesita cambiar el motor, NO lo
edita: lo pide en su informe ("PEDIDO AL MOTOR") y el integrador lo agenda.

Contratos (los crea F0.1; los rellenan F0.2/F0.3/F0.4):
- `src/game/events/GameEvents.h/.cpp`: cola de eventos de juego (`EmitGameEvent`, `DrainGameEvents`). El audio solo
  consume eventos: **ningun bloque de audio toca archivos de combate**.
- `src/game/combat/HitTypes.h`: `HitInfo`, `HitResult`, `HitKind`, `StageHitStyle`.
- `src/game/combat/CombatMath.h`: geometria 3D (capsula vs caja, carril, arco de miembro). Header puro, testeable.
- `src/game/moves/MoveTypes.h`: `Ease`, `RootCurve`, `LimbArc`, `FxKey`, `PoseFx`, `PosePiece`, `PoseSegment`, `ClipSeg`,
  `MoveVisual`, `MoveOverride`, `MoveSet`, capacidades. `src/game/moves/MoveRegistry.h/.cpp`: `GetMoveSet(skin)`.
- `src/game/moves/Moves_<Bloque>.cpp` (uno por bloque de Fase 1) con `const MoveSet* MovesetFor_<Bloque>(int skin)`.
- `src/game/visual/PoseTables.h` + `Poses_<Bloque>.cpp`: tablas de poses por personaje de poses sueltas.
- `src/game/player/PlayerVisualState.h`: estado SOLO de dibujo (estelas, fase visual) como miembro `mutable` de `Player`.
- `src/game/beta/BetaDerive.h/.cpp` + `BetaDerive_Warriors.cpp` + `BetaDerive_Beasts.cpp`: derivaciones en memoria.
- `src/audio/AudioDirector.*`, `src/audio/SfxBank.*`, `src/audio/MusicPlayer.*`.
- `data/manifest/<personaje>_moves.json`: clips nuevos que se AGREGAN a un juego del manifiesto (no pueden pisar clips
  existentes salvo `"override": true`, prohibido en el juego `rayden`).

Mapa de propiedad (quien puede editar cada archivo en cada fase; "-" = congelado):

| Archivo / carpeta | F0.1 | F0.2 | F0.3 | F0.4 | Fase 1 | Fase 2 |
|---|---|---|---|---|---|---|
| CMakeLists.txt | F0.1 | - | - | - | - | - |
| src/game/Player.h | F0.1 | F0.2 | - | F0.4 | - | F2.1 |
| src/game/Player.cpp, player/PlayerCombat.cpp, player/PlayerCommon.h | F0.1 (solo mover) | F0.2 | - | F0.4 | - | F2.1 |
| src/game/player/PlayerDraw.cpp, PlayerPoseDraw.cpp, PlayerAtlasDraw.cpp, PlayerVisualState.h, PoseFxPresets.h | F0.1 (solo mover) | - | F0.3 | - | - | - |
| src/game/combat/AttackData.*, AttackAnimation.h | - | F0.2 (solo campos) | - | F0.4 | - | F2.1 (filas de agarre/defensa) |
| src/game/combat/HitResolver.*, CombatMath.h, HitTypes.h, CombatWorld.*, HitstopClock.h | F0.1 (contratos) | F0.2 | - | - | F1.9 (solo HitResolver, lado enemigo) | F2.1 |
| src/game/StreetEnemy.*, enemies/EnemyBrain.* | - | F0.2 | - | - | F1.9 | - |
| Stage1*/Stage2-5Game.cpp, VSMode.cpp | - | F0.2 | - | F0.4 (solo victoria) | F1.1 (solo DrawBoss de Stage3/4) / F1.10 (solo lineas de UI en VSMode) | F2.3 (pausa) |
| src/game/combat/Boss.cpp | - | F0.2 | - | - | F1.1 (dibujo Titan) | F2.2 |
| src/game/InputBuffer.h, core/InputMap.* | - | F0.2 (solo hitstop en buffer) | - | F0.4 | - | - |
| src/ui/TouchControls.cpp | - | - | - | F0.4 (boton SALTO) | - | F2.3 (solo abrir lista en pausa) |
| src/game/CharacterVisual.* | F0.1 | - | F0.3 | - | - | - |
| src/game/visual/Poses_TitanX.cpp | F0.1 | - | F0.3 | - | F1.1 | - |
| src/game/visual/Poses_CloneGrinder.cpp | F0.1 | - | F0.3 | - | F1.2 | - |
| src/game/moves/MoveTypes.h, MoveRegistry.* | F0.1 | - | F0.3 (solo campos visuales) | F0.4 | - | - |
| src/game/moves/Moves_TitanX.cpp / _CloneGrinder / _Rayden / _Max / _Brakk / _Kf / _Warriors / _Beasts | F0.1 (stubs) | - | - | F0.4 (defaults) | F1.1 / F1.2 / F1.3 / F1.4 / F1.5 / F1.6 / F1.7 / F1.8 | - |
| src/rendering/BossSprite.h, Animator.*, SpriteManifest.*, AssetManager.cpp | - | - | F0.3 | - | - | - |
| data/sprite_manifest.json | - | - | F0.3 (includes) | - | F1.4 (solo juegos max / max_white / rayder_kf_white) | - |
| data/manifest/rayden_moves.json / brakk_moves.json | - | - | F0.3 (vacios) | - | F1.3 / F1.5 | - |
| assets/bosses/brakk/brakk_v2_atlas_clean.png | - | - | - | - | F1.5 (OPCIONAL, limpieza de mate por script) | - |
| src/game/lab/KfReference.* | F0.1 (gancho) | - | - | - | F1.6 | - |
| src/game/beta/BetaDerive*.cpp | F0.1 (stubs) | - | - | - | F1.7 (Warriors) / F1.8 (Beasts) | - |
| src/audio/AudioSystem.*, AudioDirector.*, SfxBank.* | F0.1 (stubs) | F0.2 (mapa legado) | - | - | F1.10 | - |
| src/audio/MusicPlayer.* | F0.1 (stub) | - | - | - | F1.12 | - |
| src/main.cpp, src/ui/MainMenu.cpp, src/core/DisplaySettings.* | - | F0.2 (eventos de modo) | - | - | F1.10 | - |
| tools/audio/genera_banco_combate.py, assets/audio/sfx/{combat,arquetipos,rayder_v2,ronin_v2}/ | - | - | - | - | F1.11 | - |
| tools/audio/genera_musica.py, assets/audio/music/ | - | - | - | - | F1.12 | - |
| GowApk/src/BetaMode.cpp, GowApk/assets/audio/, tools/audio/genera_beta_audio.py | F0.1 (1 linea flag) | - | - | - | F1.13 | - |
| src/game/RivalAI.*, RivalAIProfile.* | - | - | - | - | - | F2.2 |
| src/ui/GameHUD.*, src/ui/MoveListScreen.* | - | - | - | - | - | F2.3 |
| tests/unit/<Bloque>Tests.cpp | cada bloque crea el suyo | | | | | |
| tests/tools/StateStrips.cpp / MovesSheet.cpp / EnemyStrips.cpp | F0.1 / - / - | - | F0.3 (StateStrips) | F0.4 (MovesSheet) | F1.9 (EnemyStrips) | - |

---

## 5. Fases, dependencias y orden de integracion

```
FASE 0 (cimientos; NADA de Fase 1/2 corre hasta que termine e integre)
  F0.1 Reorganizacion + contratos (solo)  ──►  F0.2 Resolvedor unico + bugs + eventos ──► F0.4 Framework de movimientos
                                          └─►  F0.3 Motor visual procedural  (en paralelo con F0.2 y F0.4)
  Integrar F0.2, F0.3, F0.4  →  correr los 6 checks en la rama base  →  etiquetar "base-fase1"

FASE 1 (13 en paralelo, archivos disjuntos; todos parten de "base-fase1")
  Personajes: F1.1 Titan-X x2 | F1.2 Clon+Grinder | F1.3 Rayden | F1.4 MAX | F1.5 Brakk | F1.6 KF (7-21)
              F1.7 RONIN+GUERRERO+RONIN-KRATOS | F1.8 Bestias BETA
  Motor de enemigos: F1.9
  Audio: F1.10 Motor+director | F1.11 Banco SFX | F1.12 Musica | F1.13 BETA sin copyright
  Integrar en este orden (minimiza conflictos): F1.9, F1.10, F1.11, F1.12, F1.13, F1.1, F1.2, F1.3, F1.4, F1.5, F1.6, F1.7, F1.8
  →  checks  →  etiquetar "base-fase2"

FASE 2 (3 en paralelo)
  F2.1 Defensa y agarre (depende de F1.9) | F2.2 IA con el moveset (depende de Fase 1 completa) | F2.3 HUD + lista de movimientos
  →  integrar → checks → FASE 3 Re-auditoria (seccion 8)
```

Reglas de integracion (para el orquestador):
- Cada brief trabaja en su worktree desde la punta de la rama base de su fase y commitea en su rama (sin push). El
  orquestador mergea a `ccr-077ad675-rlrg28` en el orden indicado y corre los 6 checks tras CADA merge.
- Si dos briefs tocan el mismo archivo, es un error del plan: parar y reasignar (la tabla de la seccion 4 es la autoridad).
- Los "PEDIDO AL MOTOR" de Fase 1 se juntan en un brief corto "F1.X-motor" que corre solo, despues de integrar Fase 1.
- Si un bloque de personaje no llega a todo, lo integrado debe compilar y pasar checks: entregar por commits pequenos.

---

## 6. Vocabulario comun (referencia; los briefs repiten lo que necesitan)

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

---

## 7. BRIEFS

Cada brief es autocontenido: pegar desde "BRIEF Fx.y" hasta el final de su "ANEXO COMUN".

<!-- BRIEFS -->
### BRIEF F0.1 — Reorganizacion de archivos y contratos del motor (va SOLO y PRIMERO)

**Objetivo.** Preparar el repo para que ~20 implementadores trabajen en paralelo sin pisarse: partir archivos que todos
necesitarian, crear los headers "contrato" que definen las interfaces comunes, crear stubs vacios de los archivos de cada
bloque y pasar CMake a globs. **Sin ningun cambio de comportamiento ni de imagen**: el juego debe verse y jugarse
exactamente igual (se verifica pixel a pixel).

**PUEDES TOCAR:** `CMakeLists.txt`; `src/game/Player.h`; `src/game/Player.cpp`, `src/game/player/PlayerCombat.cpp`,
`src/game/player/PlayerCommon.h` (solo cubrir enums nuevos en switches); `src/game/player/PlayerDraw.cpp` (solo mover
codigo); NUEVOS `src/game/player/PlayerPoseDraw.cpp`, `src/game/player/PlayerAtlasDraw.cpp`,
`src/game/player/PlayerVisualState.h`; `src/game/CharacterVisual.cpp/.h` (solo mover `CharacterPose`); NUEVOS
`src/game/visual/PoseTables.h`, `src/game/visual/Poses_TitanX.cpp`, `src/game/visual/Poses_CloneGrinder.cpp`,
`src/game/visual/Poses_Brakk.cpp`; NUEVOS `src/game/events/GameEvents.h/.cpp`; NUEVOS `src/game/combat/HitTypes.h`,
`src/game/combat/CombatMath.h` (solo declaraciones/inline vacias o triviales); NUEVOS `src/game/moves/MoveTypes.h`,
`src/game/moves/MoveRegistry.h/.cpp` y los stubs `src/game/moves/Moves_{TitanX,CloneGrinder,Rayden,Max,Brakk,Kf,Warriors,Beasts}.cpp`;
NUEVOS `src/game/beta/BetaDerive.h/.cpp`, `BetaDerive_Warriors.cpp`, `BetaDerive_Beasts.cpp`; `src/game/lab/KfReference.cpp`
(UNA linea: llamada al gancho de derivacion); `GowApk/src/BetaMode.cpp` (UNA linea: apagar la derivacion); NUEVOS
`src/audio/AudioDirector.h/.cpp`, `src/audio/SfxBank.h/.cpp`, `src/audio/MusicPlayer.h/.cpp` (stubs); `src/audio/AudioSystem.h`
(solo declarar `enum class Bus` y `float BusVolume(Bus) const` que devuelve 1); NUEVOS `tests/tools/StateStrips.cpp`,
`tests/unit/ContractsTests.cpp`.
**NO TOQUES:** nada de assets/ ni data/, ni logica de combate, ni ningun otro archivo.

**Tareas (en commits separados):**
1. **CMake por globs.** En `district_fury_core` conservar la lista actual y anadir
   `file(GLOB DF_GLOB_SOURCES CONFIGURE_DEPENDS src/game/moves/*.cpp src/game/visual/*.cpp src/game/events/*.cpp src/game/player/*.cpp src/game/combat/*.cpp src/game/beta/*.cpp src/audio/*.cpp)`
   + `list(REMOVE_DUPLICATES ...)`. Tests: `file(GLOB DF_UNIT_TESTS CONFIGURE_DEPENDS tests/unit/*Tests.cpp)` y por cada uno
   `add_executable` (nombre = stem en snake_case o el stem tal cual) enlazado a `district_fury_core` +
   `add_test(NAME ... COMMAND ... WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})`. Herramientas: `file(GLOB DF_TOOLS CONFIGURE_DEPENDS tests/tools/*.cpp)`
   -> `add_executable` sin `add_test` (nombre del ejecutable en snake_case: `StateStrips.cpp` -> `state_strips`). Verificar
   que `tools/android/build_apk.sh` sigue configurando (no lo edites; si algo falla, reportalo).
2. **Partir `PlayerDraw.cpp`** (moviendo codigo, sin cambiar una sola expresion): la rama de poses sueltas
   (`if (skin != 0 && cv.folder != nullptr && cv.atlasId == nullptr)` + `if (altTex.id != 0) {...}`) pasa a
   `void Player::DrawPoseBody(Vector2 p, float scale, Color tint) const` en `PlayerPoseDraw.cpp`; la rama `animator`
   (estelas + KF + switch procedural de sx/sy/angle + giro + aterrizaje + dibujo) pasa a
   `void Player::DrawAtlasBody(Vector2 p, float scale, float spriteScale, Color tint) const` en `PlayerAtlasDraw.cpp`.
   `Player::Draw` queda como orquestador. Declarar ambas en `Player.h` (privadas).
3. **Partir `CharacterPose`**: el `switch` por id pasa a funciones `const char* PoseClone(...)` (id 1), `PoseGrinder` (3)
   en `Poses_CloneGrinder.cpp`, `PoseTitanX` (4), `PoseTitanXM` (5) en `Poses_TitanX.cpp`, `PoseBrakkLegacy` (2) en
   `Poses_Brakk.cpp`, declaradas en `src/game/visual/PoseTables.h`; `CharacterPose` en CharacterVisual.cpp solo despacha.
   Anadir en `PoseTables.h` (sin uso aun) `bool PoseFacesRight(const char* folder, const char* pose)` implementada en cada
   archivo devolviendo el valor actual (`facesRightByDefault` de la ficha) — F1.1/F1.2 la corregiran.
4. **Contratos (headers).** Crear con comentarios en espanol y valores por defecto que NO alteren nada:
   - `GameEvents.h`: `enum class GameEventType { HitLanded, HitBlocked, HitArmored, CounterHit, PlayerHurt, PlayerBlocked, PlayerGuardBreak, PlayerKnockdown, PlayerDefeat, EnemyKilled, EnemyGuardBreak, BossHit, BossPhase, BossAttack, ProjectileSpawn, ProjectileHit, Swing, SpecialStart, RageStart, TransformStart, Dash, Jump, Land, KnockdownLand, Footstep, Pickup, PropHit, ComboMilestone, Telegraph, UiNavigate, UiConfirm, UiCancel, UiOpen, UiPause, ModeEnter, ArenaLock, ArenaUnlock, WaveStart, StageClear, GameOver, Victory, Count };`
     `enum class HitKind { Light, Medium, Heavy, Kick, Slash, Energy, Block, GuardBreak, Prop };`
     `enum class GameMode { Boot, Menu, CharacterSelect, Options, Reward, Stage1, Stage2, Stage3, Stage4, Stage5, Boss, VS, Beta };`
     `struct GameEvent { GameEventType type; float x = 0, y = 0; int skin = -1; HitKind kind = HitKind::Light; int weight = 0; int value = 0; int attack = 0; bool heavy = false; bool fromEnemy = false; };`
     `void EmitGameEvent(const GameEvent&);` `template<class F> void DrainGameEvents(F&& f);` (cola circular de 256; si se
     llena, descarta el mas viejo y cuenta `DroppedGameEvents()`).
   - `HitTypes.h`: `struct HitInfo { int damage = 0; float dirX = 1; float knockback = 0, launch = 0, hitstun = 0.3f, blockstun = 0.2f, hitstop = 0.06f, shake = 0.06f; int juggleCost = 1; bool otg = false, breaksGuard = false, heavy = false, counter = false, wallBounce = false, groundBounce = false, unblockable = false, low = false, overhead = false, knockdown = false, pull = false; int sourceAttack = 0; int hitIndex = 0; int attackerSkin = -1; Vector3D contact{}; };`
     `enum class HitResult { Miss, Hit, Blocked, Armored, Killed, Ignored };`
     `struct StageHitStyle { int rageBonus = 0; Color spark{255,235,150,255}; Color sparkHeavy{255,150,45,255}; int scorePerDamage = 10; int scorePerCombo = 7; };`
   - `CombatMath.h`: firmas (inline, implementacion trivial o `return false` si no se usa todavia):
     `struct Capsule2 { Vector2 a, b; float r; }; struct Rect2 { float x0, y0, x1, y1; }; struct Hurt3 { float x0, x1, lane, halfDepth, h0, h1; };`
     `bool CapsuleRect(const Capsule2&, const Rect2&); float DistPointSegment2(...);` — F0.2 las implementa de verdad.
   - `MoveTypes.h`: `enum class Ease { Linear, InQuad, OutQuad, OutCubic, InCubic, InOutSine, OutBack, Bezier, Profile };`
     `struct RootCurve { float dx = 0, dz = 0, t0 = 0, t1 = 1; Ease ease = Ease::OutCubic; float bezP1 = .3f, bezP2 = .9f; bool slowOnHit = true; };`
     `struct LimbArc { float px = 8, ph = -84, L0 = 36, L1 = 74, theta0 = -8, theta1 = 0, r = 14; Ease ease = Ease::OutCubic; bool behind = false; bool bothSides = false; };`
     `struct FxKey { char phase = 'A'; float v = 0; float sx = 1, sy = 1, angle = 0, dx = 0, dy = 0; };`
     `enum class FxPreset { None, Anticipa, Impacto, Recobro, Saltito, Upper, Giro, Barrida, CaidaRot, Temblor, Latido, Tinte };`
     `struct PresetUse { FxPreset preset = FxPreset::None; float a = 1, b = 0; char phase = 'A'; };`
     `struct OverlayFx { int first = -1, count = 0; float x = 0, y = -70, scale = 1, rot = 0; char phase = 'A'; Color tint = WHITE; };`
     `struct PoseFx { PresetUse presets[6]; int presetCount = 0; FxKey keys[8]; int keyCount = 0; int ghostCount = 0; float ghostInterval = .03f; Color ghostTint{110,190,255,255}; OverlayFx overlay[2]; int overlayCount = 0; };`
     `struct PosePiece { const char* pose = nullptr; Rectangle src{}; Vector2 offset{}; bool facesRight = true; };`
     `struct PoseSegment { char phase = 'A'; float v0 = 0, v1 = 1; PosePiece pieces[2]; int pieceCount = 1; };`
     `struct ClipSeg { const char* clip = nullptr; int from = -1, to = -1; float speed = 1; bool reverse = false; };`
     `struct MoveVisual { ClipSeg clips[3]; int clipCount = 0; const char* clipFallback = nullptr; PoseSegment segs[4]; int segCount = 0; PoseFx fx; };`
     `struct ProjectileSpec { bool enabled = false; float speed = 620, life = 1.2f; int damage = 0; Vector2 origin{60,-76}; float scale = 1; Color tint = WHITE; int count = 1; float spreadLane = 0, interval = 0; bool arc = false; float arcRange = 260; };`
     `struct MoveOverride { int id = 0; bool enabled = true; bool replaceDef = false; AttackDef def{}; float damageScale = 1, timeScale = 1, reachScale = 1; bool hasArc = false; LimbArc arc; RootCurve root; int hitCount = 1; float multiHitInterval = 0; float armorFrom = -1, armorTo = -1; int armorHits = 0; float areaRadius = 0; float pullPx = 0; ProjectileSpec projectile; MoveVisual visual; const char* displayName = nullptr; };`
     `enum MoveCaps : unsigned { CapJump = 1u<<0, CapAir = 1u<<1, CapLauncher = 1u<<2, CapSweep = 1u<<3, CapCommandNormals = 1u<<4, CapSpecials = 1u<<5, CapGrab = 1u<<6, CapRun = 1u<<7, CapBackDash = 1u<<8, CapHeavyJump = 1u<<9, CapAll = 0x3FF };`
     `enum class FeelLevel { None, Light, Full };`
     `struct MoveSet { const char* name = "generico"; unsigned caps = CapAll; FeelLevel feel = FeelLevel::Full; float bodyScale = 1; int chainJ[6]; int chainJCount = 4; int chainK[4]; int chainKCount = 3; int skillSlots[5]; const MoveOverride* overrides = nullptr; int overrideCount = 0; MoveVisual hit, knockdown, getup, victory, jumpRise, jumpFall, walk, block; float jumpVz = 720; };`
     (Los ids se guardan como `int` = `static_cast<int>(AttackId)` para no acoplar el header. Si hace falta un campo que no
     esta, anadelo AHORA con valor por defecto neutro y documentalo: despues de esta fase el contrato solo crece al final.)
   - `MoveRegistry.h/.cpp`: `const MoveSet& GetMoveSet(int skin);` que pregunta, en orden, a `MovesetFor_TitanX(skin)`,
     `_CloneGrinder`, `_Rayden`, `_Max`, `_Brakk`, `_Kf`, `_Warriors`, `_Beasts` (cada stub devuelve `nullptr`) y si nadie
     responde devuelve un `MoveSet` generico estatico; `const MoveOverride* FindOverride(const MoveSet&, int id);`.
   - `PlayerVisualState.h`: `struct PlayerVisualState { };` vacio (F0.3 lo llena) y en `Player.h` el miembro
     `mutable PlayerVisualState visual;`.
   - `Player.h`: agregar al FINAL de `PlayerState` los valores `Jump, Victory` (nadie entra todavia) y cubrir todos los
     `switch` que el compilador marque (tratar como `Idle`). Agregar `bool jump = false;` a `PlayerInput` (sin leerlo).
   - `BetaDerive.h/.cpp`: `void SetBetaDeriveEnabled(bool); bool BetaDeriveEnabled(); void DeriveBetaClipsForDistrict(KfReference& ref, const char* betaId);`
     que llama a `DeriveWarriors(ref, betaId)` y `DeriveBeasts(ref, betaId)` (stubs vacios en sus archivos). En
     `KfReference.cpp` `Load`: `if (who.betaId) { KfReference r = LoadBetaCharacter(who.betaId); if (BetaDeriveEnabled()) DeriveBetaClipsForDistrict(r, who.betaId); return r; }`.
     En `BetaMode.cpp`, al iniciar el modo (antes de cargar personajes): `SetBetaDeriveEnabled(false);`.
   - Audio: `AudioDirector.h/.cpp` con `void AudioDirectorUpdate(float dt);` (stub vacio, F0.2 lo rellena); `MusicPlayer.h/.cpp`
     con `class MusicPlayer { public: static MusicPlayer& Get(); void OnEvent(const GameEvent&); void Update(float dt); void SetDuck(float gain); void SetVolume(float v); };`
     (todo vacio); `SfxBank.h/.cpp` vacio con comentario de proposito. En `AudioSystem.h`: `enum class Bus { Master, Sfx, Ui, Voice, Music, Ambient };`
     y `float BusVolume(Bus) const { return 1.0f; }`.
5. **Herramienta de tiras de estados.** Copiar `docs/auditoria_movimientos/herramientas/states.cpp`
   a `tests/tools/StateStrips.cpp` (target `state_strips`): uso `state_strips <dir_salida> [id]`, escribe
   `<dir>/states_<id>.png` (y nada en la raiz del repo). Ajustar includes/rutas si hace falta.
6. **Test de contratos** `tests/unit/ContractsTests.cpp`: emite 300 eventos y verifica que `DrainGameEvents` entrega 256
   y `DroppedGameEvents()==44`; `GetMoveSet(0..41)` devuelve el generico sin crash; `AttackId::Count` sigue en 10.

**Criterios de aceptacion medibles.**
- Pixel identico: haz PRIMERO las tareas 1 y 5 (CMake + herramienta, sin tocar codigo del juego), commitea, ejecuta
  `xvfb-run -a ./build/state_strips /tmp/f01_antes`; al terminar todo, `xvfb-run -a ./build/state_strips /tmp/f01_despues`
  y compara los 42 PNG con PIL/numpy: 0 pixeles distintos (si la herramienta usa `GetTime()`, fija el reloj o compara
  con la misma semilla/tiempo; documenta como). Informa el resultado.
- `ctest` 8/8 (7 historicos + ContractsTests). `all_modes_check` y `gow_check` `problemas: 0` con los mismos tiempos (+-1 s).
- `git grep -n "CharacterPose\|DrawBossPose" src/game/player/PlayerDraw.cpp` -> ya no contiene la rama de poses.
- Un archivo nuevo vacio en `src/game/moves/` compila sin tocar CMake (pruebalo y borralo).

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F0.2 — Resolvedor unico de golpes, bugs del motor y eventos de juego

**Parte de:** la rama base con F0.1 integrado. Corre en paralelo con F0.3 (archivos disjuntos). F0.4 empieza cuando
termines.

**Contexto.** Hoy cada modo resuelve los golpes a su manera: `Stage1StoryGame::HandlePlayerHits/HandleEnemyHits/HandleBossHits`
(`src/game/stage1/Stage1Combat.cpp` ~104-170), `Stage1Boss.cpp`, `Stage2Game.cpp` ~157-200, `Stage3Game.cpp` ~278-320,
`Stage4Game.cpp` ~166-205, `Stage5Game.cpp` ~176-205, `VSMode.cpp` ~250-342. `CombatWorld::ResolvePlayerMelee` y compania
(CombatWorld.cpp 233-460) son codigo muerto: solo se llama `ResolveHazards`. Por eso `Player::OnAttackConnected` nunca se
ejecuta (combo siempre 0, Finisher inalcanzable, `rageGain` muerto, `comboWindowBonus` sin efecto). Detalle completo de
los 24 bugs: `docs/auditoria_movimientos/combate.md` secciones 1-3 y 5
(leela entera antes de empezar).

**PUEDES TOCAR:** `src/game/combat/HitResolver.h/.cpp` (nuevo), `src/game/combat/CombatMath.h`, `src/game/combat/HitTypes.h`
(solo agregar campos al final), `src/game/combat/CombatWorld.h/.cpp`, `src/game/combat/HitstopClock.h`,
`src/game/combat/AttackData.h/.cpp` (solo agregar CAMPOS a `AttackDef` con valores que reproduzcan el comportamiento
actual; NO filas nuevas), `src/game/combat/Boss.cpp/.h` (resolucion y eventos), `src/game/Player.h`, `src/game/Player.cpp`,
`src/game/player/PlayerCombat.cpp`, `src/game/player/PlayerCommon.h`, `src/game/InputBuffer.h` (solo B15),
`src/game/StreetEnemy.h/.cpp`, `src/game/enemies/EnemyBrain.h/.cpp`, `src/game/stage1/*.cpp`, `src/game/Stage1StoryGame.*`,
`src/game/Stage2Game.*`..`Stage5Game.*`, `src/game/VSMode.*`, `src/game/events/GameEvents.*`, `src/audio/AudioDirector.*`, `src/audio/AudioSystem.cpp` (solo llamar a
`AudioDirectorUpdate`), `src/main.cpp` y `src/ui/MainMenu.cpp` (solo emitir eventos de UI/modo), `tests/unit/HitResolverTests.cpp`,
`tests/unit/CombatMathTests.cpp`, y los tests existentes `tests/CombatSmokeTests.cpp`, `tests/InputBufferTests.cpp`,
`tests/Stage1PlaythroughTest.cpp`, `tests/Stage1Bot.h` solo si un cambio de comportamiento lo exige (documentado).
**NO TOQUES:** `GowApk/**` (BetaMode conserva su resolvedor), `src/game/player/PlayerDraw.cpp`, `PlayerAtlasDraw.cpp`,
`PlayerPoseDraw.cpp` (son de F0.3, que en paralelo QUITA de ahi los `Play` del impacto y emite `Land`), `CharacterVisual.*`,
`src/game/moves/*`, `src/game/visual/*`, `src/rendering/*`, assets, data.

**Tareas.**
1. **`HitResolver`** (unico punto de resolucion para Stage1-5 y VS):
   - `HitReport ResolvePlayerVsEnemies(Player&, std::vector<StreetEnemy>&, const StageHitStyle&, CombatFeedback&)`,
     `void ResolveEnemiesVsPlayer(std::vector<StreetEnemy>&, Player&, CombatFeedback&, int maxAttackersPerFrame = 2)`,
     `bool ResolvePlayerVsBoss(Player&, BossProxy&, const StageHitStyle&, CombatFeedback&)` (`BossProxy` = pos, ancho, alto,
     `int* hp`, `bool blocking`, `float* invuln`, callback `onDefeat`; cada stage adapta su struct de jefe),
     `void ResolvePlayerVsPlayer(Player& a, Player& b, CombatFeedback&)` (rival de VS), y proyectiles:
     `Player::ConsumeProjectileEvent()` devuelve true UNA sola vez por activacion (arregla B01: hoy ~21 proyectiles).
     `CombatFeedback` = hitstop (HitstopClock), shake, timeScale, chispas en el punto de contacto.
   - Reglas: lista `hitTargets` por activacion (multi-objetivo: B08) con re-golpe solo cada `multiHitInterval` (campo
     nuevo de AttackDef, 0 = un golpe); prioridad y trade (B16: si ambos conectan en el mismo frame gana `priority` mayor;
     empate = trade con hitstop compartido); counter-hit si el objetivo esta en startup (enemigo: `telegraphing` o antes del
     34 % de su ataque): dano x1.25, hitstun +0.12 s, hitstop x1.5, evento `CounterHit`; llama SIEMPRE a
     `OnAttackConnected` (abre combo, suma `rageGain`, `comboCount`: B03); aplica `def.hitstun`, `def.launch`, knockback con
     direccion; ajustes por stage (+5/+6/+8/+9 de rabia, color de chispa, puntuacion) via `StageHitStyle`.
   - Sustituir las copias de cada stage y de VS por llamadas al resolvedor (conservar puntuacion/score y contadores de cada
     stage via `HitReport`). Borrar o marcar como obsoletas las `Resolve*` muertas de CombatWorld (sin dejar dos caminos).
2. **Colision 3D (B05, B06, B23)** en `CombatMath.h` (header puro, sin raylib salvo tipos): capsula vs rectangulo en el plano
   (X, altura), test de carril `abs(dLane) <= def.depth + 18`, arco de miembro (V5 del vocabulario) con barrido
   `[tipPrev, tip]`. Hurtbox enemiga `X [ex-w/2, ex+w/2]`, altura `[z, z + bodyHeight]` (ahora con z), carril +-18.
   Hurtbox del jugador derivada del alto real de su sprite (no 48x105 fijos; en Knockdown 76 x 36). `bodyScale` =
   (alto del sprite / 112) x `DepthScaleFor(y)` para cajas y arcos (Titan-X x1.5, Titan-X Mejorado x2.1). Los 9 ataques
   historicos reciben un `LimbArc` equivalente a su caja actual (tabla abajo) y el resolvedor usa el arco. KF/BETA con caja
   por cuadro: conservar la caja del cuadro pero limitar `back` a 30 px TAMBIEN en Skill (hoy 400 px) y aplicar el filtro de
   carril. Punto de contacto = punto del segmento mas cercano al centro de la hurtbox (para chispas y numeros).
   Arcos historicos (pivote px,ph; L0->L1; theta0->theta1; r): Punch1 (8,-84; 36->74; -8->0; 14), Punch2 (8,-86; 40->82;
   -4->2; 15), Punch3 (6,-92; 50->92; 20->-15; 22), Kick (0,-58; 50->110; -20->-5; 18), DashAttack (6,-72; 50->100; -5->0; 22),
   RageAttack (0,-78; 60->130; -40->40; 34, bothSides), Finisher (0,-80; 70->140; -30->30; 40), Skill: caja del clip con el
   limite de arriba. Ajusta los numeros si un test de alcance lo pide y reportalo.
3. **Recepcion de golpes con `HitInfo` (B10, B17)**: `Player::ApplyHit(const HitInfo&) -> HitResult` y
   `StreetEnemy::ApplyHit(const HitInfo&) -> HitResult`; los `TakeDamage(int)` y `TakeDamage(int, Vector3D)` actuales quedan
   como envoltorios con el comportamiento de hoy (los tests los usan). En el jugador: retroceso en `dirX` del golpe (no de su
   facing), hitstun = `max(0.13, info.hitstun)`, knockdown por `info.knockdown` (el envoltorio conserva `damage >= 28`).
   Superarmadura de habilidades intacta (regla del usuario). Enemigos: `knockdown` por perfil (Mutant y Brute pesado), no por
   dano.
4. **Bugs de estado/tiempo**: B04 (separar `chainAttack` de `currentAttack`; no tocar `currentAttack` si `state == Attack`),
   B09 (integrar `velocity.x` en todos los estados de suelo con `v *= exp(-12 dt)` y limpiar al cambiar de estado), B11
   (todas las amortiguaciones por dt: `v *= pow(k, dt*60)`), B12 (el J/K pulsado durante P3/Kick no se pierde: queda
   en buffer hasta el fin del recovery), B13 (cancel en hit desde `startup + 1` frame activo hasta fin de recovery; en
   whiff como hoy), B15 (el buffer no envejece durante hitstop: `Tick(0)` mientras el reloj de hitstop este activo), B19
   (armadura de especiales solo en startup+activo, no en recovery; las HABILIDADES `activeSkill>=0` siguen con armadura
   total por regla del usuario).
5. **Sensacion**: hitstop/shake/temblor/camara/kill-slowmo con las formulas V7; un solo `HitstopClock` (o equivalente) para
   Stage1-5 y VS (borra los floats sueltos); chispa en el punto de contacto con FX distinto para golpe/bloqueo/contra.
6. **Eventos de juego**: reemplazar TODA llamada `AudioSystem::Get().Play(...)` de combate de tus archivos (PlayerCombat.cpp,
   Player.cpp, StreetEnemy.cpp, CombatWorld.cpp; los de PlayerDraw los quita F0.3) por `EmitGameEvent`
   con `type`, `x`, `skin`, `kind` (`HitKind` segun el ataque: puno ligero/medio/pesado, patada, energia...), `heavy`,
   `fromEnemy`. Un dueno por evento: impacto del jugador = HitResolver (`HitLanded/HitBlocked/CounterHit`), dano al
   jugador = `Player::ApplyHit` (`PlayerHurt` en TODA rama, incluida la de dano < 28 que hoy es muda; `PlayerBlocked`,
   `PlayerGuardBreak`, `PlayerKnockdown`), `EnemyKilled`, `BossHit`, `ProjectileSpawn/Hit`, `Swing` al iniciar ataque,
   `SpecialStart`, `RageStart`, `TransformStart`, `Dash`, `Land`/`KnockdownLand`, `ComboMilestone` (10/20/30),
   `ModeEnter` (cada stage, VS, menu, BETA no), `ArenaLock/Unlock`, `WaveStart`, `StageClear`, `GameOver`, `Victory`, UI
   (pausa de Stage1, menu principal, seleccion de VS).
   `AudioDirectorUpdate(dt)` (llamado desde `AudioSystem::UpdateAmbient`) drena la cola y mapea al `Sfx` LEGADO 1:1 para
   que suene lo mismo que hoy MAS lo que faltaba (`PlayerHurt` ligero -> `Hit`, `EnemyKilled` -> `EnemyDeath`, `BossHit` ->
   `HeavyHit`, `ProjectileSpawn` -> `EnergyShot`; MAX/RONIN conservan sus Sfx propios como hoy) y reenvia `ModeEnter`,
   `ArenaLock/Unlock`, `BossPhase`, `Victory`, `GameOver`, `StageClear` a `MusicPlayer::Get().OnEvent(e)`. F1.10 reescribira
   este director: deja el mapeo en UNA funcion clara.
7. **Tests nuevos** (`tests/unit/HitResolverTests.cpp`, `tests/unit/CombatMathTests.cpp`), sin ventana (usa datos, no texturas;
   si algo necesita ventana, marcalo y llevalo a `all_modes_check` en vez de a ctest):
   - Onda: exactamente 1 proyectil por lanzamiento (y por habilidad 1).
   - FURIA con 3 enemigos dentro del arco: los 3 golpeados una vez.
   - JAB: conecta con abs(dy) = 20, no con 70; no conecta por la espalda; con `behind` si.
   - Golpe por la espalda al jugador: se desplaza en la direccion del golpe (dx de signo correcto).
   - 5 bloqueos seguidos y 30 frames despues: `abs(velocity.x) < 5`.
   - Deslizamiento del enemigo tras Kick a dt=1/30 vs 1/60: diferencia < 10 %.
   - J J J con enemigo delante: la ventana de combo se abre (`comboCount >= 3`) y Finisher sale con rabia + combo 4 + SP 35.
   - Pulsar J durante hitstop de 11 frames: el J no caduca.
   - CombatMath: capsula-rectangulo (dentro, cruce de lado, esquina por radio, fuera), simetria izq/der del arco, escala.

**Criterios de aceptacion medibles.** Los tests de arriba en verde; `grep -rn "GetAttackHitbox().Intersects\|hit.Intersects(" src/game`
solo aparece dentro de `HitResolver.cpp` (o ninguna); `stage1_playthrough_test` en verde y reporta el tiempo (hoy 342.7 s:
debe seguir < 600 s; si sube mucho por B01, compensa en datos de la onda y reportalo); `all_modes_check`: los rivales
{7,8,18,19,6,21,2} siguen golpeando y recibiendo dano; `gow_check` sin cambios (no tocaste GowApk). En tu informe: tabla
"bug -> como se arreglo -> test que lo cubre" para B01-B19, B21, B23.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F0.3 — Motor visual procedural (PoseFx, poses sueltas, manifiesto extensible, feel ligero KF/BETA)

**Parte de:** la rama base con F0.1 integrado. Corre en paralelo con F0.2 y F0.4 (archivos disjuntos).

**Objetivo.** Que cualquier movimiento (los 9 de hoy y los ~31 nuevos que agrega F0.4) se pueda "dibujar con matematica"
sobre arte existente, declarando datos en el `MoveSet` del personaje (`src/game/moves/MoveTypes.h`, contrato de F0.1),
sin que los bloques de personaje de Fase 1 tengan que tocar codigo de dibujo. Arreglar de raiz los problemas de dibujo
transversales: poses sueltas congeladas y que cambian de tamano (`DrawBossPose` normaliza cada PNG a la misma altura),
estelas que guardan un indice del atlas de Rayden, orientacion unica por personaje, KF/BETA sin ningun efecto.

**PUEDES TOCAR:** `src/game/player/PlayerDraw.cpp`, `PlayerPoseDraw.cpp`, `PlayerAtlasDraw.cpp`, `PlayerVisualState.h`,
NUEVO `src/game/player/PoseFxPresets.h`, NUEVO `src/game/player/PoseFxEval.cpp/.h`; `src/rendering/BossSprite.h` (solo
AGREGAR funciones; `DrawBossPose`/`DrawSpriteUniform` quedan identicas porque las usan los jefes);
`src/rendering/Animator.h/.cpp`, `src/rendering/SpriteManifest.h/.cpp`, `src/rendering/AssetManager.cpp/.h` (solo lo
necesario para el manifiesto extensible y recortes); `src/game/CharacterVisual.h/.cpp` (campos visuales nuevos con
defaults neutros, despacho de poses); `src/game/visual/PoseTables.h` y la infraestructura comun de poses (NO el contenido de
`Poses_TitanX.cpp`/`Poses_CloneGrinder.cpp` mas alla de adaptarlos a la nueva firma conservando las poses de hoy);
`src/game/moves/MoveTypes.h` (solo AGREGAR campos visuales al final de structs, con default neutro); `data/sprite_manifest.json`
(solo la clave `"includes"`); NUEVOS `data/manifest/rayden_moves.json` y `data/manifest/brakk_moves.json` (juegos vacios);
`tests/tools/StateStrips.cpp`; NUEVOS `tests/unit/PoseFxTests.cpp`, `tests/unit/ManifestIncludeTests.cpp`;
`tests/SpriteManifestTests.cpp` (solo agregar aserciones).
**NO TOQUES:** `Player.h`, `Player.cpp`, `PlayerCombat.cpp`, combate, stages, audio (salvo emitir eventos, abajo), assets
de imagen, los juegos de clips existentes del manifiesto.

**Tareas.**
1. **Evaluador PoseFx** (`PoseFxEval`): dada la fase actual (S/A/R y `v` normalizada dentro de la fase; para estados no
   ataque, tiempo del estado) devuelve `{sx, sy, angle, dx, dy, alpha}` sumando: presets (`PresetUse`, ver V6), claves
   (`FxKey`, interpoladas con la curva del movimiento) y extras (hop `RootCurve.dz sin(pi u)` como desplazamiento visual
   si el motor no lo aplica como `z`). Implementar los presets de V6 en `PoseFxPresets.h` como funciones puras testeables.
   Rotacion SIEMPRE alrededor de los pies (origen = pivote del sprite), no del centro.
2. **Personajes de atlas no-KF** (Rayden 0, Brakk 2, MAX 6): en `DrawAtlasBody`, si el `MoveSet` del personaje tiene un
   `MoveOverride` con `visual.fx` para el ataque en curso, usar el evaluador; si no, el comportamiento ACTUAL exacto (el
   switch de hoy). Rayden 0 en sus 9 ataques historicos y estados historicos: **pixel identico** a la base (no tienen
   override). Soportar `MoveVisual.clips[0..2]` (secuencia de hasta 3 clips con subrango `from/to`, `speed`, `reverse`)
   repartidos en la duracion del ataque, `clipFallback`, y estados no-ataque con `MoveSet.hit/knockdown/getup/victory/jumpRise/jumpFall/walk/block`
   cuando esten definidos.
3. **Poses sueltas** (ids 1, 3, 4, 5) en `DrawPoseBody`:
   - Nueva `DrawBossPoseEx(tex, src, feet, scale, flipX, angleDeg, sx, sy, tint)` en BossSprite.h (rotacion con origen en
     los pies, `source rect` para recortar restos, escala uniforme). Escala POR PERSONAJE = `targetHeight / altoDe(idle1)`
     (Grinder: la uniforme de hoy), la misma para todas las poses -> se acaba el "muerte x2.25".
   - Anclaje: cada `PosePiece` tiene `offset` (px de textura) y `facesRight` propio: `flip = pieza.facesRight ? facing==Left : facing==Right`.
     Esto permite que F1.1 arregle Titan-X sin tocar la ficha.
   - Secuencias por movimiento/estado: `PoseSegment` por fase; composicion de 2 piezas (p.ej. las dos mitades del puno de
     Titan-X, dibujadas a la vez con desfase).
   - Interfaz para las tablas de Fase 1 en `PoseTables.h`: `bool ResolvePoseFrame(int skin, const PoseContext& ctx, PoseFrame& out)`
     donde `PoseContext` = estado, ataque (int), fase S/A/R + v, tiempo del estado, `walkPhase`, rabia, transformado, z, y
     `PoseFrame` = hasta 2 `PosePiece` + `PoseFx`. `Poses_TitanX.cpp`/`Poses_CloneGrinder.cpp` se adaptan devolviendo
     EXACTAMENTE las poses de hoy (los cambios de contenido son de F1.1/F1.2).
   - Aplicar a las poses el mismo feel que la rama atlas (respiracion, rebote al caminar, inclinacion de dash, impacto,
     retroceso al recibir golpe, aterrizaje) a traves del evaluador, controlado por `MoveSet.feel` (por defecto `Full`).
4. **Estelas con textura**: `PlayerVisualState` guarda por estela `{Texture2D tex; Rectangle src; Vector2 pos; float scale, angle, sx, sy; bool flip; Color tint; float life;}`
   (hoy guardan un indice del atlas: en poses sueltas dibuja el atlas de Rayden invisible). `ESTELA(n, intervalo, color)` de
   cada movimiento + la del dash de hoy.
5. **FX superpuestos** (`OverlayFx`): cuadros de `assets/fx/efectos_azules.png` (rejilla 8x5) anclados a los pies con escala y
   rotacion, en la fase indicada.
6. **Feel ligero para KF/BETA** (`FeelLevel::Light`): hoy `IsKfCharacter()` dibuja `animator.Draw` sin nada. Con `Light`:
   inclinacion -7 grados + temblor al recibir golpe, `CAIDA_ROT(85)` en derribo/derrota si el clip tiene <= 2 cuadros,
   arco de salto con sombra, rebote de victoria. Por defecto KF = `None` (pixel identico) y BETA/RONIN = `None`; F1.6-F1.8
   lo encienden por personaje.
7. **Transformacion con fallback**: si el `altAnimator` (forma transformada) no tiene el clip pedido, dibujar ese clip con el
   `animator` base y tinte blanco-azulado (MAX transformado usara celdas que solo existen en `max`).
8. **Manifiesto extensible**: `data/sprite_manifest.json` acepta `"includes": ["data/manifest/rayden_moves.json", "data/manifest/brakk_moves.json"]`;
   cada include tiene `{"clips": {"<juego>": {"<clip>": {...}}}}`. Un include puede AGREGAR clips a un juego; si el clip ya
   existe se ignora con aviso salvo `"override": true`, y `override` esta PROHIBIDO en el juego `rayden` (error y se
   ignora). Campos opcionales nuevos por clip: `"scale"` (float) o `"scales"` (uno por cuadro), `"offsets"` ([[dx,dy],...]),
   que el Animator aplica al dibujar (para igualar escalas de MAX). Indices fuera de la rejilla: el clip se descarta con aviso
   (NO romper la carga del personaje). Rutas Android: resolver como el manifiesto principal (`platform::LoadTextFile`).
9. **Sombra y salto**: sombra elipse con radio `x 1/(1 + z/150)` y alfa decreciente con `z` (el estado `Jump` lo crea F0.4:
   tu solo usas `position.z`).
10. **Audio en dibujo**: QUITAR las `AudioSystem::Get().Play` de `UpdateMotionFeel` (impacto en el flanco de `hasHit` y
    aterrizaje): el impacto lo emite el resolvedor de F0.2; aqui solo emites `EmitGameEvent({GameEventType::Land,...})` o
    `KnockdownLand` (si venia de derribo) en el aterrizaje.
11. **StateStrips**: anadir modo `--fixed-time` (reloj fijo) para comparaciones pixel a pixel.
12. **Tests**: `PoseFxTests` (cada preset en v=0 y v=1 da neutro donde corresponde; IMPACTO simetrico en dir +1/-1; GIRO
    cambia de signo n*2 veces; CAIDA_ROT llega a -a), `ManifestIncludeTests` (include agrega; no pisa; override prohibido en
    `rayden`; indice fuera de rejilla se descarta sin romper; los 19 clips de `rayden` identicos).

**Criterios de aceptacion medibles.**
- `state_strips --fixed-time` de Rayden (0) y de los KF/BETA: identico a la base (0 pixeles distintos).
- Poses sueltas (1, 3, 4, 5): ninguna pose supera 1.15x la altura del reposo en `state_strips` (mide con un script: alto
  del bounding box opaco); estelas visibles en dash con la textura del personaje (no el atlas de Rayden).
- Un `MoveOverride` de prueba (en el test, no en los archivos de personaje) con `GIRO(1)+ESTELA(3)` se ve en la herramienta.
- Rendimiento: `all_modes_check` no sube su tiempo total mas de 10 %.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F0.4 — Framework de movimientos: 31 ataques nuevos, cadenas por personaje, salto, victoria, input direccional

**Parte de:** la rama base con F0.1 y F0.2 integrados (F0.3 puede estar aun en curso: no toques sus archivos).

**Objetivo.** Convertir el moveset de 9 ataques en un sistema de datos con ~40 movimientos genericos que TODOS los
personajes reciben (con su visual por defecto), cadenas configurables de hasta 6 golpes, ramas direccionales, salto y
aereos con lanzador y persecucion, poderes "Signature" definidos por personaje, y estado de victoria. Los bloques de Fase 1
solo escribiran datos en su `Moves_<Bloque>.cpp`.

**PUEDES TOCAR:** `src/game/combat/AttackData.h/.cpp`, `src/game/combat/AttackAnimation.h` (conservar firmas y valores para
skins 2 y 6 en los 8 ids historicos), `src/game/player/PlayerCommon.h`, `src/game/player/PlayerCombat.cpp`, `src/game/Player.h`,
`src/game/Player.cpp`, `src/game/InputBuffer.h`, `src/core/InputMap.h/.cpp`, `src/ui/TouchControls.cpp/.h`,
`src/game/moves/MoveTypes.h` (solo agregar al final), `src/game/moves/MoveRegistry.*`, los 8 `src/game/moves/Moves_*.cpp` (SOLO
para dejar en cada uno un ejemplo comentado; sus defaults siguen devolviendo `nullptr`), `src/game/combat/HitResolver.cpp`
(solo lo necesario para aire/OTG/proyectiles de Signature), `src/game/VSMode.cpp` y `src/game/Stage1StoryGame.cpp`,
`src/game/stage1/Stage1Waves.cpp`, `Stage2Game.cpp`..`Stage5Game.cpp` (SOLO para entrar en `Victory` al ganar),
`tests/InputBufferTests.cpp`, `tests/CombatSmokeTests.cpp` (si un cambio lo exige, documentado), NUEVOS
`tests/unit/MovesetTests.cpp` y `tests/tools/MovesSheet.cpp`.
**NO TOQUES:** dibujo (`PlayerDraw*.cpp`, `PlayerPoseDraw.cpp`, `PlayerAtlasDraw.cpp`, `src/rendering/*`, `CharacterVisual.*`,
`src/game/visual/*`), `StreetEnemy.*` (el juggle es de F1.9), audio, GowApk, assets, data.

**Tareas.**
1. **Ids y datos.** Agregar los 31 `AttackId` de V1 en ese orden exacto. Ampliar `AttackDef` (al final, con defaults que
   dejen los 9 historicos identicos): `LimbArc arc; RootCurve root; int hitCount; float multiHitInterval; float cancelFrom; unsigned cancelOnHit, cancelOnWhiff; float blockstun, pushback, selfPushOnBlock; int juggleCost; bool otg, launcher, jumpCancel, groundBounce, wallBounce, knockdown, low, overhead, hitsBehind, air; float armorFrom, armorTo; int armorHits; float hitstopScale, shakeScale; bool requiresOverride;`.
   Filas genericas (S/A/R en frames, dano, Kb, hitstun s, Lch; arco = pivote px,ph; L0->L1; theta0->theta1; r):
   | Id | S/A/R | Dano | Kb | Hs | Lch | Arco / propiedades |
   |---|---|---|---|---|---|---|
   | PunchHook | 7/5/11 | 13 | 300 | .34 | 0 | (4,-88; 60->70; -55->20; 18) barrido horizontal |
   | Punch3 (REMATE, tiempos de hoy) | 7/9/10 | 18 | 520 | .45 | 260 | (6,-92; 50->92; 20->-15; 22) knockdown, breaksGuard, hitstopScale 1.3 |
   | Kick2 | 8/6/12 | 16 | 400 | .36 | 0 | (0,-62; 100->100; -70->15; 20) |
   | Kick3 | 11/6/18 | 22 | 600 | .40 | 300 | (0,-78; 105; 60->-10; 22) knockdown, wallBounce |
   | KickLauncher | 8/5/16 | 14 | 80 | .55 | 720 | (0,-70; 95; -40->-110; 18) launcher, jumpCancel |
   | FwdPunch | 12/5/14 | 16 | 420 | .36 | 0 | (8,-86; 60->96; 0; 18) root +70 outCubic, overhead |
   | FwdKick | 10/6/14 | 15 | 430 | .34 | 0 | (0,-66; 70->110; -15->-5; 18) root +60 |
   | BackElbow | 6/4/12 | 12 | 380 | .34 | 0 | (-6,-84; 30->62; 180; 16) behind, sin girar |
   | SpinKick | 9/8/16 | 16 | 420 | .34 | 0 | (0,-64; 90; -10->10; 20) bothSides, multi-objetivo |
   | Sweep | 7/6/16 | 12 | 120 | .50 | 0 | (0,-10; 40->110; -2; 14) low, knockdown -> Down |
   | Uppercut | 6/6/18 | 14 | 60 | .60 | 680 | (6,-70; 50->95; -30->-95; 18) launcher, jumpCancel, armadura en S, hop 10 |
   | Stomp | 10/4/14 | 10 | 0 | .30 | 0 | caja (24,-6) 60x30, otg |
   | DashKick | 4/10/14 | 13 | 300 | .40 | 0 | (0,-20; 60->100; -5; 16) root +120 outQuad, low, knockdown |
   | AirPunch | 4/6/8 | 9 | 160 | .30 | 220 | (10,-80; 70; -20->10; 14) air |
   | AirPunch2 | 4/6/8 | 10 | 180 | .30 | 220 | (10,-80; 78; -20->10; 15) air |
   | AirKick | 5/12/6 | 14 | 380 | .34 | 0 | (0,-50; 95; -25; 18) air, knockdown si el objetivo esta en suelo |
   | DiveKick | 6/hasta suelo/10 | 14 | 300 | .34 | -500 | pie, trayectoria 45 grados a 700 px/s, groundBounce |
   | AirSpike | 8/5/14 | 18 | 200 | .40 | -700 | (8,-70; 80; 60->-60; 20) groundBounce |
   | RisingUppercut | 3/12/22 | 16 x2 (cada 6 f) | 200 | .50 | 760 | (6,-80; 60->100; -60->-100; 20) invuln f1-5, root dz 90 |
   | Hurricane | 8/30/16 | 6 x5 (cada 6 f) | 260 | .30 | 0 | bothSides, root +160 inOutSine, SP 15 |
   | DrillKick | 10/14/18 | 8 x3 | 300 | .30 | 0 | (0,-60; 100; -10; 18) root +140 inOutSine, ultimo golpe knockdown, SP 15 |
   | Signature1-3, Chain5, Chain6 | - | - | - | - | - | `requiresOverride=true` (sin override = deshabilitado) |
   | Grab..WakeupAttack | - | - | - | - | - | filas con datos de combate.md 3.2/3.10 pero deshabilitadas (`requiresOverride`) hasta F2.1 |
   Actualizar juntos `kAttacks`, `ClipNameFor` (nombres canonicos: `punch_hook, kick2, kick3, kick_launcher, fwd_punch, fwd_kick, back_elbow, spin_kick, sweep, uppercut, stomp, dash_kick, air_punch, air_punch2, air_kick, dive_kick, air_spike, rising_uppercut, hurricane, drill_kick, signature1..3, grab, grab_knee, throw_forward, throw_back, guard_counter, wakeup_attack, chain5, chain6`),
   `ClipFor`, `PhaseFor` (patadas -> `Kick`, punos -> `Punch3`/`Special` segun corresponda, documentar), `LegacyType`
   (patadas -> Kick, punos -> Punch, proyectiles -> Energy; los stages usan `attackType`).
2. **B18: nunca `idle` como ataque.** Resolucion del clip de un ataque: override del personaje (`MoveVisual.clips`) ->
   clip canonico si existe -> `clipFallback` -> clip historico de la familia (`punch1`/`punch2`/`punch3`/`kick`/`energy`) ->
   NUNCA `idle`. En KF, los clips rellenados con `idle` por KfReference cuentan como inexistentes (F1.6 los marcara; tu
   expon `bool CharacterHasRealClip(name)` usando la info disponible).
3. **MoveSet en el jugador.** `const MoveSet& ms = GetMoveSet(skin)` al aplicar personaje. `CurrentAttackDef()` devuelve la
   definicion EFECTIVA (fila generica o `replaceDef`, con `damageScale/timeScale/reachScale`, arco, root, hitCount,
   armadura, area, proyectil del override). Capacidades (`ms.caps`) filtran entradas (lo no soportado cae al base).
   `skillSlots[0..4]` reemplaza `kOurSkills` para no-KF (default = EnergyWave, Uppercut, RageAttack, DashAttack, Finisher hasta
   que cada bloque los cambie). `ProjectileSpec` del override: el HitResolver/stage lo dispara una vez por activacion
   (count/interval/spread/arc).
4. **Cancelaciones en datos.** `CancelRule { from, cmd, dir, onHit, onWhiff, to }` en tabla; la cadena J usa `ms.chainJ`, la K
   `ms.chainK` (siguiente eslabon), `J J K` -> KickLauncher desde el 2o eslabon J, especiales cancelan normales (en hit desde
   `startup+1`, en whiff desde recovery), Signature/especial cancelan en Super (Rage/Finisher) si hay furia. Conservar
   `ResolveCancel(AttackId, InputCommand)` como envoltorio (lo usa el test). Ultimo eslabon de la cadena + J = nada (B12: el
   input no se pierde ni dispara un golpe 1 fantasma).
5. **Input direccional relativo (B14, B21).** `InputBuffer::Entry` guarda `relX` (-1/0/+1 relativo al facing al pulsar) y
   `down` (S sin horizontal). Resolver segun V2 (comando > direccional > cadena). `MotionHistory`: agregar
   `QuarterCircleBack` (QCB); QCF/QCB/DP exigen que el "abajo" puro dure >= 2 frames (caminar en diagonal + J no saca onda).
   Mientras se esta en Attack/Recovery el `v` no mueve de carril. `<`J ejecuta BackElbow sin girar; `<`J J: el 2o gira.
6. **Salto y aire** (V9): `PlayerState::Jump` propio (no reutilizar `Airborne`, B22), `PlayerInput.jump`, tecla `KEY_I`,
   `kVirtualJump = 409` en InputMap y boton tactil "SALTO" en `TouchControls` (combate) en (1160, 512) radio 30 (no solapa
   GOLPE 1170,615 r62 ni PATADA 1057,574 r40: verificalo con un assert en el test de TouchControls). Ataques aereos
   (`air=true`) solo en Jump; al tocar suelo durante uno -> recovery de aterrizaje 4 f. `CapHeavyJump` usa `ms.jumpVz`
   (Titan-X 560). Jump cancel tras KickLauncher/Uppercut/RisingUppercut que conecta: ventana 10 f, salto de persecucion.
   Hurtbox en aire sube con z (ya 3D desde F0.2). `CanAct`, `IsKnockedDown`, `SetState`, `UpdateTransformForm` cubren Jump.
7. **Victoria**: `PlayerState::Victory` (no actua, no recibe golpes, 1.6 s y luego Idle); entrar al ganar ronda en VS y al
   terminar oleada final/jefe en cada stage (una linea por sitio). Emitir `GameEventType::Victory`.
8. **Fluidez de movimiento** (V2/combate 3.5): aceleracion/frenado al caminar `v += (vObj - v)(1 - exp(-14 dt))`; carrera con
   doble toque adelante en 0.25 s (x1.45 velocidad, clip `run` si existe, `CapRun`); back-dash (`<`+Shift, `CapBackDash`);
   `turnTimer` al girar (clip `turn` si existe). Emitir `Footstep` en cada cruce de `walkPhase` por 0 y pi (con `weight` =
   1 normal, 3 pesado si `bodyScale >= 1.4`), `Jump`, `Swing` con `weight` 1/2/3 segun dano.
9. **Finisher alcanzable por los dos caminos** (cola de buffer y cancel), K en furia con combo >= 4 y SP >= 35.
10. **Herramienta `moves_sheet`** (`tests/tools/MovesSheet.cpp`): para cada personaje (o el id dado) y cada movimiento
    habilitado, ejecuta la entrada con `scriptedInput` y un enemigo de prueba, captura 6 cuadros (S, A inicio, A medio,
    A fin, R medio, fin) con la caja de golpe y la hurtbox dibujadas, y escribe `<dir>/moves_<id>.png` + `<dir>/moves.tsv`
    (`id, nombre, movimiento, clip usado, origen = override|canonico|fallback|historico|IDLE, golpeo s/n`). Imprime
    `idle-como-ataque: N` (debe ser 0) y el conteo de movimientos por personaje.
11. **Tests** (`tests/unit/MovesetTests.cpp`, sin ventana si es posible): rutas J J J J -> P1, P2, PunchHook, Punch3; J J K ->
    KickLauncher; K K K -> Kick, Kick2, Kick3; `v`K Sweep; `<`J BackElbow con caja detras; `>`J FwdPunch avanza ~70 px; DP+K ->
    RisingUppercut (actualiza `TestSpecialCommands` de InputBufferTests: hoy espera Punch3; documentalo); QCB+K Hurricane;
    QCF+K DrillKick; QCF+J -> 1 proyectil; caminar abajo-adelante + J -> NO onda; salto + J J K y aterrizaje 4 f; jump cancel
    tras KickLauncher que conecta; chain de 6 con un `MoveSet` de prueba; capacidad sin `CapJump` -> I no salta; ningun id
    1..40 mapea a `"idle"` en `ClipNameFor`. Mantener verdes `TestChainWithCancel` (P1->P2 tras Advance(14)), `TestRecoveryBeforeIdle`,
    `TestExpiredInputDoesNotFire`, `TestHitClearsBuffer`, CombatSmoke (P1 activo a 0.15 s, Punch3 de Rayden avanza > 1 px).

**Criterios de aceptacion medibles.** Tests en verde; `moves_sheet` corre para los 42 sin crash y reporta >= 28 movimientos
habilitados por defecto en los ids 0-6 (con visual de fallback) e `idle-como-ataque: 0`; `stage1_playthrough_test` en verde
(el bot usa J J J K: ahora el 3er J es PunchHook y K tras el 3er J es Kick; reporta el tiempo); `all_modes_check` y `gow_check`
`problemas: 0` (BETA usa el mismo Player: vigila la peor oleada). Informe: tabla de movimientos genericos con sus numeros
finales.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.1 — TITAN-X (skin 4) y TITAN-X MEJORADO (skin 5): orientacion, poses, ~30 movimientos (jugador y jefes)

**Parte de:** "base-fase1" (Fase 0 integrada). Es la queja principal del usuario: "titan x camina de espaldas, le faltan golpes".

**Diagnostico (verificado).** Arte en `assets/bosses/titanx/` (12 PNG) y `assets/bosses/titanx_mejorado/` (11 PNG); ficha con
`facesRightByDefault=false` pero casi todo el arte mira a la DERECHA -> camina/dashea/patea/dispara de espaldas. Titan-X:
`idle1-4`, `lean`, `uppercut`, `shoot_orb` (en realidad un PUNO RECTO) miran a la derecha; `recoil` mira a la IZQUIERDA y es
el DISPARO del orbe (nombres cruzados); `punch1` (130 px, piernas+torso sin cabeza+puno suelto a la izquierda) y `punch2`
(77 px, la tira con la cabeza) son las dos MITADES de una pose de carga de puno que mira a la derecha; `slam` frontal con un
trozo de puno ajeno en el borde izquierdo; `death` tumbado. Mejorado: `idle1,3,4`, `cannon_aim`, `charge_orb`, `rage_aura`
miran a la derecha; `idle2` casi de frente; `shoot_orb` a la IZQUIERDA; `dash`/`slam` de frente; `death` tumbado;
`charge_orb`/`cannon_aim` con el canon cortado a la derecha, `rage_aura` con un trozo ajeno abajo a la derecha. Hoy: golpe
recibido = pose de disparo/apuntar, nunca cae, golpes 1-3 y habilidades = la misma media pose, tamano distinto por pose
(derrota del Mejorado x2.25), orbe de carga en la espalda. Los JEFES usan los mismos PNG con el mismo supuesto
(`Boss.cpp` ~259, `Stage3Game::DrawBoss` ~567, `Stage4Game::DrawBoss` ~447). Imagenes: `.../audit/img/titanx_detail.png`,
`titanx_punch_joined.png`, `poses_titanx*.png`, `states/s4_small.png`, `states/s5_small.png`, `vs_titanx/titanx_2.png`.

**PUEDES TOCAR:** `src/game/visual/Poses_TitanX.cpp`, `src/game/moves/Moves_TitanX.cpp`, `src/game/combat/Boss.cpp` (SOLO el
dibujo/eleccion de pose y flip de titanx/titanx_mejorado), `src/game/Stage3Game.cpp` y `src/game/Stage4Game.cpp` (SOLO
`DrawBoss` y helpers de dibujo del jefe), NUEVO `tests/unit/PoseTitanXTests.cpp`, NUEVO `tools/poses/medir_titanx.py`.
**NO TOQUES:** los PNG (se recorta con `source rect` y se compone en runtime), `CharacterVisual.cpp` (la orientacion va por
pose en la tabla), motor, otros personajes.

**Tareas.**
1. **Tabla de poses con orientacion y ancla** (`PoseFacesRight` + `ResolvePoseFrame` en Poses_TitanX.cpp): `facesRight=true`
   para todas salvo Titan-X `recoil` y Mejorado `shoot_orb` (false). Mide con `tools/poses/medir_titanx.py` (PIL) por PNG:
   alto/ancho opaco, x del pie trasero y del centro del torso; usa esos numeros para `offset` (ancla en el centro del
   torso, no del PNG) y para los `source rect` que recortan: puno suelto de `punch1` (franja izquierda), trozo de puno de
   `slam` (izquierda), trozo de `rage_aura` (abajo-derecha), restos de borde. Composicion "CARGA" = `punch1` recortado +
   `punch2` a su derecha con el desfase medido (2 piezas).
2. **Caminar sin arte nuevo**: Titan-X ciclo `idle3 -> idle1 -> idle4 -> idle2` al ritmo de `walkPhase` (no del reloj), rebote
   pesado `dy = -5|sin(walkPhase)|`, inclinacion +3 grados hacia delante, polvo `FX` en cada pisada. Mejorado: `idle3 -> idle1 -> idle4 -> idle1`
   (sin `idle2`, que parpadea de orientacion), rebote `-6|sin|`, `LATIDO(0.01, 1.2)`.
3. **Estados**: golpe recibido = `idle4` (TX) / `idle1` (TXM) + `TEMBLOR(3,28)` + inclinacion -7 grados + retroceso; derribo =
   `lean`/`idle1` con `CAIDA_ROT(80)` -> `death` (escala UNIFORME: el motor ya la aplica); levantarse = `death` -> `slam`
   (agachado) -> `idle1`; bloqueo = `idle1` `sy 0.96` + escudo verde; victoria = TX `uppercut` + `LATIDO(0.04,3)` + aura verde,
   TXM `rage_aura` + `LATIDO`; dash = `lean`/`dash` + `ESTELA(4,.03,verde)`; orbe de carga en la MANO real (ancla medida).
4. **Movesets** (pesados: `bodyScale` lo da el motor ~1.5 y ~2.1; `caps`: TX con `CapHeavyJump` (jumpVz 560), sin Hurricane;
   TXM sin salto, sin Hurricane ni DrillKick). Numeros = dano base antes del multiplicador del personaje (x1.20 / x1.40):

   **TITAN-X ("prototipo mutante, lento y demoledor") — meta >= 26 movimientos.**
   | Entrada | Nombre | Visual (poses + presets V6) | S/A/R | Dano | Propiedades |
   |---|---|---|---|---|---|
   | J | Martillo | CARGA (S, `ANTICIPA(1.4)`) -> `shoot_orb` (A, `IMPACTO(1.3)`, `ESTOCADA(18)`) -> `idle1` | 9/5/12 | 12 | Kb 320 |
   | J J | Cruzado | `shoot_orb` +6 grados, dx +10, `ESTELA(2)` verde | 7/5/12 | 13 | |
   | J J J | Uppercut mutante | `uppercut` + `UPPER(14)` | 8/6/18 | 16 | Lch 640, jumpCancel |
   | J J J J | Golpe sismico | `slam` + `FX` onda en el suelo + `TEMBLOR` | 12/6/20 | 20 | area r140 delante, low, knockdown, hitstopScale 1.4 |
   | K | Pisoton | `slam` `sy .9` + onda r90 | 10/5/16 | 15 | low, knockdown (trip) |
   | K K | Rodillazo bajo | `lean` + `ESTOCADA(30)` + ang +10 | 8/5/14 | 13 | |
   | K K K | Embestida de hombro | `lean` + `ESTOCADA(90, outBack)` + `ESTELA(3)` | 10/8/18 | 18 | armadura 1 golpe en S+A, wallBounce |
   | J J K | Lanzador | `uppercut` `UPPER(10)` | 8/5/16 | 14 | Lch 720 |
   | `>`J | Ariete | `lean` + `ESTOCADA(120)` | 12/8/16 | 16 | armadura 1 golpe, Kb 560 |
   | `<`J | Codazo trasero | `shoot_orb` espejado (pieza con facesRight invertido) | 6/4/12 | 12 | behind |
   | `v`J | Gancho al cielo | `uppercut` + hop 12 | 6/6/18 | 14 | Lch 680 |
   | `v`K | Barrido de brazo | `slam` + `BARRIDA` | 8/6/16 | 12 | low |
   | `<`K | Giro de brazos | `shoot_orb` + `GIRO(1)` | 9/8/16 | 14 | bothSides |
   | `>`K | Patada pesada | `lean` ang -12 + `ESTOCADA(40)` | 10/6/16 | 15 | |
   | Dash J / Dash K | Carga verde / Deslizamiento | `lean` + `ESTELA(5)` + `ESTOCADA(140)` / `slam` `sy .8` + `ESTOCADA(100)` | gen. | 15 / 13 | |
   | Salto (pesado) | — | subida `lean` ang -10, caida `uppercut`; aterrizaje con polvo | | | |
   | aire J / aire K | Caida sismica / Patada aerea | `slam` cayendo + onda al aterrizar / `lean` ang +25 | gen. | 14 / 14 | groundBounce / |
   | L, QCF+J | Orbe toxico | `idle2` + `LATIDO(.05,4)` + `TINTE` verde (12 f) -> `recoil` (disparo) | 12/-/14 | 20 | proyectil verde desde la mano (`origin` medido), SP 20 |
   | DP+K | Uppercut volcanico | `uppercut` + pilar verde (`FX`) + `UPPER(24)` | 3/12/22 | 16+10 | Lch 800, invuln f1-5 |
   | QCF+K | Bola de demolicion | `lean` + `GIRO(2)` + `ESTOCADA(160)` | 10/14/18 | 7x3 | ultimo knockdown |
   | QCB+J (Signature1) | Aplastamiento doble | `slam` x2 (2a onda r180) | 10/20/20 | 14+18 | armadura, area |
   | `>`L (Signature2) | Lluvia acida | `recoil` + 3 orbes en abanico (carriles -18/0/+18) | 12/-/16 | 10x3 | SP 25 |
   | `v`L (Signature3) | Terremoto | `slam` x3 + `TEMBLOR` grande | 10/36/20 | 8x3 | area r220, ultimo knockdown, SP 30 |
   | Furia (Espacio) | Triple sismo | `slam` x3 con onda creciente | gen. | gen. | area |
   | Finisher | Ejecucion | `uppercut` + `recoil` a quemarropa + estallido | gen. | 44 | |
   Habilidades 1-5: Orbe toxico, Aplastamiento doble, Uppercut volcanico, Bola de demolicion, Terremoto.
   Rutas a validar (en el test con muneco): J J J J; J J K -> salto -> aire J; K K K; `v`K -> `v`J (pisoton/OTG si F1.9 ya
   esta, si no solo que no rompe); J J QCF+J; `>`J (armadura).

   **TITAN-X MEJORADO ("forma final con canon") — meta >= 22 movimientos.**
   | Entrada | Nombre | Visual | S/A/R | Dano | Propiedades |
   |---|---|---|---|---|---|
   | J | Culatazo | `cannon_aim` + `IMPACTO(1.2)` | 8/5/12 | 13 | |
   | J J | Rafaga corta | `charge_orb` + destello de boca (`FX`) x3 | 8/12/12 | 5x3 (cada 4 f) | |
   | J J J | Embestida | `rage_aura` + `ESTOCADA(70)` | 10/6/16 | 16 | Kb 520, wallBounce |
   | J J J J | Canonazo a quemarropa | `shoot_orb` (pre-espejada) + estallido grande | 12/6/20 | 22 | knockdown, Lch 300 |
   | K / K K | Onda de choque / Pisoton doble | `slam` + onda r160 / `slam` x2 | 10/5/16 / 10/14/18 | 15 / 9+12 | low, knockdown |
   | J J K | Lanzador | `rage_aura` ang -15 + hop 10 | 8/5/16 | 14 | Lch 700 |
   | `>`J | Carga de tanque | `dash` + `ESTOCADA(160)` + `ESTELA(4)` | 12/10/18 | 18 | armadura 2 golpes |
   | `<`J / `v`J / `v`K / `<`K | Codazo / Gancho / Barrido / Giro | `cannon_aim` espejado / `rage_aura` ang -20 hop 16 / `slam` `BARRIDA` / `charge_orb` `GIRO(1)` | gen. | 12/14/12/14 | behind / Lch 680 / low / bothSides |
   | L | Canonazo cargado | `charge_orb` (mantener L hasta 1.0 s: escala del orbe 1 -> 2.2, dano 18 -> 34) -> `shoot_orb` | 12/-/16 | 18-34 | SP 20 |
   | `>`L (Sig2) | Rafaga continua | `charge_orb` parpadeando + 6 proyectiles pequenos cada 0.08 s | 10/-/18 | 5x6 | SP 25 |
   | QCB+J (Sig1) | Mortero | `shoot_orb` ang -30 + proyectil en parabola que cae a 260 px | 14/-/18 | 20 | area r120 al caer |
   | `v`L (Sig3) | Aura mutante | `rage_aura` + `LATIDO` + anillo | 8/40/16 | 6x4 | area r130 |
   | DP+K | Uppercut de canon | `rage_aura` + pilar verde | 3/12/22 | 16+10 | Lch 780, invuln f1-5 |
   | Furia / Finisher | Bombardeo / Juicio final | `slam` + 5 orbes que llueven / `slam` + `shoot_orb` encadenados | gen. | gen. / 44 | |
   Habilidades 1-5: Canonazo cargado, Mortero, Rafaga continua, Uppercut de canon, Aura mutante.
   (Si "mantener L" no se puede expresar en el motor, hazlo con 3 niveles fijos por tiempo de pulsacion y pidelo al motor.)
5. **Jefes**: `Boss.cpp`, `Stage3Game::DrawBoss` y `Stage4Game::DrawBoss` usan `PoseFacesRight(folder, pose)` para el flip y
   `DrawBossPoseEx` con escala uniforme por personaje; pose de golpe recibido del jefe = `idle4`/`idle1` con tinte (no
   `recoil`/`cannon_aim`); el jefe mira al jugador.
6. **Test** `PoseTitanXTests`: toda pose referenciada en las tablas existe como archivo; `PoseFacesRight("titanx","recoil")==false`,
   `("titanx_mejorado","shoot_orb")==false`, el resto true; ninguna secuencia de ataque usa `recoil`/`cannon_aim` como
   golpe recibido; la caja de `Martillo` de TX llega a >= 1.4x el alcance del JAB de Rayden.

**Criterios de aceptacion.** `state_strips` 4 y 5: en "walk->" y "walk<-" la cara/pecho mira hacia la flecha (revisalo a ojo
y adjunta las imagenes); ninguna pose > 1.15x el alto del reposo (script); derrota del Mejorado dentro de pantalla;
`vs_boss_visual_check` (o captura equivalente) con TITAN-X como jugador frente al jefe: se miran de frente; `moves_sheet` 4 y
5 con >= 26 / >= 22 movimientos, todos `override`, golpeo = s.

**Como se trabaja un personaje (comun a F1.1-F1.8).**
- Tras Fase 0 el motor ya da a TODOS los personajes los ~40 movimientos genericos (V1/V2) con un visual de reserva. Tu
  trabajo: que cada movimiento de TU personaje tenga visual propio (arte existente + transformaciones matematicas), numeros
  ajustados a su peso, cadenas largas, 3 poderes `Signature`, habilidades 1-5 que no repitan normales, y estados (golpe,
  derribo, levantarse, victoria, salto, caminar, bloqueo) que se vean bien. Deshabilita (con motivo escrito) lo que no tenga
  sentido para el personaje.
- Todo se declara en DATOS en tu `src/game/moves/Moves_<Bloque>.cpp`: `const MoveSet* MovesetFor_<Bloque>(int skin)`
  devuelve un `MoveSet` estatico para tus skins (y `nullptr` para los demas). Campos (ver `src/game/moves/MoveTypes.h`):
  `caps`, `feel`, `chainJ/chainK` (hasta 6/4 eslabones; puedes usar `Chain5/Chain6` con override completo), `skillSlots[5]`,
  `jumpVz`, `overrides[]` (por movimiento: `displayName` en espanol, `replaceDef`+`def` o escalas, `arc` (LimbArc), `root`
  (RootCurve), `hitCount`/`multiHitInterval`, armadura, `areaRadius`, `pullPx`, `projectile`, `visual` = clips/segmentos de
  pose + `PoseFx` con presets V6 y claves), y visuales de estado `hit, knockdown, getup, victory, jumpRise, jumpFall, walk,
  block`. Define tambien el visual de `Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack` (se activan en
  Fase 2; deja `enabled` como venga).
- Cajas: el arco (`LimbArc`) debe cubrir el miembro DIBUJADO en el cuadro de impacto (tolerancia 12 px): compruebalo con
  `moves_sheet` (dibuja la capsula) o F9 en juego. Las escalas por tamano (`bodyScale`) y profundidad las aplica el motor.
- Herramientas: `xvfb-run -a ./build/moves_sheet /tmp/<tu_dir> <id>` (6 cuadros por movimiento + `moves.tsv` con el
  origen del clip y si golpeo al muneco) y `xvfb-run -a ./build/state_strips /tmp/<tu_dir> <id>` (estados). Mira las
  imagenes (Read) antes de dar algo por bueno.
- Si el motor no puede expresar algo que necesitas, NO lo cambies: describelo como `PEDIDO AL MOTOR` y usa la mejor
  aproximacion disponible.
- Aceptacion comun: `moves.tsv` sin `IDLE`; cada movimiento habilitado de tu personaje con origen `override` (salvo los que
  justifiques); cada ataque habilitado "golpeo = s" contra el muneco; habilidades 1-5 = movimientos distintos de la cadena;
  numero de movimientos habilitados >= la meta de tu brief; capturas `moves_<id>.png` y `states_<id>.png` en tu informe.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.2 — CLON ANTIGUO (skin 1) y GRINDER (skin 3): de poses congeladas a ~28 movimientos

**Parte de:** "base-fase1".

**Diagnostico.** CLON ANTIGUO: `assets/bosses/rayder_clone/` (11 PNG: `idle1-4, ready, punch, kick, dash, release_orb, hurt,
death`), todas miran a la derecha (correcto); caminar = ciclo idle a 9 fps (patina); golpes 1/2/3 iguales; nunca cae;
`death` 2.1x y `dash` 1.34x mas grandes (ya corregido por la escala uniforme de F0.3: compruebalo); restos de recorte:
`kick.png` pierna suelta abajo-izquierda, `hurt.png`/`ready.png` trozo de bota en el borde derecho, `idle4.png` pico oscuro
en la bota, `punch.png` puno cortado por el borde. GRINDER: `assets/bosses/grinder/` (7 PNG de pixel art 30-75 px, escala x3
uniforme: `idle, hurt, death, saw, slam, ram, overdrive`), 1 sola pose de reposo, golpes identicos, no cae. Imagenes:
`.../audit/img/poses_rayder_clone.png`, `poses_grinder.png`, `states/s1_small.png`, `s3_small.png`, `clone_walk_cmp.png`.

**PUEDES TOCAR:** `src/game/visual/Poses_CloneGrinder.cpp`, `src/game/moves/Moves_CloneGrinder.cpp`, NUEVO
`tests/unit/PoseCloneGrinderTests.cpp`, NUEVO `tools/poses/medir_clon_grinder.py`.
**NO TOQUES:** los PNG (recorta con `source rect`), `CharacterVisual.cpp`, motor, otros personajes.

**Tareas.**
1. Medir con PIL cada PNG (bbox opaco, islas pequenas separadas del cuerpo) y definir `source rect` que excluya los restos
   de recorte; anclas en el centro del torso.
2. **CLON ANTIGUO ("rival espejo de Rayden, energia morada") — meta >= 28 movimientos.** Caminar: `idle3`/`idle4` (piernas
   abiertas) <-> `idle1` al ritmo de `walkPhase` + rebote 3 px + vaiven. Estados: golpe = `hurt` + retroceso; derribo =
   `hurt` `CAIDA_ROT(80)` -> `death` (arrodillado = "en el suelo"); levantarse = `death` -> `ready`; bloqueo = `ready` `sy .97`;
   victoria = `ready` + aura morada `LATIDO`; salto: subida `dash` ang -10, caida `ready`.
   | Entrada | Nombre | Visual | S/A/R | Dano | Prop. |
   |---|---|---|---|---|---|
   | J / J J / J J J / J J J J | Jab / Cruzado / Gancho / Puno volador | `ready`->`punch` `IMPACTO` / `punch` dx+10 ang+4 `ESTELA(2)` morada / `punch` ang -12 `SALTITO(6)` / `dash` + `ESTOCADA(90)` + `ESTELA(4)` | gen. | gen. | el 4o knockdown |
   | K / K K / K K K | Patada / Patada giratoria / Patada alta | `kick` / `kick` `GIRO(1)` / `kick` ang -25 `SALTITO(20)` | gen. | gen. | K K K Lch 300, wallBounce |
   | J J K / `v`J | Lanzador / Gancho ascendente | `kick` ang -35 hop 12 / `punch` ang -20 hop 10 | gen. | gen. | Lch 720 / 680 |
   | `>`J / `>`K / `<`J / `<`K / `v`K | Embestida / Patada de avance / Codazo / Giratoria / Barrida | `dash` `ESTOCADA(70)` / `kick` `ESTOCADA(60)` / `punch` espejado / `kick` `GIRO(1)` / `kick` `BARRIDA` | gen. | gen. | |
   | aire J, J J, K, `v`K, `>`J | Aereos | `punch` ang +15 / `punch` ang +22 / `kick` ang +25 / `kick` ang +40 `ESTELA(4)` / `punch` ang 60 -> -60 | gen. | gen. | |
   | L, QCF+J | Orbe oscuro | `ready` -> `release_orb` + proyectil morado (`TINTE` del proyectil) | gen. | 20 | |
   | DP+K | Gancho de sombra | `punch` ang -25 + pilar morado | gen. | gen. | |
   | QCB+K / QCF+K | Ciclon morado / Taladro | `kick` `GIRO(3)` hop 30 / `dash` `GIRO(2)` `ESTOCADA(140)` | gen. | gen. | |
   | QCB+J (Sig1) | Sombra espejo | una "sombra" (estela intensa alfa 160) avanza 160 px y golpea con `dash`+`punch` | 10/12/16 | 14 | |
   | `>`L (Sig2) | Doble orbe | `release_orb` x2 (carriles -12/+12) | 12/-/18 | 12x2 | SP 25 |
   | `v`L (Sig3) | Explosion oscura | `release_orb` `LATIDO` + anillo r110 | 8/10/18 | 16 | Lch 400 |
   Habilidades 1-5: Orbe oscuro, Sombra espejo, Gancho de sombra, Ciclon morado, Explosion oscura.
3. **GRINDER ("maquina pesada con sierra") — meta >= 22 movimientos; sin salto.** Reposo: `idle` + `LATIDO(0.02, 1.2)` +
   `TEMBLOR(1, 30)` continuo (motor de la sierra). Caminar: `idle` + paso pesado `dy -3|sin|`, ang +-2, polvo. Estados:
   golpe = `hurt` + retroceso; derribo `hurt` `CAIDA_ROT(75)` -> `death`; levantarse `death` -> `slam` -> `idle`; bloqueo
   `idle` `sy .94` + escudo; victoria `overdrive` + `LATIDO`.
   | Entrada | Nombre | Visual | S/A/R | Dano | Prop. |
   |---|---|---|---|---|---|
   | J / J J / J J J / J J J J | Sierra / Sierra cruzada / Sierra larga / Sierra descendente | `saw` `IMPACTO` + chispas `FX` / `saw` ang +8 / `saw` `ESTOCADA(40)` / `saw` ang -30 -> +30 | 6/10/10, 7/12/10, 8/16/12, 10/6/18 | 5x2, 4x3, 4x4, 18 | multigolpe cada 5-6 f; el 4o overhead + knockdown |
   | K / K K | Golpe al suelo / Doble golpe | `slam` + onda r100 / `slam` x2 | gen. | 15 / 9+12 | low |
   | J J K / `v`J | Lanzador | `saw` ang -40 hop 6 | gen. | 14 | Lch 680 |
   | `>`J | Embestida | `ram` `ESTOCADA(140)` | 10/10/16 | 16 | armadura 2 golpes |
   | `<`J / `v`K / `<`K | Sierra trasera / Barrido / Giro | `saw` espejado / `slam` `BARRIDA` / `saw` `GIRO(1)` | gen. | gen. | |
   | L, QCF+J | Disco de sierra | `saw` + proyectil = `FX` de chispa girando (rot 720 grados/s) | 12/-/14 | 16 | |
   | QCB+J (Sig1) | Molinillo | `overdrive` + `GIRO(4)` | 8/36/18 | 4x6 | bothSides r90 |
   | `>`L (Sig2) | Embestida turbo | `ram` + `ESTELA(5)` roja + `ESTOCADA(220)` | 10/16/20 | 6x3 | armadura |
   | `v`L (Sig3) | Sobrecarga | `overdrive` aura que dana | 8/40/18 | 5x4 | area r120 |
   | DP+K | Sierra ascendente | `saw` ang -45 + chispas en pilar | gen. | gen. | Lch 760 |
   Habilidades 1-5: Disco de sierra, Molinillo, Embestida turbo, Sierra ascendente, Sobrecarga.
4. **Test** `PoseCloneGrinderTests`: poses referenciadas existen; ningun `source rect` sale de la textura; las 3 primeras
   secuencias de la cadena J del Clon producen poses o transformaciones distintas entre si.

**Criterios de aceptacion.** `state_strips` 1 y 3: caminar con alternancia de pose (Clon) / rebote (Grinder), cae al suelo y
se levanta, sin restos de recorte visibles; `moves_sheet` 1 y 3 >= 28 / >= 22 movimientos `override`, golpeo = s.

**Como se trabaja un personaje (comun a F1.1-F1.8).**
- Tras Fase 0 el motor ya da a TODOS los personajes los ~40 movimientos genericos (V1/V2) con un visual de reserva. Tu
  trabajo: que cada movimiento de TU personaje tenga visual propio (arte existente + transformaciones matematicas), numeros
  ajustados a su peso, cadenas largas, 3 poderes `Signature`, habilidades 1-5 que no repitan normales, y estados (golpe,
  derribo, levantarse, victoria, salto, caminar, bloqueo) que se vean bien. Deshabilita (con motivo escrito) lo que no tenga
  sentido para el personaje.
- Todo se declara en DATOS en tu `src/game/moves/Moves_<Bloque>.cpp`: `const MoveSet* MovesetFor_<Bloque>(int skin)`
  devuelve un `MoveSet` estatico para tus skins (y `nullptr` para los demas). Campos (ver `src/game/moves/MoveTypes.h`):
  `caps`, `feel`, `chainJ/chainK` (hasta 6/4 eslabones; puedes usar `Chain5/Chain6` con override completo), `skillSlots[5]`,
  `jumpVz`, `overrides[]` (por movimiento: `displayName` en espanol, `replaceDef`+`def` o escalas, `arc` (LimbArc), `root`
  (RootCurve), `hitCount`/`multiHitInterval`, armadura, `areaRadius`, `pullPx`, `projectile`, `visual` = clips/segmentos de
  pose + `PoseFx` con presets V6 y claves), y visuales de estado `hit, knockdown, getup, victory, jumpRise, jumpFall, walk,
  block`. Define tambien el visual de `Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack` (se activan en
  Fase 2; deja `enabled` como venga).
- Cajas: el arco (`LimbArc`) debe cubrir el miembro DIBUJADO en el cuadro de impacto (tolerancia 12 px): compruebalo con
  `moves_sheet` (dibuja la capsula) o F9 en juego. Las escalas por tamano (`bodyScale`) y profundidad las aplica el motor.
- Herramientas: `xvfb-run -a ./build/moves_sheet /tmp/<tu_dir> <id>` (6 cuadros por movimiento + `moves.tsv` con el
  origen del clip y si golpeo al muneco) y `xvfb-run -a ./build/state_strips /tmp/<tu_dir> <id>` (estados). Mira las
  imagenes (Read) antes de dar algo por bueno.
- Si el motor no puede expresar algo que necesitas, NO lo cambies: describelo como `PEDIDO AL MOTOR` y usa la mejor
  aproximacion disponible.
- Aceptacion comun: `moves.tsv` sin `IDLE`; cada movimiento habilitado de tu personaje con origen `override` (salvo los que
  justifiques); cada ataque habilitado "golpeo = s" contra el muneco; habilidades 1-5 = movimientos distintos de la cadena;
  numero de movimientos habilitados >= la meta de tu brief; capturas `moves_<id>.png` y `states_<id>.png` en tu informe.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.3 — RAYDEN (ORIGINAL), skin 0: SOLO AGREGAR ~30 movimientos nuevos

**Parte de:** "base-fase1".

**Regla especial (del usuario, inviolable):** Rayden no se reemplaza ni se toca su arte. Concretamente: no se edita
`assets/characters/rayden_128.png` ni `rayden_clean.png`; los 19 clips del juego `rayden` en `data/sprite_manifest.json`
quedan identicos; los 9 ataques historicos (Punch1, Punch2, Punch3, Kick, EnergyWave, DashAttack, RageAttack, Finisher,
Skill) y TODOS sus estados historicos (idle, walk, dash, block, hit, airborne, knockdown, getup, defeat, recovery) se ven
**pixel identicos** a la base (sin override). Solo se AGREGAN movimientos nuevos con clips nuevos de nombre nuevo.

**Arte disponible** (atlas 4x6, celdas 128x128, pivote (64,126); imagen `.../audit/img/rayden_atlas.png`): 0-3 reposo,
4-7 caminar, 8 jab con chispa azul, 9 gancho con arco azul, 10 patada media con arco, 11 patada alta con arco, 12 aura
electrica, 13 golpe recibido, 14 dash con estela, 15 tumbado, 16 guardia, 17 golpe bajo recibido, 18 cayendo de espaldas,
19 caida, 20 agachado (levantarse), 21 guardia/bloqueo. (Verifica los indices mirando la imagen antes de usarlos.)

**PUEDES TOCAR:** `src/game/moves/Moves_Rayden.cpp`, `data/manifest/rayden_moves.json` (clips NUEVOS en el juego `rayden`,
sin `override`), NUEVO `tests/unit/RaydenAddOnlyTests.cpp`.
**NO TOQUES:** todo lo demas (en particular el manifiesto principal, el atlas, `CharacterVisual.cpp`).

**Moveset ("peleador callejero rapido, electricidad azul") — meta >= 30 movimientos.** Cadena J = P1, P2 (historicos),
PunchHook, Punch3 (historico = remate con patada alta) -> 4 golpes; cadena K = Kick (historico), Kick2, Kick3. Clips nuevos
sugeridos en `rayden_moves.json` (nombre: cuadros): `punch_hook: [16, 9]`, `kick2: [16, 10]`, `kick3: [20, 11]`,
`kick_launcher: [20, 11]`, `fwd_punch: [16, 8]`, `back_elbow: [9]`, `spin_kick: [10]`, `sweep: [20, 10]`, `uppercut: [20, 9]`,
`stomp: [21, 20]`, `dash_kick: [14, 10]`, `jump_rise: [20, 14]`, `jump_fall: [16]`, `air_punch: [8]`, `air_kick: [10]`,
`dive_kick: [11]`, `air_spike: [9]`, `rising_uppercut: [20, 9]`, `hurricane: [10, 11]`, `drill_kick: [11]`, `sig1: [16, 14, 8]`,
`sig2: [16, 12, 8]`, `sig3: [12]`, `victory: [12, 21]`.
| Entrada | Nombre | Visual (clip + presets V6) | Dano / propiedades (sobre la fila generica) |
|---|---|---|---|
| J J J | Gancho electrico | `punch_hook` + claves ang -8 -> +10 (barre el arco) + `ESTELA(3)` azul | gen. |
| K K | Media luna | `kick2` + `GIRO(0.5)` + ang -10 -> +12 | gen. |
| K K K | Talon giratorio | `kick3` + `GIRO(1)` + `SALTITO(10)` | gen. (wallBounce) |
| J J K | Patada al cielo | `kick_launcher` + `UPPER(8)` | Lch 720, jumpCancel |
| `>`J | Directo en carrera | `fwd_punch` + `ESTOCADA(70)` + `ESTELA(3)` | gen. |
| `>`K | Patada de avance | `kick2` + `ESTOCADA(60)` | gen. |
| `<`J | Codazo trasero | `back_elbow` espejado (dibujado hacia atras) | behind |
| `<`K | Patada giratoria | `spin_kick` + `GIRO(1)` | bothSides |
| `v`J | Gancho ascendente | `uppercut` + `UPPER(12)` | Lch 680 |
| `v`K | Barrida | `sweep` + `BARRIDA` | low, Down |
| `v`J sobre caido | Pisoton | `stomp` + `TEMBLOR` | otg |
| Dash K | Deslizamiento | `dash_kick` + `sy .72` + `ESTOCADA(120)` | low |
| Salto | — | `jump_rise` (`sy .9` en S, ang -10) / `jump_fall` (ang +8); aterrizaje con squash | jumpVz 720 |
| aire J / J J | Puno aereo 1/2 | `air_punch` ang +12 / `air_spike` ang +18 | gen. |
| aire K | Patada voladora | `air_kick` ang +25 + `ESTELA(2)` | gen. |
| aire `v`K | Patada picada | `dive_kick` ang +45 + `ESTELA(4)` | gen. |
| aire `>`J | Martillo | `air_spike` ang +60 -> -60 | groundBounce |
| DP+K | Puno trueno | `rising_uppercut` + `UPPER(60)` + pilar de rayo (`FX` 8-13 en vertical) | Lch 760, invuln f1-5 |
| QCB+K | Ciclon | `hurricane` + `GIRO(3)` + hop 40 | 6x5, bothSides |
| QCF+K | Taladro electrico | `drill_kick` ang +30 + `GIRO(2)` + `ESTOCADA(140)` | 8x3 |
| QCB+J (Sig1) | Puno relampago | `sig1` (16 -> 14 -> 8) + `ESTOCADA(180, outBack)` + `ESTELA(5)` | 6x3 + 10, SP 15 |
| `>`L (Sig2) | Onda triple fase | `sig2` (anticipo -> aura con `LATIDO` -> empuje) + proyectil grande x1.4 | 24, SP 25 |
| `v`L (Sig3) | Descarga | `sig3` + `LATIDO(.05,6)` + anillo electrico | 14, area r110, Lch 400, SP 20 |
| (Fase 2) Agarre / rodillazo / lanzamientos / contra / levantarse | — | `punch2`(9) quieto / `kick` hop 6 / `punch3` + rotacion / `fwd_punch` / `sweep` | solo visual |
Victoria (estado nuevo, no historico): `victory` (aura + guardia) + rebote leve, 1.6 s.
Habilidades 1-5 (hoy repiten normales): 1 Onda (EnergyWave, como hoy), 2 Puno relampago, 3 Puno trueno, 4 Ciclon, 5 Descarga.

**Test** `RaydenAddOnlyTests`: los 19 clips del juego `rayden` tienen los mismos cuadros y duraciones que en la base (lee el
JSON con el parser del proyecto); ningun clip de `rayden_moves.json` repite un nombre existente; `GetMoveSet(0)` no tiene
override para los 9 ids historicos.

**Criterios de aceptacion.** `state_strips --fixed-time 0` identico a la base en los estados historicos (0 pixeles distintos;
la herramienta puede tener filas nuevas para estados nuevos); `moves_sheet 0` >= 30 movimientos, nuevos con origen
`override`, golpeo = s; md5 del atlas igual.

**Como se trabaja un personaje (comun a F1.1-F1.8).**
- Tras Fase 0 el motor ya da a TODOS los personajes los ~40 movimientos genericos (V1/V2) con un visual de reserva. Tu
  trabajo: que cada movimiento de TU personaje tenga visual propio (arte existente + transformaciones matematicas), numeros
  ajustados a su peso, cadenas largas, 3 poderes `Signature`, habilidades 1-5 que no repitan normales, y estados (golpe,
  derribo, levantarse, victoria, salto, caminar, bloqueo) que se vean bien. Deshabilita (con motivo escrito) lo que no tenga
  sentido para el personaje.
- Todo se declara en DATOS en tu `src/game/moves/Moves_<Bloque>.cpp`: `const MoveSet* MovesetFor_<Bloque>(int skin)`
  devuelve un `MoveSet` estatico para tus skins (y `nullptr` para los demas). Campos (ver `src/game/moves/MoveTypes.h`):
  `caps`, `feel`, `chainJ/chainK` (hasta 6/4 eslabones; puedes usar `Chain5/Chain6` con override completo), `skillSlots[5]`,
  `jumpVz`, `overrides[]` (por movimiento: `displayName` en espanol, `replaceDef`+`def` o escalas, `arc` (LimbArc), `root`
  (RootCurve), `hitCount`/`multiHitInterval`, armadura, `areaRadius`, `pullPx`, `projectile`, `visual` = clips/segmentos de
  pose + `PoseFx` con presets V6 y claves), y visuales de estado `hit, knockdown, getup, victory, jumpRise, jumpFall, walk,
  block`. Define tambien el visual de `Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack` (se activan en
  Fase 2; deja `enabled` como venga).
- Cajas: el arco (`LimbArc`) debe cubrir el miembro DIBUJADO en el cuadro de impacto (tolerancia 12 px): compruebalo con
  `moves_sheet` (dibuja la capsula) o F9 en juego. Las escalas por tamano (`bodyScale`) y profundidad las aplica el motor.
- Herramientas: `xvfb-run -a ./build/moves_sheet /tmp/<tu_dir> <id>` (6 cuadros por movimiento + `moves.tsv` con el
  origen del clip y si golpeo al muneco) y `xvfb-run -a ./build/state_strips /tmp/<tu_dir> <id>` (estados). Mira las
  imagenes (Read) antes de dar algo por bueno.
- Si el motor no puede expresar algo que necesitas, NO lo cambies: describelo como `PEDIDO AL MOTOR` y usa la mejor
  aproximacion disponible.
- Aceptacion comun: `moves.tsv` sin `IDLE`; cada movimiento habilitado de tu personaje con origen `override` (salvo los que
  justifiques); cada ataque habilitado "golpeo = s" contra el muneco; habilidades 1-5 = movimientos distintos de la cadena;
  numero de movimientos habilitados >= la meta de tu brief; capturas `moves_<id>.png` y `states_<id>.png` en tu informe.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.4 — MAX (skin 6, ex-RAYDER) + forma blanca: la mina de arte (~190 celdas sin usar) -> ~34 movimientos

**Parte de:** "base-fase1". MAX es OTRO personaje que RAYDEN (skin 0): no los confundas ni toques nada de Rayden.

**Diagnostico.** Atlas `max` (`assets/characters/max/max_atlas.png`, rejilla 14x22, celda 270x175, pivote (135,168)); 25 clips;
~190 celdas sin usar (hojas: `.../audit/img/max_unused_a.png`, `max_unused_b.png`, `max_a.png`..`max_d.png`). Problemas:
(1) la forma transformada `max_white` (rejilla 14x18) nunca carga: su juego de clips sigue llamandose `rayder_kf_white`
(`SpriteManifest.cpp` busca el juego con el id del atlas); (2) los clips mezclan el set "chaqueta" (reposo/caminar/golpe 1-2) y
el set "camiseta + cinta azul" (punch3, kick, energy, dash...) con ~1.6x de escala: cambia de tamano a mitad de combo;
(3) `knockdown` empieza con 277 (explosion) y 278 (patada giratoria): parece que ataca al caer; (4) `defeat` termina
sentado (106). Las celdas 245-285 son poses de las hojas v3 del usuario (commit 550c16e): se conservan.
Rangos utiles (VERIFICA cada celda a ojo antes de usarla; descarta fragmentos con poco opaco, p.ej. 25, 26, 29, 59, 81, 85):
21-24 despegue/salto, 25-27 patadas en el aire, 28-38 giros/patadas aereas con estela azul, 39-42 aterrizaje, 43-50
jab/cruzado, 51-56 gancho, 57-60 media luna ascendente, 61-71 patadas baja/media, 72-76 barridas, 77 y 86 arcos grandes,
112-125 golpes agachado, 128-149 cargas/pilares/haces de energia, 204-205 orbes, 218-242 set pequeno (variante) con media
luna, 286-297 giros con media luna/rayo (set grande), 298-302 derrota con sangre. **PROHIBIDO 153-203** (otro personaje) y
la celda 142 de `max_white` (texto incrustado).

**PUEDES TOCAR:** `data/sprite_manifest.json` SOLO dentro de los juegos `max`, `max_white`/`rayder_kf_white` y la entrada de
atlas `max_white` (nada mas del archivo); `src/game/moves/Moves_Max.cpp`; NUEVOS `tools/max/*.py` (medicion, hojas de
contacto, derivacion de arcos); NUEVO `tests/unit/MaxManifestTests.cpp`.
**NO TOQUES:** ningun PNG de `assets/characters/max/` (ni celdas, ni tamano, ni pivote), el juego `rayden`, motor, otros.

**Tareas.**
1. **Forma blanca real**: renombra el juego `rayder_kf_white` -> `max_white` (o anade `"clips": "rayder_kf_white"` al atlas);
   quita 142 de cualquier clip; todo indice < 252. Los movimientos que usan celdas inexistentes en `max_white` (>= 242 y las
   nuevas) caen al atlas base con tinte (lo hace el motor F0.3): compruebalo en `state_strips` con la transformacion.
2. **Escala coherente**: `tools/max/medir_celdas.py` mide por celda el alto del CUERPO (excluye pixeles de efecto: azul
   saturado, H 190-230 grados, S > 0.5, V > 0.5) y escribe la tabla; para cada clip pon `"scales"` (uno por cuadro) para que
   el cuerpo tenga el alto del reposo (celda 0) +-5 %. Genera hoja antes/despues (`/tmp/max_escala_{antes,despues}.png`).
   No elijas un "diseno": la decision de ropa queda abierta para el usuario (reportalo).
3. **Estados**: `knockdown` = 102-104 + `CAIDA_ROT` si hace falta (sin 277/278); `defeat` = 298-302 (termina tumbado); `getup`
   sin saltos de set; `victory` 243/244 con escala igualada; salto `jump_rise` 21-24, `jump_fall` (elige de 25-38 una pose
   neutra), aterrizaje = `landing` (39-42, ya existe); `turn` ya existe (lo usa F0.4).
4. **Arcos derivados del arte**: `tools/max/derivar_arcos.py`: para cada clip de ataque, en el cuadro de impacto busca el pixel
   opaco mas adelantado por bandas de altura y calcula `L1`, `theta1` respecto a hombro (0,-84) o cadera (0,-58) escalados;
   vuelca la tabla y usala en los `LimbArc` de `Moves_Max.cpp`.
5. **Moveset ("artista marcial de energia azul") — meta >= 34 movimientos.** Cadena J de 5: P1, P2 (existentes), PunchHook
   (51-56), Chain5 = Media luna ascendente (57-60, Lch 420, `UPPER(10)`), Punch3 (remate volador existente) -> `chainJ =
   {Punch1, Punch2, PunchHook, Chain5, Punch3}`. Cadena K de 4: `chainK = {Kick2, Kick3, Chain6, Kick}` con Kick2 = patada
   baja (61-66), Kick3 = patada media (67-71), Chain6 = arco grande (77 u 86, Lch 300, wallBounce) y Kick = la patada giratoria
   historica de 550c16e como cierre (knockdown). Resto:
   | Entrada | Nombre | Celdas sugeridas | Propiedades |
   |---|---|---|---|
   | J J K | Media luna al cielo | 57-60 | Lch 720, jumpCancel |
   | `>`J / `>`K | Directo / Patada voladora | 43-46 + `ESTOCADA(70)` / 28-33 + `ESTOCADA(90)` | |
   | `<`J / `<`K | Codazo / Giratoria | elige de 51-56 espejado / 77 u 86 + `GIRO(1)` | behind / bothSides |
   | `v`J / `v`K | Gancho bajo->alto / Barrida | 112-115 -> 53/58/60 / 72-76 (+112-118) | Lch 680 / low |
   | Dash J / Dash K | Embestida (existente) / Deslizamiento | existente / 72-75 | |
   | Salto + aire J, J J, K, `v`K, `>`J | Aereos | 25-27, 34-38, 28-33 (picada con `ESTELA(4)`) | |
   | QCF+J / L | Onda (existente: energy + projectile 176-180) | existente | |
   | DP+K | Shoryu azul | 53/58/60 + pilar 128-137 | Lch 780, invuln f1-5 |
   | QCB+K | Huracan aereo | 28-38 + hop 50 | 6x5 |
   | QCF+K | Taladro | 86-91 + `GIRO(2)` | 8x3 |
   | QCB+J (Sig1) | Rayo azul | 138-140 -> 293-294 (haz) | multigolpe 5 cada 4 f, alcance 420, SP 20 |
   | `>`L (Sig2) | Pilares de energia | 128-137 en 3 posiciones (120, 220, 320 px) | 10 x3, Lch 500, SP 25 |
   | `v`L (Sig3) | Nova | 144-147 | area r150, 22, Lch 450, SP 30 |
   | Furia / Finisher | existentes | existentes (escala igualada) | |
   | Transformado + `v`L | Supernova | 286-297 | solo en forma blanca: area r200, 30 |
   Habilidades 1-5: Onda, Rayo azul, Shoryu azul, Pilares de energia, Nova. Visuales de Fase 2: agarre = un cuadro de 112-124 o
   de 43-50 (NUNCA 181-188: es el personaje prohibido).
6. **Test** `MaxManifestTests`: el juego `max_white` existe y carga; ningun clip de `max`/`max_white` usa 153-203 ni 142 (en
   `max_white`); todo indice dentro de la rejilla; `knockdown` no contiene 277/278; `defeat` termina en un cuadro de 298-302.

**Criterios de aceptacion.** md5 de `assets/characters/max/*.png` sin cambios; `state_strips 6` (normal y transformado) sin
saltos de tamano > 10 % entre cuadros de un mismo clip; transformacion muestra el atlas blanco; `moves_sheet 6` >= 34
movimientos `override`, golpeo = s; cadenas de 5 (J) y 4 (K) verificadas en `MovesetTests`-like propio o en tu test.

**Como se trabaja un personaje (comun a F1.1-F1.8).**
- Tras Fase 0 el motor ya da a TODOS los personajes los ~40 movimientos genericos (V1/V2) con un visual de reserva. Tu
  trabajo: que cada movimiento de TU personaje tenga visual propio (arte existente + transformaciones matematicas), numeros
  ajustados a su peso, cadenas largas, 3 poderes `Signature`, habilidades 1-5 que no repitan normales, y estados (golpe,
  derribo, levantarse, victoria, salto, caminar, bloqueo) que se vean bien. Deshabilita (con motivo escrito) lo que no tenga
  sentido para el personaje.
- Todo se declara en DATOS en tu `src/game/moves/Moves_<Bloque>.cpp`: `const MoveSet* MovesetFor_<Bloque>(int skin)`
  devuelve un `MoveSet` estatico para tus skins (y `nullptr` para los demas). Campos (ver `src/game/moves/MoveTypes.h`):
  `caps`, `feel`, `chainJ/chainK` (hasta 6/4 eslabones; puedes usar `Chain5/Chain6` con override completo), `skillSlots[5]`,
  `jumpVz`, `overrides[]` (por movimiento: `displayName` en espanol, `replaceDef`+`def` o escalas, `arc` (LimbArc), `root`
  (RootCurve), `hitCount`/`multiHitInterval`, armadura, `areaRadius`, `pullPx`, `projectile`, `visual` = clips/segmentos de
  pose + `PoseFx` con presets V6 y claves), y visuales de estado `hit, knockdown, getup, victory, jumpRise, jumpFall, walk,
  block`. Define tambien el visual de `Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack` (se activan en
  Fase 2; deja `enabled` como venga).
- Cajas: el arco (`LimbArc`) debe cubrir el miembro DIBUJADO en el cuadro de impacto (tolerancia 12 px): compruebalo con
  `moves_sheet` (dibuja la capsula) o F9 en juego. Las escalas por tamano (`bodyScale`) y profundidad las aplica el motor.
- Herramientas: `xvfb-run -a ./build/moves_sheet /tmp/<tu_dir> <id>` (6 cuadros por movimiento + `moves.tsv` con el
  origen del clip y si golpeo al muneco) y `xvfb-run -a ./build/state_strips /tmp/<tu_dir> <id>` (estados). Mira las
  imagenes (Read) antes de dar algo por bueno.
- Si el motor no puede expresar algo que necesitas, NO lo cambies: describelo como `PEDIDO AL MOTOR` y usa la mejor
  aproximacion disponible.
- Aceptacion comun: `moves.tsv` sin `IDLE`; cada movimiento habilitado de tu personaje con origen `override` (salvo los que
  justifiques); cada ataque habilitado "golpeo = s" contra el muneco; habilidades 1-5 = movimientos distintos de la cadena;
  numero de movimientos habilitados >= la meta de tu brief; capturas `moves_<id>.png` y `states_<id>.png` en tu informe.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.5 — BRAKK (skin 2): bruto de cadena con los clips de jefe sin usar -> ~24 movimientos

**Parte de:** "base-fase1".

**Diagnostico.** Atlas `brakk_v2` (`assets/bosses/brakk/brakk_v2_atlas_clean.png`, 12x9 celdas 256x176, pivote (128,168)), juego
de 29 clips; el jugador usa 19 y NUNCA `run, charge, chain, chain_throw, smash, fury, explosive, hit, victory`; 50 celdas
sin usar (7, 10, 12, 15, 18-22, 28, 32-35, 37, 38, 40, 45, 47-57, 62, 63, 67-76, 81-84, 87, 89, 90, 100, 103; utiles: 10 cadena
sobre la cabeza, 22, 47, 50-57 golpe al suelo con cadena, 67-76 latigazos y punetazos, 100). Bugs: `rage_attack` cuadro 88 =
solo el arco de cadena (el cuerpo desaparece); `hit_high/hit_low` de 1 cuadro; `kick` es "lanzar cadena" (no hay patadas);
mate marron en `walk/charge/dash_attack/kick` y linea suelta en el cuadro 102. Imagenes: `.../audit/img/brakk_atlas_small.png`,
`crops/brakk_*.png`, `states/s2_small.png`. Tiene perfil de animacion de ataque (skin 2) en `AttackAnimation.h`: los 8 ids
historicos lo conservan.

**PUEDES TOCAR:** `data/manifest/brakk_moves.json` (clips nuevos del juego `brakk_v2`; `override: true` permitido solo para
arreglar `rage_attack`, `hit_high`, `hit_low`), `src/game/moves/Moves_Brakk.cpp`, NUEVO `tests/unit/BrakkMovesTests.cpp`,
OPCIONAL al final: NUEVO `tools/brakk/limpiar_mate.py` + `assets/bosses/brakk/brakk_v2_atlas_clean.png` (ver tarea 4).
**NO TOQUES:** `data/sprite_manifest.json`, motor, otros personajes, `assets/bosses/brakk/*.png` de poses sueltas (los usa el
jefe antiguo).

**Tareas.**
1. Arreglos de clips (override): `rage_attack` sin el cuadro 88 (reemplazalo por el cuadro de cuerpo completo mas cercano);
   `hit_high` = `hit` (93, 94) y `hit_low` = `hit` invertido; `victory` en el estado Victoria; `run` para la carrera.
2. **Moveset ("bruto de cadena: lento, alcance largo, sin salto") — meta >= 24 movimientos.** `caps` sin `CapJump/CapAir`,
   sin Hurricane/DrillKick genericos (los reemplazan sus Signature). Alcance x1.3 en ataques de cadena (`reachScale`).
   Cadena J de 5: `{Punch1 (latigazo), Punch2 (latigazo), PunchHook = chain (cadena giratoria, 2 golpes), Punch3 (golpe al suelo historico), Chain5 = smash (gran golpe con onda r160, knockdown)}`.
   Cadena K de 3: `{Kick (latigazo largo historico, reach x1.5), Kick2 = charge (embestida de hombro, armadura 1), Kick3 = smash bajo (trip)}`.
   | Entrada | Nombre | Clip / celdas | Propiedades |
   |---|---|---|---|
   | J J K | Gancho de cadena al cielo | `chain` ang -20 | Lch 680 |
   | `>`J | Jalon | `chain_throw` | `pullPx` 80 (atrae al enemigo hacia Brakk), 10 dano, abre combo |
   | `>`K | Hombro | `charge` + `ESTOCADA(90)` | armadura 1 |
   | `<`J / `<`K | Cadena trasera / Remolino corto | `chain` espejado / `chain` + `GIRO(1)` | behind / bothSides |
   | `v`J / `v`K | Cadena ascendente / Barrida de cadena | `chain` ang -30 / `smash` `sy .9` | Lch 680 / low |
   | Dash J / Dash K | Embestida | `charge` + `ESTOCADA(160)` | armadura 2 |
   | L, QCF+J | Cadena con sierra (historico: energy) | existente | alcance 260, multigolpe 3 |
   | DP+K | Gancho de cadena | `chain` ang -40 + `UPPER(16)` | Lch 760 |
   | QCB+J (Sig1) | Remolino de cadena | `chain` + `GIRO(3)` | 5x6, area r150, bothSides |
   | `>`L (Sig2) | Ancla | `chain_throw` -> `smash` | atrae a todos en 220 px (`pullPx` 120) y remata |
   | `v`L (Sig3) | Explosivo | `explosive` | area r170, 22, Lch 500, SP 30 |
   | Furia | Furia de cadena | `fury` (entrada) + `explosive` | area |
   | Finisher | Aplastamiento final | `smash` + `explosive` | 44 |
   | (Fase 2) Agarre / lanzamiento | Estrangular con cadena / Lanzar | `chain_throw` / `chain` + rotacion | solo visual |
   Habilidades 1-5: Cadena con sierra, Remolino de cadena, Ancla, Gancho de cadena, Explosivo.
3. Celdas sin usar: usa las utiles (67-76 latigazos/punetazos, 50-57 golpe al suelo, 10 cadena sobre la cabeza) para dar
   variedad a los eslabones (p.ej. Punch2 con 67-70, Chain5 con 50-57) mediante clips nuevos.
4. OPCIONAL (solo si terminaste lo anterior): `tools/brakk/limpiar_mate.py` que, SOLO dentro de las celdas de
   `walk/charge/dash_attack/kick` y del cuadro 102, pone alfa 0 a los pixeles del mate marron (rango RGB medido) conectados al
   borde de la silueta; genera diff visual antes/despues y cuenta pixeles cambiados por celda. Si algun cuadro cambia mas de
   lo esperado (> 3 % de su opaco), no lo apliques. Commit separado.
5. **Test** `BrakkMovesTests`: indices dentro de la rejilla; `rage_attack` sin 88; cadenas de 5/3; Brakk sin `CapJump`.

**Criterios de aceptacion.** `moves_sheet 2` >= 24 movimientos `override`, golpeo = s; el cuerpo no desaparece en ningun
cuadro de ataque; `AttackAnimationTimingTests` en verde (perfil skin 2 intacto).

**Como se trabaja un personaje (comun a F1.1-F1.8).**
- Tras Fase 0 el motor ya da a TODOS los personajes los ~40 movimientos genericos (V1/V2) con un visual de reserva. Tu
  trabajo: que cada movimiento de TU personaje tenga visual propio (arte existente + transformaciones matematicas), numeros
  ajustados a su peso, cadenas largas, 3 poderes `Signature`, habilidades 1-5 que no repitan normales, y estados (golpe,
  derribo, levantarse, victoria, salto, caminar, bloqueo) que se vean bien. Deshabilita (con motivo escrito) lo que no tenga
  sentido para el personaje.
- Todo se declara en DATOS en tu `src/game/moves/Moves_<Bloque>.cpp`: `const MoveSet* MovesetFor_<Bloque>(int skin)`
  devuelve un `MoveSet` estatico para tus skins (y `nullptr` para los demas). Campos (ver `src/game/moves/MoveTypes.h`):
  `caps`, `feel`, `chainJ/chainK` (hasta 6/4 eslabones; puedes usar `Chain5/Chain6` con override completo), `skillSlots[5]`,
  `jumpVz`, `overrides[]` (por movimiento: `displayName` en espanol, `replaceDef`+`def` o escalas, `arc` (LimbArc), `root`
  (RootCurve), `hitCount`/`multiHitInterval`, armadura, `areaRadius`, `pullPx`, `projectile`, `visual` = clips/segmentos de
  pose + `PoseFx` con presets V6 y claves), y visuales de estado `hit, knockdown, getup, victory, jumpRise, jumpFall, walk,
  block`. Define tambien el visual de `Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack` (se activan en
  Fase 2; deja `enabled` como venga).
- Cajas: el arco (`LimbArc`) debe cubrir el miembro DIBUJADO en el cuadro de impacto (tolerancia 12 px): compruebalo con
  `moves_sheet` (dibuja la capsula) o F9 en juego. Las escalas por tamano (`bodyScale`) y profundidad las aplica el motor.
- Herramientas: `xvfb-run -a ./build/moves_sheet /tmp/<tu_dir> <id>` (6 cuadros por movimiento + `moves.tsv` con el
  origen del clip y si golpeo al muneco) y `xvfb-run -a ./build/state_strips /tmp/<tu_dir> <id>` (estados). Mira las
  imagenes (Read) antes de dar algo por bueno.
- Si el motor no puede expresar algo que necesitas, NO lo cambies: describelo como `PEDIDO AL MOTOR` y usa la mejor
  aproximacion disponible.
- Aceptacion comun: `moves.tsv` sin `IDLE`; cada movimiento habilitado de tu personaje con origen `override` (salvo los que
  justifiques); cada ataque habilitado "golpeo = s" contra el muneco; habilidades 1-5 = movimientos distintos de la cadena;
  numero de movimientos habilitados >= la meta de tu brief; capturas `moves_<id>.png` y `states_<id>.png` en tu informe.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.6 — Personajes KF de la APK (skins 7-21): clips reales, sin rellenos, combos largos con su propio arte

**Parte de:** "base-fase1". 15 personajes; casi todo es trabajo de TABLAS. Los rivales de `all_modes_check` {7, 8, 18, 19, 21}
estan aqui: deben seguir usando >= 1 habilidad, golpear y recibir dano en 25 s.

**Diagnostico** (detalle por personaje en `.../audit/personajes.md` §1.5 y §2.7-2.21, datos de clips en
`docs/auditoria_movimientos/herramientas/dump.txt`): `KfReference.cpp` mapea acciones de la APK a
nombres fijos (`kHeroClips` / `kEnemyClips`) y RELLENA en silencio los que faltan con `idle` (`kRequired`, linea ~581). Varios
nombres apuntan a la misma accion: `energy = special = finisher`, `dash_attack = dash` (heroes) o `= run` (enemigos),
`rage_attack = kick` (enemigos), `block = idle` (enemigos), `recovery = idle`. En enemigos KF `energy/special/finisher` son 1
cuadro de 0.05 s (golpe invisible) y la 1a habilidad "SOMBRAS" es su animacion de levantarse (accion 20 = `getup`). Heroes:
doble caida (`airborne` muestra lanzado+cae+tumbado y luego `knockdown` vuelve a caer), derrota termina arrodillado. KF
HEROE TRANSFORMADO (18): `rage_attack = idle`. KF HEROINA TRANSFORMADA (19): `hit = hit_high = hit_low = defeat = idle`,
`knockdown = air = airborne` 1 cuadro, `block = getup = super = rage_attack = recovery` 1 cuadro. RAYDER CLON BETA (20) y
RAYDER CLON (21) = estructura del heroe 7; el 21 es casi negro sobre negro.

**PUEDES TOCAR:** `src/game/lab/KfReference.cpp/.h`, `src/game/moves/Moves_Kf.cpp`, NUEVO `tests/unit/KfClipsTests.cpp` (si
necesita ventana, convierte la comprobacion en salida de `moves_sheet`/`all_modes_check` y no en ctest).
**NO TOQUES:** `apk_reference/**` (solo lectura), `src/game/beta/*` (es de F1.7/F1.8), motor, `CharacterVisual.cpp`, BETA.
Ojo: `KfReference.cpp` tambien carga los BETA (rama `betaId`): no cambies esa rama salvo la linea del gancho que ya existe.

**Tareas.**
1. **Sin rellenos silenciosos**: los clips de `kRequired` que no existen se registran como `synthetic` (expon
   `bool KfReference::IsSynthetic(const std::string& clip) const`); el motor (F0.4) ya trata los sinteticos como inexistentes
   para ataques. Para estados (`hit`, `defeat`, `block`...) define en el `MoveSet` un visual procedural (feel `Light`).
2. **Mapeos distintos** (heroes 7, 8, 20, 21 y transformados 18, 19): `dash_attack` = VOLTERETA (ab14) o PATADA AEREA (ab17)
   (heroina: RODADA o GIRO); `finisher` = INFIERNO (ab11) / FENIX (ab4) (heroina: FENIX) en vez de repetir el especial;
   `rage_attack` = SUPER; 18: furia = atk3 + atk4 encadenados (secuencia de 2 clips); 19: golpe recibido = `idle` + feel
   `Light` (inclinacion, retroceso, tinte), derrota/derribo = ultimo cuadro de `atk3` con `CAIDA_ROT(85)`, furia = atk3 + atk4.
   Doble caida: si `airborne` ya mostro la caida, `knockdown` arranca en el cuadro "tumbado" (subrango) y la derrota termina
   tumbada (sostener ultimo cuadro tumbado; si no existe, `CAIDA_ROT`).
3. **Enemigos KF (9-17)**: quitar `getup` (accion 20) de `abilityClips`; `energy/special/finisher` de 1 cuadro -> subrangos
   reales de sus ataques (p.ej. MATON: carga de atk1 cuadros 3-6); bloqueo = 1er cuadro de un ataque mantenido + `sy .97`;
   `rage_attack` != `kick`. 21: subir lectura con contorno claro de 1 px o brillo +15 % (si el motor no lo permite: PEDIDO).
4. **Movesets** (los `abN` citados son los indices del HEROE 7; en los demas busca la habilidad por su NOMBRE en
   `abilityNames`, nunca por indice fijo) (`Moves_Kf.cpp`, una tabla por kfRoster; usa subrangos `from/to` y `speed` de `ClipSeg` para crear golpes
   distintos del mismo clip). Metas: heroes (7, 8, 18-21) >= 22 movimientos; enemigos (9-17) >= 12. Movimientos genericos que
   el personaje no puede mostrar con su arte: deshabilitar (`enabled=false`) y anotarlo.
   - **HEROE 7 / CLON BETA 20 / CLON 21**: cadena J = atk1, atk2, atk3, atk4; cadena K = kick, PATADA AEREA (ab17), MEDIA LUNA
     (ab9); `v`K = BARRIDA (ab6, cuadros finales); `>`K = PATADA AEREA; `>`J = TAJO (ab10); `<`K = GIRO (ab15); dash J =
     VOLTERETA (ab14); dash K = RODADA (ab16); salto = SALTO TIGRE (ab22) o VOLTERETA como visual; aire K = PATADA AEREA; DP+K =
     PILAR (ab2) o FENIX (ab4); QCB+K = TORBELLINO (ab19); QCF+K = TORNADO (ab5); L/QCF+J = especial; Sig1 (QCB+J) = ERUPCION
     (ab8); Sig2 (`>`L) = CIRCULO FUEGO (ab7); Sig3 (`v`L) = LLAMARADA (ab3); furia = SUPER; Finisher = INFIERNO (ab11);
     GuardCounter (Fase 2) = CONTRA (ab21); victoria = `transform` o DESPERTAR (ab13). Habilidades 1-5 por paginas (como hoy).
     Estela de color propio (20 morada, 21 roja) en dash/especiales.
   - **HEROINA 8**: J = atk1, atk3, ESTALLIDO (ab1); K = atk2, atk4, PATADA AEREA (ab10); dash J = RODADA; `v`K = GIRO; L =
     LLAMARADA; DP+K = FENIX; QCB+K = VOLTERETA; Sig1 = PILAR; Sig2 = ERUPCION; Sig3 = SOMBRAS (solo si es un ataque real);
     furia = SUPER; victoria = `transform`.
   - **TRANSFORMADOS 18 / 19**: J = atk1..atk4; L = especial; furia = atk3+atk4; dash J = skill5 (18) / VOLTERETA (19); `v`K y
     `>`K con subrangos de atk2/atk4; Sig1-3 de sus habilidades reales.
   - **Enemigos 9-17** (minimo J x3, K, L, dash J, `v`K, `>`J, 2 Signature, furia distinta): MATON J = atk2, atk2 (`IMPACTO(1.4)`),
     cabezazo (atk1 0-6), K = pisoton (atk1 completo), L = embestida (`run` + `ESTOCADA(120)`); NAVAJERA J x4 = atk1, atk2,
     atk1 (speed 1.3), atk2 + `ESTOCADA(60)` + `ESTELA` (cuchillo), L = navaja lanzada (proyectil pequeno); SOLDADO J =
     atk1, atk3, atk2, K = culatazo (atk2 + `ESTOCADA`), L = granada (skill5, proyectil en arco); RUBIA J = atk1, atk3, K =
     atk2, L = FENIX (skill5), dash J = atk3 + `ESTOCADA`; GORRA J J K = atk1, atk2, atk4, L = skill5; PELEADOR J = atk1,
     atk2, atk3, K = atk4 `IMPACTO(1.3)`, L = especial real; CUCHILLERO J = atk1, atk3, atk2, L = cuchillo lanzado; JEFE GARRA
     (16) J = atk1, atk2, atk3, K = atk4, L = especial, furia = atk2+atk3, levantarse = `knockdown` al reves; BUFONA (17) J =
     atk1, atk2, atk3, K = atk4, L = especial, dash J = BARRIDA, levantarse = `knockdown` al reves.
5. **Feel**: `Light` para 19 y para los que tengan golpe recibido/derribo de 1-2 cuadros; `None` para el resto (su arte ya trae
   movimiento).

**Criterios de aceptacion.** `moves_sheet` de 7-21: 0 `IDLE`, metas cumplidas, golpeo = s; ninguna habilidad de enemigo KF es
`getup`; `state_strips` 7 y 19: sin doble caida, derrota tumbada, 19 reacciona al golpe; `all_modes_check` `problemas: 0`.

**Como se trabaja un personaje (comun a F1.1-F1.8).**
- Tras Fase 0 el motor ya da a TODOS los personajes los ~40 movimientos genericos (V1/V2) con un visual de reserva. Tu
  trabajo: que cada movimiento de TU personaje tenga visual propio (arte existente + transformaciones matematicas), numeros
  ajustados a su peso, cadenas largas, 3 poderes `Signature`, habilidades 1-5 que no repitan normales, y estados (golpe,
  derribo, levantarse, victoria, salto, caminar, bloqueo) que se vean bien. Deshabilita (con motivo escrito) lo que no tenga
  sentido para el personaje.
- Todo se declara en DATOS en tu `src/game/moves/Moves_<Bloque>.cpp`: `const MoveSet* MovesetFor_<Bloque>(int skin)`
  devuelve un `MoveSet` estatico para tus skins (y `nullptr` para los demas). Campos (ver `src/game/moves/MoveTypes.h`):
  `caps`, `feel`, `chainJ/chainK` (hasta 6/4 eslabones; puedes usar `Chain5/Chain6` con override completo), `skillSlots[5]`,
  `jumpVz`, `overrides[]` (por movimiento: `displayName` en espanol, `replaceDef`+`def` o escalas, `arc` (LimbArc), `root`
  (RootCurve), `hitCount`/`multiHitInterval`, armadura, `areaRadius`, `pullPx`, `projectile`, `visual` = clips/segmentos de
  pose + `PoseFx` con presets V6 y claves), y visuales de estado `hit, knockdown, getup, victory, jumpRise, jumpFall, walk,
  block`. Define tambien el visual de `Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack` (se activan en
  Fase 2; deja `enabled` como venga).
- Cajas: el arco (`LimbArc`) debe cubrir el miembro DIBUJADO en el cuadro de impacto (tolerancia 12 px): compruebalo con
  `moves_sheet` (dibuja la capsula) o F9 en juego. Las escalas por tamano (`bodyScale`) y profundidad las aplica el motor.
- Herramientas: `xvfb-run -a ./build/moves_sheet /tmp/<tu_dir> <id>` (6 cuadros por movimiento + `moves.tsv` con el
  origen del clip y si golpeo al muneco) y `xvfb-run -a ./build/state_strips /tmp/<tu_dir> <id>` (estados). Mira las
  imagenes (Read) antes de dar algo por bueno.
- Si el motor no puede expresar algo que necesitas, NO lo cambies: describelo como `PEDIDO AL MOTOR` y usa la mejor
  aproximacion disponible.
- Aceptacion comun: `moves.tsv` sin `IDLE`; cada movimiento habilitado de tu personaje con origen `override` (salvo los que
  justifiques); cada ataque habilitado "golpeo = s" contra el muneco; habilidades 1-5 = movimientos distintos de la cadena;
  numero de movimientos habilitados >= la meta de tu brief; capturas `moves_<id>.png` y `states_<id>.png` en tu informe.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.7 — RONIN (22), GUERRERO x4 armas (23-26) y RONIN-KRATOS x4 (38-41): derivaciones en memoria + movesets

**Parte de:** "base-fase1".

**Regla critica.** `data/kratos/*.txt` (Kratos, RONIN, RONIN-KRATOS) y `assets/kratos/actor/*` NO se modifican ni se
regeneran (regla md5 del usuario; `tools/ronin_kratos/check_kratos_md5.py`). Todo se hace DERIVANDO clips en memoria al
cargar, solo en District Fury: `DeriveWarriors(KfReference&, const char* betaId)` en `src/game/beta/BetaDerive_Warriors.cpp`
(el gancho ya se llama desde `KfReference.cpp`; `BetaMode` lo apaga, asi GowApk no cambia). Ademas el estilo visual de RONIN
original no se altera: solo se agregan movimientos y se corrigen estados que hoy no tienen arte (procedural).

**Diagnostico.** RONIN (`ronin.txt`, 150 cuadros, 31 clips, habilidades ONDA DE KI, ESPADA RAPIDA, TORMENTA, RAFAGA,
RODILLAZO): caminar = correr = 6 cuadros de guardia arrastrando los pies; `jump` (5) y `victory` (3) existen y no se usan;
golpe recibido = 1 cuadro de guardia erguida; derribo/derrota/aire = 1 cuadro agachado (nunca cae). GUERRERO y RONIN-KRATOS
(misma estructura, 35 clips, 263-272 cuadros, habilidades HABILIDAD A, B, C, COMBO 1, COMBO 2; Q cambia de arma):
`recovery == hit` -> gesto de dolor tras CADA golpe propio; `air/airborne == hit` (vuela con pose de golpe de pie); caminar =
correr a toda velocidad (patina); bloqueo 1 cuadro de 0.03 s; furia/super de 76 cuadros (5.6 s); `victory` (12) sin usar.
Imagenes: `.../audit/img/states/s22_small.png`, `s23_small.png`, `s38_small.png`, `crops/ronin_p*.png`, `crops/kratos_p*.png`.

**PUEDES TOCAR:** `src/game/beta/BetaDerive_Warriors.cpp`, `src/game/moves/Moves_Warriors.cpp`, NUEVO
`tests/unit/WarriorsDeriveTests.cpp` (si necesita ventana, usa la salida de `moves_sheet` en su lugar).
**NO TOQUES:** `data/kratos/**`, `assets/kratos/**`, `GowApk/**`, `src/game/beta/BetaCharacter.cpp`, `BetaDerive_Beasts.cpp`,
`KfReference.cpp`, motor.

**Tareas.**
1. **Derivaciones** (crear clips NUEVOS en el `Animator` plantilla o reasignar nombres de estado; nunca escribir archivos):
   - `recovery` = ultimo cuadro del ataque en curso sostenido 0.08 s (o cuadro 0 de `idle`), NUNCA `hit`.
   - `air`/`airborne` = cuadros 0-2 de `knockdown` (Kratos/RK); RONIN = cuadro 3 de `jump` con `CAIDA_ROT(40)`.
   - Caminar = `run` con duraciones x2.2 (Kratos/RK) o alternar los 2 cuadros de `idle` con la mitad delantera de `dash`
     (RONIN); correr (`CapRun`) = `run` normal.
   - RONIN: golpe recibido = cuadro de guardia + feel `Light` (inclinacion -10, retroceso, tinte); derribo y derrota =
     cuadro de `knockdown` + `CAIDA_ROT(85)` y sombra estirada; levantarse = derribo al reves (T8) + los 3 cuadros de `getup`.
   - Bloqueo Kratos/RK: cuadro de bloqueo mantenido + ang -2.
   - Partir clips largos en golpes (subrangos) donde el brief de moveset lo pida.
2. **Movesets** (meta >= 18 por personaje; `feel = Light` para los 9):
   - **RONIN ("artista marcial de ki")**: cadena J = punch1, punch2, punch3, RODILLAZO (ab de RODILLAZO) ; cadena K = kick,
     RODILLAZO como 2o, patada voladora (`jump` cuadros 2-4 + `ESTOCADA(80)`) como 3o; J J K = lanzador (`jump` 0-2 + hop,
     Lch 700); `>`K = patada voladora; `v`K = barrida (kick subrango bajo + `BARRIDA`); `<`J/`<`K procedurales sobre punch1/kick;
     dash J = `dash_attack`; salto = `jump` (5 cuadros) + aereos (aire K = cuadro 3 de `jump`); L = ONDA DE KI; QCB+J (Sig1)
     = RAFAGA; DP+K = ESPADA RAPIDA (subrango corto) con Lch 760; `>`L (Sig2) = ESPADA RAPIDA completa; `v`L (Sig3) = TORMENTA
     (area); furia = `super`; victoria = `victory`.
   - **GUERRERO (4 armas) y RONIN-KRATOS (4 armas)** — mismo esquema, numeros por arma: cadena J = atk1, atk2, atk3
     (barrido), + 4o = 1a mitad de `dash_attack` (22 cuadros: subrango 0-10); cadena K = atk4/remate (`kick` = 14 cuadros),
     2o = HABILIDAD A corta (subrango); `>`J = HABILIDAD A; DP+K = HABILIDAD B; QCB+K = HABILIDAD C; Sig1 = COMBO 1; Sig2 =
     COMBO 2; Sig3 = `energy` (22); furia (Espacio) = COMBO 1 (corta); Finisher = `super` de 76 cuadros (cuesta toda la barra
     de rabia, armadura total); salto solo si el `.txt` trae `jump` (si no, `CapJump` off); victoria = `victory` (12).
     Por arma: ESPADAS alcance x1.15 y corte; CESTUS dano x1.15, alcance x0.9, hitstop x1.3; RAYO alcance x1.3 en Sig3;
     GARRAS multigolpe +1 en J J J. El cambio de arma con Q se conserva.
3. **No** reproducir los `clipsound` del juego original en District Fury (copyright): los sonidos los da el director de audio
   por arquetipo (F1.10).

**Criterios de aceptacion.** md5: `distintos: 0`; `state_strips` 22/23/38: tras un golpe propio NO aparece la pose de dolor;
caminar no patina (avance por paso coherente); RONIN cae al suelo y se levanta; `moves_sheet` >= 18 por personaje, 0 `IDLE`,
golpeo = s; `gow_check` identico (la derivacion esta apagada en BETA: compruebalo).

**Como se trabaja un personaje (comun a F1.1-F1.8).**
- Tras Fase 0 el motor ya da a TODOS los personajes los ~40 movimientos genericos (V1/V2) con un visual de reserva. Tu
  trabajo: que cada movimiento de TU personaje tenga visual propio (arte existente + transformaciones matematicas), numeros
  ajustados a su peso, cadenas largas, 3 poderes `Signature`, habilidades 1-5 que no repitan normales, y estados (golpe,
  derribo, levantarse, victoria, salto, caminar, bloqueo) que se vean bien. Deshabilita (con motivo escrito) lo que no tenga
  sentido para el personaje.
- Todo se declara en DATOS en tu `src/game/moves/Moves_<Bloque>.cpp`: `const MoveSet* MovesetFor_<Bloque>(int skin)`
  devuelve un `MoveSet` estatico para tus skins (y `nullptr` para los demas). Campos (ver `src/game/moves/MoveTypes.h`):
  `caps`, `feel`, `chainJ/chainK` (hasta 6/4 eslabones; puedes usar `Chain5/Chain6` con override completo), `skillSlots[5]`,
  `jumpVz`, `overrides[]` (por movimiento: `displayName` en espanol, `replaceDef`+`def` o escalas, `arc` (LimbArc), `root`
  (RootCurve), `hitCount`/`multiHitInterval`, armadura, `areaRadius`, `pullPx`, `projectile`, `visual` = clips/segmentos de
  pose + `PoseFx` con presets V6 y claves), y visuales de estado `hit, knockdown, getup, victory, jumpRise, jumpFall, walk,
  block`. Define tambien el visual de `Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack` (se activan en
  Fase 2; deja `enabled` como venga).
- Cajas: el arco (`LimbArc`) debe cubrir el miembro DIBUJADO en el cuadro de impacto (tolerancia 12 px): compruebalo con
  `moves_sheet` (dibuja la capsula) o F9 en juego. Las escalas por tamano (`bodyScale`) y profundidad las aplica el motor.
- Herramientas: `xvfb-run -a ./build/moves_sheet /tmp/<tu_dir> <id>` (6 cuadros por movimiento + `moves.tsv` con el
  origen del clip y si golpeo al muneco) y `xvfb-run -a ./build/state_strips /tmp/<tu_dir> <id>` (estados). Mira las
  imagenes (Read) antes de dar algo por bueno.
- Si el motor no puede expresar algo que necesitas, NO lo cambies: describelo como `PEDIDO AL MOTOR` y usa la mejor
  aproximacion disponible.
- Aceptacion comun: `moves.tsv` sin `IDLE`; cada movimiento habilitado de tu personaje con origen `override` (salvo los que
  justifiques); cada ataque habilitado "golpeo = s" contra el muneco; habilidades 1-5 = movimientos distintos de la cadena;
  numero de movimientos habilitados >= la meta de tu brief; capturas `moves_<id>.png` y `states_<id>.png` en tu informe.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.8 — Bestias BETA jugables (skins 27-37): partir el ataque unico en combos, estados sin dolor fantasma

**Parte de:** "base-fase1".

**Regla.** Los `.txt` de `GowApk/data/characters/*.txt` los usa tambien GowApk: no se editan ni se regeneran. Todo se deriva en
memoria en `DeriveBeasts(KfReference&, const char* betaId)` (`src/game/beta/BetaDerive_Beasts.cpp`), activo solo en District
Fury (BetaMode lo apaga).

**Diagnostico** (`.../audit/personajes.md` §2.27-2.37): un solo ataque largo para punch1 = punch2 = punch3 = kick = dash_attack
= finisher = rage_attack = special = super (esqueleto 17 cuadros 1.37 s; elefante 36 cuadros 2.43 s; ave 33; centauros 25;
brutos 24); `recovery == hit` (dolor tras cada golpe propio); `air` 1 cuadro; bloqueo = idle (la momia lleva escudo!);
`intro` y `projectile` sin usar (esqueleto); centauros con circulo magico permanente bajo los cascos.
Personajes: 27 SOLDADO ESQUELETO, 28 BESTIA EXCAVADORA, 29 MOMIA CON ESCUDO, 30 BESTIA ELEFANTE, 31 AVE SANADORA, 32/33
CENTAURO (+ROJO), 34/35 BRUTO DE LA BOLA (+ROJO), 36/37 MEDUSA (+ROJA).

**PUEDES TOCAR:** `src/game/beta/BetaDerive_Beasts.cpp`, `src/game/moves/Moves_Beasts.cpp`, NUEVO `tests/unit/BeastsDeriveTests.cpp`
(o salida de `moves_sheet` si requiere ventana).
**NO TOQUES:** `GowApk/**`, `data/**`, `assets/**`, `BetaCharacter.cpp`, `BetaDerive_Warriors.cpp`, `KfReference.cpp`, motor.

**Tareas.**
1. **Derivaciones comunes**: `recovery` = ultimo cuadro del ataque sostenido 0.08 s; `air/airborne` = primeros cuadros de
   `knockdown` (o `hit` + `CAIDA_ROT(40)` si no hay); bloqueo = 1er cuadro del ataque mantenido (MOMIA: el escudo delante) +
   `sy .97`; victoria = `idle` + rebote (`SALTITO(6)` en bucle 0.6 s) si no hay clip.
2. **Partir el ataque largo** en 3-4 golpes con subrangos medidos (busca los cuadros donde cambia mas la silueta o donde la
   caja de ataque del cuadro aparece/desaparece: es tu frontera natural). Cada golpe = clip nuevo `df_atk_a/b/c/d`.
3. **Movesets** (meta >= 9 por personaje; `caps` sin salto ni aire; `feel = Light`):
   | Personaje | J (cadena) | K | L | Dash J / `v`K / `>`J | Signature (2) |
   |---|---|---|---|---|---|
   | 27 ESQUELETO | 3 tajos partidos (0-6, 6-11, 11-16) | tajo al reves (T8, `reverse`) + `GIRO(1)` | `energy` + clip `projectile` como proyectil (hueso) | tajo + `ESTOCADA(80)` / tajo bajo `sy .85` / 1er tajo + `ESTOCADA` | Sig1 = `intro` como golpe de entrada (si tiene caja), Sig2 = 3 huesos en abanico |
   | 28 EXCAVADORA | ataque en 3 | ataque `reverse` | excavar/emerger (`energy`): desaparece y reaparece 120 px delante golpeando | `run` + polvo / barrida | Sig1 temblor de tierra (area r140), Sig2 doble emersion |
   | 29 MOMIA | escudazo partido en 2 + 3o empujon | empujon con escudo + `ESTOCADA(60)` | `energy` propia | carga con escudo (armadura 2) | Sig1 muro de escudo (armadura total 1 s + empuje), Sig2 vendas (proyectil lento que frena: Kb 0, hitstun 0.8) |
   | 30 ELEFANTE | 36 cuadros en 4 golpes | pisoton (`TEMBLOR`, onda r120, low) | embestida (`energy` 3 cuadros + `ESTOCADA(180)`, armadura) | embestida / pisoton bajo | Sig1 estampida (`ESTOCADA(260)` multigolpe 3), Sig2 doble pisoton |
   | 31 AVE | picotazo partido en 2 + 3o aleteo | aleteo (`GIRO(1)`) | especial ampliado con `FX` de curacion | picada (`ESTOCADA(120)` + ang +20) | Sig1 cura +15 % de vida al activar furia (una vez por furia), Sig2 tornado de plumas (area) |
   | 32/33 CENTAURO | 3 estocadas partidas de 25 cuadros | coz (subrango `reverse` + behind) | especial (22) | carga de lanza (`run` + `ESTOCADA(220)`, armadura 1) | Sig1 lluvia de lanzas (3 proyectiles en arco), Sig2 pisoton magico (area r150) |
   | 34/35 BRUTO | mitades del ataque (2) + 3o bola al suelo | bola al suelo (`TEMBLOR`, low, knockdown) | `energy` (51 cuadros) como super largo | arrastre de bola (`ESTOCADA(140)`) | Sig1 remolino de bola (`GIRO(2)`, bothSides), Sig2 lanzamiento de bola (proyectil pesado) |
   | 36/37 MEDUSA | coletazo (atk1) + atk1 rapido (speed 1.3) | ataque 2 | mirada petrificante: ataque 2 + `TINTE` gris al rival y hitstun 1.0 s | coletazo con `ESTOCADA` | Sig1 mirada en cono (area frontal 200 px), Sig2 coletazo giratorio (`GIRO(1)`) |
   Furia = el clip largo original (special/super) con armadura; Finisher = el mismo + `TEMBLOR`. Habilidades 1-5 = L, Sig1,
   Sig2, `>`J y furia corta (no repiten la cadena).
4. Centauros: si el circulo magico es una pieza separable en el cuadro, ocultarla (derivacion); si no, dejarlo y reportarlo.

**Criterios de aceptacion.** `moves_sheet` 27-37: >= 9 movimientos cada uno, 0 `IDLE`, cada golpe de la cadena con subrango
distinto, golpeo = s; `state_strips`: sin dolor tras golpe propio; `gow_check` identico.

**Como se trabaja un personaje (comun a F1.1-F1.8).**
- Tras Fase 0 el motor ya da a TODOS los personajes los ~40 movimientos genericos (V1/V2) con un visual de reserva. Tu
  trabajo: que cada movimiento de TU personaje tenga visual propio (arte existente + transformaciones matematicas), numeros
  ajustados a su peso, cadenas largas, 3 poderes `Signature`, habilidades 1-5 que no repitan normales, y estados (golpe,
  derribo, levantarse, victoria, salto, caminar, bloqueo) que se vean bien. Deshabilita (con motivo escrito) lo que no tenga
  sentido para el personaje.
- Todo se declara en DATOS en tu `src/game/moves/Moves_<Bloque>.cpp`: `const MoveSet* MovesetFor_<Bloque>(int skin)`
  devuelve un `MoveSet` estatico para tus skins (y `nullptr` para los demas). Campos (ver `src/game/moves/MoveTypes.h`):
  `caps`, `feel`, `chainJ/chainK` (hasta 6/4 eslabones; puedes usar `Chain5/Chain6` con override completo), `skillSlots[5]`,
  `jumpVz`, `overrides[]` (por movimiento: `displayName` en espanol, `replaceDef`+`def` o escalas, `arc` (LimbArc), `root`
  (RootCurve), `hitCount`/`multiHitInterval`, armadura, `areaRadius`, `pullPx`, `projectile`, `visual` = clips/segmentos de
  pose + `PoseFx` con presets V6 y claves), y visuales de estado `hit, knockdown, getup, victory, jumpRise, jumpFall, walk,
  block`. Define tambien el visual de `Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack` (se activan en
  Fase 2; deja `enabled` como venga).
- Cajas: el arco (`LimbArc`) debe cubrir el miembro DIBUJADO en el cuadro de impacto (tolerancia 12 px): compruebalo con
  `moves_sheet` (dibuja la capsula) o F9 en juego. Las escalas por tamano (`bodyScale`) y profundidad las aplica el motor.
- Herramientas: `xvfb-run -a ./build/moves_sheet /tmp/<tu_dir> <id>` (6 cuadros por movimiento + `moves.tsv` con el
  origen del clip y si golpeo al muneco) y `xvfb-run -a ./build/state_strips /tmp/<tu_dir> <id>` (estados). Mira las
  imagenes (Read) antes de dar algo por bueno.
- Si el motor no puede expresar algo que necesitas, NO lo cambies: describelo como `PEDIDO AL MOTOR` y usa la mejor
  aproximacion disponible.
- Aceptacion comun: `moves.tsv` sin `IDLE`; cada movimiento habilitado de tu personaje con origen `override` (salvo los que
  justifiques); cada ataque habilitado "golpeo = s" contra el muneco; habilidades 1-5 = movimientos distintos de la cadena;
  numero de movimientos habilitados >= la meta de tu brief; capturas `moves_<id>.png` y `states_<id>.png` en tu informe.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.9 — Enemigos de calle: juggle real, en el suelo/OTG/levantarse, rebotes, 2-3 ataques por tipo, telegrafia

**Parte de:** "base-fase1". Los lanzadores, aereos, pisoton y barridas de todos los personajes dependen de esto.

**Diagnostico** (`.../audit/combate.md` B07, 1.4, 3.7, 3.11): con el resolvedor unico de Fase 0 los enemigos ya reciben
`HitInfo` (`StreetEnemy::ApplyHit`) con hurtbox 3D, pero: cualquier golpe en el aire los relanza sin limite (juggle
infinito), `velocity = knockback` en el aire los saca volando, al caer pasan a `Hit` 0.45 s (no hay estado en el suelo,
OTG ni invulnerabilidad al levantarse), y cada tipo tiene UN ataque (activo 34-74 % de `attackDuration`, caja fija en y-66
sin escala). Arte: 12 cuadros por enemigo (idle 0-3, ataque 4-7, golpe 8-9, muerte 10-11).

**PUEDES TOCAR:** `src/game/StreetEnemy.h/.cpp`, `src/game/enemies/EnemyBrain.h/.cpp`, `src/game/combat/HitResolver.cpp`
(solo lado enemigo: cuerpos lanzados que golpean a otros, OTG, telegrafia), NUEVO `tests/unit/JuggleTests.cpp`,
NUEVO `tests/unit/EnemyAttacksTests.cpp`, NUEVO `tests/tools/EnemyStrips.cpp` (target `enemy_strips`).
**NO TOQUES:** `Player.*`, `PlayerCombat.cpp`, `AttackData.*`, stages, dibujo del jugador, audio (solo EMITES eventos).

**Tareas.**
1. **Estados nuevos** al final de `StreetEnemyState`: `Launched, Down, WakeUp, WallBounce, Grabbed, Thrown` (cubrir los
   `switch`). La IA no decide en Launched/Down/WakeUp/Grabbed/Thrown.
2. **Juggle** con las formulas V8 (presupuesto 6, Brute/Mutant/Armored 4; gravedad creciente; relanzamiento decreciente;
   degradacion de hitstun y escalado de dano por `comboHits`); en el aire el knockback horizontal se limita a
   `min(abs(kb), 180)` (no salen volando 80 px por golpe); hurtbox sube con `z` (ya 3D). `groundBounce` (una vez, vz 300)
   y `wallBounce` (al tocar el borde de arena: `vx = -0.5 vx`, `vz = 260`, estado WallBounce 0.25 s).
3. **Suelo**: al caer -> `Down` 0.55 s (solo golpeable por `otg`, UNA vez por derribo; Sweep deja en Down directo) ->
   `WakeUp` 0.35 s invulnerable -> Idle. Si queda presupuesto agotado -> Down.
4. **Visual procedural sin arte nuevo**: Launched = cuadro 8/9 rotado `angulo = atan2(vz, abs(vx)) x 0.6` (en grados, hacia
   atras); Down = cuadro 10/11 sin desvanecer con `sy 0.55`; WakeUp = 11 -> 10 -> idle; Grabbed = cuadro 8 anclado a la mano
   del agarrador (posicion que le pasa F2.1); Thrown = cuadro 8 con rotacion `540 u` grados.
5. **API para agarres (la usara F2.1)**: `bool CanBeGrabbed() const` (no Brute/Mutant/Armored, no en Attack activo con
   armadura), `void SetGrabbed(bool on, Vector3D hand)`, `float grabEscapeTimer` (escapan a 0.6 s los que tengan armor),
   `void Throw(const HitInfo&, Vector3D vel)`: parabola `x = x0 + vx t`, `z = vz t - g t^2/2`; al tocar suelo dano de
   impacto 8 + Down. Mientras vuela es un "proyectil-cuerpo": golpea a otros enemigos que toque (8 dano, knockdown) — esa
   resolucion va en `HitResolver.cpp` (lado enemigo).
6. **Ataques de datos por perfil** (`EnemyAttackDef` en EnemyBrain: S/A/R en segundos, `LimbArc` escalado por
   `visualScale`, dano, kb, launch, low/high/knockdown/unblockable, root lunge): 2-3 por tipo reutilizando los cuadros 4-7 con
   distinta velocidad, root motion y `sy`:
   | Tipo | Ataque A | Ataque B | Ataque C |
   |---|---|---|---|
   | Punk | jab rapido (S .18) | patada baja (low, `sy .85`) | — |
   | Charger | embestida (existente) | golpe con hombro (lunge 60) | — |
   | Brute | golpe pesado (existente, knockdown) | pisoton (low, area r80) | carga corta (lunge 80, armadura) |
   | Enforcer | porra (existente) | golpe alto (overhead) | contra en guardia (existente) |
   | Chemical soldier | nube (existente) | culatazo corto | — |
   | Urban ninja | flanqueo (existente) | patada voladora (lunge 120, launch 300) | 2 golpes rapidos |
   | Mutant | golpe pesado (existente) | barrido (low, knockdown) | rugido (aturde 0.4 s si cerca) |
   | Armored guard | escudo (existente) | empujon con escudo (kb 520) | — |
   Elegir por distancia/estado del jugador (low si el jugador esta en guardia alta, etc.), con pesos en el perfil.
7. **Telegrafia**: `EmitGameEvent(Telegraph)` cuando `telegraphing` pasa a true (lo usara el audio) y destello rojo 0.15 s
   en el sprite (via tinte existente del enemigo).
8. **Contra (counter-hit)**: el resolvedor ya multiplica; aqui expon `bool InStartup() const` fiable para cada ataque de datos.
9. **Tests**: `JuggleTests`: tras un lanzador, 7 golpes ligeros -> el enemigo cae antes del 7o (presupuesto); OTG solo 1 vez
   por derribo; WakeUp invulnerable; groundBounce una vez; en el aire 5 golpes no lo alejan mas de 200 px; dt 1/30 vs 1/60
   misma altura maxima +-5 %. `EnemyAttacksTests`: cada tipo tiene >= 2 ataques validos; la caja escala con `visualScale`.

**Criterios de aceptacion.** Tests en verde; `stage1_playthrough_test` en verde (reporta tiempo); `all_modes_check` y
`gow_check` `problemas: 0`; capturas con `xvfb-run -a ./build/enemy_strips /tmp/f19` (una tira por tipo: Launched, Down, WakeUp, Thrown).

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.10 — Audio A: motor de mezcla + director de eventos + arquetipos por personaje + OPCIONES

**Parte de:** "base-fase1". En paralelo con F1.11 (banco de sonidos), F1.12 (musica) y F1.13 (BETA). Contrato de nombres de
archivo con F1.11 abajo: si un archivo aun no existe, cae al WAV actual (nunca silencio, nunca crash).

**Contexto.** `src/audio/AudioSystem.*` (raylib 5.5): un `Sound` por Sfx, `Play` reinicia y corta la cola, sin pitch/pan, sin
buses, `fury_charge.wav` y `combat_ko.wav` cargados dos veces (GameOver suena igual que matar a un enemigo), `ui_open.wav`
nunca se carga. Desde Fase 0 el combate ya NO llama a `Play`: emite `GameEvent` (`src/game/events/GameEvents.h`) y
`AudioDirectorUpdate(dt)` (`src/audio/AudioDirector.cpp`, mapeo legado 1:1 hecho por F0.2) los drena desde
`AudioSystem::UpdateAmbient()`. Lee `docs/auditoria_movimientos/audio.md`
§1, §6 y §7 (P0 y P2). Ojo: su §2.2/2.5 sobre "dobles por CombatWorld" era incorrecto (ese codigo estaba muerto); la regla
"un dueno por evento" ya la garantiza la cola de eventos.

**PUEDES TOCAR:** `src/audio/AudioSystem.h/.cpp`, `src/audio/AudioDirector.h/.cpp`, `src/audio/SfxBank.h/.cpp`, `src/main.cpp`
(solo audio), `src/ui/MainMenu.cpp/.h` (OPCIONES: deslizadores + sonidos de UI), `src/game/VSMode.cpp` (SOLO lineas que
emiten eventos de UI en sus menus/seleccion), `src/core/DisplaySettings.*` (solo si ahi se persisten opciones; si no, crea
`src/audio/AudioSettings.*`), NUEVO `tests/unit/AudioMixTests.cpp`.
**NO TOQUES:** `src/audio/MusicPlayer.*` (F1.12; tu solo le llamas `OnEvent`, `Update`, `SetDuck`, `SetVolume`), combate,
stages, `GowApk/**`, assets.

**Tareas (P0 + director).**
1. **Pool de voces**: N alias por sonido con `LoadSoundAlias` (4 golpes/impactos, 2 UI, 1 jingles); `Play` usa un alias libre
   (`!IsSoundPlaying`) o roba el mas viejo. **Limite global** 12 SFX simultaneos con prioridad (0 UI/ambiente, 1
   movimiento/pasos, 2 golpe ligero, 3 golpe pesado/dano al jugador, 4 furia/KO/jefe): si no hay sitio, se descarta el nuevo
   de menor prioridad o se roba la voz de menor prioridad. **Antirrafaga**: mismo Sfx < 30 ms -> no se repite.
2. **Variacion**: `pitchJitter` (+-6 % golpes, +-3 % pesados, 0 UI/jingles) y `volJitter` (+-1.5 dB) con `SetSoundPitch` /
   `SetSoundVolume` sobre el alias; **variantes** por Sfx (lista de archivos) en round-robin aleatorio sin repetir la ultima.
3. **Buses** (`Bus::Master, Sfx, Ui, Voice, Music, Ambient`, contrato de F0.1): `vol = tabla x bus x master`;
   `SetBusVolume/BusVolume` reales (BetaMode y MusicPlayer los leen). **Ducking**: golpe pesado, KO, furia, golpe al jugador
   -> musica y ambiente -6..-9 dB durante 250-400 ms con recuperacion exponencial (`MusicPlayer::Get().SetDuck(g)` + ambiente).
   **Pan** por X del evento: `pan = 0.5 + clamp((x - camX - 640)/1280, -0.35, 0.35)` (si no hay camara accesible, usa x
   relativa al jugador; documenta).
4. **Limitador suave** en el bus maestro con `AttachAudioMixedProcessor`: `tanh(1.2 x)/tanh(1.2)` (barato: corre en el hilo
   de audio de Android; sin asignaciones ni locks).
5. **Normalizacion al cargar**: RMS de ventana 50 ms (st50) por archivo y ganancia hacia objetivo por categoria (golpes -10
   dBFS, UI -16, ambiente -28) en vez de los volumenes a mano. Cargar cada archivo una sola vez (alias).
6. **Director de eventos** (reescribe el mapeo legado de `AudioDirector.cpp`, una tabla clara): cada `GameEventType` ->
   sonido o "mudo intencional", segun `HitKind`, `weight`, `heavy`, `fromEnemy` y el **arquetipo del personaje** (`skin`):
   | Arquetipo | Personajes (por nombre de `CharacterVisual`) | Impactos | Lanzamiento | Especial |
   |---|---|---|---|---|
   | Brawler | RAYDEN (0), KF MATON, KF PELEADOR, KF GORRA, BRAKK (2), CLON ANTIGUO (1) | `combat/hit_*` | `whoosh` por peso | energia |
   | Agil | KF RUBIA, KF BUFONA, KF HEROINA (+TRANSF.), KF HEROE (+TRANSF.), RAYDER CLON (BETA) | ligero + patada pitch 1.12 | whoosh ligero | energia brillante |
   | Cuchillas | KF NAVAJERA, KF CUCHILLERO, KF JEFE GARRA, GUERRERO/RONIN-KRATOS (ESPADAS, GARRAS) | `slash_*` | swish | corte + energia |
   | Pesado | GRINDER (3), TITAN-X (4), TITAN-X MEJORADO (5), KF SOLDADO, bestias BETA, GUERRERO/RK (CESTUS) | pesado pitch 0.85 + sub | whoosh pesado | pesado |
   | Electrico | GUERRERO/RK (RAYO), MAX transformado | energia | zap | energia |
   | MAX (6) | pack propio `rayder_v2/*` (si existe) o `rayder/*` | | `silbido` | `rayder/*` |
   | RONIN (22) | pack propio `ronin_v2/*` o `ronin/*`; dash = `espada_desenvaine` pitch 1.3; usa `salto`/`aterrizaje` | | | `espada_*` |
   Distinguir: dar vs recibir (`PlayerHurt` usa `player_hurt_*`, mas sordo); bloqueo (`block_*`) vs golpe limpio; rotura de
   guardia (`guard_break_1`); disparo enemigo vs propio; `Prop` (madera/metal); KO de enemigo (`ko_enemy_*`) vs GameOver
   (`game_over`, propio) vs StageClear (`stage_clear`); pasos (`step_*`, `step_wet_*` en Stage1 con lluvia) con volumen
   bajo y `weight` 3 = pesado (pitch 0.8); `Telegraph` (`telegraph_1`, -12 dB); `ComboMilestone` (`combo_10/20/30`);
   pickups (`pickup_health`, `pickup_energy`); jefe (`boss_hit_*`, `boss_phase_1`). Reenviar a `MusicPlayer::Get().OnEvent(e)`
   los eventos `ModeEnter, ArenaLock, ArenaUnlock, BossPhase, Victory, GameOver, StageClear` y llamar `MusicPlayer::Get().Update(dt)`.
   El ambiente (lluvia) suena solo en Menu y Stage1 (segun `ModeEnter`); los demas modos sin lluvia (F1.12 pone su propio
   ambiente si lo hay).
7. **OPCIONES** (MainMenu): deslizadores MUSICA, EFECTOS, AMBIENTE (0-100 %, paso 10) + el mute global existente; persisten
   entre sesiones con el mecanismo de ajustes que ya use el juego; sonidos de UI (`UiNavigate/Confirm/Cancel/Open`) en menu
   principal, OPCIONES, seleccion de VS y pantalla de recompensa (via eventos o llamadas directas a la API de UI de audio).
8. **Tests** `AudioMixTests` (sin dispositivo de audio: separa la logica de seleccion de voz/prioridad/antirrafaga/variantes
   en funciones puras y testea esas): 20 `Play` del mismo golpe en 10 ms -> 1 sonido; 16 golpes simultaneos -> 12 voces y las
   de prioridad 4 sobreviven; variantes nunca repiten la anterior; pan dentro de [0.15, 0.85]; ducking vuelve a 1.0 en < 0.6 s.

**Contrato de nombres con F1.11** (mono, 44.1 kHz, 16 bit, en `assets/audio/sfx/`): `combat/hit_light_1..4`,
`combat/hit_medium_1..4`, `combat/hit_heavy_1..3`, `combat/kick_1..3`, `combat/slash_1..3`, `combat/energy_1..3`,
`combat/block_1..3`, `combat/guard_break_1`, `combat/player_hurt_1..3`, `combat/whoosh_light_1..3`, `combat/whoosh_heavy_1..3`,
`combat/jump_1`, `combat/land_light_1`, `combat/land_knockdown_1`, `combat/step_1..4`, `combat/step_wet_1..4`,
`combat/ko_enemy_1..2`, `combat/game_over`, `combat/stage_clear`, `combat/telegraph_1`, `combat/boss_hit_1..2`,
`combat/boss_phase_1`, `combat/pickup_health`, `combat/pickup_energy`, `combat/combo_10`, `combat/combo_20`, `combat/combo_30`,
`combat/prop_wood_1..2`, `combat/prop_metal_1..2`, `combat/energy_charge_1`, `combat/energy_shot_1..2`,
`combat/energy_impact_1..3`, `combat/rage_start`, `combat/transform`, `combat/ui_select_char`, `combat/ui_deny`;
`arquetipos/<agil|cuchillas|pesado|electrico>/<mismo nombre>` (opcionales, pisan al generico); `rayder_v2/<nombres de rayder/>`,
`ronin_v2/<nombres de ronin/>`. Fallback si falta: el WAV actual equivalente (`combat_punch_light`, etc.).

**Criterios de aceptacion.** Tests en verde; en `all_modes_check` (o un log de depuracion activable) cuenta de eventos por tipo
y sonidos disparados: ningun evento de combate queda sin sonido salvo los marcados "mudo intencional"; max voces simultaneas
medido <= 12; sin crash si faltan todos los archivos nuevos; tabla final evento -> sonido en tu informe.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.11 — Audio B: banco de sonidos de combate sintetizado (original, apto para altavoz de movil)

**Parte de:** "base-fase1". En paralelo con F1.10 (que cargara estos archivos por nombre; contrato abajo).

**Contexto** (`.../audit/audio.md` §3 y §7-P1): el "starter pack" (`assets/audio/sfx/combat_*`, `move_*`, `fury_*`) tiene
centroide ~13 kHz y < 1 % de energia entre 500 Hz y 3 kHz (siseo sin cuerpo; en movil suena a "tsss"), y 5 archivos recortan;
los packs `rayder/*` (MAX) y `ronin/*` son 96-98 % graves 120-500 Hz y varios tienen > 60 % bajo 120 Hz (no se oyen en movil).
Hay un generador de estilo a seguir: `tools/audio/genera_sonidos_rayder.py` (numpy, semilla fija, WAV 16 bit 44.1 kHz).

**PUEDES TOCAR:** NUEVOS `tools/audio/genera_banco_combate.py`, `tools/audio/analiza_banco.py`, `tools/audio/mejora_medios.py`;
NUEVAS carpetas `assets/audio/sfx/combat/`, `assets/audio/sfx/arquetipos/{agil,cuchillas,pesado,electrico}/`,
`assets/audio/sfx/rayder_v2/`, `assets/audio/sfx/ronin_v2/`; `assets/audio/LEEME_audio.txt` (agregar seccion).
**NO TOQUES:** los WAV existentes (`assets/audio/sfx/*.wav`, `rayder/`, `ronin/`: se conservan como respaldo), codigo C++,
`GowApk/**`.

**Reglas de sintesis.** Todo generado por codigo (sin muestras externas, sin grabaciones, sin material con copyright).
Semilla fija por archivo; mono; 44.1 kHz; 16 bit; pico -3 dBFS (nunca recorta); paso alto 20 Hz (sin DC); fundido de 2 ms al
final; silencio inicial < 2 ms. **Regla movil**: cada golpe con energia real entre 300 Hz y 4 kHz (cuerpo + chasquido): >= 25 %
de la energia en 0.5-3 kHz; el subgrave es un extra. Tamano total del banco < 3 MB.

**Recetas** (capas -> envolvente; duraciones aproximadas):
| Archivo(s) | Receta |
|---|---|
| `hit_light_1..4` (90-120 ms) | chasquido: ruido blanco paso banda 2-6 kHz, 4 ms; cuerpo: seno+triangulo 420->180 Hz (barrido exponencial 30 ms, tau 35 ms); "carne": ruido rosa paso banda 300-1200 Hz tau 40 ms; `tanh(2.5x)` suave. Variantes: semillas y +-8 % de frecuencias |
| `hit_medium_1..4` (130-170 ms) | lo anterior + golpe grave seno 160->70 Hz tau 60 ms; chasquido 1.5-5 kHz |
| `hit_heavy_1..3` (250-320 ms) | chasquido 0.8-8 kHz + cuerpo 260->60 Hz + sub 90->40 Hz + "crunch" granular (30-60 granos/s, paso banda 1-3 kHz) + cola de reverb corta (convolucion con ruido decayente 120 ms) + saturacion |
| `kick_1..3` (160-200 ms) | pre-whoosh 40 ms (ruido paso banda 800->2500 Hz) + impacto medio con cuerpo 1/3 mas grave + "tela" (ruido 3-7 kHz tau 20 ms) |
| `slash_1..3` (200-300 ms) | swish (ruido paso banda 6->2 kHz, Q alto, 60 ms) + ring metalico (parciales 1.9/3.1/4.7/6.2 kHz, tau 80-150 ms) + carne 300-900 Hz |
| `energy_1..3`, `energy_impact_1..3`, `energy_shot_1..2`, `energy_charge_1` | zap: sierra 1200->200 Hz con FM (moduladora 60-90 Hz, indice alto) + crackle (impulsos de Poisson filtrados 2-9 kHz) + golpe grave 150->50 Hz; la carga: sierra ascendente 200->900 Hz 0.6 s con tremolo creciente |
| `block_1..3` (70-90 ms) | "tok": senos 900 + 1400 Hz tau 15 ms + ruido 1-3 kHz 10 ms + ring 2.2 kHz corto; sin subgrave |
| `guard_break_1` (400 ms) | 3 "tok" en cascada (0/25/55 ms) bajando de tono + golpe pesado + vidrio (ruido paso alto 5 kHz granulado) |
| `player_hurt_1..3` (150 ms) | cuerpo medio + "thud" sordo (paso bajo 2 kHz): debe distinguirse de DAR un golpe |
| `whoosh_light_1..3` / `whoosh_heavy_1..3` | ruido rosa paso banda con centro 600->2400->900 Hz en campana; ligero 120 ms Q medio, pesado 250 ms Q bajo |
| `jump_1`, `land_light_1`, `land_knockdown_1` | whoosh ascendente 500->1500 Hz + ropa (2-5 kHz) / tap 200-800 Hz + polvo (paso alto 3 kHz tau 60 ms) / cuerpo 180->60 Hz + rebote a -8 dB a los 90 ms + polvo |
| `step_1..4`, `step_wet_1..4` (60 ms, -14 dB) | tap paso banda 150-600 Hz + roce 2-4 kHz; mojado: + chapoteo (ruido 1-3 kHz con barrido rapido) |
| `ko_enemy_1..2` (600 ms) | golpe pesado + "cuerpo al suelo" a los 180 ms + swell de 2 notas a -10 dB |
| `game_over` (2 s) | acorde menor descendente con sierras desafinadas filtradas 220->110 Hz, cola 1.5 s |
| `stage_clear` (1.5 s) | arpegio mayor rapido 1-3-5-8 con pulso + campana FM y cola |
| `telegraph_1` (200 ms, -12 dB) | seno 2.5 kHz con vibrato rapido y fade-in 150 ms |
| `boss_hit_1..2`, `boss_phase_1` | pesado con tono al 70 % + sub / rugido (ruido marron paso banda 100-600 Hz con AM 30 Hz) + impacto + ring metalico grave |
| `pickup_health`, `pickup_energy`, `combo_10/20/30` | dos notas ascendentes / brillo FM / ping FM que sube un semitono por hito |
| `prop_wood_1..2`, `prop_metal_1..2` | madera: senos 300-700 Hz inarmonicos tau 40 ms + crujido; metal: parciales inarmonicos 1-5 kHz tau 300 ms |
| `rage_start`, `transform`, `ui_select_char`, `ui_deny` | swell + zap grave / estallido brillante con cola / golpe + metal / dos tonos bajos |
Arquetipos (`arquetipos/agil/*`, `cuchillas/*`, `pesado/*`, `electrico/*`): mismas recetas variando: agil +12 % tono y mas
chasquido; cuchillas = `slash` en vez de `hit`; pesado -15 % tono + sub; electrico = capa de crackle sobre los golpes. Al menos
`hit_light/medium/heavy`, `kick`, `whoosh` por arquetipo.
**MAX y RONIN v2** (`tools/audio/mejora_medios.py`): lee `rayder/*.wav` y `ronin/*.wav`, anade un chasquido sintetico de 3 ms
(2-5 kHz), realce +4 dB en 1.5-3 kHz y saturacion armonica (`tanh`) para que el sub genere armonicos audibles; escribe con los
MISMOS nombres en `rayder_v2/` y `ronin_v2/`. Conserva su caracter (comparalo a oido con el original y reportalo).

**`analiza_banco.py`**: por archivo dur, pico, rms, st50, muestras recortadas, silencio inicial, centroide y % de energia por
bandas (<120, 120-500, 500-3k, >3k); falla (exit 1) si algun golpe incumple la regla movil, recorta, tiene pico > -1 dBFS o
silencio inicial > 2 ms; resumen de spread de st50 por categoria (objetivo <= 4 dB dentro de cada categoria).

**Criterios de aceptacion.** `python3 tools/audio/genera_banco_combate.py && python3 tools/audio/mejora_medios.py &&
python3 tools/audio/analiza_banco.py assets/audio/sfx/combat assets/audio/sfx/arquetipos assets/audio/sfx/rayder_v2 assets/audio/sfx/ronin_v2`
-> exit 0; regenerar dos veces da archivos identicos (md5); todos los nombres del contrato existen; tabla de analisis en el
informe. (Los checks C++ del anexo deben pasar igual: no tocaste codigo.)

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.12 — Audio C: musica original adaptativa por modo (sintetizada, pre-renderizada a OGG)

**Parte de:** "base-fase1". En paralelo con F1.10 (el director te reenvia eventos) y F1.11.

**Contexto.** El juego principal no tiene musica: solo `world_rain_loop.wav` (2 s en bucle, en todos los modos) y el jingle de
arranque `intro_arranque.wav` (original, `tools/audio/genera_intro_ronin.py`: reutiliza su estilo/receta de percusion). La
regla del usuario prohibe musica con copyright: TODO se compone por algoritmo, con semilla fija, sin citar melodias
existentes ni usar muestras externas. `ffmpeg` con `libvorbis` esta disponible; raylib decodifica OGG.

**PUEDES TOCAR:** `src/audio/MusicPlayer.h/.cpp` (contrato de F0.1: `Get()`, `OnEvent(const GameEvent&)`, `Update(dt)`,
`SetDuck(gain)`, `SetVolume(v)`; puedes AGREGAR metodos), NUEVOS `tools/audio/genera_musica.py`, `assets/audio/music/*.ogg`,
`assets/audio/ambient/*.ogg|wav`, NUEVO `tests/unit/MusicPlayerTests.cpp`, NUEVO `assets/audio/music/LEEME_musica.txt`
(NO edites `assets/audio/LEEME_audio.txt`: es de F1.11).
**NO TOQUES:** `AudioSystem.*`, `AudioDirector.*` (F1.10: ellos llaman a tu `OnEvent/Update/SetDuck/SetVolume` y te dan el
volumen de bus `BusVolume(Bus::Music)` / `Bus::Ambient`), combate, stages, `GowApk/**` (su musica es de F1.13).

**Tareas.**
1. **Generador** `genera_musica.py` (numpy; sintesis sustractiva/FM propia; mezcla estereo; render WAV temporal en `/tmp` ->
   `ffmpeg -c:a libvorbis -q:a 3`). Bucles de 32-64 compases con el punto de union en el limite de compas (sin click: union
   con crossfade de 10 ms medido en el render). Mismo tempo y tonalidad en las capas de un tema.
   | Archivo | Pieza | Caracter |
   |---|---|---|
   | `menu.ogg` | Menu | darksynth La menor 96 BPM, pad de sierras desafinadas con paso bajo lento, arpegio de 16avos, bombo suave |
   | `stage1_calma.ogg` + `stage1_combate.ogg` | Calles con lluvia | 120 BPM, bajo de sierra en semicorcheas, caja con reverb, lead de cuadrada con portamento; capa combate: hi-hats + bajo saturado |
   | `stage2..5_{calma,combate}.ogg` | Industrial / puerto / azotea / guarida | misma paleta, otra tonalidad y textura, 110-140 BPM; stage2 con drone industrial |
   | `boss_a.ogg` + `boss_b.ogg` | Jefe | 140 BPM, percusion tipo taiko (receta del intro), ostinato de bajo cromatico, stabs de "bronces" de sierra; la capa B entra en `BossPhase` |
   | `vs.ogg` | VS | 150 BPM, corto y energico |
   | `victoria.ogg`, `derrota.ogg` | Stingers 3-5 s | sin bucle |
   Tamano total <= 12 MB. Normalizacion: integrado aprox -16 LUFS (o RMS equivalente), pico -1 dBFS.
2. **Reproductor**: `PlayMusic(id)` con crossfade de 1 s; capas sincronizadas (calma/combate o A/B) reproducidas a la vez
   con volumen cruzado 1.0 s al recibir `ArenaLock` (entra combate) / `ArenaUnlock` (vuelve calma) / `BossPhase` (capa B);
   `ModeEnter` elige tema; `Victory/StageClear` -> stinger y luego silencio o menu; `GameOver` -> `derrota`. Volumen =
   `SetVolume(bus musica x master)` x `duck` (de F1.10). `Update(dt)` llama `UpdateMusicStream` de cada stream activo.
   Ambiente por modo: lluvia solo en Menu y Stage1 (ya la gestiona AudioSystem; si quieres ambientes nuevos por stage
   — p.ej. stage2 = `world_city_industrial` filtrado — exponlos aqui y documenta a F1.10 como pedirlos).
3. **Rendimiento Android**: como mucho 2 streams activos + 1 en fundido; buffers por defecto de raylib.
4. **Test** `MusicPlayerTests` (logica pura, sin dispositivo): maquina de estados (ModeEnter -> tema; ArenaLock -> capa
   combate con rampa de 1 s; BossPhase -> capa B; Victory -> stinger); los nombres de archivo resueltos existen en disco.

**Criterios de aceptacion.** `python3 tools/audio/genera_musica.py` regenera todo de forma determinista (md5 iguales dos veces);
cada bucle sin click en la union (diferencia de muestra en la union < -40 dBFS); tamanos dentro del limite; en el juego,
cada modo suena con su tema (escribe un log de depuracion `MUSICA: <tema>` activable) y el combate de arena cambia de capa.
En el informe: tabla de piezas (BPM, tonalidad, duracion, MB) y confirmacion de originalidad (todo generado, sin muestras).

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F1.13 — Audio D: modo BETA (GowApk) sin copyright y con sonido que funcione

**Parte de:** "base-fase1". Prioridad tan alta como el motor: hoy viola una regla del usuario.

**Contexto** (`.../audit/audio.md` §4-§5, verificado): `GowApk/src/BetaMode.cpp` reproduce `GowApk/assets/audio/<name>.ogg`
(`PlaySound`, ~linea 521) y musica (`PlayMusic`: `menu.ogg`, `gate1music.ogg`, `gate2music.ogg`, `gameCG.ogg`,
`music_001.ogg`, `music_002.ogg` victoria, `music_003.ogg` derrota; los stages de `GowApk/data/stages/*.txt` eligen gate1/gate2).
Esa musica y los `soundNN.ogg` son audio EXTRAIDO del juego original (copyright). 17 `.ogg` son stubs mudos de 23 ms (md5
`7ddad058...`): `sound8, 22, 23, 24, 28, 30, 41, 44, 45, 49, 50, 53, 54, 57, 58, 71, 72` (el codigo usa `sound24`, `44`, `45`).
Los sonidos de Kratos/RONIN-KRATOS (`sound3, 4, 34, 36, 37, 38, 61-65, 70, 73-75`) solo estan en `assets/kratos/audio/`, que
BetaMode no busca: no suenan. Los `data/kratos/*.txt` repiten cada `clipsound` (se dispara 2 veces el mismo frame; hasta 8
veces en `ab1`). Nombres referenciados hoy por codigo/datos: `sound2, 3, 4, 6, 13, 14, 24, 25, 31, 32, 34, 36-40, 44, 45, 48,
51, 52, 55, 56, 59-70, 73-75` (vuelve a sacarlos con `grep -rhoE "sound[0-9]+" GowApk/src GowApk/data data/kratos | sort -u`).

**PUEDES TOCAR:** `GowApk/src/BetaMode.cpp/.h` (solo audio), `GowApk/assets/audio/*.ogg` (reemplazar contenido con los MISMOS
nombres y agregar los que falten), NUEVO `tools/audio/genera_beta_audio.py`, NUEVO `GowApk/assets/audio/PROCEDENCIA.txt`.
**NO TOQUES:** `data/kratos/**` (congelado: md5), `assets/kratos/**` (incluido `assets/kratos/audio/`: su borrado es decision
del usuario), `GowApk/data/**`, codigo de District Fury, `src/audio/*` (solo LEES la API de buses: `AudioSystem::Get().BusVolume(Bus::Music)` etc.).

**Tareas.**
1. **Musica original** (reemplazo 1:1, mismos nombres): estilo "epico griego" compuesto por algoritmo — modo frigio, drones de
   quinta, percusion tribal (receta de taiko de `tools/audio/genera_intro_ronin.py`), coro sintetizado (formantes "aah": 3 filtros
   paso banda sobre una sierra), lira (pulsada Karplus-Strong). `menu.ogg` 32-48 s bucle, `gate1music.ogg` y `gate2music.ogg`
   (distinta tonalidad/tempo, 30 s bucle), `gameCG.ogg` (cinematico 17 s), `music_001.ogg` (35 s), `music_002.ogg` victoria
   (8 s), `music_003.ogg` derrota (7 s). Sin muestras externas. OGG `-q:a 3`.
2. **SFX originales** con los mismos nombres para TODOS los `soundNN` referenciados (y los 17 stubs): identifica que es cada
   uno por su uso (busca en `BetaMode.cpp` y en los `clipsound`/`sound hurt|die|dash` de los `.txt`: golpe de arma, dash,
   dolor, muerte, UI, rugido de jefe...) y sintetiza con recetas de golpe/corte/energia/whoosh (mismo estilo que el banco de
   F1.11: chasquido 2-6 kHz + cuerpo 300-1200 Hz + sub opcional; pico -3 dBFS; sin silencio inicial). Los de Kratos (`sound3,
   4, 34, 36-38, 61-65, 70, 73-75`) se GENERAN en `GowApk/assets/audio/` (asi BetaMode los encuentra sin tocar rutas).
3. **Procedencia verificable**: `genera_beta_audio.py` escribe todos los archivos de forma determinista y registra
   `nombre -> md5 -> receta` en `PROCEDENCIA.txt`; `genera_beta_audio.py --verificar` recalcula y falla si algun `.ogg` de
   `GowApk/assets/audio/` no coincide con lo generado (asi nadie vuelve a colar el audio original).
4. **BetaMode**: deduplicar en `PollSounds` (mismo sonido en el mismo frame del mismo personaje = 1 vez); volumen de musica y
   SFX multiplicado por `BusVolume(Bus::Music)`/`BusVolume(Bus::Sfx)` x master y respeta `IsMuted()`; cache sin cambios.
5. **Android**: confirma leyendo `tools/android/build_apk.sh` que `GowApk/assets/audio` entra en el APK de GowApk (lineas ~64);
   no lo edites; reportalo.
6. **Decision del usuario (no ejecutar)**: lista en tu informe los archivos con copyright que quedan en el repo y no carga
   nadie (`assets/kratos/audio/*.ogg`, `apk_reference/king_fighter_iii/bin/music/*`) con el comando exacto para borrarlos.

**Criterios de aceptacion.** `python3 tools/audio/genera_beta_audio.py --verificar` exit 0; ningun `.ogg` de
`GowApk/assets/audio/` con md5 igual al de la base (`git show $BASE:<ruta> | md5sum`) — todos reemplazados; 0 stubs de 23 ms;
`gow_check` `problemas: 0` y en su log (o en un log de depuracion activable) se ve que suenan los sonidos de Kratos; el
`check_kratos_md5.py` da `distintos: 0`.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F2.1 — Defensa y agarre: JustDefend, contraataque, bloqueo direccional, blockstun, tech roll, ataque al levantarse, agarres y lanzamientos

**Parte de:** "base-fase2" (Fase 0 y Fase 1 integradas; usa la API de agarre de enemigos de F1.9 y los visuales de
`Grab/GrabKnee/ThrowForward/ThrowBack/GuardCounter/WakeupAttack` que cada personaje definio en su `MoveSet`).

**PUEDES TOCAR:** `src/game/Player.h/.cpp`, `src/game/player/PlayerCombat.cpp`, `src/game/player/PlayerCommon.h`,
`src/game/combat/AttackData.cpp` (solo filas de los 6 ids de agarre/defensa), `src/game/combat/HitResolver.cpp`,
`src/game/StreetEnemy.cpp` (solo si la API de F1.9 necesita un arreglo puntual: documentalo), NUEVO
`tests/unit/DefenseGrabTests.cpp`, `tests/InputBufferTests.cpp`/`tests/CombatSmokeTests.cpp` solo si un cambio lo exige.
**NO TOQUES:** `src/game/moves/Moves_*.cpp` (si un personaje no tiene visual de agarre, usa el de reserva y reportalo), dibujo,
audio, stages, GowApk.

**Tareas** (numeros en frames a 60 fps):
1. **Bloqueo direccional** (B20): solo bloquea si el atacante esta delante `(ax - px) * facing > -10`; por la espalda entra
   el golpe. **Blockstun** por ataque (`def.blockstun`; enemigos 0.18-0.30 s) y **pushback** (`selfPushOnBlock` al atacante).
   No se puede salir del bloqueo y atacar en el mismo frame (1 f minimo).
2. **JustDefend**: B pulsado <= 6 f antes del impacto -> 0 dano de escudo, +10 escudo, blockstun 0, flash azul, evento
   `PlayerBlocked` con `value = 1` (el audio lo distingue), hitstop 0.10 s.
3. **GuardCounter** (CONTRA): J+K durante blockstun, cuesta 25 SP o 30 de escudo; 4/6/18, 12 dano, Kb 500, invuln f1-8.
4. **TechRoll**: Shift en los 6 f antes/despues de tocar suelo tras `Airborne` -> rueda 90 px por el carril (direccion del
   stick, por defecto atras), sin knockdown, invuln durante el giro. **WakeupAttack**: J en los ultimos 0.2 s del getup ->
   barrido de levantarse 8 dano, low, invuln f1-6.
5. **Agarre** (V2): caminar contra un enemigo en Idle/Hit/Stun con `CanBeGrabbed()` a < 34 px del borde de su hurtbox,
   mismo carril abs(dy) < 16, durante 8 f; o J+K a la vez cerca. Estado de agarre 1.2 s max (el enemigo con armor escapa a
   0.6 s). Mano del jugador `hand = Pw + s (facing 30, -10)` -> `SetGrabbed(true, hand)` cada frame. Dentro: J = GrabKnee
   (x3 max, 6 dano, 0.32 s c/u), K o `>`K = ThrowForward (22 dano, `Throw` con parabola de 160 px de alcance, knockdown; el
   cuerpo golpea a otros enemigos), `<`K = ThrowBack (24 dano, suplex por encima del hombro: el enemigo aterriza detras).
   Recibir un golpe durante el agarre lo suelta (salvo superarmadura de habilidades).
6. **Tests** `DefenseGrabTests`: golpe por la espalda con B no se bloquea; JustDefend en ventana y fuera; GuardCounter cuesta y
   sale en blockstun; TechRoll evita el knockdown; agarre por caminar 8 f; 3 rodillazos y lanzamiento: el enemigo cae a la
   distancia esperada +-20 px y golpea a un segundo enemigo en la trayectoria; Brute no es agarrable.

**Criterios de aceptacion.** Tests en verde; `moves_sheet` muestra los 6 movimientos habilitados en los personajes con
`CapGrab`; `all_modes_check`/`gow_check` `problemas: 0`; `stage1_playthrough_test` en verde (tiempo reportado).

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F2.2 — IA de rivales y jefes que usa el moveset nuevo

**Parte de:** "base-fase2".

**Contexto.** `src/game/RivalAI.*` + `src/game/RivalAIProfile.*` (commit 851c73b "IA 2.0": cada rival pelea segun su perfil)
manejan al rival de VS reutilizando `Player` con `scriptedInput`; hoy su vocabulario es J J J K con pausas de 0.16 s y
habilidades. `all_modes_check` exige que los rivales {7, 8, 18, 19, 6, 21, 2} usen >= 1 habilidad, golpeen al jugador y
reciban dano en 25 s (semilla fija). Los jefes (`src/game/combat/Boss.cpp`, `BossDefinition.cpp`) tienen sus propios ataques.

**PUEDES TOCAR:** `src/game/RivalAI.h/.cpp`, `src/game/RivalAIProfile.h/.cpp`, `src/game/combat/BossDefinition.cpp/.h` (solo
datos de ataque/IA de jefes), `src/game/combat/Boss.cpp` (solo IA, no dibujo), NUEVO `tests/unit/RivalAITests.cpp`.
**NO TOQUES:** `Player.*`, `PlayerCombat.cpp`, `src/game/moves/*`, stages, dibujo, audio.

**Tareas.**
1. **Rutas por personaje**: la IA lee `GetMoveSet(skin)` (cadenas, capacidades, Signature, habilidades) y elige entre rutas
   que el personaje SI puede hacer: cadena J completa, J J K + persecucion aerea (si `CapJump`), K K K, `v`K -> pisoton, `>`J
   para cerrar distancia, `<`J/`<`K si el jugador esta detras, especial a media distancia (QCF/DP/QCB por `scriptedInput`
   con la secuencia de direcciones frame a frame), Signature con SP suficiente, agarre si `CapGrab` y el jugador esta pegado,
   bloqueo y JustDefend con probabilidad por perfil, GuardCounter, TechRoll.
2. **Perfiles**: agresividad, alcance preferido, ratio de bloqueo, gusto por aereos/agarres/proyectiles por personaje (MAX y
   KF heroes aereos; Titan-X/Brakk/Grinder pesados y de agarre; NAVAJERA rapida; MOMIA defensiva). Dificultad escalada por
   ronda o stage.
3. **Jefes**: los Titan-X (jefes de Stage3/Stage4) y demas usan 1-2 de los movimientos nuevos que su arte permite (p.ej. Titan-X
   jefe: Golpe sismico con onda y Ariete con armadura), con telegrafia (`EmitGameEvent(Telegraph)`).
4. **Test** `RivalAITests` (sin ventana si es posible): con una semilla, en 30 s simulados cada perfil ejecuta >= 4 movimientos
   distintos y >= 1 especial/Signature; ninguna entrada pide un movimiento deshabilitado.

**Criterios de aceptacion.** `all_modes_check` `problemas: 0` con los mismos requisitos (>= 1 habilidad, golpea y es golpeado
en 25 s); en un log de depuracion de VS (activable) se ven >= 6 movimientos distintos por rival en 60 s; `gow_check` igual.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

### BRIEF F2.3 — HUD de combate y LISTA DE MOVIMIENTOS en pausa (para que el jugador descubra todo lo nuevo)

**Parte de:** "base-fase2".

**Objetivo.** Con ~30 movimientos por personaje, el jugador necesita verlos: lista de movimientos por personaje en la pausa,
contador de combo con escalado visible, numeros de dano, avisos ("CONTRA!", "JUGGLE x5", "PERFECTO" del JustDefend, nombre del
especial al lanzarlo).

**PUEDES TOCAR:** `src/ui/GameHUD.h/.cpp`, NUEVOS `src/ui/MoveListScreen.h/.cpp`, `src/game/Stage1StoryGame.cpp` y
`src/game/stage1/Stage1Draw.cpp` (solo menu de pausa + llamada al HUD), `src/game/VSMode.cpp` (solo pausa + HUD),
`src/game/Stage2Game.cpp`..`Stage5Game.cpp` (solo pausa + HUD, si tienen), `src/ui/TouchControls.cpp` (solo boton/area para
abrir la lista en pausa, sin mover botones de combate), `src/ui/SpanishText.h`, NUEVO `tests/unit/MoveListTests.cpp`.
**NO TOQUES:** combate, `src/game/moves/*` (solo LEES `displayName`, entradas y capacidades), audio, GowApk.

**Tareas.**
1. **Lista de movimientos** (`MoveListScreen`): desde la pausa (opcion "MOVIMIENTOS"); paginas: Cadenas, Direccionales, Aire,
   Especiales/Signature (con coste de SP), Furia, Defensa/Agarre, Habilidades 1-5. Cada fila: nombre en espanol
   (`displayName`) + entrada con iconos (`>` `<` `v` J K L, QCF/DP/QCB dibujados como flechas) + dano base. Solo movimientos
   habilitados para ESE personaje. Navegable con teclado y tactil.
2. **HUD**: contador "N GOLPES" con dano acumulado y color segun escalado de juggle; numeros de dano flotantes en el punto de
   contacto (lee eventos `HitLanded/CounterHit` o el `HitReport`; sin tocar el resolvedor); avisos breves "CONTRA!",
   "PERFECTO", "JUGGLE xN", nombre del Signature/especial 0.8 s.
3. **Test** `MoveListTests`: para los 42 personajes la lista se genera sin crash, sin entradas duplicadas, y solo con
   movimientos habilitados; el texto cabe en su caja (ancho medido con la fuente por defecto <= ancho de columna).

**Criterios de aceptacion.** Capturas `/tmp/f23/movelist_<id>.png` de 0, 4, 6, 7, 22, 23; HUD visible en una captura de
combate; checks del anexo en verde.

**Vocabulario comun (V1-V9)**

**V1. AttackIds nuevos (se agregan AL FINAL de `enum class AttackId`, despues de `Skill` y antes de `Count`, en ESTE orden
exacto; los valores historicos 0-9 no cambian):**
`PunchHook, Kick2, Kick3, KickLauncher, FwdPunch, FwdKick, BackElbow, SpinKick, Sweep, Uppercut, Stomp, DashKick,
AirPunch, AirPunch2, AirKick, DiveKick, AirSpike, RisingUppercut, Hurricane, DrillKick, Signature1, Signature2, Signature3,
Grab, GrabKnee, ThrowForward, ThrowBack, GuardCounter, WakeupAttack, Chain5, Chain6` (31 nuevos; `Count` = 41).
`Punch3` sigue siendo el REMATE (ahora 4o golpe). `Signature1-3`, `Chain5`, `Chain6` no tienen datos genericos: solo
existen si el `MoveSet` del personaje los define completos. `Grab..WakeupAttack` se activan en Fase 2.
Estados nuevos de jugador (al final de `PlayerState`): `Jump, Victory`. De enemigo (al final de `StreetEnemyState`):
`Launched, Down, WakeUp, WallBounce, Grabbed, Thrown`.

**V2. Mapa de entradas (direccion RELATIVA al facing: `>` adelante, `<` atras, `v` = abajo puro (S sin horizontal)):**
| Contexto | Entrada | Movimiento |
|---|---|---|
| Suelo | J, J J, J J J, J J J J (hasta 6) | cadena J del personaje (`MoveSet.chainJ`, por defecto Punch1, Punch2, PunchHook, Punch3) |
| Suelo | K, K K, K K K (hasta 4) | cadena K (`MoveSet.chainK`, por defecto Kick, Kick2, Kick3) |
| Suelo | J J K (K tras el 2o golpe J) | KickLauncher (lanzador; persecucion en el aire si conecta) |
| Suelo | `>`J / `>`K / `<`J / `<`K | FwdPunch / FwdKick / BackElbow (sin girar) / SpinKick |
| Suelo | `v`J / `v`K | Uppercut (o Stomp si hay enemigo en `Down` a <= 70 px delante y abs(dy) <= 20) / Sweep |
| Dash | Shift+J / Shift+K / `<`+Shift | DashAttack / DashKick / back-dash (0.20 s, -110 px, invuln 0.12 s) |
| Aire | I (tecla) o boton SALTO; luego J, J J, K, `v`K, `>`J | salto; AirPunch, AirPunch2, AirKick, DiveKick, AirSpike |
| Comando | QCF+J (o L, ESPECIAL, habilidad 1) / DP+K / QCB+K / QCF+K / QCB+J / `>`L / `v`L | EnergyWave / RisingUppercut / Hurricane / DrillKick / Signature1 / Signature2 / Signature3 |
| Furia | Espacio con barra llena; Espacio en furia; K en furia con combo >= 4 y SP >= 35 | entrar en furia; RageAttack; Finisher |
| Habilidades | 1-5 / 6 | `MoveSet.skillSlots[0..4]` (no repiten normales) / transformar |
| Defensa (Fase 2) | B pulsado <= 6 f antes del impacto; J+K en blockstun; Shift al tocar suelo tras derribo; J al final del getup | JustDefend; GuardCounter (25 SP); TechRoll; WakeupAttack |
| Agarre (Fase 2) | caminar contra enemigo (< 34 px, abs(dy) < 16, 8 f) o J+K cerca; luego J / K o `>`K / `<`K | Grab; GrabKnee (x3); ThrowForward; ThrowBack |
Prioridad al resolver una pulsacion: comando especial > modificador direccional > cadena. Un movimiento sin soporte en el
personaje (mascara de capacidades) cae al movimiento base (p.ej. `>`J -> J).

**V3. Frames y unidades.** S/A/R = startup/active/recovery en frames a 60 fps (1 f = 1/60 s). Distancias en px de pantalla
para un personaje "estandar" de 112 px de alto; todo se multiplica por `bodyScale = (alturaSprite/112) * DepthScaleFor(y)`.
`Kb` = knockback px/s, `Lch` = launch px/s (altura alcanzada = Lch^2 / 3000 con g = 1500).

**V4. Curvas (u en [0,1]):** `linear u`; `inQuad u^2`; `outQuad 1-(1-u)^2`; `outCubic 1-(1-u)^3`; `inOutSine -(cos(pi u)-1)/2`;
`outBack(s=1.70158) 1+(s+1)(u-1)^3+s(u-1)^2`; `bezier(p1,p2) 3(1-u)^2 u p1 + 3(1-u) u^2 p2 + u^3`.

**V5. Arco del miembro (LimbArc) — la caja "matematica".** Por movimiento: pivote `P=(px,ph)` (hombro o cadera, px desde
los pies, ph negativo hacia arriba), longitud `L0->L1`, angulo `theta0->theta1` (grados; 0 = horizontal al frente,
positivo = arriba), radio `r`. Con `u = clamp((t - startup)/active, 0, 1)` y `e = ease(u)` (por defecto outCubic):
`theta = theta0 + (theta1-theta0) e`, `L = L0 + (L1-L0) e`, `tip = Pw + s (facing L cos(theta), L sin(theta))`,
`Pw = (x + facing s px, z + s ph)`. Forma activa = capsula del segmento `[Pw + 0.35 L dir, tip]` con radio `s r`, UNION la
capsula `[tipPrev, tip]` (barrido entre frames, anti-tunel). Colision = solape capsula-rectangulo en el plano (X, altura)
Y `abs(laneEnemigo - laneAtacante) <= depth + 18`. `behind=true` usa `-facing`.

**V6. Presets procedurales (PoseFx, `src/game/player/PoseFxPresets.h`, los implementa F0.3).** Cada preset genera claves
`{sx, sy, angulo, dx, dy}` por fase (S, A, R normalizadas a [0,1]); `dir = +1/-1` segun facing. Se combinan sumando
desplazamientos/angulos y multiplicando escalas.
| Preset | Formula |
|---|---|
| `ANTICIPA(k)` | en S: `e=outQuad(v)`; `sx=1-0.06k e`, `sy=1+0.04k e`, `ang=-4k dir e`, `dx=-6k dir e` |
| `IMPACTO(k)` | en A: `q=sin(pi v)`; `sx=1+0.10k q`, `sy=1-0.05k q`, `ang=+6k dir q`, `dx=+8k dir q` |
| `RECOBRO(k)` | en R: vuelve a neutro con `outBack(1.4)`, sobrepaso `0.03k` |
| `ESTOCADA(d, curva)` | root motion real: `x += facing d curva(u)` sobre S+A (frena x0.3 al conectar) |
| `SALTITO(h)` | `dy = -h sin(pi v)` en A (o en el tramo indicado) |
| `UPPER(h)` | `ang: +6 dir -> -18 dir` con outCubic en A, `dy=-h sin(pi v)` |
| `GIRO(n)` | `sx = cos(2 pi n v)` con `abs(sx) >= 0.15` y flip visual cuando cambia de signo; `ang += 4 sin(2 pi n v)`; estelas 3 |
| `BARRIDA` | `sy=0.72`, `dy=+18`, `ang=-10 dir`, estelas 3 a ras de suelo |
| `CAIDA_ROT(a)` | rota alrededor de los pies `0 -> -a dir` grados con inQuad; `dy` 0 -> +10 |
| `TEMBLOR(amp,f)` | `dx = amp sin(2 pi f t) exp(-t/0.06)` |
| `LATIDO(a,hz)` | `s = 1 + a sin(2 pi hz t)` sobre sx y sy |
| `ESTELA(n, intervalo, color)` | n copias de la textura+recorte+transformacion actuales, alfa 110 -> 0 en 0.22 s |
| `FX(i, n, ancla, escala, rot)` | superpone cuadros i..i+n-1 de `assets/fx/efectos_azules.png` (rejilla 8x5: impactos 8-13, estela de dash 42-45, escudo 16-18) en `ancla` relativo a los pies |
| `TINTE(color, t)` | multiplica el color del sprite durante t |

**V7. Formulas de sensacion (F0.2).** `hitstop = clamp(0.035 + 0.0035 dano, 0.035, 0.20) x (contra 1.5) x (ultimo de la ruta
1.35) x (bloqueado 0.6) x def.hitstopScale`; temblor de victima `A sin(2 pi 28 t) exp(-t/0.06)`, `A = 2 + 0.15 dano` px;
camara `1.5 + 0.22 dano` px, decae `exp(-t/0.12)`, en la direccion del golpe; muerte del ultimo enemigo de oleada o del jefe:
`timeScale 0.35` durante 0.35 s.

**V8. Juggle (F1.9).** `presupuesto = 6` por lanzamiento (Brute/Mutant/Armored 4); si `usado + coste > presupuesto` el golpe
no levanta, hace 30 % y el enemigo cae a `Down`. `gEff = 1500 (1 + 0.12 golpesEnAire)`; `vz = max(def.launch, 220) 0.9^golpesEnAire`;
`hitstun = def.hitstun 0.92^comboHits`; `dano = base max(0.35, 1 - 0.08 max(0, comboHits - 3))`. `Down` 0.55 s (solo OTG,
1 vez por derribo); `WakeUp` 0.35 s invulnerable; `groundBounce` vz = 300 una vez; `wallBounce` en el borde: `vx = -0.5 vx, vz = 260`.

**V9. Salto (F0.4).** `vz0 = 720` (apex 173 px), gravedad de subida 1500 y de caida 1875, control aereo 60 % de caminar, el
carril no cambia en el aire, aterrizaje 4 f de recovery. Pesado (Titan-X): `vz0 = 560` (105 px). Persecucion tras lanzador:
pulsar SALTO/J/K en los 10 f tras el impacto -> salto con `vx` tal que alcanza la X del enemigo en 0.30 s.

**ANEXO COMUN (obligatorio, identico en todos los briefs)**

Proyecto: District Fury, beat'em up 2.5D en C++17 + raylib 5.5. Repo `/home/user/GeneralApk`, rama base
`ccr-077ad675-rlrg28`. Trabajas en un worktree de git propio. Plan completo (contexto, opcional):
`docs/PLAN_MAESTRO_MOVIMIENTOS.md`; auditorias de origen en la
misma carpeta (`combate.md`, `personajes.md`, `audio.md`, imagenes en `img/`).

Reglas inviolables del usuario:
- RAYDEN (ORIGINAL) = skin 0. No se reemplaza ni se toca su arte (`assets/characters/rayden_128.png`, `rayden_clean.png`)
  ni sus 19 clips del juego `rayden` en `data/sprite_manifest.json`. Solo se le AGREGA.
- MAX = skin 6 (ex-RAYDER) es OTRO personaje: no confundirlo con Rayden. No tocar `assets/characters/max/*.png` (celdas
  usadas), ni el tamano de celda (270x175) ni el pivote (135,168).
- Kratos (`data/kratos/kratos_*.txt`, `assets/kratos/actor/*` salvo 9500-9503) y RONIN original (`data/kratos/ronin.txt`)
  no cambian. Trata TODOS los `data/kratos/*.txt` como congelados. Verificar md5 (abajo).
- Nada de musica ni sonidos con copyright: solo originales o sintetizados por codigo.
- No recrear `AGENTS.md`.
- Superarmadura: una habilidad en curso (`activeSkill >= 0`) NO se cancela al recibir golpe: solo flash rojo
  (`abilityHitFlashTimer`) + retroceso leve. Pierde vida igual.

Entorno (al empezar, dentro de tu worktree):
```
git fetch origin ccr-077ad675-rlrg28 && git reset --hard origin/ccr-077ad675-rlrg28
BASE=$(git rev-parse HEAD)
ln -sfn /home/user/raysan5/raylib .raylib-src
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/user/raysan5/raylib
cmake --build build -j8
```
(Si tu brief dice que partes de otra rama/etiqueta, el orquestador ya la habra integrado en la rama base: igual haces el
reset de arriba.) Las capturas y salidas de herramientas van a `/tmp/<algo>`, nunca a la raiz del repo.

Verificacion obligatoria antes del commit final (copia las lineas de resultado en tu informe):
1. `cmake --build build -j8` sin errores ni warnings nuevos (en especial `-Wswitch`).
2. `ctest --test-dir build --output-on-failure` -> 100 % de tests pasan (hoy son 7; tras Fase 0 hay mas: TODOS deben pasar).
3. `xvfb-run -a ./build/all_modes_check /tmp/x` -> ultima linea `problemas: 0`.
4. `xvfb-run -a ./build/gow_check /tmp/y` -> `problemas: 0` y la peor oleada BETA <= 225 s (hoy 213.1 s).
5. `python3 tools/ronin_kratos/check_kratos_md5.py $BASE` -> `distintos: 0`; y
   `for f in assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png; do cmp <(git show $BASE:$f) $f || echo CAMBIO $f; done` -> nada.
6. `rm -f ./*.png ./district_fury_beta.dat` en la raiz del repo; `git status` limpio salvo tus cambios.
Tests: si cambias un comportamiento que un test asume, actualiza ESE test en el mismo commit y explica por que en el
mensaje. Nunca desactives, borres, saltes ni relajes un test o umbral para que pase. Tests nuevos van en
`tests/unit/<Nombre>Tests.cpp` (se registran solos en ctest por glob, con directorio de trabajo = raiz del repo).

Propiedad de archivos: SOLO editas/creas lo listado en "PUEDES TOCAR". Si necesitas algo de un archivo ajeno, no lo edites:
anotalo en tu informe como `PEDIDO A <brief o MOTOR>: <que y por que>`. Si un cambio te obliga a salirte de tu lista,
detente en ese punto y reportalo.

Commits: en la rama de tu worktree, mensajes en espanol, pequenos y que compilen cada uno. NO hagas push. Informe final:
commits (hash + titulo), archivos tocados, resultados literales de los 6 checks, capturas generadas (rutas absolutas),
tabla de lo implementado vs pedido (con numeros finales), pedidos a otros, y lo que quedo pendiente.

---

## 8. Re-auditoria (Fase 3: despues de integrar Fase 2)

Lanzar 3 auditores (combate, personajes, audio), SOLO LECTURA, sobre la punta integrada. Cada uno entrega un informe con
la misma estructura que esta ronda y la tabla "cumplido / parcial / no" de abajo. El agente pensador siguiente recibe esos
tres informes.

### 8.1 Checks automaticos (todos deben dar lo indicado)
| # | Comando | Esperado |
|---|---|---|
| 1 | `cmake --build build -j8` | sin errores ni warnings `-Wswitch` |
| 2 | `ctest --test-dir build --output-on-failure` | 100 % (>= 7 historicos + HitResolver, CombatMath, Moveset, Juggle, PoseTables, ManifestInclude, AudioMix, DefenseGrab, ...) |
| 3 | `xvfb-run -a ./build/all_modes_check /tmp/x` | `problemas: 0` |
| 4 | `xvfb-run -a ./build/gow_check /tmp/y` | `problemas: 0`, peor oleada BETA <= 225 s |
| 5 | `python3 tools/ronin_kratos/check_kratos_md5.py 1567205` | `distintos: 0` |
| 6 | `md5sum assets/characters/rayden_128.png assets/characters/rayden_clean.png assets/characters/max/*.png` vs `git show 1567205:<ruta> | md5sum` | identicos |
| 7 | `git diff 1567205 -- data/sprite_manifest.json` filtrado al juego `rayden` | sin cambios en los 19 clips |
| 8 | `xvfb-run -a ./build/moves_sheet /tmp/m` | tabla por personaje: movimientos habilitados, clip usado, `fallback`/`idle` -> `idle-como-ataque: 0`; conteos >= metas de la seccion 1 |
| 9 | `xvfb-run -a ./build/state_strips /tmp/s` | genera `states_<id>.png` de los 42 |
| 10 | Sondeo de combate (reutilizar `.../audit/probe/probe.cpp` contra `libdistrict_fury_core.a`) | 1 proyectil por onda; FURIA golpea 3 de 3; JAB no conecta a |dy|=70; golpe por la espalda empuja hacia delante del atacante; 5 bloqueos no dejan `velocity.x` residual tras 30 frames; deslizamiento a 30 fps dentro de +-10 % del de 60 fps; Finisher alcanzable; juggle se corta al agotar presupuesto |
| 11 | `python3 tools/audio/analiza_banco.py assets/audio/sfx/combat` (F1.11) | cada golpe con >= 25 % de energia en 0.5-3 kHz, pico <= -1 dBFS, 0 muestras recortadas |
| 12 | `git ls-files GowApk/assets/audio` + `python3 tools/audio/genera_beta_audio.py --verificar` (F1.13) | todos los .ogg cargados por BetaMode generados por el script (hash registrado), 0 stubs de 23 ms |
| 13 | Rendimiento: `all_modes_check` reporta ms/frame | media < 8 ms y percentil 99 < 16 ms en el stage mas cargado |
| 14 | Raiz del repo | sin `*.png` sueltos ni `district_fury_beta.dat` |

### 8.2 Capturas que DEBEN generarse y revisarse a ojo (guardar en `/tmp/reaudit/`)
- `state_strips` de los ids 1, 3, 4, 5 (poses): caminar a la derecha Y a la izquierda: la cara/pecho mira hacia la flecha;
  ninguna pose cambia de tamano mas de 15 % respecto al reposo (la derrota tumbada se mide por ANCHO <= 2.2x).
- `vs_boss_visual_check` con TITAN-X y TITAN-X MEJORADO como jugador y como jefe: se miran de frente.
- `moves_sheet` de 0, 2, 4, 5, 6, 1, 3, 7, 9, 22, 23, 27: 6 cuadros por movimiento; revisar que el golpe "apunta" al
  frente, que las cajas debug (F9) coinciden con el miembro, que no hay `idle` en ataques.
- Rayden 0: `state_strips --fixed-time 0` de los 9 ataques y estados historicos identico pixel a pixel a la etiqueta
  "base-fase1" (tras Fase 0, que cambia fisica como el retroceso pero no el dibujo); contra `1567205` se exige que cada
  ataque/estado historico use los MISMOS cuadros y duraciones de clip (no se compara posicion). Los movimientos nuevos SI cambian.
- MAX 6: transformacion = atlas `max_white` real (no solo aura); derribo sin cuadros de ataque; derrota termina tumbado.
- Una partida grabada (secuencia de capturas cada 0.1 s, 20 s) del Stage1 con Rayden haciendo J J J J, J J K + aereos,
  v K + pisoton, QCB+K, y del VS MAX vs TITAN-X.

### 8.3 Preguntas que la re-auditoria debe responder
- Combate: quedo algun modo con resolvedor propio? (grep `GetAttackHitbox().Intersects` fuera de HitResolver; esperado:
  solo BetaMode). Se aplica `def.depth`? Hay re-golpe por frame en algun camino? El hitstop congela el buffer?
- Juggle: presupuesto, gravedad creciente, OTG 1 vez, invulnerabilidad al levantarse; se puede hacer un infinito?
- Personajes: tabla de 42 filas: movimientos habilitados, cuantos usan arte propio vs procedural vs fallback; poses con
  orientacion o escala mal; clips `recovery == hit` restantes; habilidades que repiten normales.
- Audio: cobertura de eventos (cada `GameEventType` -> sonido o "mudo intencional"), voces simultaneas maximas medidas,
  diferencia de volumen percibido entre personajes (st50 de golpe ligero +-3 dB), musica por modo, ducking audible,
  OPCIONES con 3 deslizadores que persisten. Ningun audio con copyright cargado por el juego.
- Que falta para la ronda siguiente (pedidos de arte, PEDIDOS AL MOTOR pendientes, BetaMode al resolvedor unico).
