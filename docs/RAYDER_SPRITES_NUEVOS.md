# Sprites nuevos de Rayder: "Rayder azul"

## Qué es esto

Son animaciones de patadas, puñetazos y un proyectil nuevas, rescatadas de 3 hojas
que subió el usuario. Van en `assets/characters/rayder/rayder_azul_atlas.png` +
`rayder_azul_clips.json`, como **complemento**, no como reemplazo. Todavía no está
conectado a ningún personaje del juego: hay que decidir en qué skin usarlo.

## La referencia de tamaño: Rayder Cruz

El proyecto llama **Rayder Cruz** (también "Rayden Cruz") a la hoja oficial y de
calidad del héroe: `assets/characters/rayder/rayder_kf_atlas.png`
(`data/sprite_manifest.json`, atlas `rayder_kf`, usado por el personaje "RAYDER").
**Esa hoja no se tocó ni se movió.** Medí el personaje dentro de ella:

- celda 270x175 px, pivote (los pies) en (135, 168) según el manifiesto
- el personaje de pie mide **~110 px de alto** y entre 56 y 65 px de ancho según
  la pose, con los pies tocando el suelo real en y≈162 dentro de la celda

Ese 110 px de alto es el que usé como objetivo para las hojas nuevas. (La primera
vez las escalé contra `rayder_atlas.png`, un atlas de 160x128 que no usa ningún
personaje del juego: era la referencia equivocada. Ya está corregido.)

## Qué rescaté de cada hoja

- **1_patadas.png**: 24 de 24 cuadros. El fondo venía pintado como un cuadriculado
  gris casi blanco (no era transparencia de verdad); se quitó por relleno desde
  los bordes para no comerse el blanco de los propios efectos de patada.
- **3_proyectil.png**: 22 cuadros con el personaje (puñetazo cargando y disparando
  el proyectil azul). Las 2 filas finales son solo la bola de energía volando sin
  el personaje: se dejaron fuera del atlas.
- **4_aura.png**: nada aprovechable. El fondo azul degradado quedó pegado a los
  bordes del personaje como píxeles semitransparentes; solo el 14% de la imagen es
  opaco de verdad, y ese 14% deja al personaje con piernas, botas o brazos
  perdidos. Por eso el combo de puñetazos (`punch1/2/3`) usa cuadros de guardia y
  brazo extendido de la hoja de patadas y del proyectil como solución provisional.

## Escala usada (vecino más cercano, sin emborronar el pixel art)

- Hoja 1 (patadas): factor 0.5528 — el personaje medía 199 px de pie, queda en 110.
- Hoja 3 (proyectil): factor 0.6077 — medía 181 px de pie, queda en 110.
- Las dos hojas no estaban a la misma escala entre sí en origen; por eso cada una
  lleva su propio factor para que ambas terminen al tamaño de Rayder Cruz.

## El atlas nuevo

- `rayder_azul_atlas.png`: 2700x875 px, rejilla 10x5, **celda 270x175 y pivote
  (135,162)** — el mismo tamaño de celda que `rayder_kf_atlas.png`, para que el
  personaje nuevo quede exactamente del mismo porte físico que Rayder Cruz.
- `rayder_azul_clips.json`: 10 clips — `punch1/2/3`, `kick`, `dash_attack`,
  `energy`, `projectile`, `rage_attack`, `finisher`, `victory` — duraciones entre
  0.06 y 0.12 s por cuadro.
- Auditoría de recortes: 19 cuadros con los pies reencuadrados a la línea de
  contacto, 259 manchitas sueltas borradas en 4 cuadros, halos de 1-2 px quitados.
  Quedan 8 cuadros donde el efecto azul se corta en el borde de la celda (el peor,
  ~16% del rayo del disparo) porque la pose no entraba entera en 270x175; si hace
  falta una celda más ancha para el proyectil, se puede ensanchar solo esa fila.

## Comparación visual

`/tmp/claude-0/-home-user/05b0c2b9-43b9-5c8e-82ee-61da3b1aff5a/scratchpad/control_vs_rayder_cruz.png`
— Rayder Cruz al lado de 7 cuadros nuevos, con la línea roja marcando donde deben
tocar los pies. El alto ya coincide; la ropa, el peinado y el color de piel siguen
siendo distintos (camiseta sin mangas azul, jeans, cinta azul, pelo afro, frente a
la chaqueta y otro peinado de Rayder Cruz): son sprites nuevos a integrar, no un
reemplazo del personaje.

## Lo que falta decidir

- A qué personaje/skin se asigna este set (¿una variante de Rayder, o un
  personaje nuevo?). No se tocó `CharacterVisual.cpp` ni `sprite_manifest.json`:
  falta decidir el nombre y registrarlo.
- El combo de puñetazos es un apaño con cuadros prestados; si el usuario consigue
  la hoja 4 sin el velo azul pegado, se puede rehacer con puñetazos y volteretas
  reales.
