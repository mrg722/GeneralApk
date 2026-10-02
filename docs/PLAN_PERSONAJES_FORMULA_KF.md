# Fórmula KF: qué compone a cada personaje y plan para crear los nuestros

Datos medidos directamente de `apk_reference/king_fighter_iii/bin/animation.bin`
(35 sprites y 213 imágenes en total). Imágenes de apoyo en `docs/formula_kf/`.

## 1. Cómo se arma un personaje KF (los 6 niveles)

| Nivel | Qué es | Ejemplo del héroe pelo blanco |
|---|---|---|
| 1. **Imágenes** (hojas PNG) | Hojas con muchos dibujos sueltos. Una es el **despiece del cuerpo**; las demás son **efectos**. | 26 imágenes |
| 2. **Recortes** (clips) | Rectángulos dentro de cada hoja: cada uno es **una pieza**. | 208 recortes en la hoja del cuerpo |
| 3. **Módulos** | Lista de piezas que el personaje puede usar (= imagen + recorte). | 311 módulos |
| 4. **Cuadros** (frames) | Cada cuadro coloca piezas: posición x,y + giro/espejo (8 transformaciones). | 521 cuadros, 9.447 piezas colocadas, 18 por cuadro de media (máx. 95) |
| 5. **Acciones** | Secuencias de cuadros con tiempo (1 tick = 50 ms). Son los movimientos y habilidades. | 61 acciones |
| 6. **Cajas** | Caja de golpe (hitbox) y caja de cuerpo (bodybox) para el combate. | 4 de golpe, 60 de cuerpo |

El dibujo nunca es un cuadro entero: **todo cuadro se compone de piezas**. Por eso
con ~200 piezas salen 500 cuadros y 60 movimientos.

## 2. Los 2 personajes principales (y sus formas transformadas)

| | **Héroe pelo blanco** (sprite 0) | Héroe transformado (sprite 1) | **Heroína pelirroja** (sprite 2) | Heroína transformada (sprite 3) |
|---|---|---|---|---|
| Imágenes que usa | **26** | 15 | **24** | 11 |
| Hoja del cuerpo (despiece) | img 1: **208 piezas** (usa 205) | img 1 (la misma): usa 131 | img 30: **132 piezas** (usa 132) | img 30 (la misma): usa 107 |
| Piezas de efectos | **105** en 24 hojas | 36 en 13 hojas | **141** en 22 hojas | 31 en 9 hojas |
| Módulos totales | **311** | 168 | **274** | 139 |
| Cuadros | **521** | 176 | **471** | 79 |
| Piezas colocadas en total | **9.447** | 3.098 | **8.937** | 1.485 |
| Piezas por cuadro (mín/media/máx) | 0 / 18,1 / 95 | 0 / 17,6 / 30 | 0 / 19,0 / 81 | 1 / 18,8 / 24 |
| Acciones | **61** | 57 (≈20 reales) | **56** | 50 (≈12 reales) |
| Cajas golpe / cuerpo | 4 / 60 | 2 / 8 | 3 / 60 | 2 / 15 |

La forma transformada **no tiene despiece propio**: reusa la hoja del cuerpo de la
normal y agrega auras, siluetas brillantes y efectos.

**Total de los 4:** 892 módulos: 575 de cuerpo (sobre 340 piezas distintas: 208 + 132) y
317 de efectos y sombra, 1.247 cuadros, 22.967 piezas colocadas y 224 acciones.

### 2.1 Despiece del cuerpo del héroe pelo blanco (img 1, 208 piezas)

Etiquetadas a mano una por una (`tools/build_kf_clone_rig_canva.py`, `LABELS`):

