# Plan maestro: RONIN-KRATOS

Objetivo: un personaje nuevo que use **todo Kratos** (cada cuadro, cada clip,
cada golpe, las 5 habilidades, las 4 armas, sus tiempos, cajas de golpe y
sonidos) pero con el **cuerpo de RONIN**: cada pieza de Kratos (cabeza, torso,
brazo, antebrazo, mano, falda, muslo, pierna, pie) se reemplaza por la parte
equivalente de RONIN, en la misma posicion, giro y escala que tiene la pieza de
Kratos en ese cuadro. Las armas y los efectos de Kratos se conservan.

Reglas que no se rompen:
- **Kratos no se toca**: sus `data/kratos/kratos_*.txt` y sus PNG en
  `assets/kratos/actor/` quedan byte a byte iguales (se verifica con md5).
- **RONIN actual no se toca**: es otro personaje (cuadros completos), sigue
  existiendo aparte.
- Todo sale de un script reproducible (`tools/ronin_kratos/`), nada a mano:
  si Kratos cambia (por ejemplo al corregir el Cestus) se vuelve a correr.

## Por que este metodo

Este proyecto ya hizo lo mismo una vez: el Rayder Clon es el heroe KF vestido
pieza por pieza (`tools/build_kf_clone_pieces.py`, `build_kf_clone_despiece.py`,
`build_kf_clone_rig.py`). Kratos funciona igual que el heroe KF: cada cuadro es
una lista de piezas `(hoja, recorte, posicion, espejo/giro)` sobre hojas
empaquetadas (`assets/kratos/actor/<id>.png`). Si se reemplazan los pixeles de
cada pieza manteniendo su recorte, el motor arma a RONIN con exactamente los
mismos movimientos de Kratos, sin tocar codigo de animacion.

## Fases

### 0. Inventario de Kratos
- Listar todas las piezas unicas `(id de hoja, sx, sy, sw, sh)` usadas en las 4
  armas, cuantas veces aparece cada una y en que clips.
- Para cada pieza: zona vertical tipica dentro del cuerpo (0 = coronilla, 1 =
  pies, medida en los cuadros donde aparece), eje principal (PCA de su mascara:
  angulo y largo), y material (piel palida con tatuaje rojo, cuero/falda,
  metal de arma, cadena, fuego/efecto).
- Clasificar: `head`, `torso`, `upper_arm`, `forearm`, `hand`, `skirt`,
  `thigh`, `shin`, `foot`, `weapon`, `chain`, `fx`. Hoja de contacto con todas
  las piezas y su clase para revision visual; las dudosas se corrigen en un
  JSON de excepciones (no en el codigo).

### 1. Biblioteca de partes de RONIN (todas las imagenes del usuario)
- Turnaround de perfil (cabeza, torso, 2 brazos, 2 antebrazos, cinto, pelvis,
  2 muslos, 2 piernas con pie), puno de perfil, piezas sueltas de la hoja de
  combos (6 punos/manos, 5 antebrazos, 5 piernas).
- Partes recortadas de todos los cuadros de sus hojas (combos, patadas, salto,
  carga de ki) para tener cada parte en muchos angulos, como se hizo con el
  despiece del clon.
- Cada parte etiquetada con su clase, eje y largo.

### 2. Reemplazo pieza por pieza
Para cada pieza corporal de Kratos:
1. Elegir la parte de RONIN de la misma clase cuyo eje/forma calce mejor
   (se prueban los 8 giros/espejos; se compara mascara contra mascara).
2. Alinearla al eje de la pieza de Kratos (angulo y largo) y escalarla a su
   medida. Brazo/torso con ancho ~0.9 para que RONIN quede mas delgado que
   Kratos.
3. Pintarla dentro del mismo recorte `(sx, sy, sw, sh)` en una COPIA de la hoja,
   respetando la silueta lo justo para que las juntas sigan tapandose entre
   piezas (sin huecos entre hombro/brazo, cadera/muslo, rodilla/pierna).
4. La cabeza: se ajusta por la cara (no por el pelo) y el pelo puntiagudo blanco
   puede sobresalir del recorte de Kratos; para eso la pieza de cabeza se
   reubica en la copia con un recorte mas grande y se corrige su `piece`.
5. `weapon`, `chain`, `fx`: se dejan como en Kratos.

Salida: hojas nuevas `assets/kratos/actor/<9500+>.png` y
`data/kratos/ronin_kratos_<arma>.txt` = copia exacta del de Kratos con los ids
de hoja remapeados (y solo los recortes de cabeza ajustados).

### 3. Integracion
- `src/game/lab/KfReference.cpp`: 4 entradas `ronin_kratos_<arma>`.
- `src/game/CharacterVisual.cpp`: "RONIN-KRATOS" jugable en Modo VS, con cambio
  de arma igual que el guerrero (hoy el cambio de arma esta fijo a las 4
  primeras entradas BETA en `src/game/VSMode.cpp`: `IsWarrior`/`VsChar`;
  se generaliza a grupos de 4 sin cambiar el comportamiento de Kratos).

### 4. Auditoria
- Por cada clip de cada arma: Kratos y RONIN-KRATOS lado a lado, mismo cuadro,
  mirando a la derecha y volteado. Revisar huecos en juntas, piezas
  mal clasificadas, cabeza flotando, piel roja de Kratos que haya quedado.
- md5 de los archivos de Kratos sin cambios.
- `ctest`, `all_modes_check`, `gow_check`, hoja de roster.

### 5. APK

## Orden
Primero Espadas del Caos (la base mas limpia). El Cestus espera a que se
corrija su deformacion en Kratos (se esta investigando aparte); como todo sale
del script, despues se regenera sin trabajo extra.
