# Estudio: como esta hecho un personaje de King Fighter III (APK)

Material de estudio. La herramienta `tools/apk/kf_decode.py` lee el respaldo
`apk_reference/king_fighter_iii/bin/animation.bin` y el juego lo usa solo en el
**Modo VS // Laboratorio** (opcion `BOT REFERENCIA KF`, tecla **N** para ver
cada accion). El arte **no** se copio a `assets/`: se lee del respaldo y se
recolorea en memoria (giro de tono de 200 grados, formas intactas).

## Jerarquia (obtenida del codigo del juego: game.CGame, game.x, game.ba)

```
animation.bin
 ├─ 35 "sprites" (luchadores, enemigos, efectos, objetos)
 │    ├─ MODULOS  = pieza: recorte (x,y,w,h) de una imagen PNG
 │    ├─ FRAMES   = lista de piezas {modulo, x, y, transformacion}
 │    │             + indice de caja de golpe y caja de cuerpo
 │    └─ ACCIONES = secuencia de frames con duracion (ticks de 50 ms) y banderas
 └─ 213 imagenes PNG (con paletas alternativas = cambio de color del mismo arte)
```
Transformaciones por pieza: ninguna, espejo y rotaciones de 90/180/270 grados
(tabla interna -> MIDP `[0,6,3,5,2,1,7,4]`).

## Luchador principal (sprite 0)

| Dato | Valor |
|---|---|
| Piezas (modulos) | 311 |
| Frames | 521 |
| Acciones | 61 |
| Piezas por frame | media 18.1, maximo 95 |
| Cajas de golpe / cuerpo | 4 / 60 |
| Velocidad del juego | 20 ticks por segundo |

Ejemplo: un frame de la patada del combo 3 se arma con 19 piezas (sombra,
torso, brazos, antebrazos, piernas, guante y la estela del golpe).

| Accion | Uso | Pasos | Frames distintos | Duracion |
|---|---|---|---|---|
| A0 | reposo | 4 | 4 | 0.50 s |
| A1 | caminar | 8 | 8 | 0.40 s |
| A2 | correr (polvo) | 8 | 8 | 0.40 s |
| A3 | dash con chispas | 12 | 12 | 0.60 s |
| A4 | salto | 13 | 11 | 1.30 s |
| A5 | paso atras | 5 | 5 | 0.40 s |
| A6 | combo 1: jab + corte | 7 | 7 | 0.50 s |
| A7 | combo 2: cadena de 3 | 15 | 15 | 1.00 s |
| A8 | combo 3: patadas | 15 | 15 | 1.30 s |
| A9 | combo aereo / voltereta | 20 | 20 | 1.95 s |
| A10 | especial: gancho de fuego + corte | 18 | 18 | 1.40 s |
| A11 | recibe golpe | 5 | 5 | 0.45 s |
| A12 | colapso lento | 12 | 9 | 1.05 s |
| A13 | golpe fuerte + caida | 16 | 15 | 1.15 s |
| A14 | lanzado girando | 13 | 9 | 1.05 s |
| A15 | pose fija | 1 | 1 | 0.05 s |
| A16 | levantarse | 5 | 5 | 0.40 s |
| A17 | KO / derrota | 21 | 14 | 3.35 s |
| A18 | pose | 3 | 3 | 0.45 s |
| A19 | super (42 pasos) | 42 | 33 | 2.65 s |
| A20 |  | 37 | 37 | 2.40 s |
| A21 |  | 35 | 35 | 2.70 s |
| A22 |  | 38 | 20 | 3.35 s |
| A23 |  | 35 | 35 | 2.45 s |
| A24 |  | 47 | 25 | 2.70 s |
| A25 |  | 43 | 37 | 3.35 s |
| A26 |  | 1 | 1 | 0.05 s |
| A27 |  | 18 | 8 | 1.20 s |
| A28 |  | 1 | 1 | 0.05 s |
| A29 |  | 1 | 1 | 0.05 s |
| A30 |  | 5 | 5 | 0.25 s |
| A31 |  | 19 | 10 | 1.25 s |
| A32 |  | 3 | 2 | 0.25 s |
| A33 |  | 1 | 1 | 0.90 s |
| A34 |  | 2 | 2 | 0.10 s |
| A35 |  | 3 | 3 | 0.20 s |
| A36 |  | 11 | 11 | 0.55 s |
| A37 |  | 8 | 8 | 0.45 s |
| A38 |  | 12 | 11 | 0.60 s |
| A39 |  | 23 | 20 | 1.15 s |
| A40 |  | 16 | 16 | 0.95 s |
| A41 |  | 27 | 14 | 1.35 s |
| A42 |  | 38 | 17 | 3.80 s |
| A43 |  | 1 | 1 | 0.05 s |
| A44 |  | 1 | 1 | 0.05 s |
| A45 |  | 14 | 11 | 0.70 s |
| A46 |  | 17 | 17 | 1.25 s |
| A47 |  | 18 | 16 | 1.85 s |
| A48 |  | 16 | 8 | 1.20 s |
| A49 |  | 14 | 10 | 1.80 s |
| A50 |  | 20 | 12 | 1.45 s |
| A51 |  | 40 | 39 | 2.50 s |
| A52 |  | 1 | 1 | 0.05 s |
| A53 |  | 5 | 5 | 0.60 s |
| A54 |  | 5 | 5 | 0.60 s |
| A55 |  | 11 | 11 | 1.20 s |
| A56 |  | 22 | 18 | 3.95 s |
| A57 |  | 18 | 18 | 1.05 s |
| A58 |  | 7 | 5 | 0.75 s |
| A59 |  | 0 | 0 | 0.00 s |
| A60 |  | 0 | 0 | 0.00 s |

## Que se aprende para District Fury

1. **Combos = acciones largas encadenadas.** El combo 2 (A7) dura 15 pasos y
   el combo aereo (A9) 20: no son 2 frames por golpe, son anticipacion, impacto,
   estela y recuperacion. Nuestros golpes necesitan de 3 a 5 frames cada uno
   (ver docs/pedido_assets/PEDIDO_ASSETS.md).
2. **Los efectos son piezas aparte** (estelas, chispas, fuego), dibujadas encima
   del cuerpo. Conviene entregar los efectos en hojas separadas del personaje.
3. **Recibir golpe, caer, quedar en el suelo y levantarse** son 4 acciones
   distintas (A11, A13/A14, A16) y ya tienen su estado en nuestra FSM
   (Hit, Airborne, Knockdown, getup).
4. **Paletas:** el mismo PNG con otra paleta da otro personaje. Es la tecnica
   para crear variantes de enemigos sin redibujar.
5. **Piezas o frames completos:** la APK usa piezas para ahorrar memoria en
   moviles de 2010. Para nuestro pixel art, lo equivalente con menos trabajo es
   dibujar frames completos y reutilizar las capas de efecto.
