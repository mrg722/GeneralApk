# Personaje nuevo por piezas (Camino A, formato Kratos)

Documento tecnico: que necesita el motor para animar un personaje nuevo
igual que Kratos (`src/game/beta/BetaCharacter.cpp` + `data/kratos/*.txt`),
sin esqueleto Spine. Escrito ANTES de tener la hoja de piezas, como
contrato para lo que hay que generar/dibujar y para la matematica que yo
(Claude) voy a escribir y auditar cuando la hoja este lista.

## 1. Que piezas se necesitan (lista minima, como Kratos)

Kratos reusa ~15-20 piezas unicas por arma (234 PNG en total para sus 4
armas, compartiendo piezas entre ellas). Categorias minimas para un
personaje nuevo, cada una **recortada sola, sin fondo, sin superponerse
con otra pieza en su propio PNG**:

| Pieza | Para que sirve | Notas |
|---|---|---|
| Cabeza | gestos, giros | 1 sola, se puede voltear en X |
| Torso/pecho | cuerpo central | ancla de todo lo demas |
| Brazo superior (x1, se refleja) | hombro->codo | igual en ambos lados por defecto |
| Antebrazo+mano (x1, se refleja) | codo->mano | si el personaje sostiene un arma en una mano especifica, esa mano va aparte (no se refleja) |
| Mano con arma / arma sola | ataques | puede ser pieza separada de la mano si el arma se suelta en algun golpe |
| Cadera/pelvis | ancla de piernas | |
| Muslo (x1, se refleja) | cadera->rodilla | |
| Pierna/bota (x1, se refleja) | rodilla->pie | |
| Capa/accesorio suelto (opcional) | si el personaje tiene | pieza aparte, se mueve con retraso (secondary motion) |

Con eso (8-9 piezas) ya alcanza para los clips basicos (idle, walk, run,
golpes, patada, bloqueo, caida). Para poderes/habilidades especiales se
agregan piezas de efecto (aura, proyectil, resplandor) como imagenes
sueltas, igual que `assets/kratos/actor/`.

## 2. Formato del archivo de datos (igual al que ya lee el motor)

```
character <id> hero <altura_base_px>
name <NOMBRE EN PANTALLA>
image <n> <id_pieza>          # una linea por cada pieza referenciada, n=indice interno

frame <n> <piezas> <cx0> <cy0> <cx1> <cy1> <ax0> <ay0> <ax1> <ay1>
piece <imagen_idx> <src_x> <src_y> <src_w> <src_h> <dst_x> <dst_y> <flip>
piece ...                      # una linea "piece" por cada pieza visible en este frame

clip <nombre> <loop 0|1> <n_frames> <frame:duracion> <frame:duracion> ...
ability <id> <NOMBRE EN PANTALLA>
sound hurt <archivo>
sound die <archivo>
sound dash <archivo>
```

- `cx0,cy0,cx1,cy1`: caja de cuerpo (hitbox que puede recibir golpes), en
  pixeles relativos a los PIES del personaje (0,0 = pies), **negativo hacia
  arriba**.
- `ax0,ay0,ax1,ay1`: caja de ataque (donde pega), misma convencion, solo
  rellena en los frames donde el golpe conecta.
- `piece`: `src_x,src_y,src_w,src_h` = que rectangulo recortar de la hoja
  de la pieza; `dst_x,dst_y` = donde pegarlo, relativo a los pies;
  `flip` = 0/1/2/3 (sin voltear / horizontal / vertical / ambos), igual
  que `FLIPS` en `GowApk/tools/beta/beta_anim.py`.

## 3. La matematica que yo tengo que generar (sin Spine)

Para cada frame de cada clip, por cada pieza visible:

1. **Posicion base**: offset fijo pieza->pieza en la pose de reposo
   (idle), medido a mano sobre la hoja de referencia (cabeza respecto al
   torso, brazo respecto al hombro, etc.) — esto define el "esqueleto"
   sin que exista un esqueleto real: son numeros fijos, no huesos con
   jerarquia.
2. **Interpolacion entre poses clave**: si solo dibujamos poses clave
   (ej. inicio y fin de un golpe) en vez de cada frame intermedio, yo
   interpolo linealmente `dst_x,dst_y` entre el frame clave anterior y
   el siguiente (y el angulo de recorte si la pieza rota, ver punto 4)
   para generar los frames intermedios — esto es lo que reemplaza al
   motor de Spine, a mano, con matematica simple (lerp), no composicion
   de matrices de huesos padre-hijo.
3. **Rotacion de una pieza (brazo, pierna)**: como no hay huesos reales,
   roto la pieza como imagen (rotar el recorte) y recalculo su `dst_x,
   dst_y` para que el punto de union (hombro, codo, cadera) quede fijo
   en el lugar correcto — la pieza entera pivotea sobre ese punto, nunca
   sobre su propio centro. Esto es trigonometria simple (seno/coseno
   sobre el punto de pivote), no jerarquia de huesos.
4. **Auditoria obligatoria antes de dar un frame por bueno** (para no
   repetir el bug que encontramos en la limpieza de sprites): despues de
   generar cada frame, lo renderizo y reviso que:
   - ninguna pieza quede "flotando" separada del cuerpo (gap visual en
     una articulacion),
   - ninguna pieza se superponga de forma rara con otra de un frame
     distinto (mismo chequeo de componentes conectados que usamos en la
     auditoria de atlas, pero aplicado a la pieza compuesta final),
   - la silueta completa del frame se vea anatomica frente a sus vecinos
     del mismo clip (igual criterio que usaron los agentes auditores).

## 4. Que necesito de la imagen que vamos a generar

- **Pose neutra tipo "A-pose"**: de pie, brazos un poco separados del
  cuerpo (no pegados), piernas un poco separadas, vista de 3/4 o de
  frente igual al resto del roster.
- **Cada pieza en su propio recuadro, sin tocar a las demas**: aunque
  sea una sola imagen grande, con separacion clara entre cabeza/torso/
  brazo/antebrazo/mano/muslo/pierna/bota, fondo transparente o un color
  solido que no aparezca en el personaje (para poder recortar limpio).
- **Mismo estilo y escala que el resto del roster**: ver
  `assets/characters/rayder/rayder_kf_atlas.png` (Rayder Cruz) como
  referencia de proporcion/calidad — NO hace falta copiar su tamano de
  celda exacto (este personaje es piezas, no una hoja de celdas fijas),
  pero si mantener una complexion/altura de personaje consistente con el
  roster para que no se vea desproporcionado al lado de los demas en
  Modo VS.
- **Brazo, antebrazo, muslo y pierna solo de un lado** (el otro se
  genera reflejado en el juego) — salvo que el personaje sea muy
  asimetrico (un arma grande en un solo brazo, por ejemplo), en cuyo
  caso se piden los dos brazos completos por separado.

## 5. Pendiente (se completa cuando exista la hoja real)

- Nombre y stats del personaje (vida, velocidad, dano) en
  `src/game/CharacterVisual.cpp`.
- Lista real de piezas con sus archivos en `assets/<personaje>/actor/`.
- El archivo `data/<personaje>/<id>.txt` con los `frame`/`piece`/`clip`
  generados segun la matematica de la seccion 3.
- 5 habilidades + sonidos, igual que Kratos.