| Parte | Piezas | Números |
|---|---|---|
| Cabeza sola | 13 | 11 16 27 99 115 125 138 167 169 170 179 194 205 |
| Torso con cabeza | 13 | 9 80 86 94 122 124 129 130 154 168 183 191 199 |
| Torso (con hombreras) | 18 | 7 31 41 49 58 63 101 105 131 143 144 145 147 152 156 176 177 193 |
| Brazo superior / manga | 9 | 0 14 33 44 59 65 84 153 201 |
| Antebrazo / brazo | 32 | 10 13 18 19 26 34 35 36 45 46 54 56 57 66–69 93 97 98 100 111 160 172 173 178 197 202–204 206 207 |
| Puño / mano | 17 | 17 23 25 55 74 85 87 104 107 108 112 117 121 132–134 164 |
| Muslo | 30 | 3 6 22 29 39 40 47 51 60 70 72 76 79 81 89 90 96 110 123 128 135 146 148 181 186 187 189 195 198 200 |
| Pierna (canilla) | 29 | 2 5 20 21 37 38 48 50 62 71 75 82 91 95 109 118 120 126 139 151 157 161 162 165 174 180 188 190 196 |
| Bota | 23 | 1 4 28 30 42 43 52 53 61 64 73 77 78 116 119 127 136 140 141 149 155 163 192 |
| Cinturón | 5 | 8 12 15 32 175 |
| Otros (cintas, brillos y estelas dentro de la hoja) | 19 | 24 83 88 92 102 103 106 113 114 137 142 150 158 159 166 171 182 184 185 |

Ver `docs/formula_kf/despiece_heroe_img1.png` (cada pieza con su número).

### 2.2 Despiece del cuerpo de la heroína (img 30, 132 piezas)

Clasificación aproximada, a ojo (todavía no está etiquetada pieza por pieza). Ver `docs/formula_kf/despiece_heroina_img30.png`.

| Parte | Piezas aprox. |
|---|---|
| Cabeza sola (pelo rojo) | 8 |
| Torso con cabeza | 15 |
| Brazos (piel) | ~25 |
| Manos y guantes rojos | ~26 |
| Piernas y botas (verde oliva) | ~31 |
| Dedos y accesorios chicos | ~10 |
| Efectos dentro de la hoja (llamas, cortes, brillos) | ~13 |
| Sombra, cuerpo caído entero | 2 |

Es más pequeña que la del héroe (132 contra 208) porque usa más piezas por cuadro
(19,0 de media) y más efectos (141 contra 105).

### 2.3 Hojas de efectos

**Héroe** (`docs/formula_kf/efectos_heroe.png`):

| Hoja | Piezas | Qué es |
|---|---|---|
| img 2 | 10 | estelas de movimiento |
| img 3 | 5 | chispas de golpe |
| img 4 | 24 | efectos grandes (fuego, explosión, rayo, cortes) |
| img 5 | 7 | silueta eléctrica |
| img 6 | 5 | chispas |
| img 7 | 3 | polvo |
| img 8 | 8 | cortes de fuego |
| img 9 | 5 | esferas de energía |
| img 10, 11, 14, 19, 21 | 1 c/u | siluetas doradas o brillantes (transformación y súper) |
| img 12 | 10 | llamas |
| img 13 | 1 | estela de giro |
| img 15 | 3 | bola de fuego (proyectil) |
| img 16, 18, 20, 24 | 1 c/u | siluetas sombra (imágenes residuales) |
| img 17 | 4 | anillos de fuego |
| img 22 | 3 | arcos de corte |
| img 23 | 4 | aura roja |
| img 25 | 4 | explosiones |

**Heroína** (`docs/formula_kf/efectos_heroina.png`):

| Hoja | Piezas | Qué es |
|---|---|---|
| img 3, 4, 6, 7, 17 | — | las mismas que el héroe |
| img 33 | 7 | energía |
| img 34 | 22 | cortes blancos |
| img 35 | 4 | arcos |
| img 36, 38, 43 | 8, 1, 3 | siluetas doradas |
| img 37 | 25 | tajos |
| img 40, 41, 45 | 7, 4, 2 | arma (lanza) |
| img 42 | 6 | tajos rojos |
| img 46 | 14 | piezas de intro |

Hojas compartidas por casi todos los personajes: img 0 (sombra), img 3 (chispas),
img 4 (efectos grandes), img 7 (polvo).

## 3. Movimientos y habilidades

Tiempos: pasos/cuadros distintos/duración. "+fx" = usa hojas de efectos.

### Héroe pelo blanco (61 acciones; **≈45 movimientos reales**, el resto son poses de 1 cuadro)

