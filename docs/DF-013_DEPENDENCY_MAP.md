# DF-013 — FASE 0: Mapa de dependencias y auditoría previa

Documento de entrada obligatorio antes de escribir código, según la sección
"ESTADO REAL DEL PROYECTO" del brief. Nada de lo que sigue es supuesto: cada
afirmación sale de leer el árbol entregado en `Proyecto.zip`.

---

## 0. Método y límites de esta auditoría

**Verificado:**

- Lectura completa de los 34 archivos de `src/` y `tests/` (3.362 líneas).
- Lectura de `README.md`, `MASTER_PROMPT.md`, `GAME_DESIGN.md`, `ARCHITECTURE.md`,
  `ROADMAP.md`, `AGENTS.md`, `CMakeLists.txt`, `.github/workflows/ci.yml`,
  `data/sprite_manifest.json`, `assets/`.
- Grafo de consumidores por símbolo (`grep` sobre declaraciones y usos).
- **Chequeo de compilación por unidad de traducción**: los 14 `.cpp` pasan
  `g++ -std=c++17 -fsyntax-only -Isrc -Ibuild/_deps/raylib-src/src`. raylib viene
  vendorizado en `build/_deps/raylib-src`, así que este bucle de verificación
  funciona y lo usaré en cada fase.

**No verificable en este entorno (importante):**

- No puedo **enlazar ni ejecutar**: no hay `cmake`, no hay red para
  `FetchContent`, y no hay display ni libs de GL/X11 para construir raylib.
  Por tanto no puedo correr `ctest`, ni abrir Stage 1/2/3, ni VS.
- Todo lo marcado abajo como *defecto de runtime* está deducido de lectura de
  código, no de reproducción en pantalla. Está señalado explícitamente.

**Estado de git:** la rama activa es `feature/DF-012-integracion-visual`, no
`main`. El árbol aparece con los 62 archivos "modificados" pero el diff con
`--ignore-all-space` es **vacío**: es solo conversión CRLF del empaquetado.
Antes de crear `feature/DF-013-core-experience` hay que normalizar esto
(`.gitattributes` con `* text=auto` o `git checkout -- .`), o el primer commit
de la rama va a ser 4.372 líneas de ruido.

---

## 1. Inventario real vs. inventario declarado

El brief describe la base como "prototipo grande". La base real es más chica y
está **medio muerta**:

| Estado | Líneas | % |
|---|---:|---:|
| Código vivo (alcanzable desde `main`) | ~1.660 | 49% |
| Código muerto (compilado o no, nunca instanciado) | ~1.700 | 51% |

Entra en producción exactamente esto:

```
main.cpp → Stage1StoryGame, Stage2Game, Stage3Game, VSMode
                  ↓
         Player, StreetEnemy, Animator, AssetManager, AudioSystem
```

---

## 2. Grafo de dependencias

```
                              main.cpp
                                 │
      ┌────────────┬─────────────┼─────────────┬────────────┐
      ▼            ▼             ▼             ▼            ▼
Stage1Story   Stage2Game    Stage3Game      VSMode    AssetManager
      │            │             │             │            ▲
      └────────────┴──────┬──────┴─────────────┘            │
                          ▼                                 │
                   Player ◄──── StreetEnemy ────────────────┤
                      │              │                      │
                      └──────┬───────┘                      │
                             ▼                              │
                         Animator ──► SpriteFrame           │
                             │                              │
                             └──────────────────────────────┘
                             │
                    Player.h ──► ui/SpanishText.h   ⚠ (ver §5, D1)
                             │
                    Player.cpp ──► AudioSystem
```

**Nodos muertos, desconectados del grafo:**

```
ProductionGame (754 líneas)  ── usa ──► Player, Scene, StreetEnemy   [sin consumidores]
GameManager    (544 líneas)  ── usa ──► Player, Scene, StreetEnemy   [sin consumidores]
Scene          (192 líneas)  ── consumido SOLO por los dos anteriores [muerto transitivo]
Enemy          (209 líneas)  ── ni siquiera está en CMakeLists.txt     [nunca compilado]
core/ApplicationState (21)   ── solo el bucle while de main + 1 test   [vivo, trivial]
```

### Consumidores por símbolo central (§28 del brief)

Antes de tocar cualquiera de estos, esta es la lista completa de archivos a
revisar. No hay más:

