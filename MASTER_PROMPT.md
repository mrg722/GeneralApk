# DISTRICT FURY — MASTER PROMPT

## Rol
Actúa como un equipo senior de desarrollo de videojuegos trabajando sobre el repositorio de District Fury.

Objetivo: convertir el repositorio en un beat'em up 2D original, jugable y mantenible, desarrollado principalmente con C++17+, raylib y CMake.

Antes de implementar cualquier cosa, inspecciona el estado real del repositorio y respeta la arquitectura existente.

## Propiedad intelectual
District Fury es una IP original. Las referencias visuales/de gameplay solo sirven como inspiración.

NO copiar nombres, logos, personajes, sprites, música, sonidos, escenarios, textos, animaciones ni assets protegidos de otros juegos. Todo contenido final debe ser original o tener licencia apropiada.

## Concepto
- Nombre: District Fury
- Género: 2D beat'em up / brawler arcade con desplazamiento lateral y ligera profundidad.
- Plataformas objetivo: PC → Web/WASM → Android.
- Prioridad: gameplay → combate → enemigos/bosses → UI/progresión → audio/VFX → historia.
- Meta inicial: PC a 60 FPS.

## Protagonista: Rayden Cruz
Personaje original:
- 24 años
- atlético y musculoso
- cabello oscuro rizado
- lentes/visor azul
- chaqueta negra sin mangas
- camiseta blanca
- jeans oscuros/rasgados
- botas negras
- protección/guantes metálicos
- energía eléctrica azul

Habilidades:
- Punch Rush
- Rising Kick
- Energy Wave
- Dash
- Rage Mode
- Finisher/Ultra

## Sistemas principales
Implementar modularmente:

### Jugador
Movimiento horizontal y por profundidad, ataques, combos, dash, habilidades, Rage, recibir daño, knockback, hitstun, invulnerabilidad, muerte y reinicio.

### Combate
Cada ataque debería definir:
- daño
- startup
- active frames
- recovery
- alcance
- hitbox
- knockback
- hitstun
- coste de SP
- posibilidad de crítico
- prioridad
- animación/VFX/SFX

Separar hitbox, hurtbox, lógica de ataque, detección de impactos y respuesta al impacto.

Usar hitstop para mejorar el impacto.

### Combo
- contador de golpes
- ventana de continuidad
- multiplicador de score/daño cuando corresponda
- reset por tiempo/interrupción
- feedback visual

### Recursos
- HP
- SP
- Rage
- XP
- coins
- gems
- level

## Enemigos
Tipos iniciales:
- Punk
- Brute
- Brawler
- Charger
- Ranged
- Chemical Soldier
- Mutant
- Armored Guard
- Urban Ninja
- Drone

Usar FSM o arquitectura equivalente:
Idle → Patrol → Chase → Attack → Block → Hit → Stun → Retreat → Special → Dead

La IA debe ser reutilizable y configurable por datos.

## Bosses

### Stage 1 — Brakk "The Chain"
Jefe de pandilla, combate físico y cadena como arma, ataques telegráficos y patrones reconocibles.

### Stage 2 — Grinder
Antiguo luchador mutado, entorno industrial/químico, mínimo 2 fases.

### Stage 3 — Titan-X
Experimento químico/mecánico gigante, boss final, múltiples fases, ataques de área, shockwave y ventanas claras de castigo.

Los bosses deben compartir una arquitectura común.

## Stages

### Stage 1 — Slum District
Barrio urbano deteriorado, motel/bar, fuego, basura, pandillas e iluminación nocturna.

### Stage 2 — Old Steel Yard
Patio industrial, metal oxidado, tuberías, maquinaria, químicos e iluminación verde.

### Stage 3 — Astra Tower
Torre tecnológica/corporativa, laboratorios, drones y maquinaria avanzada.

## Flujo de stage
Intro → exploración → oleada → evento → oleada → mini-boss → oleada → boss → victoria → Stage Clear.

Los datos del stage deben permitir cambiar fácilmente enemigos, posiciones, eventos, boss y recompensas.

## Stage Clear
Mostrar:
- tiempo
- HP restante
- combo máximo
- score
- rango

Rangos: D → C → B → A → S → SS → SSS.

Mostrar XP, coins, gems y desbloqueos.

## Dificultad
- Easy
- Normal
- Hard

Modificar parámetros mediante datos, no duplicando código.

## Controles

### PC
- WASD: movimiento
- J: punch
- K: kick
- L: energy
- Shift: dash
- Space: Rage
- ESC: pausa

Preparar gamepad.

### Android
Joystick virtual y botones táctiles adaptables.

## UI
Crear como mínimo:
- Main Menu
- Stage Select
- HUD
- Pause
- Game Over
- Stage Clear
- Options
- controles
- HP/SP/Rage
- score
- combo
- recursos
- boss indicators
- feedback de daño

La UI debe estar desacoplada del gameplay.

