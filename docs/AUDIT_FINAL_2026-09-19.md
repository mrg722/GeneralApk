# District Fury — Auditoría completa tras integrar los 20 escenarios

Fecha: 19-09-2026. Rama `DF-013.2`.
Todo lo que sigue sale de leer el código, medir los assets y compilar.
**No verificado**: ejecución en pantalla (este entorno no tiene GPU ni Wine).

---

## 1. Estado general

| Bloque | % | Cómo se calculó |
|---|---|---|
| Técnico / build | 95% | Compila y enlaza exe + 2 tests desde cero; 1 archivo fuera del build (`Enemy.cpp`) |
| Stages y escenarios | 90% | 20 de 20 escenarios con fondo propio y encuentro; falta pulir transiciones |
| Campaña | 80% | Encadenada 1→5 con recompensa entre stages; sin cinemáticas ni selección de stage |
| Combate | 70% | Ataques, combos, dash, rage, guardia, hitstop reales; sin derribo/levantarse en enemigos |
| Enemigos | 75% | 8 tipos completos con IA; falta ciclo de caminar propio y derribo |
| Bosses | 70% | 5 bosses con sprite, fases reales y barra; una pose por estado, sin animación |
| Personajes | 70% | 6 jugables en VS; el moveset es el de Rayden en todos |
| VS | 85% | 5 stages, 8 enemigos, 5 bosses, 6 personajes; sin jugador 2 ni rounds |
| Arte | 80% | 91 PNG, 0 con matte blanco, 0 corruptos; falta animación |
| Audio | 20% | SFX generados por código; **cero archivos de audio**, sin música |
| UI / HUD | 85% | HUD común, menú, pausa, game over, stage clear, pantalla de suero |
| QA | 35% | 2 tests de humo + validador de assets; sin pruebas jugadas |

**Media ponderada: ~72%.** El salto desde el 60% de la mañana viene de los
escenarios, la campaña encadenada, los sprites de bosses y la corrección del
halo blanco.

---

## 2. Los 20 escenarios

| Stage | Escenarios | Fondo propio | Encuentro | Estado |
|---|---|---|---|---|
| 1 Slum District | 4 | 4/4 | oleadas + gatekeeper por escenario, Brakk al final | COMPLETO |
| 2 Old Steel Yard | 4 sectores (5 oleadas) | 4/4 | oleadas + Grinder | COMPLETO |
| 3 Astra Tower | 3 + sala de boss | 4/4 | gatekeepers + Titan-X | COMPLETO |
| 4 Kessler Tower | 4 oleadas | 4/4 | guardias de élite + Titan-X Mejorado | COMPLETO |
| 5 Cámara del Clon | 4 zonas | 4/4 | 3 salas con guardias + Rayder Clone | COMPLETO |

**20/20 escenarios con fondo propio y contenido.** Los fondos son 256x144 y se
dibujan x5 exacto (1280x720) con filtro POINT y parallax: pixel art nítido, sin
interpolación.

---

## 3. Personajes

| Personaje | Rol | Sprites | Estado |
|---|---|---|---|
| Rayden original | Jugable (campaña + VS) | atlas 4x4, 16 frames | Funcional; le falta el pack de animaciones de su hoja |
| Rayden clon | Jugable en VS + boss final | 11 poses | Funcional |
| Brakk | Boss Stage 1 + jugable en VS | 14 poses | Funcional |
| Grinder | Boss Stage 2 + jugable en VS | 7 poses | Funcional |
| Titan-X | Boss Stage 3 + jugable en VS | 12 poses | Funcional |
| Titan-X Mejorado | Boss Stage 4 + jugable en VS | 11 poses | Funcional |
| Dr. Kessler | NPC narrativo | 3 poses | Funcional, sin combate (por diseño) |
| 8 enemigos de calle | Enemigos | 8 atlas de 12 frames | Funcionales |

**Total: 15 personajes, todos con sprite real.** Ninguno queda dibujado con
figuras geométricas salvo como respaldo si faltara un PNG.

---

## 4. Assets

91 PNG: 23 fondos, 1 atlas de jugador, 8 atlas de enemigos, 55 poses de boss,
3 de Kessler, 1 de UI.