| Símbolo | Consumidores vivos | Consumidores muertos |
|---|---|---|
| `Player` | Stage1, Stage2, Stage3, VSMode, StreetEnemy (`Update(dt, const Player&)`), tests/CombatSmokeTests | ProductionGame, GameManager, Enemy |
| `StreetEnemy` | Stage1, Stage2, Stage3, VSMode, tests/CombatSmokeTests | ProductionGame, GameManager |
| `Animator` | Player, StreetEnemy, tests/ApplicationStateTests | Enemy |
| `AssetManager` | main, Player, StreetEnemy, Stage1, Stage2, VSMode | Scene, Enemy, GameManager, ProductionGame |
| `AudioSystem` | main, **Player y nadie más** | — |
| `CombatBox` / `Vector3D` (Types.h) | todo lo anterior | todo lo anterior |

Dos observaciones que condicionan el diseño del Combat Core:

1. **`StreetEnemy::Update` toma `const Player&`.** El enemigo lee al jugador
   directamente. Eso ata IA y jugador; cualquier FSM nueva hereda ese
   acoplamiento salvo que se introduzca un `CombatContext`/`TargetInfo`.
2. **`AudioSystem` solo lo llama `Player`.** Los 15 `Sfx` existen, pero 11 nunca
   se disparan: impactos, muerte de enemigo, fases de boss, stage clear y UI son
   silenciosos. No hay que "auditar AudioSystem" (§21): hay que **conectarlo**.

---

## 3. Duplicación real (lo que §1 del brief pide eliminar)

Las funciones que el brief nombra textualmente existen **4 veces cada una**
(3 stages vivos + ProductionGame muerto):

| Función | Stage1 | Stage2 | Stage3 | ProductionGame |
|---|:-:|:-:|:-:|:-:|
| `HandlePlayerHits` | ✔ | ✔ | ✔ | ✔ |
| `HandleEnemyHits` | ✔ | ✔ | ✔ | ✔ |
| `HandleBossHits` | ✔ | ✔ | ✔ | ✔ |
| `UpdateProjectiles` | ✔ | ✔ | ✔ | ✔ |
| `UpdateParticles` | ✔ | ✔ | ✔ | ✔ |
| `SpawnImpact` | ✔ | ✔ | ✔ | ✔ |
| `DrawHUD` / `DrawBoss` | ✔ | ✔ | ✔ | ✔ |
| `ApplyDifficulty` | ✔ | ✘ | ✔ | ✔ |

Y no son copias idénticas, que es lo peor: **divergieron**.

- `HandlePlayerHits`: Stage1 y Stage3 golpean al **primer** enemigo que
  intersecta; Stage2 elige el **más cercano**. Tres stages, dos reglas de
  targeting distintas.
- `HandleEnemyHits`: Stage1 ignora `dashInvulnerability` (delega en
  `Player::TakeDamage`), Stage3 lo chequea explícito **y** limita a 2 atacantes
  simultáneos, Stage2 no limita nada. El dash protege distinto según el stage.
- Proyectiles: `StoryProjectile` (con `fromBoss`, `radius`), `Stage2::Projectile`
  (sin `fromBoss`, radio fijo 18 hardcodeado en la colisión), `Stage3::Projectile`
  (con ambos). Tres structs para el mismo concepto.
- Bosses: `StoryBoss` + `StoryBossAttack`, `Grinder` + `BossAttack`, `TitanX`
  (sin enum, ataques por `GetRandomValue` inline). Tres arquitecturas, cero
  compartido. §14 no es un refactor cosmético: es reescribir tres cosas en una.

---

## 4. Lógica hardcodeada (lo que §2 y §24 piden mover a datos)

- **Ataques del jugador** (`Player.cpp`): ventanas activas en `AttackIsActive()`
  con literales (`.09f`–`.22f` punch, `.11f`–`.28f` kick, `.20f`–`.43f` energy);
  daño en `GetAttackDamage()` con ternarios; alcance, profundidad y knockback
  igual. Añadir DASH ATTACK / RAGE ATTACK / FINISHER hoy significa tocar 5
  funciones distintas de `Player` más los 4 `HandlePlayerHits`.
- **Stats de enemigos**: `GetStats()` en `StreetEnemy.cpp`, `switch` de 8 casos
  con 10 campos por tipo. Candidato directo a JSON.
- **Spawns**: tablas `kScenarioWaves[4][5]` (Stage1), `waves[][5]` (Stage2),
  `kScenarios[3][5]` (Stage3) con coordenadas absolutas en el `.cpp`.
