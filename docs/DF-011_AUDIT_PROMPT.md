# DF-011 - Prompt de auditoria y mejora

Audita el beat-em-up C++17/raylib a 1280x720 y 60 FPS antes de editar. Conserva la arquitectura reutilizable, Stage 1/2/3 y la animacion authored de Rayden.

## Prioridades

1. Reproduce y localiza cualquier elemento que siga a `player.position.x`; las barreras de arena deben usar coordenadas de mundo `arenaStart`/`arenaEnd`.
2. Verifica cada atlas enemigo con sus dimensiones reales. No uses un mapping universal: define por tipo los clips idle, walk, attack, hit y death, pivots y escala. Si falta el PNG, usa fallback estable y registra el asset faltante.
3. Modela el combate como estados Idle, Walk, Dash, Attack, Block, Hit, GuardBreak y Defeat. `B` bloquea, reduce daño y consume un escudo de 100 puntos. Tras 1.25 s sin impactos, regenera gradualmente; al llegar a cero entra en GuardBreak.
4. Mantiene hitstun corto, invulnerabilidad post-impacto, slots de ataque y separacion fisica para que los enemigos no se apilen.
5. Energy Wave debe cumplir CHARGE -> RELEASE -> RECOVERY. El proyectil aparece solo en `energyReleased`, nunca al iniciar la carga.
6. Toda la UI gameplay debe estar en espanol y medir el texto antes de dibujarlo. El HUD debe mostrar VIDA, ENERGIA, ESCUDO, FURIA, COMBO, PUNTOS y ESCENARIO.
7. Valida con CMake configure, build Release, tests de estado/combate y una prueba visual de Stage 1, Stage 2 y Stage 3.

## Evidencia de esta auditoria

- `origin/main` es la base de DF-010; la implementacion se realiza en `feature/DF-011-gameplay-overhaul`.
- Las barreras de Stage 1/2 seguian al jugador; ahora se dibujan desde el final fijo de cada arena.
- El ataque enemigo usaba frames 4..6, pertenecientes al clip de walk; ahora usa layout authored por tipo con clips attack/hit/death.
- Los PNG enemigos declarados en `data/sprite_manifest.json` no estan presentes en este checkout; el runtime conserva fallback y no introduce arte externo sin licencia verificada.
- Kenney fue revisado como fuente externa de assets con licencia permisiva; no se incorporo ningun archivo sin descargar, revisar y registrar su licencia.
