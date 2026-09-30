# Pedido de assets — District Fury V1

Lo que falta para que las mecánicas nuevas se vean completas en pantalla. La
lógica ya existe y funciona: buffer de 15 frames, cadena J-J-J-K, cancelación,
especiales por comando, estado en el aire, levantarse y bloqueo. Hoy varios
movimientos reutilizan el mismo frame, y los personajes quedarían mucho mejor
con frames propios.

## Reglas comunes (valen para todo lo que entregues)

| Regla | Valor |
|---|---|
| Formato | **PNG con transparencia real (canal alfa)**. Sin fondo blanco, negro ni degradado. |
| Números/etiquetas | **Fuera** del dibujo, o mejor sin números. Si hay número, que no toque al personaje. |
| Orientación | Mirando a la **derecha**. |
| Suelo | Todos los frames de una fila con los **pies en la misma línea de base**. En los saltos, el personaje se dibuja más arriba respecto de esa misma línea. |
| Tamaño | Personaje de pie de **180–200 px de alto** (el juego lo reduce a la mitad). |
| Separación | Al menos **24 px de aire** entre frames. Los efectos (arcos azules, polvo) no deben tocar al frame vecino. |
| Sombra | **Sin sombra en el piso** (el juego dibuja la suya). |
| Hoja | Una fila por movimiento, frames en orden de izquierda a derecha. |

Imágenes de referencia en esta carpeta:
- `rayden_frames_actuales.png`: los 22 frames que tiene hoy Rayden. Los marcados con `*` los derivé yo de sus píxeles y conviene reemplazarlos por arte hecho a mano.
- `rayder_frames_actuales.png`: las 50 poses de Rayder ya recortadas (índice = número de pose − 1).

---

## 1. Rayden Cruz — prioridad ALTA (36 frames)

Diseño: el mismo de `rayden_clean.png` (pañuelo azul, camiseta negra sin
mangas, jeans rotos, guantes negros, energía azul).

| Movimiento | Frames | Uso en el juego |
|---|---|---|
| Golpe 1 (jab) | 3: anticipo, impacto, retorno | J (1.er golpe de la cadena) |
| Golpe 2 (gancho) | 3: anticipo, impacto con arco azul, retorno | J (2.º golpe) |
| Golpe 3 (gancho ascendente) | 4: agacharse, subida, puño arriba con energía, caída | J (3.er golpe) y comando → ↓ ↘ + K |
| Patada media | 3 | K |
| Patada giratoria | 4 | K tras 2 golpes |
| Onda de energía | 4: carga, lanzamiento, brazo extendido, recuperación | ↓ ↘ → + J (o L) |
| Proyectil de energía | 4 (ciclo) | Sprite aparte, ~96×64 px |
| Recibe golpe alto | 2 | hit_high |
| Recibe golpe bajo | 2 | hit_low |
| Lanzado por el aire | 3 | airborne |
| En el suelo | 1 | knockdown (ya existe) |
| Levantarse | 3 | getup |
| Bloqueo | 2 | B |
| Victoria | 2 | fin de nivel |

**Prompt (copiar tal cual en el generador):**

```
Pixel art sprite sheet, 2D beat 'em up fighter, SNK arcade style, character "Rayden Cruz":
athletic young man, curly black hair, blue bandana over the eyes, black sleeveless tank top,
ripped blue jeans with holster, black fingerless gloves and arm guards, blue energy effects.
Facing right, full body, feet aligned on the same baseline in every frame of a row,
character height about 190 px, 24 px empty space between frames, TRANSPARENT BACKGROUND (alpha),
no floor shadow, no text, no numbers, no border lines.
Rows: (1) jab 3 frames: anticipation, impact, recovery; (2) hook punch 3 frames with blue arc on impact;
(3) rising uppercut 4 frames: crouch, rising, fist up with blue energy, landing;
(4) mid kick 3 frames; (5) spinning roundhouse kick 4 frames;
(6) energy wave 4 frames: charge, release, arm extended, recovery;
(7) hit high 2 frames, hit low 2 frames; (8) knocked into the air 3 frames;
(9) getting up 3 frames; (10) guard block 2 frames; (11) victory 2 frames.
Consistent lighting and palette with the idle frame, crisp pixel edges, no anti-aliasing to white.
```