- **Límites de escenario**: `ScenarioStartX()/EndX()` son cadenas de ternarios
  con números mágicos (180/1450/1500/2850/…, boss clamp `5050..5850`).
- **Rangos D→SSS**: tres fórmulas distintas. Stage1 pondera tiempo/combo/HP;
  Stage3 usa score y otra escala; **Stage2 imprime `"RANK S"` literal** (ver D7).
- **Layout de animación**: `LayoutFor()` devuelve la misma constante
  `{4,3,0,3,0,3,4,7,8,10,11}` para los 8 tipos. Es un punto de extensión que ya
  está preparado pero sin datos detrás.

---

## 5. Defectos concretos encontrados

Ordenados por impacto sobre el "Definition of Done" del brief.

### D1 — `#define DrawText` global *(bloqueante para UI 2.0)*
`src/ui/SpanishText.h:15` termina con:
```cpp
#define DrawText district_fury::DrawTextTranslated
```
Ese header lo incluye `Player.h`, que incluye **todo el juego**. Consecuencias:

- La macro secuestra `DrawText` en cada TU. Cualquier módulo de UI nuevo la
  hereda sin saberlo; un header de terceros incluido *después* de `Player.h` que
  declare algo llamado `DrawText` rompe la compilación.
- No cubre `DrawTextEx`, `DrawTextPro` ni `MeasureText`. El código ya mide con
  `MeasureText` (sin traducir) y dibuja traducido → los centrados calculados en
  `Stage1StoryGame::DrawArenaLock`, `DrawHUD`, etc. quedan desalineados cuando la
  cadena traducida cambia de longitud ("GAME OVER" 9 → "HAS CAIDO" 9 ok, pero
  "STAGE 2 CLEAR" 13 → "ESCENARIO 2 COMPLETADO" 22 no).
- Construye un `std::string` y corre 48 búsquedas `find/replace` **por cada
  llamada a DrawText, cada frame**. El HUD hace ~20 llamadas → ~1.000
  operaciones de string por frame solo para traducir.

Esto se saca de `Player.h` en la Fase 1, sí o sí. Va a un módulo `ui/Text` con
función explícita, no macro.

### D2 — Energy Wave no existe en Modo VS *(viola el DoD: "L funcione en todas las modalidades")*
`VSMode::Update` filtra el golpe con `player.attackType != AttackType::Energy` y
**nunca** lee `player.energyReleased` ni spawnea proyectil. En VS, pulsar L
consume 20 SP, reproduce la animación y **no hace absolutamente nada**. Además
`energyReleased` queda en `true` para siempre (nadie lo limpia), así que el
primer stage que se abra después heredaría el flag si compartieran instancia
(hoy no la comparten, pero es una bomba de relojería para el Combat Core).

### D3 — Stage 2 no acota posiciones *(defecto de runtime, deducido)*
`Stage2Game.cpp` no tiene **ni un solo** `std::clamp` de posición: ni del
jugador ni de los enemigos. `Player::Update` solo acota en los estados `Hit` y
`Dash`. Durante caminata normal la profundidad del jugador es libre: se puede
salir del carril hacia arriba/abajo hasta que reciba un golpe, momento en el que
se teletransporta al rango válido. Stage1 y Stage3 sí acotan cada frame.

### D4 — Stage 2 se queda colgado en Clear
`Stage2Game::Update` hace `if(flow==Flow::Clear) return;` antes de leer nada. La
pantalla dice "ESC — EXIT" pero ESC no está conectado en ese estado. Tras matar a
Grinder **no hay salida**, salvo F1/F2/F3 (que el brief quiere retirar del flujo
de jugador). Mismo problema estructural: Stage2 y Stage3 no tienen forma de
volver al menú, porque el menú es propiedad de Stage1 (`stage1.IsMenu()`).

### D5 — Dificultad aplicada dos veces en Stage 3
`BuildScenario()` llama `ApplyDifficulty()`; en el escenario 1, pulsar ENTER en
el intro vuelve a llamar `ApplyDifficulty()`. Los enemigos del primer escenario
quedan con HP ×1.22² = ×1.49 en Hard y ×0.84² = ×0.71 en Easy. Los escenarios 2
y 3 no sufren el doble escalado → la curva de dificultad es incoherente dentro
del propio stage.

