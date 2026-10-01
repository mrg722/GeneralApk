# Rayder / Rayder Clone — auditoria y pase de animacion

## Alcance protegido

Esta rama mejora exclusivamente la presentacion y la respuesta visual de Rayder y Rayder Clone usando recursos que ya existen en el repositorio. No se modifican los perfiles/atlases de los personajes KF ni se reemplazan sus movimientos. Rayder Cruz/KF se usa solamente como referencia de calidad de timing y lectura visual.

## Hallazgos confirmados

- El runtime ya tiene FSM de jugador, buffer de entrada de 15 frames, cancelaciones, hitstop, knockdown, dash, ataques con startup/active/recovery y animacion por manifiesto.
- El atlas rayder tiene clips reales para idle, walk, dash, punch1-3, kick, energy, dash_attack, rage_attack, finisher, block, hit, airborne, knockdown, getup, defeat, recovery, victory, turn y projectile.
- El clon jugable usa poses sueltas (idle1-4, ready, punch, kick, dash, release_orb, hurt, death) y antes seleccionaba una sola pose durante cada ataque.
- El boss final tambien usaba una sola pose por ataque aunque el asset del clon contiene varias poses reutilizables.

## Cambios de esta rama

1. Rayder Clone jugable: secuencia anticipacion -> impacto -> recuperacion con poses existentes.
2. Rayder Clone boss: misma idea en Stage 5; cada ataque cambia de pose segun el tiempo real del ataque.
3. Rayder normal: micro squash/stretch, transferencia de peso, anticipacion antes del startup, acento de impacto y recuperacion visual. Es render-only: no cambia hitboxes, dano, velocidad, IA ni estadisticas.
4. KF y Rayder Cruz: no se modifican sus atlas, clips ni logica de movimiento.

## Criterios

- Anticipacion antes del impacto.
- Impacto corto y legible; no se agregan frames ficticios.
- Recuperacion visible despues del golpe.
- Pies/pivote se conservan; la escala/rotacion se hace alrededor del pivote existente.
- Se mantiene el pipeline actual de raylib. DrawTexturePro permite controlar escala/rotacion alrededor del origen del sprite; raylib tambien documenta el patron de sprite animation por frames.

## No se hizo

- No se generaron sprites nuevos.
- No se cambiaron danos, hitboxes, stats, controles, sistema KF ni assets KF.
- No se eliminaron funciones existentes.

## Validacion

Se usa el CI existente del repositorio: validate_assets.py, CMake y CTest. El workflow actual corre en pull requests y en pushes configurados; esta rama se deja lista para validacion mediante PR.

## Fase 2 — sincronización combate/animación

La segunda fase añade un perfil de timing reutilizable para los atlas propios de Rayder normal (skin 6) y Brakk (skin 2).

- AttackData sigue siendo la fuente de verdad para startup/active/recovery, daño, prioridad, hitstop, shake y reglas de combo.
- AttackAnimationProfile únicamente asocia el clip existente, frame visual de impacto y root motion por ataque.
- Los clips existentes se retimean a la duración total de AttackData; no se crean ni reemplazan sprites.
- El root motion se aplica incrementalmente y con curva de anticipación/aceleración, termina al final de active y no desplaza al personaje durante recovery.
- El frame de impacto se expone mediante AttackIsImpactFrame() y el cambio startup -> active se registra con attackImpactTriggered.
- Rayder Clone (skin 1) conserva sus poses sueltas, pero sus transiciones ahora usan AttackData.startup y AttackData.active en vez de tiempos visuales independientes.
- KF/Rayder Cruz siguen fuera del perfil: IsKfCharacter() evita retime, root motion nuevo y squash/stretch de esta fase.
- Se añadió F3 como overlay de depuración de Rayder/Brakk: ataque, tiempo total, frame actual, frame de impacto, fase, hitbox y root motion. Es solo diagnóstico y no altera el combate.
- Se añadió tests/AttackAnimationTimingTests.cpp para comprobar perfiles, fases y root-motion.

### Ataques cubiertos

Punch1, Punch2, Punch3, Kick, EnergyWave, DashAttack, RageAttack y Finisher.

### Brakk

Se usan directamente sus clips existentes del manifiesto brakk_v2: punch1, punch2, punch3, kick, energy, dash_attack, rage_attack y finisher. No se cambia su daño ni sus cajas de combate.

### Rayder

Se usan sus clips existentes de rayder_kf para los mismos ocho ataques. La duración visual se adapta al timing real del ataque para evitar que el arte termine antes/después que el estado de combate.

### Validación

El PR #2 ejecuta el workflow de CI con validate_assets.py, CMake, build y CTest. La ejecución más reciente observada estaba en progreso; no se marca como pasada hasta que GitHub reporte success.
