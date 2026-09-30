# Personajes extraidos de la APK y sus habilidades

Temporales, solo en **Modo VS // Laboratorio**: se eligen como `PERSONAJE` (para jugar) y como `RIVAL KF (LAB)` (enemigo 1). Se leen de `apk_reference/` en tiempo de ejecucion y se recolorean en memoria. Para quitarlos: borrar `src/game/lab/`, las entradas `KF ...` de `CharacterVisual.cpp` y el campo del Modo VS.

Asignacion a nuestro juego (clip del juego <- accion de la APK):

| Nuestro clip | Heroe | Enemigo/jefe |
|---|---|---|
| reposo / caminar | A0 / A1 | A0 / A1 |
| golpe 1-2-3 (J J J) | A6 / A7 / A8 | A3 / A4 / A5 |
| patada (K) | A9 | A6 |
| onda / especial (L, comando) | A10 | A7 |
| dash / embestida | A3 | A2 |
| furia (Espacio en Furia) | A19 super | A6 |
| recibe golpe / en el aire / caida / levantarse / KO | A11 / A14 / A13 / A16 / A17 | A8 / A14 / A13 / A20 / A11 |

## KF HEROE (PELO BLANCO) (sprite 0)

311 piezas, 521 frames, 61 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.50 s |
| A1 | caminar | 8 | 0.40 s |
| A2 | correr | 8 | 0.40 s |
| A3 | dash | 12 | 0.60 s |
| A4 | salto | 13 | 1.30 s |
| A5 | paso atras | 5 | 0.40 s |
| A6 | combo 1 | 7 | 0.50 s |
| A7 | combo 2 | 15 | 1.00 s |
| A8 | combo 3 | 15 | 1.30 s |
| A9 | combo aereo / voltereta | 20 | 1.95 s |
| A10 | especial | 18 | 1.40 s |
| A11 | recibe golpe | 5 | 0.45 s |
| A12 | colapso | 12 | 1.05 s |
| A13 | golpe fuerte + caida | 16 | 1.15 s |
| A14 | lanzado | 13 | 1.05 s |
| A15 | pose fija | 1 | 0.05 s |
| A16 | levantarse | 5 | 0.40 s |
| A17 | KO | 21 | 3.35 s |
| A18 | pose | 3 | 0.45 s |
| A19 | super | 42 | 2.65 s |
| A20 | (pose/variacion) | 37 | 2.40 s |
| A21 | (pose/variacion) | 35 | 2.70 s |
| A22 | (pose/variacion) | 38 | 3.35 s |
| A23 | (pose/variacion) | 35 | 2.45 s |
| A24 | (pose/variacion) | 47 | 2.70 s |
| A25 | (pose/variacion) | 43 | 3.35 s |
| A26 | (pose/variacion) | 1 | 0.05 s |
| A27 | (pose/variacion) | 18 | 1.20 s |
| A28 | (pose/variacion) | 1 | 0.05 s |
| A29 | (pose/variacion) | 1 | 0.05 s |
| A30 | (pose/variacion) | 5 | 0.25 s |
| A31 | (pose/variacion) | 19 | 1.25 s |
| A32 | (pose/variacion) | 3 | 0.25 s |
| A33 | (pose/variacion) | 1 | 0.90 s |
| A34 | (pose/variacion) | 2 | 0.10 s |
| A35 | (pose/variacion) | 3 | 0.20 s |
| A36 | (pose/variacion) | 11 | 0.55 s |
| A37 | (pose/variacion) | 8 | 0.45 s |
| A38 | (pose/variacion) | 12 | 0.60 s |
| A39 | (pose/variacion) | 23 | 1.15 s |
| A40 | (pose/variacion) | 16 | 0.95 s |
| A41 | (pose/variacion) | 27 | 1.35 s |
| A42 | (pose/variacion) | 38 | 3.80 s |
| A43 | (pose/variacion) | 1 | 0.05 s |
| A44 | (pose/variacion) | 1 | 0.05 s |
| A45 | (pose/variacion) | 14 | 0.70 s |
| A46 | (pose/variacion) | 17 | 1.25 s |
| A47 | (pose/variacion) | 18 | 1.85 s |
| A48 | (pose/variacion) | 16 | 1.20 s |
| A49 | (pose/variacion) | 14 | 1.80 s |
| A50 | (pose/variacion) | 20 | 1.45 s |
| A51 | (pose/variacion) | 40 | 2.50 s |
| A52 | (pose/variacion) | 1 | 0.05 s |
| A53 | (pose/variacion) | 5 | 0.60 s |
| A54 | (pose/variacion) | 5 | 0.60 s |
| A55 | (pose/variacion) | 11 | 1.20 s |
| A56 | (pose/variacion) | 22 | 3.95 s |
| A57 | (pose/variacion) | 18 | 1.05 s |
| A58 | (pose/variacion) | 7 | 0.75 s |

