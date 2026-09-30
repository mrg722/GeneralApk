# District Fury — Progreso del modo autónomo (DF-013.2)

Documento vivo requerido por el "MODO AUTÓNOMO DE FINALIZACIÓN TOTAL".
Se actualiza en cada bloque de trabajo real. Nada aquí se afirma sin haber
sido verificado leyendo el código actual del repo o compilando.

Rama activa: `DF-013.2`. Último commit al abrir este bloque: `a3138bb`
(ya empujado a `origin/DF-013.2`, confirmado con `git fetch` + diff vacío).

---

## 0. Método

- Auditoría por lectura directa de `src/` y `grep` de consumidores reales
  (quién instancia qué, no solo quién lo declara).
- **No verificable en este entorno**: no hay display/GPU/Wine, así que
  ninguna afirmación visual ("se ve bien", "la animación fluye") puede
  confirmarse aquí. Lo que sí se verifica: compilación real (cross-compile
  a Windows con llvm-mingw, produce `district_fury.exe` PE32+ real) y
  lectura exhaustiva de la lógica.
- Push bloqueado desde el proxy git de este sandbox (403 "not in this
  session's authorized repository set"); el mecanismo de trabajo real es
  edición + commit directo en github.dev/VS Code Web con la sesión de
  GitHub del usuario, o entrega de parches para aplicar manualmente.

---

## 1. Estado real por sistema (IMPLEMENTADO / PARCIAL / NO INTEGRADO / LEGACY / FALTANTE)

> Auditoría inicial de la sesión. Varias filas quedaron superadas por los bloques 5–12; el estado vigente está en `docs/FINAL_REPORT.md`.

| Sistema | Estado | Evidencia |
|---|---|---|
| `AttackData` (tabla de ataques de Rayder) | **IMPLEMENTADO** | `Player.cpp` llama `GetAttack(id)`, `AttackTotalDuration`, `NextPunchInChain` para punch/kick/energy/dash/rage/finisher. Es la fuente de verdad real de los ataques del jugador. |
| `EnemyBrain` (perfiles/IA por tipo) | **IMPLEMENTADO** | `StreetEnemy.cpp` llama `GetEnemyProfile(type)` en Init/Update/RunSpecial/hitbox; controla hp, velocidad, agresión, guardChance, specials. No es una FSM separada sino una tabla de comportamiento consumida por la FSM propia de `StreetEnemy` (`StreetEnemyState`), lo cual cumple el objetivo de "cada enemigo requiere una respuesta distinta". |
| `CombatWorld` | **NO INTEGRADO** | Ninguno de los 4 modos de juego (`Stage1StoryGame`, `Stage2Game`, `Stage3Game`, `VSMode`) instancia `CombatWorld` ni lo pasa a `enemy.Update(dt, player, world)` — siempre se llama con `world` implícito = `nullptr`. Consecuencia real (no cosmética menor, verificada leyendo `RunSpecial`): el daño cuerpo a cuerpo normal SÍ funciona (cada stage tiene su propio `HandlePlayerHits/HandleEnemyHits` con hitbox/hurtbox independiente del `CombatWorld`), pero los efectos que dependen de `world` nunca ocurren: `EnemySpecial::ChemicalCloud` no crea el hazard de ácido (`world->AddHazard`) y como la condición es `world != nullptr && !hasHit`, con `world == nullptr` el `hasHit` de ese enemigo NUNCA se pone en true, así que ese enemigo repite el intento cada frame sin efecto secundario (no rompe el juego, pero el hazard prometido en el diseño de Chemical Soldier/Mutant nunca aparece). Igual para `HeavySlam` (falta el impacto/shake extra). |
| `RageState` / Rage Mode | **IMPLEMENTADO** | `Player.h/.cpp`: medidor, umbral, `isRageMode`, bonus de daño en `HandlePlayerHits` de los 3 stages (`+5` o `+8` según boss). |
| `Knockdown` | **PARCIAL** | Existen 10 referencias en el árbol pero concentradas en tipos/knockback (`launch`, `knockback` en `AttackDef`); no hay un estado "en el suelo, se puede rematar" explícito verificado en `StreetEnemyState` (no existe `Downed`/`Knockdown` como estado de la FSM). Pendiente confirmar si es solo terminología en comentarios o si falta el estado real. |
| `Dash Attack` | **IMPLEMENTADO** (jugador) | `AttackId::DashAttack` en `AttackData`, usado por `Player`. |
| `Finisher` | **IMPLEMENTADO** (jugador) | `AttackId::Finisher`, gate por `sp >= GetAttack(Finisher).spCost` y cooldown, en `Player.cpp` línea ~311. |
| `Pickups` / `Buffs` / `Hazards` / `Destructibles` | **PARCIAL/NO INTEGRADO** | Los tipos existen en `CombatWorld.h/.cpp` (`Pickup`, `Destructible`, `Hazard`) pero como `CombatWorld` no está instanciado en ningún stage, estos sistemas están escritos pero inalcanzables desde el juego real. `Player.cpp/.h` y `StreetEnemy.cpp` tienen referencias (probablemente a structs/flags locales, no al `CombatWorld` compartido) — pendiente de trazar exactamente en el siguiente bloque. |
| Boss `Brakk` (Stage 1) | **IMPLEMENTADO**, geometría primitiva | 3 fases reales (`StoryBossAttack`: ChainSwing/GroundSmash/Charge/PowerWave/Frenzy), cambia de fase por `ratio` de HP, `boss.blocking` adaptativo. Dibujado con rectángulos/círculos/líneas de raylib (`DrawBoss()`), NO con sprite real — no hay atlas de Brakk cargado en `AssetManager`. |
| Boss `Grinder` (Stage 2) | **IMPLEMENTADO**, geometría primitiva | Mismo patrón: 3 fases (`BossAttack`: Saw/Slam/Ram/Overdrive), sin sprite real. |
| Boss `Titan-X` (Stage 3) | **IMPLEMENTADO**, geometría primitiva | 3 fases, HP 1500/1250/1800 según dificultad (coincide con lo documentado). Sin sprite real. **Aclaración de diseño recibida del usuario y aún no reflejada en código:** Titan-X tiene DOS apariciones — la primera (Stage 3, prototipo) debe ser una forma más humanoide/pequeña con el suero verde inyectado pero menos musculatura; la aparición como penúltimo jefe (propuesta, no implementada aún como stage) debe ser la forma gigante verde de la biblia de arte. Actualmente solo existe UNA definición de Titan-X (Stage 3) sin distinción de forma — ver sección 3. |
| Boss `Rayder Clone` (final) | **FALTANTE** | Confirmado por lectura de `Stage1/2/3` + `main.cpp`: no existe ningún stage ni boss para el clon. Es la etapa final propuesta en la documentación de diseño, no implementada en el ejecutable. |
| Sprites reales de bosses | **FALTANTE** | Los 4 bosses (Brakk, Grinder, Titan-X, Rayder Clone) se dibujan con primitivas raylib. Existen referencias visuales (concept art) subidas por el usuario, pero no archivos PNG con canal alfa listos para recortar en `Animator`/`SpriteAtlas`. Sin esto no se puede reemplazar el dibujo primitivo sin inventar sprites falsos (prohibido explícitamente). |
| HUD | **PARCIAL, triplicado** | `Stage1StoryGame::DrawHUD`, `Stage2Game` y `Stage3Game` implementan cada uno su propio HUD manualmente (sin componente compartido). Todos muestran HP/Shield/SP/Rage/combo/score, pero con layout y código distintos → violan la regla "HUD común y coherente". Ningún HUD muestra portrait/ícono de Rayden. |
| VS Mode | **IMPLEMENTADO parcialmente**, aislado | `VSMode.cpp` tiene su propio `DrawBackground()` y su propio manejo de combate, no reutiliza `CombatWorld` ni el HUD de los stages. |
| Código muerto legacy (`GameManager`, `ProductionGame`, `Scene`) | **ELIMINADO** (este bloque de sesión) | Confirmado sin consumidores vía grep exhaustivo de `main.cpp` y el resto de `src/`; borrado y pusheado en `a3138bb`. |
| Bug `DrawCircleGradient` (Scene.cpp) | **RESUELTO** (ya no aplica; el archivo fue borrado junto con el resto de `Scene`) | Corregido primero (`a0860cd` local) y luego el archivo completo se retiró por no tener consumidores. |
| Build Windows (.exe) | **VERIFICADO** | Cross-compile real con llvm-mingw (`x86_64-w64-mingw32-clang++`) produce `district_fury.exe` PE32+ válido. Verificado antes y después de los cambios de este bloque. **No verificado**: ejecución real (sin Wine/GPU en este sandbox). |
| Build Android (.apk) | **FALTANTE** | No hay configuración de NDK/Gradle en el repo. Cero trabajo iniciado. |

---

## 2. Hallazgo de esta sesión: Titan-X, dos formas (aclaración del usuario)

El usuario corrigió explícitamente el diseño: Titan-X **no** debe ser el
"robot rojo" de versiones anteriores. Es un mutante/experimento
biomecánico (cuerpo orgánico + partes mecánicas, tubos, verde químico).
Tiene dos apariciones:

1. **Primera aparición (Stage 3, boss actual)**: forma más humanoide y
   pequeña, ya con el suero verde inyectado pero menos musculosa — un
   prototipo, no la forma final.
2. **Segunda aparición (penúltimo jefe, stage propuesto aún no
   implementado)**: la forma gigante verde de la biblia de arte —
   "Titan-X mejorado", evolución visible del mismo personaje, no un
   personaje nuevo.

Esto queda registrado en `GAME_DESIGN.md` (sección añadida en este mismo
bloque) para que ninguna sesión futura reinvente a Titan-X como el robot
rojo genérico. **No se ha tocado el código de `Stage3Game` boss todavía**
— cambiar su tamaño/proporción sin sprite real y sin poder verificar
visualmente sería inventar un resultado, así que queda como tarea
pendiente explícita hasta tener el asset o una instrucción de escala
concreta y verificable por lectura de datos (no por ojo).

---

## 3. Tareas realizadas en este bloque

- Confirmado y pusheado a `origin/DF-013.2` (commit `a3138bb`): fix de
  `DrawCircleGradient` + retiro de código muerto (`GameManager`,
  `ProductionGame`, `Scene`). Verificado con `git fetch` + `git diff`
  vacío contra el remoto.
- Auditoría real de integración de `AttackData`, `EnemyBrain`,
  `CombatWorld`, bosses, HUD, VS — con evidencia de grep/lectura, no
  supuestos.
- Registrada la aclaración de diseño de Titan-X (dos formas) para
  evitar que se reinvente el personaje.

## 4. Tareas pendientes (por prioridad, según brief: movimiento → combate → IA → stages → progresión → presentación)

1. **CombatWorld → hazards reales**: instanciar `CombatWorld` en los 4
   modos y pasarlo a `enemy.Update()` para que `ChemicalCloud` y
   `HeavySlam` produzcan hazard/impacto reales (hoy son no-ops
   silenciosos). Cambio aditivo, bajo riesgo, pendiente de compilar y
   confirmar en el siguiente bloque.
2. **Knockdown real**: confirmar si falta un estado `Downed` en
   `StreetEnemyState` o si el sistema ya cubre esto con otro nombre.
3. **HUD unificado**: extraer un componente HUD compartido usado por los
   3 stages + VS, con portrait de Rayden.
4. **Sprites de bosses**: bloqueado hasta recibir PNG con canal alfa
   real (las imágenes de concept art recibidas son ilustraciones de
   referencia, no spritesheets recortables).
5. **Titan-X dos formas**: requiere activo visual o dato de escala
   concreto antes de tocar código, para no inventar resultado.
6. **Rayder Clone / stage final**: no iniciado; depende de que la
   campaña base (HUD, CombatWorld) esté estable primero.
7. **Android APK**: no iniciado, cero configuración.

## 5. Bloque 2 — CombatWorld cableado en los 4 modos (este bloque)

Se instanció `CombatWorld combatWorld;` como miembro de `Stage1StoryGame`,
`Stage2Game`, `Stage3Game` y `VSMode`, y se conectó en cada uno:

- `enemy.Update(dt, player, &combatWorld)` en vez de `enemy.Update(dt, player)`
  — activa de verdad `EnemySpecial::ChemicalCloud` (crea el hazard de ácido)
  y el impacto/shake extra de `HeavySlam`.
- `combatWorld.Update(dt)` + `combatWorld.ResolveHazards(player, enemies)`
  cada frame de combate — sin esto los hazards se creaban pero nunca
  envejecían ni dañaban a nadie (bug propio que habría introducido yo si
  paraba en el paso anterior).
- `combatWorld.DrawGround()` / `combatWorld.DrawEffects()` en el dibujo de
  mundo de cada modo, para que hazards/impactos/partículas de `CombatWorld`
  sean visibles.
- `combatWorld.Reset()` en cada `ResetRun()/Init()/ResetFight()`.

**Deliberadamente NO se tocó**: `HandlePlayerHits`, `HandleEnemyHits`,
`UpdateProjectiles`, el cálculo de combo/score ni el HUD de ningún stage.
Esos siguen siendo la ruta real de daño cuerpo a cuerpo (ya funcionaba) —
migrarlos a los métodos equivalentes de `CombatWorld`
(`ResolvePlayerMelee`/`ResolveEnemyMelee`/`ResolveProjectiles`) sería el
paso siguiente hacia "CombatWorld como único sistema", pero es un cambio
de mayor riesgo (cambiaría fórmulas de daño/combo existentes) que no se
puede verificar visualmente en este entorno (sin display/Wine). Se deja
como tarea explícita, no se hizo a medias ni se ocultó.

**Verificado real**: compilación limpia cruzada a Windows de los 4
`.cpp` modificados + `main.cpp`, `district_fury.exe` PE32+ regenerado,
y los dos binarios de test (`combat_smoke_tests.exe`,
`application_state_tests.exe`) linkan sin error.
**No verificado**: ejecución de los tests ni del juego (no hay Wine/GPU
en este sandbox) — esto sigue siendo una limitación del entorno, no una
afirmación de que "funciona en pantalla".

## 6. Última tarea completada
Cableado aditivo de `CombatWorld` en los 4 modos de juego (Stage1,
Stage2, Stage3, VSMode), compilado y verificado (ver sección 5).

## 7. HUD unificado + Stage4/Stage5 (bloque completado)

- `src/ui/GameHUD.h/.cpp`: nuevo `ui::PlayerVitals`/`DrawPlayerVitals`
  que reemplaza los 4 paneles HUD hand-rolled de Stage1/Stage2/Stage3/
  VSMode por un único componente (mismas fórmulas de HP/Shield/SP/Rage
  verificadas contra el código original de cada stage antes de
  reemplazarlo). Corrige un hueco real: Stage3 nunca mostraba Shield ni
  SP en su HUD anterior.
- `Stage4Game` (Kessler Tower / Titan-X Mejorado) y `Stage5Game`
  (Cámara del Clon / Rayder Clone) creados siguiendo la campaña de 5
  stages confirmada por el usuario. Reutilizan `EnemyBrain`/`AttackData`/
  `CombatWorld`; bosses dibujados con primitivas raylib (mismo enfoque
  que los 3 bosses existentes, no es un placeholder nuevo).
- `main.cpp`: F4/F5 activan Stage4/Stage5 (mismo patrón que F1-F3). **No
  hay encadenamiento automático de campaña** (Stage1→2→3→4→5 al vencer
  cada Clear) — los 5 stages siguen siendo seleccionables solo por tecla
  de debug, igual que ya ocurría entre Stage1/2/3 antes de este bloque.
  Esto sigue pendiente (ver sección 8).
- **Verificado real**: `district_fury.exe`, `application_state_tests.exe`
  y `combat_smoke_tests.exe` compilan y enlazan limpio via cross-compile
  llvm-mingw tras agregar `Stage4Game.cpp`/`Stage5Game.cpp`/
  `GameHUD.cpp` a `CMakeLists.txt`. Un error real de compilación
  (narrowing `int`→`unsigned char` en un `Color` literal de
  `Stage5Game::DrawBoss`) se detectó y corrigió antes de dar el bloque
  por cerrado.
- **No verificado**: ejecución/pantalla (sin Wine/GPU en este sandbox).

## 8. Decisión de arquitectura: Boss/BossDefinition (usuario, 2026-09-17)

El usuario propuso explícitamente la jerarquía `CombatEntity → Player /
StreetEnemy(EnemyProfile) / Boss(BossDefinition)` para evitar duplicar
miles de líneas por enemigo/boss. Auditoría real (ver detalle completo en
`ARCHITECTURE.md`, sección "Decisión de arquitectura"):

- `StreetEnemy` + `EnemyProfile` (`src/game/enemies/EnemyBrain.h/.cpp`):
  **ya implementado exactamente así** — 1 clase, 8 filas de datos. No
  requiere migración.
- `Boss` + `BossDefinition`: **no existe, duplicación real confirmada**.
  Cada uno de los 5 stages tiene su propio struct de boss
  (`StoryBoss`/`Grinder`/`TitanX`/`TitanXImproved`/`RayderClone`) y su
  propia copia de `UpdateBoss/DrawBoss/HandleBossHits`. Stage3 además ni
  siquiera usa un `enum BossAttack` (usa temporizadores sueltos), lo cual
  es evidencia adicional de la divergencia.
- **Paso 1 completado en este bloque** (bajo riesgo, no integrado
  todavía): `src/game/combat/BossDefinition.h/.cpp` — estructuras de
  datos `BossDefinition`/`BossPhaseDef`/`BossAttackDef` + tabla real con
  los 5 bosses (`Brakk`, `Grinder`, `TitanX`, `TitanXMejorado`,
  `RayderClone`). HP máximo y umbrales de fase se copiaron literalmente
  de cada `Stage*Game.cpp` (`ratio<=` real de cada boss). Los timings/
  daños por ataque de Grinder (el candidato piloto de migración) también
  se copiaron del código real; los de Brakk/Titan-X/Titan-X Mejorado/
  Rayder Clone son valores de partida razonables, **no** migrados número
  por número todavía — esto se declara explícitamente para no violar la
  regla de "nunca afirmar terminado sin evidencia".
  **Este archivo se agregó a `CMakeLists.txt` y compila, pero NO se
  consume desde ningún `Stage*Game` — cero cambio de comportamiento en
  el juego.**

## 9. Siguiente tarea automática

Fase 2 de la unificación de bosses (ver plan completo en
`ARCHITECTURE.md`): crear una clase `Boss` única en
`src/game/combat/Boss.h/.cpp` que interprete un `BossDefinition` (Update
genérico de fases/ataques, Draw genérico parametrizado), y migrar los 5
stages **uno a la vez**, empezando por `Stage2Game`/Grinder (ya tiene los
datos reales completos), comparando el comportamiento resultante contra
el actual antes de tocar el siguiente stage. No se ejecuta a los 5 stages
de una vez porque ese sí sería el tipo de refactor masivo no verificable
que las reglas del proyecto piden evitar salvo necesidad clara — aquí la
necesidad es clara (duplicación real), pero la ejecución debe ser
incremental para poder compilar/revisar cada paso por separado.

## 10. Sprites reales de bosses + VS con 5 stages (bloque completado)

- `assets/bosses/{titanx,titanx_mejorado,rayder_clone}/*.png`: poses recortadas
  de las hojas del usuario (2079x756, alfa real). Verificado por script: 0%
  de pixeles semitransparentes casi-blancos (sin matte/halo blanco). Cortes
  entre poses revisados visualmente uno a uno; se corrigieron poses fusionadas
  (idle4+lean de Titan-X, idle3+idle4 de Rayder Clone) y fragmentos sueltos
  (dash1/charge_orb del clon se eliminaron por estar incompletos). Queda un
  fragmento minimo de puño vecino en `titanx/uppercut.png` y `titanx_mejorado/dash.png`.
- `src/rendering/BossSprite.h` (`DrawBossPose`): dibuja por altura objetivo con
  pivote en los pies. Escalas segun las hojas de referencia: Titan-X 170px,
  Titan-X Mejorado 280-310px (~2.5x Rayden), Rayder Clone 130px (humano).
- Stage3/4/5 `DrawBoss` usan los sprites en vez de primitivas. Stage4/5 eligen
  pose por `BossAttack` real; Stage3 no guarda ataque activo (lo resuelve
  instantaneo), asi que usa idle/lean/recoil segun temporizadores.
- **Limite honesto**: son poses por estado, no animacion frame a frame. Las hojas
  de referencia grandes (IDLE 8, CAMINAR 8, etc.) tienen frames de ~60px dentro
  de una lamina de diseño; no son atlas de produccion recortables sin perder calidad.
- VS: agregados Stage 4 y Stage 5 al selector. Los 8 tipos de enemigo ya eran
  seleccionables por slot. **Falta**: pelear bosses en VS (no hay ruta de boss
  en VSMode) y el Dr. Kessler como NPC narrativo (no golpeable, camina y deja al boss).

## 11. Dr. Kessler — NPC narrativo (bloque completado, sprite pendiente)

- `src/game/npc/KesslerCameo.h/.cpp`: estados Hidden → Entering → Talking →
  Leaving → Done. Entra caminando por detras del boss (linea de profundidad
  mas alta), dice 3 lineas en caja de dialogo, y se va caminando dejando al
  boss. ENTER lo salta. **No tiene hitbox ni hurtbox y no se agrega a ninguna
  lista de combate**: no se le puede golpear ni puede atacar.
- Integrado en el BossIntro de Stage4 (antes de Titan-X Mejorado) y Stage5
  (antes de Rayder Clone). El boss no empieza a pelear hasta que Kessler se fue.
- **Sprite**: carga `assets/npc/kessler/{idle,walk1,walk2}.png` si existen
  (transparentes, mirando a la derecha). Las laminas recibidas tienen fondo de
  panel opaco (alfa ~247) y el pantalon negro se confunde con el fondo, asi que
  no se pueden recortar limpias. Mientras no lleguen esos PNG se dibuja una
  silueta temporal (bata blanca, pelo gris, gafas) — SUSTITUTO declarado.

## 12. Bosses en Modo VS con clase Boss compartida (bloque completado)

- `src/game/combat/Boss.h/.cpp`: clase unica que interpreta `BossDefinition`
  (fases por umbral de HP, seleccion de ataques por fase, startup/active/
  recovery, hitbox por ataque, proyectiles, multiplicadores de fase).
- `VSMode`: nuevo campo BOSS en el selector (NINGUNO + 5 bosses). Con boss
  elegido la pelea es 1 vs 1; barra de HP y fase del boss en el HUD; pantalla
  "BOSS DERROTADO".
- Titan-X / Titan-X Mejorado / Rayder Clone usan sus sprites reales; Brakk y
  Grinder usan silueta generica (no hay sprites de produccion recortables).
- **Limites**: acercamiento generico (sin embestidas/teletransporte reales);
  los stages de Historia siguen con su logica propia (no migrados a `Boss`).
- **Verificado**: compila y enlaza (exe + 2 tests). **No verificado**: ejecucion.