| Grupo | Acciones |
|---|---|
| Base | A0 reposo (4 c, 0,5 s) · A1 caminar (8) · A2 correr (8 +polvo) · A3 dash (12) · A4 (13 c, 1,3 s) · A5 retroceso (5) |
| Golpes | A6 (7 c) · A7 (15) · A8 (15) · A9 (20 c, 1,95 s) · A10 especial (18 c, 7 hojas fx) |
| Recibir y caer | A11 golpe (5) · A12 (9) · A13 derribo (15) · A14 aire (9) · A15 bloqueo (1) · A16 levantarse (5) · A17 derrota (14 c, 3,35 s) · A18 (3) |
| **Habilidades grandes** | **A19 súper** (42 pasos, 33 c, 2,65 s, 8 fx) · **A20–A25** (35–47 pasos, 2,4–3,35 s, 5–10 fx cada una) · A27 · A30 · **A31 transformación** · **A42 despertar** (3,8 s) · A46 · A47 · A50 · **A51** (40 pasos, 7 fx) · A56 (3,95 s) |
| Acrobacias | A36 voltereta · A37 giro · A38 rodada · A39 patada aérea · A40 caída · A41 torbellino · A45 mortal · A55 contra · A57 salto tigre |

En el juego, el héroe y Rayder Clon tienen **23 habilidades** en páginas de 6 botones.

### Heroína pelirroja (56 acciones; **≈30 movimientos reales**)

| Grupo | Acciones |
|---|---|
| Base | A0 reposo (+fx46) · A1 caminar · A2 correr · A3 dash · A4 · A5 |
| Golpes | A6 · A7 · A8 (22 c, 1,5 s) · A9 · A10 (9 hojas fx) |
| Recibir y caer | A11–A17 |
| **Habilidades grandes** | **A19 súper** (42 c, 2,5 s) · **A20** (50 pasos, 3,65 s) · **A21** (50 p, 3,25 s) · **A22** (46 p) · **A23** (59 pasos, **4,3 s**, la más larga) · **A24** (48 p) · A31 (16 p) · A46 |
| Acrobacias | A36 · A37 · A38 · A39 |

### Receta de una habilidad KF (promedio de A19–A25)

- **35–59 pasos**, entre 20 y 59 cuadros distintos, **2,4–4,3 s**.
- **5–10 hojas de efectos** a la vez: estela, chispa, fuego o rayo, silueta residual, polvo y explosión.
- Estructura: anticipación (4–8 cuadros) → carga con aura → 2–4 impactos con efecto → remate → recuperación.

## 4. Lo que ya tenemos con esta fórmula

| Personaje | Cómo está hecho |
|---|---|
| Héroe KF pelo blanco | Original, intacto (regla). |
| **Rayder Clon** (rojo) | Mismos cuadros y acciones del héroe; cada una de las 208 piezas del cuerpo cambiada por una pieza del clon (sin mangas, del boceto) a 3x de resolución; efectos pintados de rojo; 23 habilidades. |
| Rayder Clon BETA | Copia del héroe con piezas recoloreadas (morado). |

## 5. Plan para crear NUESTROS personajes y habilidades con la fórmula KF

### Fase 0: herramientas (base para todo)

1. **Formato propio** `data/rigs/<personaje>.json` con piezas, cuadros (pieza + x,y + giro/espejo), acciones (cuadro + ms) y cajas de golpe y de cuerpo. Es el mismo modelo de 6 niveles que el KF, pero en un archivo nuestro y editable.
2. **Exportador** `animation.bin` → formato propio. Así cualquier acción KF sirve de plantilla de tiempos y posiciones.
3. **Cargador C++** general: hoy `KfReference` solo lee `animation.bin`; pasa a leer también `data/rigs/*.json`.
4. **Visor de cuadros** (Python): dibuja cualquier acción pieza por pieza, con números, para revisar antes de meterla al juego.
5. **Etiquetado del despiece de la heroína** (132 piezas), como ya está el del héroe.

### Fase 1: despiece de un personaje nuevo (lo que hay que dibujar o generar)

Mínimo, tomando la heroína como referencia (132 piezas); completo, como el héroe (208).

| Parte | Mínimo | Completo | Ángulos |
|---|---|---|---|
| Cabezas | 8 | 13 | frente, 3/4, perfil, nuca, arriba, abajo, grito, golpeado |
| Torso con cabeza | 8 | 13 | los mismos |
| Torsos | 10 | 18 | frente, 3/4, lado, espalda, inclinado, girado |
| Brazo superior | 6 | 9 | |
| Antebrazos | 16 | 32 | estirado, doblado, en guardia |
| Puños y manos | 10 | 17 | cerrado, abierto, golpe |
| Muslos | 16 | 30 | |
| Canillas | 16 | 29 | |
| Botas | 12 | 23 | lado, frente, punta, suela |
| Cinturón y accesorios | 5 | 10 | |
| **Total** | **~107** | **~200** | |