## VFX / feedback
Priorizar sensación de impacto:
- partículas
- flash de impacto
- números de daño
- electricidad azul
- explosiones
- shockwaves
- screenshake moderado
- hitstop
- efectos de Rage y bosses

Lenguaje visual:
- azul: protagonista/energía
- rojo/naranja: peligro/impacto
- verde: químicos/mutación
- gris/negro: entorno industrial

## Arquitectura
Estructura orientativa:

src/
├── core/
├── game/
│   ├── player/
│   ├── enemies/
│   ├── bosses/
│   ├── combat/
│   ├── stages/
│   └── progression/
├── ui/
├── audio/
├── rendering/
├── input/
├── data/
└── main.cpp

No concentrar todo en main.cpp.

Preferir clases pequeñas, composición, sistemas independientes, datos configurables y pocas dependencias.

## Datos
Separar datos de lógica cuando sea conveniente.

Configurar por datos:
- stats
- enemigos
- ataques
- stages
- rewards
- dificultad
- balance

Evitar hardcodear valores que probablemente cambien.

## Build
- C++17 o superior
- raylib
- CMake

Cada cambio importante debe:
1. compilar
2. ejecutar tests relevantes
3. revisar warnings/errores importantes
4. comprobar manualmente la característica modificada cuando corresponda

Nunca afirmar que algo funciona sin comprobarlo.

## Testing
Crear tests para sistemas críticos cuando sea razonable:
- daño
- recursos
- combate
- estados
- progresión
- score
- desbloqueos
- guardado/carga

## Guardado
Guardar progreso de stages, nivel, recursos, configuración, dificultad y desbloqueos. Usar un formato simple y resistente cuando sea posible.

## Assets
Estructura:

assets/
├── sprites/
├── animations/
├── backgrounds/
├── ui/
├── vfx/
├── audio/
└── fonts/

Durante prototipo se permiten placeholders procedurales/temporales.

## Fases de desarrollo

### V0.1 — Vertical Slice
Rayden + movimiento + punch + kick + enemigo básico + daño + hitstun + combo + HUD + arena pequeña + win/game over + build funcional.

### V0.2
Más enemigos, combate, habilidades, IA y VFX.

### V0.3
Stage 1 completo + Brakk + Stage Clear + progresión básica.

### V0.4
Stage 2 + Grinder.

### V0.5
Stage 3 + Titan-X.

### V0.6
Menús completos + save/load + dificultad + balance.

### V0.7
Audio + VFX + pulido + optimización.

### V1.0
Juego PC completo + QA + documentación + build reproducible.

Posteriormente:
- V1.1 WebAssembly
- V1.2 Android

## Orden de prioridades
1. El juego arranca.
2. El personaje se mueve.
3. El combate se siente bien.
4. Los enemigos responden correctamente.
5. Se puede completar un stage.
6. Hay progresión.
7. UI y presentación.
8. Audio/VFX.
9. Optimización.
10. Extras.

No construir sistemas gigantes antes de tener un núcleo jugable.

## Reglas para agentes
Antes de modificar:
1. Leer README.md.
2. Leer ARCHITECTURE.md si existe.
3. Leer GAME_DESIGN.md si existe.
4. Revisar tareas/issues relacionadas.
5. Inspeccionar código existente.
6. Identificar dependencias y conflictos.

Al implementar:
- modificar solo lo necesario
- mantener compatibilidad
- evitar refactors masivos no solicitados
- documentar decisiones relevantes
- compilar
- ejecutar tests
- revisar que no se rompa funcionalidad existente

## Git / multi-agente
GitHub es la fuente central de verdad.

Nunca hacer que varios agentes modifiquen simultáneamente la misma rama principal o los mismos archivos sin coordinación.

Preferir ramas:
`feature/<task-id>-<name>`

Ejemplo:
`feature/DF-001-player-movement`

Cada tarea debería producir:
- cambios
- tests
- documentación necesaria
- commit claro
- Pull Request

Evitar merge directo a main cuando exista un flujo de PR.

## Criterio de finalización
Una tarea está terminada cuando:
- el comportamiento solicitado existe
- compila
- tests relevantes pasan
- no introduce errores obvios
- documentación actualizada cuando corresponde
- respeta arquitectura
- no rompe funcionalidades existentes
- está lista para revisión

## Regla fundamental
No optimices para generar mucho código. Optimiza para construir un juego real, jugable, mantenible y verificable.

Cuando una decisión sea entre añadir complejidad o mantener el sistema simple y extensible, elige la solución simple que cumpla el objetivo.

## Primera misión
Crear un V0.1 vertical slice jugable.

No intentar construir los 3 stages completos inmediatamente.

Meta:
Rayden → movimiento → ataque → enemigo → daño → combo → HUD → victoria/game over → compilación reproducible.

Después expandir incrementalmente.

**District Fury debe construirse paso a paso con GitHub + ramas + Pull Requests + CI como columna vertebral.**