### D6 — Dos sistemas de carriles en conflicto
`Types.h` declara `kLaneMinY=485, kLaneMaxY=650`. Stage1 y Stage3 declaran
**sus propias** constantes locales `kLaneMin=505, kLaneMax=625`. `StreetEnemy`
acota contra las de `Types.h`; los stages acotan contra las locales. Resultado:
los enemigos pueden ocupar profundidades (485–505 y 625–650) que el jugador no
alcanza, y `DepthScale()` normaliza sobre 485–650 → el rango de escala efectivo
es 0.89–1.08 en vez del 0.86–1.12 diseñado.

### D7 — Stage 2 es el hermano pobre
Sin dificultad, sin score, sin save, sin XP/monedas, y `DrawClear()` imprime
`"RANK        S"` como texto fijo: el rango **no se calcula**. Cualquier
`ProgressionSystem` que lea "mejor rango de Stage 2" hoy no tiene de dónde.

### D8 — Save frágil y parcial
Solo Stage1 guarda (`district_fury_save.dat`, relativo al CWD). Formato: siete
enteros separados por espacios, `ifstream >>` sin versión, sin validación, sin
checksum. Un archivo truncado deja las variables a medio leer sin error. XP,
monedas y gemas solo se incrementan al matar a Brakk, y **ningún sistema las
consume**: no hay upgrades. §10/§12 parten de cero.

### D9 — Menú duplicado y divergente
`main.cpp::DrawMenuPrincipal()` y `Stage1StoryGame::DrawMenu()` dibujan dos
menús principales distintos (distinto layout, distinto texto, uno menciona VS y
el otro no). Cuál se ve depende de `activeStage`. Son 40 líneas de raylib crudo
en `main.cpp`, justo lo que `AGENTS.md` prohíbe.

### D10 — La hitbox Energy del jugador es código muerto
`Player::GetAttackHitbox()` calcula una caja de 220×76 para `AttackType::Energy`,
pero los cuatro `HandlePlayerHits` empiezan con
`if (... || player.attackType==AttackType::Energy) return;`. Esa rama nunca se
evalúa. El daño de Energy lo hace solo el proyectil.

### D11 — IA "diferenciada" que no lo está
`StreetEnemy::Update` es **un solo** cuerpo para los 8 tipos. Estados: Idle,
Chase, Attack, Hit, Defeat. La única diferenciación de comportamiento son dos
multiplicadores de velocidad (`Mutant` a >280px y `ChemicalSoldier` a <360px) y
la velocidad de frame del ataque. Todo lo demás es números en `GetStats()`. El
brief §6 pide 11 estados y respuestas distintas del jugador: eso es un sistema
nuevo, no un ajuste.

### D12 — Idioma mezclado
Stage3 está escrito en inglés ("Break the security cordon.", "GATEKEEPER",
"PHASE") y depende de la tabla de D1, que no cubre casi ninguna de esas cadenas.
Stage1 está en español nativo. El jugador ve dos idiomas según el stage.

### D13 — Asset declarado y ausente
`data/sprite_manifest.json` declara `old_steel_yard_deep_clean.png`; el archivo
no está en `assets/backgrounds/`. `AssetManager` lo carga como `bg_steel_deep`,
falla con warning, y `Stage2Game::DrawWorld` cae al rectángulo sólido
`{18,25,28,255}`. **Stage 2 se juega hoy sin fondo.**

### D14 — Cobertura de tests ~4%
140 líneas en 2 ejecutables: intersección de `CombatBox`, daño/escudo/guard-break
del jugador, y daño/muerte de `StreetEnemy`. Cero cobertura de: Rage, SP, Energy
Wave, combos, transiciones de IA, flujo de stage, recompensas, save/load,
desbloqueos. El §25 pide los 14; hay 3.

---

## 6. Riesgos de la migración