## KF HEROINA (PELIRROJA) (sprite 2)

274 piezas, 471 frames, 56 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.60 s |
| A1 | caminar | 8 | 0.40 s |
| A2 | correr | 8 | 0.40 s |
| A3 | dash | 9 | 0.50 s |
| A4 | salto | 16 | 1.00 s |
| A5 | paso atras | 8 | 0.40 s |
| A6 | combo 1 | 11 | 0.80 s |
| A7 | combo 2 | 13 | 0.80 s |
| A8 | combo 3 | 22 | 1.50 s |
| A9 | combo aereo / voltereta | 20 | 1.40 s |
| A10 | especial | 22 | 1.55 s |
| A11 | recibe golpe | 5 | 0.40 s |
| A12 | colapso | 9 | 0.65 s |
| A13 | golpe fuerte + caida | 14 | 1.15 s |
| A14 | lanzado | 12 | 0.90 s |
| A15 | pose fija | 1 | 0.05 s |
| A16 | levantarse | 5 | 0.45 s |
| A17 | KO | 19 | 1.60 s |
| A18 | pose | 1 | 0.05 s |
| A19 | super | 42 | 2.50 s |
| A20 | (pose/variacion) | 50 | 3.65 s |
| A21 | (pose/variacion) | 50 | 3.25 s |
| A22 | (pose/variacion) | 46 | 2.95 s |
| A23 | (pose/variacion) | 59 | 4.30 s |
| A24 | (pose/variacion) | 48 | 2.95 s |
| A26 | (pose/variacion) | 1 | 0.05 s |
| A28 | (pose/variacion) | 1 | 0.05 s |
| A29 | (pose/variacion) | 1 | 0.05 s |
| A30 | (pose/variacion) | 1 | 0.05 s |
| A31 | (pose/variacion) | 16 | 0.80 s |
| A32 | (pose/variacion) | 1 | 0.05 s |
| A36 | (pose/variacion) | 12 | 0.80 s |
| A37 | (pose/variacion) | 14 | 0.70 s |
| A38 | (pose/variacion) | 12 | 0.80 s |
| A39 | (pose/variacion) | 23 | 1.15 s |
| A40 | (pose/variacion) | 1 | 0.05 s |
| A41 | (pose/variacion) | 1 | 0.05 s |
| A42 | (pose/variacion) | 1 | 0.05 s |
| A43 | (pose/variacion) | 1 | 0.05 s |
| A44 | (pose/variacion) | 1 | 0.05 s |
| A45 | (pose/variacion) | 1 | 0.05 s |
| A46 | (pose/variacion) | 11 | 0.95 s |
| A47 | (pose/variacion) | 1 | 0.05 s |
| A48 | (pose/variacion) | 1 | 0.05 s |
| A49 | (pose/variacion) | 1 | 0.05 s |
| A50 | (pose/variacion) | 3 | 1.00 s |
| A51 | (pose/variacion) | 1 | 0.05 s |
| A52 | (pose/variacion) | 1 | 0.05 s |
| A53 | (pose/variacion) | 1 | 0.05 s |
| A54 | (pose/variacion) | 1 | 0.05 s |
| A55 | (pose/variacion) | 1 | 0.05 s |

## KF MATON (sprite 6)

