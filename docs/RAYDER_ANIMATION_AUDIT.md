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