| # | Riesgo | Mitigación propuesta |
|---|---|---|
| R1 | Extraer `DrawText` de `Player.h` toca los 4 stages a la vez (todos dibujan texto). | Fase 1 aislada, un commit propio, solo mecánica: macro → `ui::DrawLocalized`. Sin cambios de gameplay en ese commit. |
| R2 | Unificar `HandlePlayerHits` cambia el targeting de Stage2 (más cercano → política común). | Elegir "más cercano" como regla del Core (es la mejor) y aceptar que Stage1/3 cambian; documentarlo como mejora, no como regresión. |
| R3 | Unificar carriles rompe posiciones de spawn afinadas a ojo. | Adoptar 505–625 como rango canónico en `Types.h`, borrar las locales, y revisar las 3 tablas de spawn (todas usan Y entre 525 y 615 → ya caen dentro). Riesgo bajo, verificado. |
| R4 | `StreetEnemy::Update(const Player&)` acopla IA y jugador; la FSM nueva lo hereda. | Introducir `struct CombatContext { Vector3D targetPos; bool targetAttacking; ... }` y hacer que el stage lo arme. VS y stages quedan iguales. |
| R5 | Borrar ProductionGame/GameManager/Scene/Enemy (1.700 líneas) puede borrar diseño útil. | No borrar en Fase 1. Mover a `legacy/` fuera de CMake, extraer de ahí lo aprovechable (el `BossState` de ProductionGame es la mejor base para el Boss común), borrar en Fase 10. |
| R6 | Sin `cmake`/red no puedo ejecutar el juego ni CI localmente. | Verificación por `-fsyntax-only` en cada TU tras cada cambio + tests nuevos escritos para ser *headless* (sin `InitWindow`). La validación visual queda de tu lado. |
| R7 | El CI corre `tools/validate_assets.py` antes de compilar; si endurece dimensiones, los assets nuevos lo tumban. | Revisar ese script antes de tocar `assets/` o `data/`. |
| R8 | `build/` (572 MB) viene en el paquete. Está en `.gitignore`, pero si alguien lo agrega, el repo se muere. | Confirmar `git check-ignore build/` antes del primer commit. |

---

## 7. Plan de fases revisado

El orden del brief (§29) es correcto en lo grande, pero le faltan dos pasos
previos que son precondición de todo lo demás. Propongo:

| Fase | Contenido | Compila | Commit |
|---|---|:-:|---|
| **0** | Este documento. Normalizar CRLF, crear `feature/DF-013-core-experience`. | ✔ | `DF-013: mapa de dependencias y saneo de rama` |
| **0.5** | Sacar `DrawText` de `Player.h` (D1). Mover código muerto a `legacy/`. Unificar carriles (D6). Corregir D5, D3, D4, D13. | ✔ | 4 commits separados (fix por defecto) |
| **1** | `game/combat/`: `CombatSystem`, `Projectile`, `Impact`, `HitResolver`. Los 3 stages + VS pasan a delegar. Se borran las 4 copias. | ✔ | `DF-013 F1: combat core común` |
| **2** | `AttackData` + tabla (JSON o tabla estática primero). PUNCH/KICK/ENERGY/DASH/RAGE/FINISHER. Resuelve D10. | ✔ | `DF-013 F2: attack data` |
| **3** | `EnemyBrain` FSM reutilizable + perfil por tipo. Resuelve D11. | ✔ | `DF-013 F3: IA diferenciada` |
| **4** | `StageEvent` + `EventRunner`. Las oleadas actuales se expresan como eventos. | ✔ | `DF-013 F4: stage events` |
| **5** | `CampaignManager` + `AppFlow`. Menú único, Stage Select, F1/F2/F3 → debug. Resuelve D9. | ✔ | `DF-013 F5: campaña y stage select` |
| **6** | `SaveData` versionado + `ProgressionSystem` + upgrades con efecto real. Resuelve D7, D8. | ✔ | `DF-013 F6: progresión` |
| **7** | UI 2.0 sobre el HUD común; un solo idioma. Resuelve D12. | ✔ | `DF-013 F7: UI` |
| **8** | Boss común (fases/telegraph/muerte) + narrativa. | ✔ | `DF-013 F8: boss system` |
| **9** | Hazards, pickups, destructibles, checkpoints, tutorial. | ✔ | `DF-013 F9: mundo` |
| **10** | Tests §25, borrar `legacy/`, docs, QA. | ✔ | `DF-013 F10: QA y limpieza` |

**Fase 0.5 no está en el brief y la agrego a propósito.** Intentar construir el
Combat Core encima de D1 (macro global), D6 (dos sistemas de carriles) y D5
(dificultad doble) significa heredar esos bugs dentro del núcleo compartido, que
es exactamente lo que §28 quiere evitar: "una modificación de gameplay no debe
arreglar una modalidad y romper otra".

---

## 8. Decisión pendiente antes de la Fase 1

Una sola, y cambia bastante el trabajo:

**¿Qué pasa con ProductionGame y GameManager?** Son 1.300 líneas muertas que
contienen la *mejor* versión de algunas cosas (el `BossState` parametrizado de
`ProductionGame.h`, el `SpawnDamageText`/`Particle` de `GameManager.h`, que ningún
stage vivo tiene). Mi recomendación: canibalizarlos como base del Combat Core y
del Boss común en vez de escribir de cero, y borrarlos en Fase 10.
