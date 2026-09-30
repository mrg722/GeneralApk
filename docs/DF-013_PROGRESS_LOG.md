# DF-013 — Registro de avance (sesion en curso)

Este documento resume, en orden, todo lo implementado sobre `mrg-gmes/`
hasta ahora. Nada de lo listado abajo borra codigo existente: los sistemas
viejos (ProductionGame, GameManager, Scene, Enemy) siguen en el arbol tal
cual estaban. Todo compila (`g++ -fsyntax-only` sobre cada `.cpp` del
proyecto, incluidos los tests) y `tools/validate_assets.py` sigue en verde.

## 1. Nucleo compartido (FASE 0.5 + FASE 1 + FASE 2 del plan)

- **`src/game/Types.h`**: unificados los dos sistemas de carriles que
  convivian (485-650 en Types.h vs 505-625 en cada stage, DF-013 D6).
  Ahora hay un unico rango canonico y una funcion `DepthScaleFor(y)`
  compartida para la escala de profundidad (antes duplicada con formulas
  ligeramente distintas en Player.cpp y StreetEnemy.cpp).
- **`src/game/combat/AttackData.h/.cpp`** (nuevo): los ataques dejaron de
  ser ternarios repartidos por `Player.cpp`. Ahora son datos: PUNCH (3
  golpes de combo), KICK, ENERGY WAVE, DASH ATTACK, RAGE ATTACK y
  FINISHER, cada uno con startup/active/recovery, hitbox, knockback,
  hitstun, coste de SP, ganancia de Rage, prioridad, cooldown y si
  cancela/rompe guardia. Agregar un ataque nuevo ya no toca los stages.
- **`src/game/combat/CombatWorld.h/.cpp`** (nuevo): el Combat Core que
  pide el brief. Centraliza proyectiles, particulas, pickups (HP/SP/Rage/
  monedas/buffs), destructibles y hazards (fuego/vapor/acido/electricidad),
  con resolucion de golpes jugador-enemigo, jugador-boss y proyectiles.
  Reemplaza el patron que existia \*4 veces\* (Stage1/2/3 +
  ProductionGame) para `HandlePlayerHits`, `HandleEnemyHits`,
  `HandleBossHits`, `UpdateProjectiles`, `SpawnImpact`, `UpdateParticles`.
  **Pendiente**: conectar los tres stages y VS a este `CombatWorld` (hoy
  conviven con sus copias originales, que se mantienen intactas para no
  romper nada mientras se migra).

## 2. Player (FASE 2)

`src/game/Player.h/.cpp` reescrito sobre `AttackData`, conservando toda
la API que consumian los stages (`attackType`, `GetAttackDamage()`,
`hasHit`, `energyReleased`, etc. siguen existiendo).

Nuevo:
- **Rage con estados reales**: `RageState::{Normal,Starting,Active,Ending}`
  con duracion perceptible (~9s activo) en vez de un simple booleano.
  El aura ahora envuelve el cuerpo (tres capas de elipses + lenguas de
  energia ascendentes), no es un circulo suelto.
- **Knockdown real**: golpes fuertes (dano >= 28) derriban al jugador en
  vez de solo aplicar el mismo `Hit` que un golpe ligero.
- **DASH ATTACK y FINISHER** funcionales (J durante el dash; K en Rage
  con combo alto).
- Buffs temporales de pickups (`damageBuffTimer`, `speedBuffTimer`).
- El acotado de carriles (`std::clamp` en Y) ahora vive en `Player::Update`
  para los tres stages por igual (antes Stage 2 no acotaba nada, DF-013 D3).

## 3. IA diferenciada (FASE 3)

- **`src/game/enemies/EnemyBrain.h/.cpp`** (nuevo): perfil de
  comportamiento por tipo (agresividad, distancia preferida, probabilidad
  de bloqueo, especial, armor). Los 8 tipos ya no comparten un unico
  cuerpo de `Update` con dos multiplicadores de velocidad como unica
  diferencia (DF-013 D11): cada uno tiene una respuesta distinta que el
  jugador debe leer (BRUTE con armor y golpe de area, CHARGER con carga
  telegrafiada, CHEMICAL_SOLDIER dejando zonas acidas, URBAN_NINJA
  flanqueando, ARMORED_GUARD bloqueando y contraatacando, etc).
- **`src/game/StreetEnemy.h/.cpp`** reescrito: la FSM paso de 5 a 11
  estados (`Idle, Patrol, Chase, Position, Attack, Block, Hit, Stun,
  Retreat, Special, Defeat`), con telegraphs visibles antes de los
  ataques y guardia con barra propia que se puede romper.
  El renderizado (atlas, medicion de frames, dibujo de respaldo sin
  sprite) se conservo tal cual estaba.

## 4. Fix del "aura blanca" / sprites mal cortados