90 piezas, 92 frames, 34 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.40 s |
| A1 | caminar | 8 | 0.80 s |
| A2 | correr | 8 | 0.80 s |
| A3 | ataque 1 | 10 | 0.65 s |
| A4 | ataque 2 | 6 | 0.55 s |
| A5 | ataque 3 | 1 | 0.05 s |
| A6 | ataque 4 | 1 | 0.05 s |
| A7 | ataque especial | 1 | 0.05 s |
| A8 | recibe golpe | 4 | 0.60 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 16 | 1.50 s |
| A12 | golpe fuerte | 5 | 0.45 s |
| A13 | caida | 8 | 0.75 s |
| A14 | lanzado | 11 | 1.00 s |
| A15 | (pose/variacion) | 3 | 0.25 s |
| A16 | en el suelo | 8 | 0.85 s |
| A17 | (pose/variacion) | 1 | 0.05 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 7 | 0.65 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A22 | (pose/variacion) | 1 | 0.05 s |
| A23 | (pose/variacion) | 1 | 0.05 s |
| A24 | (pose/variacion) | 1 | 0.05 s |
| A25 | (pose/variacion) | 1 | 0.05 s |

## KF NAVAJERA (sprite 10)

112 piezas, 92 frames, 34 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.60 s |
| A1 | caminar | 8 | 0.80 s |
| A2 | correr | 8 | 0.40 s |
| A3 | ataque 1 | 6 | 0.45 s |
| A4 | ataque 2 | 12 | 0.70 s |
| A5 | ataque 3 | 12 | 0.65 s |
| A6 | ataque 4 | 6 | 0.45 s |
| A7 | ataque especial | 1 | 0.05 s |
| A8 | recibe golpe | 4 | 0.30 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 10 | 1.25 s |
| A12 | golpe fuerte | 5 | 0.50 s |
| A13 | caida | 8 | 0.80 s |
| A14 | lanzado | 15 | 1.00 s |
| A15 | (pose/variacion) | 6 | 0.30 s |
| A16 | en el suelo | 8 | 0.80 s |
| A17 | (pose/variacion) | 1 | 0.05 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 10 | 0.65 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A22 | (pose/variacion) | 1 | 0.05 s |
| A23 | (pose/variacion) | 1 | 0.05 s |
| A24 | (pose/variacion) | 1 | 0.05 s |
| A25 | (pose/variacion) | 1 | 0.05 s |

## KF SOLDADO (sprite 11)

116 piezas, 87 frames, 35 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.45 s |
| A1 | caminar | 8 | 0.80 s |
| A2 | correr | 8 | 0.80 s |
| A3 | ataque 1 | 9 | 0.75 s |
| A4 | ataque 2 | 7 | 0.70 s |
| A5 | ataque 3 | 9 | 0.90 s |
| A6 | ataque 4 | 7 | 0.60 s |
| A7 | ataque especial | 1 | 0.05 s |
| A8 | recibe golpe | 4 | 0.40 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 16 | 1.40 s |
| A12 | golpe fuerte | 6 | 0.50 s |
| A13 | caida | 10 | 0.75 s |
| A14 | lanzado | 9 | 1.00 s |
| A15 | (pose/variacion) | 3 | 0.25 s |
| A16 | en el suelo | 10 | 0.75 s |
| A17 | (pose/variacion) | 1 | 0.05 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 8 | 0.80 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A22 | (pose/variacion) | 1 | 0.05 s |
| A23 | (pose/variacion) | 1 | 0.05 s |
| A24 | (pose/variacion) | 1 | 0.05 s |
| A25 | (pose/variacion) | 1 | 0.05 s |
| A26 | (pose/variacion) | 1 | 0.05 s |
| A27 | (pose/variacion) | 1 | 0.05 s |
| A28 | (pose/variacion) | 4 | 0.20 s |
| A29 | (pose/variacion) | 1 | 0.05 s |
| A30 | (pose/variacion) | 1 | 0.05 s |
| A32 | (pose/variacion) | 4 | 1.05 s |
| A33 | (pose/variacion) | 1 | 0.10 s |
| A34 | (pose/variacion) | 2 | 0.40 s |

## KF RUBIA (sprite 13)

