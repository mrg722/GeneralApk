# Personajes jugables (lista y hojas)

`LISTA.md` enumera **todos los personajes que se pueden elegir** (Modo VS: campo PERSONAJE; Nivel 1: Rayden Cruz y Rayder) y enlaza su hoja. Cada hoja la genera el propio juego (`./build/roster_sheets docs/personajes_jugables`): una fila por clip, cuadro a cuadro, tal como se dibuja en combate.

| Grupo | Personajes | De dónde sale el arte |
|---|---|---|
| Propios | Rayden Cruz, Rayder, Brakk | `assets/` (hojas del usuario) |
| Jefes con poses sueltas | Rayden clon, Grinder, Titan-X, Titan-X mejorado | `assets/bosses/<nombre>/` |
| **Laboratorio KF (temporales)** | Héroe (pelo blanco), Heroína, Matón, Navajera, Soldado, Rubia, Gorra, Peleador, Cuchillero, Jefe Garra, Bufona, **Héroe transformado**, **Heroína transformada** | Se leen en tiempo de ejecución de `apk_reference/` y se repintan en memoria |

## Cómo funciona un personaje de King Fighter (lo que lo hace fluido)

1. **Piezas, no dibujos enteros** (`kf_piezas_heroe.png`). El héroe tiene 311 piezas (cabeza, torso, brazos, antebrazos, piernas, guantes, estelas y fuego). Cada frame junta unas 18, cada una con su posición y su giro o espejo. Así salen 521 frames a partir de pocas imágenes.
2. **Muchos frames cortos.** Cada paso dura 50 ms (20 por segundo): reposo 4, caminar 8, combo 1 7, combo 2 15, súper 42. Nuestros personajes tenían de 2 a 4 dibujos de 100 a 200 ms.
3. **El movimiento está en el dibujo.** El personaje avanza, se inclina y retrocede dentro de los propios frames, y la estela del golpe está en el mismo cuadro.
4. **Los poderes vienen en la animación.** El fuego, los pilares y los cortes de cada habilidad (A20 a A24) y la transformación dorada (A31) son piezas del mismo frame. Por eso en el juego cada habilidad KF reproduce su propia animación completa, y la caja de golpe llega hasta donde llega el fuego en ese cuadro.

En nuestros personajes se imita así:

- **Rayder:** hoja nueva con 6 a 8 frames por acción.
- **Avance real durante el golpe.**
- **Respiración, balanceo, inclinación, giro, aterrizaje y estelas.**
- **6 habilidades con espera de 15 s.**
- **Transformación:** Rayder carga energía (12 cuadros), pasa a su hoja de pelo blanco durante 12 s y luego vuelve.

## Sprites del archivo `animation.bin`

`kf_todos_los_sprites.png` muestra los 35 sprites:

| Sprites | Qué son |
|---|---|
| 0 a 3 y 6 a 18 | Luchadores. El 1 es el héroe transformado (llamas rojas) y el 3 la heroína transformada (lanza): al usar TRANSFORMAR, el héroe y la heroína pasan a esas formas con sus propios golpes. |
| 9 y 10 | Retratos y textos de la interfaz. |
| 19 en adelante | Objetos y vehículos. |

`kf_sprite1_heroe_transformado.png`, `kf_sprite2_heroina.png` y `kf_sprite3_heroina_transformada.png` muestran todas sus acciones.

Para quitar el laboratorio, ver `docs/ESTRUCTURA_CODIGO.md`.