- **Matte blanco**: 0 archivos (auditados los 91).
- **PNG corruptos**: 0 (se repararon 2 con CRC inválido).
- **Rutas rotas**: 0 (el fondo del Stage 2 tiene respaldo).
- **Audio**: 0 archivos. Los SFX se sintetizan en código; no hay música.

---

## 5. Bugs

**Críticos**: ninguno conocido por lectura de código.

**Altos**: ninguno pendiente. Los dos de la mañana (fondo inexistente del
Stage 2 y halo blanco) están corregidos.

**Medios**
1. Enemigos sin derribo ni levantarse: la FSM no tiene ese estado.
2. Titan-X del Stage 3 resuelve el ataque en un instante: no telegrafía.
3. Bosses sin animación por frames: una pose por estado.
4. Los personajes jugables comparten el moveset de Rayden.

**Bajos**
5. `shake` en VS se asigna pero nunca se aplica ni decae.
6. `src/game/Enemy.cpp` (170 líneas) está fuera del build. **No se borró**.
7. Poses cargadas sin usar: `brakk_death/grab`, `grinder_death`, `titanx_*` de muerte.
8. F4/F5 sirven de atajo de stage y en el plan de debug eran hitboxes e IA.

---

## 6. Qué falta para el 100%

**CRÍTICO**
- Nada bloquea jugar la campaña de principio a fin.

**IMPORTANTE**
1. Animación por frames de bosses y personajes jugables (hoy: pose por estado).
2. Ciclo de caminar propio, derribo y levantarse en los 8 enemigos.
3. Audio: no hay ni un archivo. Música por stage y SFX reales.
4. Cinemáticas entre stages (hoy solo hay texto).
5. Movesets propios por personaje en VS.

**OPCIONAL**
6. Jugador 2 y rounds en VS.
7. Selección de stage y dificultad global.
8. Build de Android.

---

## 7. Compilación

Verificado en este ciclo: `district_fury.exe`, `application_state_tests.exe` y
`combat_smoke_tests.exe` compilan y enlazan limpio (cross-compile a Windows con
llvm-mingw), y `tools/validate_assets.py` pasa. 52 archivos de código, 6.342
líneas.

---

## 8. Fondos definitivos (lámina del diseñador, 19-09 tarde)

Llegaron los 20 escenarios nuevos, pero dentro de **una sola lámina de
contacto** de 1672x941, no como archivos sueltos.

Qué se hizo:
- Se detectó la rejilla (4 columnas x 5 filas), se recortaron los 20 paneles
  y se quitó la barra del rótulo de cada uno.
- Cada panel real mide **408x132 píxeles**. Se escalaron x2 con Lanczos y una
  máscara de enfoque suave: 816x276.
- El motor los escala a 720 de alto, así que ahora se cargan con filtro
  **BILINEAR** (son arte pintado, no pixel art de rejilla; POINT los dejaba
  dentados).

**Límite honesto**: de una lámina de contacto no se puede recuperar detalle
que no existe. Cada panel aporta unos 54.000 píxeles reales y la pantalla
pide 921.600. Se ven mucho mejor que los fondos planos anteriores, pero para
calidad final hacen falta los archivos individuales del diseñador, exportados
a 1280x720 (o como mínimo 1024x576). Al llegar, entran como reemplazo directo:
mismos nombres, misma ruta, sin tocar código.

---

## 9. Stage 1: arte bloqueado (decisión del usuario)

El Stage 1 vuelve a su fondo original y **queda congelado**: arte procedural
por escenario + la textura `bg_industrial`
(`assets/backgrounds/old_steel_yard_clean.png`, 1280x720).

`Stage1StoryGame::DrawScenarioArt()` quedó **byte a byte igual** a como estaba
antes de tocar los fondos (verificado comparando con el commit anterior).

Regla para el futuro: **no conectar ningún pack de fondos nuevo al Stage 1**.
Aunque lleguen fondos nuevos para el resto de los stages, este no se toca sin
que el usuario lo pida de forma explícita. Está escrito como comentario en el
propio archivo para que no se pierda.