100 piezas, 80 frames, 29 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.40 s |
| A1 | caminar | 8 | 0.80 s |
| A2 | correr | 8 | 0.80 s |
| A3 | ataque 1 | 7 | 0.75 s |
| A4 | ataque 2 | 5 | 0.45 s |
| A5 | ataque 3 | 7 | 0.70 s |
| A6 | ataque 4 | 5 | 0.45 s |
| A7 | ataque especial | 1 | 0.05 s |
| A8 | recibe golpe | 4 | 0.40 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 13 | 1.80 s |
| A12 | golpe fuerte | 3 | 0.30 s |
| A13 | caida | 9 | 0.75 s |
| A14 | lanzado | 9 | 0.85 s |
| A15 | (pose/variacion) | 3 | 0.30 s |
| A16 | en el suelo | 8 | 0.80 s |
| A17 | (pose/variacion) | 1 | 0.05 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 7 | 0.60 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A22 | (pose/variacion) | 1 | 0.05 s |
| A23 | (pose/variacion) | 1 | 0.05 s |
| A24 | (pose/variacion) | 6 | 1.95 s |
| A25 | (pose/variacion) | 1 | 0.05 s |
| A26 | (pose/variacion) | 1 | 0.05 s |
| A27 | (pose/variacion) | 1 | 0.05 s |
| A28 | (pose/variacion) | 10 | 1.00 s |

## KF GORRA (sprite 14)

92 piezas, 83 frames, 29 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.40 s |
| A1 | caminar | 8 | 0.75 s |
| A2 | correr | 8 | 0.80 s |
| A3 | ataque 1 | 6 | 0.50 s |
| A4 | ataque 2 | 7 | 0.70 s |
| A5 | ataque 3 | 6 | 0.50 s |
| A6 | ataque 4 | 6 | 0.60 s |
| A7 | ataque especial | 1 | 0.05 s |
| A8 | recibe golpe | 4 | 0.40 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 18 | 1.65 s |
| A12 | golpe fuerte | 5 | 0.50 s |
| A13 | caida | 10 | 0.75 s |
| A14 | lanzado | 9 | 0.85 s |
| A15 | (pose/variacion) | 3 | 0.25 s |
| A16 | en el suelo | 10 | 0.75 s |
| A17 | (pose/variacion) | 9 | 0.90 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 6 | 0.50 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A22 | (pose/variacion) | 1 | 0.05 s |
| A23 | (pose/variacion) | 1 | 0.05 s |
| A24 | (pose/variacion) | 1 | 0.05 s |
| A25 | (pose/variacion) | 1 | 0.05 s |
| A26 | (pose/variacion) | 7 | 1.25 s |
| A27 | (pose/variacion) | 1 | 0.05 s |
| A28 | (pose/variacion) | 1 | 0.05 s |

## KF PELEADOR (sprite 16)

98 piezas, 123 frames, 26 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.70 s |
| A1 | caminar | 8 | 0.80 s |
| A2 | correr | 8 | 0.80 s |
| A3 | ataque 1 | 11 | 0.90 s |
| A4 | ataque 2 | 10 | 1.00 s |
| A5 | ataque 3 | 15 | 1.20 s |
| A6 | ataque 4 | 15 | 1.25 s |
| A7 | ataque especial | 20 | 1.65 s |
| A8 | recibe golpe | 5 | 0.35 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 15 | 1.75 s |
| A12 | golpe fuerte | 6 | 0.40 s |
| A13 | caida | 9 | 0.80 s |
| A14 | lanzado | 12 | 1.00 s |
| A15 | (pose/variacion) | 3 | 0.30 s |
| A16 | en el suelo | 4 | 0.40 s |
| A17 | (pose/variacion) | 1 | 0.05 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 8 | 0.60 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A25 | (pose/variacion) | 1 | 0.05 s |

## KF CUCHILLERO (sprite 18)

