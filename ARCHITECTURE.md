# District Fury — Arquitectura

## Tecnologías principales
- C++17+
- raylib
- CMake

## Estructura objetivo

```text
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
```

## Principios
- Mantener el gameplay modular.
- Separar entrada, simulación, render y UI cuando sea práctico.
- Mantener hitbox/hurtbox y resolución del combate independientes del render.
- Diseñar enemigos y bosses con sistemas reutilizables.
- Mantener los parámetros de balance configurables.
- Aislar código específico de plataforma para facilitar Web y Android posteriormente.
- Evitar dependencias innecesarias.

## Decisión de arquitectura: CombatEntity / EnemyProfile / BossDefinition (2026-09-17)

El usuario propuso explícitamente esta jerarquía como objetivo para evitar
20.000-50.000 líneas de lógica duplicada:

```text
CombatEntity
├── Player
├── StreetEnemy → EnemyProfile (Punk, Charger, Brute, Enforcer,
│                 ChemicalSoldier, UrbanNinja, ArmoredGuard, Mutant)
└── Boss → BossDefinition (Brakk, Grinder, Titan-X, Titan-X Mejorado,
           Rayder Clone)
```

Estado real verificado en el código (no supuesto):

- **StreetEnemy + EnemyProfile: YA IMPLEMENTADO segun este mismo patrón.**
  `src/game/enemies/EnemyBrain.h/.cpp` define `struct EnemyProfile` (hp,
  velocidad, daño, rango, comportamiento de IA, `EnemySpecial`) y
  `GetEnemyProfile(StreetEnemyType)` devuelve la tabla de datos por tipo.
  `StreetEnemy.cpp` es una única clase que consume el perfil — los 8
  enemigos NO son 8 clases, son 1 clase + 8 filas de datos, exactamente
  como pide la propuesta. No hay nada que migrar aquí.

- **Boss + BossDefinition: NO IMPLEMENTADO — duplicación real confirmada.**
  Cada stage tiene su propio struct de boss, su propio `enum BossAttack`,
  y su propia lógica de `UpdateBoss/DrawBoss/HandleBossHits` copiada y
  ajustada a mano:
  - `Stage1StoryGame.h` → `struct StoryBoss` (Brakk)
  - `Stage2Game.h` → `struct Grinder`
  - `Stage3Game.h` → boss Titan-X (struct propia)
  - `Stage4Game.h` → `struct TitanXImproved`
  - `Stage5Game.h` → `struct RayderClone`
  Los 4 bosses SÍ comparten ya `AttackData`/`CombatWorld` para el daño al
  jugador y los proyectiles, pero la máquina de estados de cada boss
  (fases, ataques, IA, dibujo) está duplicada por stage, no en una clase
  `Boss` común + `BossDefinition` de datos.

Plan de migración (siguiente fase mayor, NO ejecutada aún en este
checkpoint porque tocar los 5 stages a la vez sin poder verificar
visualmente el juego es el tipo de refactor masivo no-solicitado que
prohíben las reglas del proyecto salvo que aporte valor claro — aquí sí
lo aporta, pero se hace de forma incremental y verificable):

1. Definir `struct BossDefinition` y `struct BossPhaseDefinition` en
   `src/game/combat/BossDefinition.h` (solo datos: hp, nº de fases,
   velocidad, lista de `AttackDefinition` reutilizando `AttackData`,
   tamaño/color para el dibujo con primitivas).
2. Definir una única clase `Boss` en `src/game/combat/Boss.h/.cpp` que
   interprete un `BossDefinition` (Update genérico de fases/ataques,
   Draw genérico parametrizado por color/escala) — igual que `EnemyBrain`
   interpreta `EnemyProfile` para `StreetEnemy`.
3. Migrar los stages UNO A LA VEZ (empezando por el más simple, Stage2/
   Grinder) sustituyendo su `struct Grinder` + `UpdateBoss/DrawBoss`
   locales por `Boss boss{GetBossDefinition(BossId::Grinder)}`,
   compilando y comparando el comportamiento resultante contra el actual
   antes de tocar el siguiente stage.
4. Repetir para Brakk (Stage1), Titan-X (Stage3), Titan-X Mejorado
   (Stage4) y Rayder Clone (Stage5) — dejando Titan-X y Titan-X Mejorado
   como dos `BossDefinition` distintos que comparten identidad visual
   (organico-mecánico verde) pero difieren en escala/fases, tal como
   exige el canon de `GAME_DESIGN.md`.
5. Solo cuando los 5 stages usen `Boss`/`BossDefinition`, eliminar los
   structs de boss antiguos de cada `Stage*Game.h`.

Introducir una interfaz abstracta `CombatEntity` (virtual) que herede
`Player`/`StreetEnemy`/`Boss` es un cambio de mayor riesgo (afecta firmas
usadas en todo `CombatWorld`, `Player.cpp`, `StreetEnemy.cpp`) y se deja
para después de completar la unificación de bosses, cuando ya exista una
clase `Boss` real con la que probar la interfaz sin tocar 5 archivos a
la vez.
