# District Fury

Videojuego 2D beat'em up / brawler original desarrollado en C++17 y raylib.

## Estado actual
La campaña contiene cinco stages (seleccionables con F1–F5; el encadenamiento automático entre stages todavía no está implementado):

- **Stage 1 — Slum District:** cuatro escenarios narrativos, gatekeepers y Brakk "The Chain" (sprite real, 14 poses).
- **Stage 2 — Old Steel Yard:** Deep Line, roster químico/industrial y Grinder (sprite real, 7 poses).
- **Stage 3 — Astra Tower:** Public Atrium, Research Floor, Executive Core y Titan-X (prototipo, sprite real).
- **Stage 4 — Kessler Tower:** guardias de élite y Titan-X Mejorado (forma final, sprite real). Intro con el Dr. Kessler (NPC narrativo, no combate).
- **Stage 5 — Cámara del Clon:** duelo final contra Rayder Clone (sprite real). Intro con el Dr. Kessler.

Estado detallado, verificado y pendiente: `docs/FINAL_REPORT.md` y `docs/AUTONOMOUS_PROGRESS.md`.

La base técnica incluye mundo horizontal, cámara 2D, combate con hitboxes/hurtboxes, combos, proyectiles, hitstop, partículas, dificultad y flujos de victoria/derrota/pausa.

## Integridad de sprites enemigos — DF-011.4

Cada `StreetEnemyType` utiliza su propio atlas. Los ocho atlas enemigos son PNG RGBA de **512x384**, organizados en **4 columnas x 3 filas de 128x128**. Los bounds alfa se miden desde cada atlas y el pivote se calcula en la zona inferior para mantener los pies anclados durante animaciones y ataques.

La secuencia común es **0–3 idle**, **4–7 ataque**, **8–9 impacto** y **10–11 derrota**. El filtro de textura para los personajes es `TEXTURE_FILTER_POINT` para evitar interpolación borrosa.

## Modo VS / Laboratorio

Desde el menú principal se pulsa **V** para entrar a un laboratorio aislado. Permite seleccionar:

- Stage 1 a Stage 5.
- El escenario disponible de cada Stage.
- Entre 1 y 4 enemigos simultáneos.
- El tipo independiente de cada enemigo entre los ocho `StreetEnemyType`.
- **PERSONAJE:** Rayden original (por defecto) o Rayden clon. El clon solo cambia el aspecto: stats, ataques y hitboxes son idénticos.
- **BOSS:** Brakk, Grinder, Titan-X, Titan-X Mejorado o Rayder Clone en duelo 1 vs 1, usando la clase compartida `Boss` + `BossDefinition` (`src/game/combat/`).

El laboratorio utiliza las mismas clases de runtime (`Player`, `StreetEnemy` y `AssetManager`) para que escala, animaciones, pivotes, hitboxes, hurtboxes y barras se comprueben sobre el juego real.

### Controles del Modo VS
- **↑ / ↓:** cambiar campo.
- **← / →:** cambiar valor.
- **ENTER / J:** iniciar.
- **R:** reiniciar.
- **ESC:** volver a configuración y luego al menú.

## DF-012 — Pase de integración visual

Esta rama añade el pase visual solicitado sin crear una segunda arquitectura de juego:

1. **Rayden:** escala visual moderada del atlas normal para que quede ligeramente por encima de la referencia de los enemigos sin sobredimensionarlo.
2. **Menú principal:** panel más limpio, jerarquía visual y acceso explícito al laboratorio VS.
3. **HUD del laboratorio:** panel de Rayden con retrato, VIDA, SP, FURIA y ESCUDO; además, cada enemigo muestra nombre y barra de vida.
4. **Mercado Antiguo:** fondo 2D pixelado independiente para el escenario narrativo de la Línea del Canal.
5. **Zona Química:** fondo 2D pixelado independiente para el escenario químico asociado a Old Steel Yard.
6. **Carga de fondos:** los dos assets nuevos se cargan como recursos opcionales con filtro punto. Esto evita que una copia local anterior del proyecto falle por no haber recibido todavía el paquete visual.
7. **Validación:** el validador mantiene obligatorios los atlas de combate y valida dimensiones/formato de los nuevos fondos cuando están presentes.

### Assets nuevos
Copiar desde el paquete de esta rama:

```text
assets/backgrounds/mercado_antiguo_clean.png
assets/backgrounds/zona_quimica_clean.png
```

Los archivos de runtime están preparados a **256x144** para escalar exactamente 5x a 1280x720 con `TEXTURE_FILTER_POINT`. No contienen personajes ni texto de menú: son fondos utilizables por el motor.

## Controles de PC
- WASD: movimiento
- J: punch
- K: kick
- L: energy
- Shift: dash
- Space: Rage
- ESC/P: pausa
- F1: Stage 1
- F2: Stage 2
- F3: Stage 3
- V: Modo VS / Laboratorio desde el menú principal

## Arquitectura
```text
src/core/       estado global de la aplicación
src/game/       jugador, enemigos, stages y game flow
src/rendering/  animator y carga/procesamiento de assets
src/audio/      eventos y audio procedural
assets/         recursos visuales
data/           datos configurables
tests/          pruebas
docs/           documentación
```

GitHub es la fuente central de verdad. Los cambios grandes se desarrollan en ramas y se integran mediante Pull Requests después de CI.