## 2. Rayder — prioridad MEDIA (12 frames)

Ya tiene 50 poses. Le faltan:

| Movimiento | Frames | Por qué |
|---|---|---|
| Caminar | 6 (ciclo) | Hoy camina con solo 2 poses (3 y 4) |
| Bloqueo | 2 | Hoy usa la pose agachada 9 |
| Girarse | 2 | Cambio de dirección |
| Proyectil de energía | 2 más | Hoy solo existe la pose 40 (esfera) |

**Prompt:**

```
Pixel art sprite sheet, same character as the provided "Rayder" sheet: spiky black hair with white streak,
black leather jacket with blue emblems, white shirt, black cargo pants, blue lightning energy.
Facing right, feet on the same baseline, ~190 px tall, TRANSPARENT BACKGROUND, no text or numbers,
24 px spacing. Rows: (1) walk cycle 6 frames; (2) guard block 2 frames; (3) turn around 2 frames;
(4) blue energy sphere projectile 3-frame loop (separate, ~120x90 px).
```

## 3. Enemigos — prioridad MEDIA (8 frames por enemigo)

Los 8 enemigos (`punk`, `charger`, `brute`, `enforcer`, `chemical_soldier`,
`urban_ninja`, `mutant`, `armored_guard`) tienen 12 frames. Hoy caminan con la
animación de reposo y no tienen pose en el aire.

| Movimiento | Frames |
|---|---|
| Caminar | 4 |
| Lanzado por el aire | 2 |
| Levantarse | 2 |

**Prompt (cambiar el nombre y describir al enemigo):**

```
Pixel art sprite sheet for the enemy "<NOMBRE>" matching the attached reference sheet exactly
(same outfit, colors and proportions). Facing right, feet on one baseline, ~190 px tall,
TRANSPARENT BACKGROUND, no floor shadow, no text, 24 px spacing between frames.
Rows: walk cycle 4 frames; launched into the air 2 frames; getting up 2 frames.
```

## 4. Efectos de impacto — prioridad MEDIA

| Efecto | Frames | Tamaño |
|---|---|---|
| Chispa de golpe ligero | 4 | 64×64 |
| Chispa de golpe fuerte | 5 | 96×96 |
| Polvo al caer | 4 | 96×48 |
| Destello de bloqueo | 3 | 64×64 |

**Prompt:**

```
Pixel art VFX sprite sheet for a 2D arcade brawler: light hit spark 4 frames (64x64),
heavy hit burst 5 frames (96x96, orange/white), landing dust 4 frames (96x48),
blue guard flash 3 frames (64x64). TRANSPARENT BACKGROUND, one row per effect, no text.
```

## 5. Fondos — prioridad ALTA para el Stage 1

Tu imagen del escenario 1 (Barrio Bajo) ya está en el juego en HD. Los otros
escenarios solo existen como tiras de 816×276 o como miniaturas de ~390 px en
las hojas de referencia, y se ven borrosos a pantalla completa. Hace falta cada
uno **por separado, a resolución completa**:

| Escenario | Estado |
|---|---|
| stage1_scenario02 — Mercado antiguo / canal | falta HD |
| stage1_scenario03 — Puerta de acero / ruta de carga | falta HD |
| stage1_scenario04 — Astillero de cadenas (arena de Brakk) | falta HD |
| stage2_scenario01 — Tuberías | ✅ entregado |
| stage2_scenario02 — Fundición | ✅ entregado |
| stage2_scenario03, stage2_scenario04 | falta HD |
| stage3 a stage5 (12 escenarios) | falta HD |

Formato: **1600×900 o mayor**, 16:9 o más ancho, sin texto. El suelo tiene que
ocupar el tercio inferior (ahí caminan los personajes) y conviene que sea plano
y despejado. Es mejor que el borde izquierdo y el derecho combinen, porque el
fondo se desplaza con la cámara.