73 piezas, 96 frames, 26 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.40 s |
| A1 | caminar | 8 | 0.90 s |
| A2 | correr | 8 | 0.40 s |
| A3 | ataque 1 | 8 | 0.60 s |
| A4 | ataque 2 | 7 | 0.60 s |
| A5 | ataque 3 | 9 | 0.70 s |
| A6 | ataque 4 | 7 | 0.70 s |
| A7 | ataque especial | 1 | 0.05 s |
| A8 | recibe golpe | 4 | 0.40 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 17 | 1.80 s |
| A12 | golpe fuerte | 6 | 0.55 s |
| A13 | caida | 9 | 0.75 s |
| A14 | lanzado | 16 | 1.00 s |
| A15 | (pose/variacion) | 4 | 0.20 s |
| A16 | en el suelo | 9 | 0.75 s |
| A17 | (pose/variacion) | 1 | 0.05 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 5 | 0.45 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A22 | (pose/variacion) | 1 | 0.05 s |
| A23 | (pose/variacion) | 1 | 0.05 s |
| A24 | (pose/variacion) | 1 | 0.05 s |
| A25 | (pose/variacion) | 1 | 0.05 s |

## KF JEFE GARRA (sprite 12)

113 piezas, 109 frames, 27 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.50 s |
| A1 | caminar | 8 | 0.80 s |
| A2 | correr | 8 | 0.80 s |
| A3 | ataque 1 | 9 | 0.65 s |
| A4 | ataque 2 | 22 | 1.40 s |
| A5 | ataque 3 | 15 | 1.20 s |
| A6 | ataque 4 | 14 | 1.15 s |
| A7 | ataque especial | 8 | 0.45 s |
| A8 | recibe golpe | 4 | 0.60 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 20 | 1.75 s |
| A12 | golpe fuerte | 4 | 0.40 s |
| A13 | caida | 4 | 0.40 s |
| A14 | lanzado | 4 | 0.40 s |
| A15 | (pose/variacion) | 4 | 0.40 s |
| A16 | en el suelo | 4 | 0.40 s |
| A17 | (pose/variacion) | 1 | 0.05 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 1 | 0.05 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A22 | (pose/variacion) | 1 | 0.05 s |
| A23 | (pose/variacion) | 1 | 0.05 s |
| A24 | (pose/variacion) | 1 | 0.05 s |
| A25 | (pose/variacion) | 1 | 0.05 s |
| A26 | (pose/variacion) | 1 | 0.05 s |

## KF BUFONA (sprite 15)

106 piezas, 97 frames, 29 acciones.

| Accion | Que es | Pasos | Duracion |
|---|---|---|---|
| A0 | reposo | 4 | 0.40 s |
| A1 | caminar | 8 | 0.40 s |
| A2 | correr | 8 | 0.40 s |
| A3 | ataque 1 | 13 | 1.10 s |
| A4 | ataque 2 | 13 | 1.05 s |
| A5 | ataque 3 | 7 | 0.70 s |
| A6 | ataque 4 | 15 | 1.65 s |
| A7 | ataque especial | 13 | 0.80 s |
| A8 | recibe golpe | 4 | 0.60 s |
| A9 | (pose/variacion) | 1 | 0.05 s |
| A10 | (pose/variacion) | 1 | 0.05 s |
| A11 | derrota | 11 | 0.65 s |
| A12 | golpe fuerte | 3 | 0.30 s |
| A13 | caida | 3 | 0.30 s |
| A14 | lanzado | 3 | 0.30 s |
| A15 | (pose/variacion) | 3 | 0.30 s |
| A16 | en el suelo | 3 | 0.30 s |
| A17 | (pose/variacion) | 1 | 0.05 s |
| A18 | (pose/variacion) | 1 | 0.05 s |
| A19 | (pose/variacion) | 1 | 0.05 s |
| A20 | levantarse | 1 | 0.05 s |
| A21 | (pose/variacion) | 1 | 0.05 s |
| A22 | (pose/variacion) | 1 | 0.05 s |
| A23 | (pose/variacion) | 1 | 0.05 s |
| A24 | (pose/variacion) | 1 | 0.05 s |
| A25 | (pose/variacion) | 1 | 0.05 s |
| A26 | (pose/variacion) | 12 | 1.10 s |
| A27 | (pose/variacion) | 19 | 1.95 s |
| A28 | (pose/variacion) | 1 | 0.05 s |