**Diagnostico verificado sobre los PNG reales** (no es una suposicion):
los atlas de `assets/enemies/*.png` ya traen alfa parcial en los bordes,
pero el color RGB de esos pixeles de alfa bajo esta contaminado hacia
blanco (p. ej., a alfa~20 el color medio es (218,213,213), casi blanco;
a alfa~230 baja a (134,121,119), el color real). Eso es "straight alpha
matteado sobre blanco sin decontaminar": al dibujarlo sobre el fondo
oscuro del juego, esa contaminacion se ve exactamente como el halo/aura
blanca reportada, incluida la version "por dentro" en zonas concavas
(axilas, entre piernas).

**Fix aplicado** en `src/rendering/AssetManager.cpp`
(`DecontaminateWhiteMatte`): para cada pixel con alfa parcial se despeja
el color real asumiendo que fue compuesto sobre blanco
(`real = (guardado - 255*(1-a)) / a`), y los pixeles casi invisibles
(alfa < 14) se descartan en vez de decontaminarlos (numericamente
inestable). Se conserva el flood-fill de borde original como red de
seguridad para PNGs futuros con fondo blanco solido real.

Verificado visualmente (Python + PIL replicando el algoritmo exacto,
comparando antes/despues sobre fondo oscuro) en los 8 tipos de enemigo
antes de tocar el `.cpp`: el halo desaparece sin perder ningun pixel del
diseño. `rayden_clean.png` (el jugador) no tiene este problema — sus
bordes no estan contaminados hacia blanco — asi que no se toco su
loader.

## 5. Menu principal con arte final

- Arte del usuario copiado a `assets/ui/menu_main_art.png` y registrado
  en `AssetManager` (`"menu_main_art"`, filtro bilinear por ser
  ilustracion, no pixel art).
- **`src/ui/MainMenu.h/.cpp`** (nuevo): dibuja el arte a pantalla
  completa, parchea la columna de texto horneada en la imagen y redibuja
  los 7 items (NUEVA PARTIDA, MODO VS, DIFICULTAD, CONTROLES, OPCIONES,
  CREDITOS, SALIR) con una barra selectora, flecha y chevrones que
  imitan el estilo del arte original. Las posiciones se midieron sobre
  el PNG real (centroide de brillo por fila), no a ojo.
  Este modulo no incluye `Player.h`, por lo que no hereda la macro
  global `#define DrawText` de `ui/SpanishText.h` (bug D1 del audit
  anterior): sus textos van directo con la API de raylib.
- **`src/game/Stage1StoryGame.h/.cpp`**: el menu ahora tiene cursor real
  (arriba/abajo mueve entre 7 items, Enter/J activa el item resaltado).
  Se agregaron dos pantallas nuevas y funcionales:
  - `StoryFlow::Options`: alterna silenciar el audio (`AudioSystem`
    ahora soporta `SetMuted`/`IsMuted`).
  - `StoryFlow::Credits`: pantalla de creditos.
  Los atajos directos (V para VS, C para Controles) se conservan.
  `DrawMenu()` (el metodo viejo) no se borro, solo quedo sin uso.
- **`src/main.cpp`**: se elimino la duplicacion detectada en el audit
  (D9) donde `DrawMenuPrincipal()` en `main.cpp` y
  `Stage1StoryGame::DrawMenu()` dibujaban dos menus distintos y solo uno
  llegaba a verse. Ahora `Stage1StoryGame::Draw()` es la unica fuente de
  verdad; `main.cpp` solo enruta VS y el cierre de la aplicacion
  (`ExitRequested()` nuevo, para el item SALIR). `DrawMenuPrincipal`
  sigue definida en el archivo, solo dejo de llamarse.

## 6. Estado de verificacion

- Los 20 `.cpp` del proyecto (incluidos los dos ejecutables de test)
  compilan con `g++ -std=c++17 -fsyntax-only` contra el raylib
  vendorizado en `build/_deps/raylib-src`.
- `python3 tools/validate_assets.py` -> OK.
- El fix de sprites se verifico pixel a pixel contra los PNG reales
  (no es un cambio a ciegas).
- El menu se verifico renderizando un mockup fiel (mismas coordenadas
  exactas que el codigo C++) con Pillow para confirmar legibilidad en
  los 7 estados de seleccion antes de darlo por terminado.
- **No pude enlazar ni ejecutar el binario real** (sin `cmake` ni red en
  este entorno) — la verificacion de compilacion es por unidad de
  traduccion, no un build completo. Recomiendo compilar localmente antes
  de mergear.

## 7. Pendiente (siguientes fases del plan original)

- Conectar Stage1/2/3/VS al `CombatWorld` nuevo (hoy coexisten con las
  copias originales de `HandlePlayerHits`/etc., que siguen intactas).
- `ProgressionSystem` + `SaveData` versionado (FASE 6).
- `CampaignManager` / Stage Select real (FASE 5).
- Hazards/pickups/destructibles ya tienen sistema (`CombatWorld`) pero
  faltan colocarlos dentro de los stages.
- Corregir el resto de defectos puntuales del audit (D3 Stage2 sin
  clamp propio — ya cubierto por el clamp movido a `Player::Update` —,
  D4 Stage2 sin salida en Clear, D5 dificultad aplicada dos veces en
  Stage3, D13 fondo faltante `old_steel_yard_deep_clean.png`).
