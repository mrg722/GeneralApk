# Estructura del código (DF-014)

Los archivos que eran una sola línea gigante se formatearon con `clang-format` (config en `.clang-format`, sin cambios de lógica). Los más grandes se dividieron por responsabilidad.

## Núcleo
| Archivo | Qué hace |
|---|---|
| `src/main.cpp` | Ventana, lienzo virtual 1280x720 escalado a la pantalla, campaña, cambio de stages, contexto táctil |
| `src/core/InputMap.*` | Entrada única: teclado + teclas virtuales (`input::Pressed`, `input::Down`) |
| `src/core/Platform.*` | Archivos iguales en PC y Android (assets dentro del APK, guardado interno) |
| `src/core/Json.*` | Lector JSON propio |
| `src/ui/TouchControls.*` | Joystick y botones táctiles por pantalla (combate, menú, fin, recompensa) |

## Jugador (`src/game/Player.h`)
| Archivo | Qué hace |
|---|---|
| `Player.cpp` | Estado, FSM (Idle, Walk, Attack, Recovery, Airborne, Knockdown…), daño, personaje elegido |
| `player/PlayerCombat.cpp` | Lectura de entrada, Input Buffer, comandos especiales, combos y cancelaciones |
| `player/PlayerDraw.cpp` | Dibujo del personaje, aura de furia, escudo y proyectil de energía |
| `player/PlayerCommon.h` | Constantes y tablas internas (cadena de combo, clips por ataque) |
| `InputBuffer.h` | Cola de 15 frames y reconocimiento de comandos (↓↘→ / →↓↘) |

## Stage 1 (`src/game/Stage1StoryGame.h`)
| Archivo | Qué hace |
|---|---|
| `Stage1StoryGame.cpp` | Ciclo de vida, flujo de pantallas (menú, elección de luchador, pausa…), dificultad, guardado, puntuación |
| `stage1/Stage1Waves.cpp` | Oleadas por línea de activación, bloqueo de cámara, guardianes de escenario |
| `stage1/Stage1Combat.cpp` | Golpes, proyectiles, impactos, partículas |
| `stage1/Stage1Boss.cpp` | Jefe Brakk |
| `stage1/Stage1Draw.cpp` | Todo el dibujo del stage |
| `stage1/Stage1Common.h` | Oleadas por escenario y helpers internos |
| `stage/ArenaDirector.*` | Cámara de scroll, oleadas, "GO >>" (reutilizable por otros stages) |

## Laboratorio (temporal)
| Archivo | Qué hace |
|---|---|
| `lab/KfReference.*` | Carga en tiempo de ejecución los personajes extraídos de la APK (`apk_reference/`) |
| `tools/apk/kf_decode.py` | Decodificador del formato de la APK, para estudio |

Para quitar el Laboratorio: borrar `src/game/lab/`, las entradas `KF ...` de `CharacterVisual.cpp`, el campo "RIVAL KF" de `VSMode` y la línea de `KfReference.cpp` en `CMakeLists.txt`.

## Herramientas de assets (`tools/`)
- **Hojas de personajes:** `build_rayden_sheet.py` y `build_rayder_sheet.py`.
- **Limpieza de recortes:** `clean_sprite_cutouts.py` (fondo atrapado y astillas) y `fix_edge_halo.py` (halo blanco).
- **Auditoría y fondos:** `audit_sprites.py` y `build_backgrounds.py`.
- **Android:** `android/build_apk.sh` (ver `docs/ANDROID_ADB.md`).

## Tests (`ctest`)
- `input_buffer_tests`: buffer, FSM, comandos especiales, cancelaciones.
- `sprite_manifest_tests`: manifiesto, celdas, clips.
- `touch_controls_tests`: botones → teclas → jugador.
- `stage1_playthrough_test`: un bot recorre el Nivel 1 completo.
- `combat_smoke_tests` y `application_state_tests`.

Herramientas visuales, que no forman parte de `ctest`: `stage1_visual_check` y `kf_bot_visual_check`.