**Formato:**
- PNG con transparencia, una pieza por casilla y 24 px de aire alrededor.
- Mirando a la derecha.
- Mismo largo de brazo y de pierna en todas las vistas.

**Fuente:** boceto u hoja de modelo (frente, lado y espalda) → Canva o Figma generan las hojas por parte del cuerpo → recorte automático (`tools/clone_sin_mangas_pieces.py` es el ejemplo).

### Fase 2: armar el cuerpo (la fórmula)

- **Opción A (rápida, ya funciona):** reemplazar las 208 piezas del héroe KF por las nuestras, cada una ajustada por forma, ángulo y largo. Con eso salen los 521 cuadros y las 61 acciones al instante. Así está hecho Rayder Clon.
- **Opción B (propia):** con el formato de la Fase 0, mover piezas, crear cuadros nuevos y cambiar los tiempos.

### Fase 3: habilidades nuevas (paso a paso, como una del KF)

1. **Ficha:** nombre, botón y página, daño, cooldown, duración objetivo (2,4–4,3 s), alcance y tipo (golpe, proyectil, área o agarre).
2. **Guion de cuadros:** anticipación (4–8) → carga (6–10, con aura) → impactos (2–4, cada uno con efecto y caja de golpe) → remate → recuperación (4–6). Total: 35–50 pasos.
3. **Poses:** se reusan cuadros que ya existen (521 en el héroe) o se crean nuevos moviendo piezas.
4. **Hoja de efectos de la habilidad (8–25 piezas):** estela, chispa, energía, silueta residual y explosión. Se genera con Canva o Figma, o se recolorea un efecto existente (así salió el rojo de Rayder Clon).
5. **Cajas:** de golpe en los cuadros de impacto y de cuerpo en todos.
6. **Datos del juego:** `abilityClips` y `abilityNames`, más el perfil de ataque (`AttackAnimationProfile` de GPT: impacto, root motion, anticipación).
7. **Pruebas:** `skills_visual_check`, `roster_sheets` y un test de tiempos (`AttackAnimationTimingTests`).

### Fase 4: personaje nuevo completo (prueba: el personaje verde pendiente)

1. Boceto y hoja de modelo (frente, lado, espalda, paleta y detalles).
2. Despiece (Fase 1): ~107–200 piezas.
3. Efectos propios: ~100–140 piezas en 15–24 hojas (las de chispas, polvo y sombra se comparten).
4. Armado (Fase 2-A) → revisión con el visor → ajustes de piezas.
5. Habilidades propias (Fase 3): 6 grandes + súper + transformación.
6. Forma transformada: la misma hoja del cuerpo + aura + siluetas + efectos (como los sprites 1 y 3).
7. Al juego: `KfRoster`, `CharacterVisual`, retrato, botones y APK.

### Fase 5: calidad (regla: el nivel de Rayden Cruz)

- **Comparar contra el boceto:** pies en la misma línea, proporciones, brazos (con o sin mangas), colores y silueta.
- **Revisar el detalle:** imágenes de cerca de 8 poses y de las 6 habilidades.
- **Validar:** probar en el teléfono y pasar los tests.

### Nota sobre originalidad

Hoy Rayder Clon usa las posiciones y tiempos del KF con piezas nuestras. Para una
versión publicable, la Fase 0-B y la Fase 3 permiten tener **cuadros y acciones
propios**: el KF queda solo como referencia de cómo se construye.

## 6. Rayder Clon: estado actual

- **Hecho:**
  - Sin mangas como el boceto: chaleco, camiseta rota, brazos desnudos con cicatrices, guanteletes con núcleo rojo en los puños, cinturón negro, pantalón roto, botas grandes y contorno.
  - 8 cabezas generadas con Canva a partir del boceto.
  - 23 habilidades en rojo.
- **Falta:** piezas de torso, brazos y piernas en alta resolución y más ángulos, generadas con Canva desde el boceto. Hoy salen de la hoja del personaje, que es chica (1024 px).
- **Bloqueos:**
  - **Canva:** sin créditos (`quota_exceeded`).
  - **Figma Weave:** la cuenta de Figma no está vinculada. Se vincula en https://app.weavy.ai/settings?section=profile.
