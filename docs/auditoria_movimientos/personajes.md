# Auditoria de personajes de District Fury (42 entradas de `kCharacters`)

Rama `ccr-077ad675-rlrg28`. Auditoria de SOLO LECTURA: no se toco ningun archivo del repo.

## 0. Como se hizo y donde estan las pruebas

- Codigo leido: `src/game/CharacterVisual.{h,cpp}`, `src/game/player/PlayerDraw.cpp`, `PlayerCombat.cpp`, `PlayerCommon.h`, `src/game/Player.cpp`, `src/rendering/BossSprite.h`, `Animator.cpp`, `SpriteManifest.cpp`, `AssetManager.cpp`, `src/game/lab/KfReference.cpp`, `src/game/beta/BetaCharacter.cpp`, `src/game/combat/{AttackData.cpp,AttackAnimation.h,Boss.cpp}`, `data/sprite_manifest.json`, `data/kratos/*.txt`, `GowApk/data/characters/*.txt`.
- Imagenes (todas en `/tmp/claude-0/-home-user/05b0c2b9-43b9-5c8e-82ee-61da3b1aff5a/scratchpad/audit/img/`):
  - `roster/*.png` + `roster/LISTA.md`: salida de `./build/roster_sheets` (un clip por fila, cuadro a cuadro).
  - `states/states_<id>.png` (y `s<id>_small.png`): **herramienta propia** (fuente en `/tmp/claude-0/-home-user/05b0c2b9-43b9-5c8e-82ee-61da3b1aff5a/scratchpad/tool/states.cpp`, enlazada contra `build/libdistrict_fury_core.a`, sin tocar el repo) que dibuja el `Player` REAL con `Player::Draw()` en cada estado: reposo, caminar a la derecha y a la izquierda (la flecha amarilla arriba a la derecha indica hacia donde DEBERIA mirar), dash, golpe 1/2/3, patada, energia, habilidades 1-5, transformar, bloqueo, golpe recibido, por los aires, derribo, levantarse, derrota. Es la prueba principal de este informe.
  - `poses_<folder>.png`, `titanx_detail.png`, `titanx_punch_joined.png`, `titanxm_idle.png`: cada PNG de `assets/bosses/<folder>/` con su nombre.
  - `rayden_atlas.png`, `max_atlas_small.png`, `max_white_small.png`, `max_unused_a.png`, `max_unused_b.png` (cuadros de MAX que ningun clip usa, numerados), `brakk_atlas_small.png`.
  - `crops/*.png`: recortes legibles de las hojas grandes.
  - `vs_titanx/*.png`: `vs_boss_visual_check` con TITAN-X como jugador (captura `titanx_2.png`: el jugador da la ESPALDA al jefe).
- Datos: `/tmp/claude-0/-home-user/05b0c2b9-43b9-5c8e-82ee-61da3b1aff5a/scratchpad/tool/dump.txt` (herramienta `dump.cpp`): por personaje, cada clip con numero de cuadros, duracion, y con que otros clips es IDENTICO (`dup:{...}`) o si es una copia de `idle` (`=IDLE`).
- Ninguna herramienta dejo PNG en la raiz del repo (comprobado con `find -maxdepth 1 -name "*.png"`).

---

## 1. Hallazgos transversales (afectan a muchos personajes)

### 1.1 La maquina de estados es la misma para todos y es pobre
`Player` solo tiene `Idle, Walk, Dash, Attack, Block, Hit, GuardBreak, Knockdown, Defeat, Recovery, Airborne`. **No existen** salto, victoria, giro, carrera ni agachado. Los clips `victory`, `jump`, `run`, `turn`, `landing`, `intro`, `fury`, `chain`, `smash`... existen en muchos personajes y **el jugador nunca los reproduce** (`grep` en `src`: `victory` solo aparece como plan B de la transformacion en `PlayerCombat.cpp:125`).
El combo es el mismo para los 42: `J,J,J` = Punch1 > Punch2 > Punch3, `K` = Kick, `L` = EnergyWave, dash+J = DashAttack, furia+Espacio = RageAttack, K con furia y combo>=4 = Finisher, cancelaciones de `ResolveCancel` (`PlayerCommon.h`). Para "mas golpes y combos" hace falta, en el nucleo, una **tabla de cadena por personaje** (p.ej. `J,J,J,J` o `J,J,K,K`, `abajo+K` = barrida, `adelante+J` = golpe fuerte) que apunte a nombres de clip; ver bloque 0.

### 1.2 Personajes de poses sueltas (ids 1, 3, 4, 5): sin ninguna animacion procedural
En `PlayerDraw.cpp` la rama `altTex` (poses sueltas) dibuja la textura **tal cual**: no recibe el balanceo de reposo, el rebote al caminar, la inclinacion del dash, el estiramiento de impacto, el retroceso al recibir golpe, el aterrizaje ni las estelas (`ghosts` guardan `animator.currentFrame`, que en estos personajes es el atlas de Rayden cargado de relleno por `EnsurePlayerAnimator`). Todo eso solo existe en la rama `animator` (ids 0, 2, 6). Resultado: poses congeladas que "patinan" por el suelo.

### 1.3 `DrawBossPose` normaliza CADA pose a la misma altura -> el personaje cambia de tamano en cada pose
`BossSprite.h` escala cada PNG a `targetHeight` por separado. Una pose agachada o tumbada sale gigante y una con el brazo arriba sale encogida. Medido (escala relativa al reposo):
- CLON ANTIGUO: `death` x2.1, `dash` x1.34.
- TITAN-X: `slam` y `death` x1.36, `punch1` x1.19, `uppercut` x0.87.
- TITAN-X MEJORADO: `death` **x2.25** (desborda la pantalla, ver `states/s5_small.png`), `slam` x1.29, `shoot_orb` x1.22, `rage_aura` x1.19.
Ademas centra por el ANCHO del PNG, no por el cuerpo: poses con el brazo, el orbe o la estela a un lado (p.ej. `release_orb` 289 px de ancho) desplazan el cuerpo hacia atras al cambiar de pose.
Arreglo sin arte: escala uniforme por personaje (`scale = targetHeight / alto_de_idle1`, la misma para todas las poses, como ya hace `DrawSpriteUniform`) + un desfase X por pose (ancla en el pie trasero o en el centro del torso).

### 1.4 Orientacion: `facesRightByDefault` es un unico bool, pero los PNG estan mezclados
`flip = facesRightByDefault ? facing==Left : facing==Right`. Si un set tiene poses mirando a ambos lados, la mitad sale al reves. Pasa en TITAN-X y TITAN-X MEJORADO (detalle en sus fichas). Arreglo: tabla por pose `{nombre, miraDerecha, dx}`.

### 1.5 KF (APK): los clips que faltan se rellenan en silencio con `idle` o con clips duplicados
`KfReference.cpp` mapea acciones de la APK a nombres fijos; si la accion no existe usa `idle` (`kRequired`). Ademas varios nombres apuntan a la MISMA accion: `energy` = `special` = `finisher`; `dash_attack` = `dash` (heroes) o = `run` (enemigos); `rage_attack` = `kick` (enemigos); `block` = `idle` (enemigos); `recovery` = `idle`. En los enemigos KF, `energy/special/finisher` son 1 cuadro de 0.05 s (golpe invisible) y la **primera habilidad "SOMBRAS" es su animacion de levantarse** (accion 20 de un enemigo = `getup`; en `dump.txt` sale `getup dup:{ab0}` en MATON, NAVAJERA, SOLDADO, RUBIA, GORRA, PELEADOR, CUCHILLERO).

### 1.6 BETA (por piezas): `recovery` = animacion de recibir golpe
En `kratos_*.txt`, `ronin_kratos_*.txt` y TODOS los enemigos de `GowApk/data/characters/*.txt`, el clip `recovery` es identico a `hit` (p.ej. `kratos_espadas.txt:1943`). `EndAttack()` pone `Recovery` tras CADA ataque normal -> el personaje hace un gesto de dolor de 0.08-0.14 s despues de cada golpe que da. Tambien `air`/`airborne` = `hit` en Kratos y Ronin-Kratos (vuela por los aires con la pose de "me pegaron de pie"). Ademas el codigo fuerza `sx = sy = 1, angle = 0` para BETA (`PlayerDraw.cpp`, "BETA: forma y tamano originales"): ningun efecto procedural.

### 1.7 Habilidades de "nuestros" personajes (ids 0-6) = golpes repetidos
`TryStartSkill` para no-KF lanza `EnergyWave, Punch3, RageAttack, DashAttack, Finisher`: las 5 habilidades son los mismos clips que el combo normal. En los de poses sueltas, como `CharacterPose` solo distingue `Energy/Kick/resto`, las habilidades 2-5 muestran **la misma pose** que el golpe normal (ver `states/s4_small.png`: habilidades 2-5 = `punch1` x4). La transformacion (habilidad 6) usa `transform` si existe; si no, `victory`, si no `energy`.

### 1.8 Otros transversales
- El orbe de carga de la energia se dibuja a +-30 px y a 76*escala de altura fijos: queda en la espalda/pecho de los personajes grandes (Titan-X).
- Las cajas de golpe de los personajes sin datos por cuadro son las de `AttackData` (tamano Rayden): Titan-X y Titan-X Mejorado, el doble de grandes, pegan con el alcance de Rayden.
- Sombra doble en KF: los cuadros de la APK traen su elipse de sombra y `Player::Draw` dibuja otra.
- No hay pose de victoria al ganar la ronda en ningun personaje.

---

## 2. Fichas por personaje

Leyenda de tecnicas SIN arte nuevo (se citan por codigo):
- **T1 rebote de paso**: `y -= |sin(fase)|*k`, vaiven X, rotacion +-3 grados, alternando 2-4 poses existentes al ritmo de la velocidad.
- **T2 anticipacion/impacto/recuperacion**: encoger antes, estirar en el impacto, retroceso despues (ya existe en la rama `animator`, hay que portarlo a poses sueltas).
- **T3 avance con curva**: root motion con easing (ya existe `AttackRootMotionProgress`).
- **T4 caida rotada**: rotar la pose de golpe recibido alrededor de los pies (-20 a -85 grados) mientras baja, y terminar en la pose tumbada.
- **T5 estela**: copias desvanecidas de la textura actual (guardar textura+flip, no `currentFrame`).
- **T6 salto parabolico**: usar `position.z` (ya existe gravedad en `Player::Update`) con una pose existente + sombra que encoge.
- **T7 giro**: `sx` de 1 a -1 con flip a mitad (giro de 180/360 grados) para ataques giratorios y para darse la vuelta.
- **T8 reproduccion inversa**: derribo al reves = levantarse; carga al reves = recuperacion.
- **T9 composicion**: superponer FX de `assets/fx/efectos_azules.png` (rejilla 8x5) o unir dos PNG; recolor/tinte para formas de furia.
- **T10 hitstop + temblor + flash** en los impactos de los golpes propios.

---

### 2.0 RAYDEN (ORIGINAL) — id 0
- **Tipo**: atlas de manifiesto `rayden` (`assets/characters/rayden_128.png`, rejilla 4x6, 22 cuadros). Ruta de dibujo `animator` con efectos procedurales (balanceo, rebote, inclinacion, estiramiento). **No se reemplaza; solo se puede agregar.**
- **Clips**: 19 (`idle, walk, dash, block, hit_high, hit_low, airborne, knockdown, getup, defeat, recovery, punch1, punch2, punch3, kick, energy, dash_attack, rage_attack, finisher`). Usa los 22 cuadros, ninguno sobra. Clips muy cortos: `punch2`, `energy`, `dash`, `block`, `knockdown`, `hit_*` son de 1 cuadro.
- **Estados** (`states/s0_small.png`): reposo bien (pero `idle` mezcla el cuadro 0 relajado con 1-3 en guardia: pequeno salto de postura en cada vuelta). Caminar bien (4 cuadros + rebote). Dash 1 cuadro con estela, bien. Golpe 1 (anticipo+jab) bien; golpe 2 = un solo cuadro de gancho con arco 0.3 s (se ve congelado); golpe 3 = patada alta (cuadro 11), patada = cuadro 10/11: golpe 3 y patada casi iguales. Energia = 1 cuadro de aura 0.55 s (congelado). Bloqueo 1 cuadro, bien. Golpe recibido bien (alto/bajo alternan). Aire/derribo/levantarse bien. Derrota = caida + tumbado, bien. Victoria: no existe.
- **Bugs**: sin bugs graves. Golpe 3, patada, furia y remate reutilizan los mismos 2 cuadros de patada (repetitivo). Habilidades 2-5 = mismos clips del combo.
- **Falta y como lograrlo sin arte**: (a) giro-patada: cuadro 10 + T7; (b) uppercut: cuadro 9 (gancho) rotado -15 grados + T6 pequeno salto; (c) barrida: cuadro 17 (golpe bajo) desplazado abajo + estela; (d) energia con 3 fases: cuadro 16 (anticipo) -> 12 (aura) con escala pulsante -> 8 (empuje) + FX azules; (e) victoria: no hay pose de brazo en alto -> usar cuadro 12 (aura) sin proyectil + T1 leve. Arte nuevo necesario para: puno alto, rodillazo, agarre, pose de victoria real.
- **Moveset propuesto** (pelea callejera rapida): `J,J,J` jab-jab-gancho(9)+estela; `J,J,K` jab-jab-patada alta(10); `K,K` patada alta + giro-patada (10+T7); `abajo+K` barrida (17+T3); `adelante+J` uppercut (9+T6); `L` onda en 3 fases; dash+J embestida (8,9 + T3 largo). Agregar clips NUEVOS con nombres nuevos en el manifiesto; no tocar los 19 existentes.

### 2.1 CLON ANTIGUO (CHAQUETA) — id 1
- **Tipo**: poses sueltas `assets/bosses/rayder_clone/` (11 PNG), `DrawBossPose` altura 118, mira a la derecha (correcto, todas las poses miran a la derecha).
- **Poses**: `idle1-4, ready, punch, kick, dash, release_orb, hurt, death`. Se usan las 11. Ninguna sobra. Faltan: caminar, bloqueo, derribo, levantarse, victoria.
- **Estados** (`states/s1_small.png`): reposo = ciclo idle1-4 (3.2 fps) bien. **Caminar = el mismo ciclo idle a 9 fps**: no mueve las piernas, patina. Dash = `dash` bien pero 1.34x mas grande. Golpe 1/2/3 = `ready`->`punch`->`idle1` los tres iguales (el golpe 3 no se distingue). Patada = `ready`->`kick`->`idle3`. Energia = `ready`->`release_orb`->`idle2` bien. Bloqueo = `ready` (aceptable). Golpe recibido = `hurt` bien. Por los aires / derribo / levantarse = `hurt` de pie (nunca cae). Derrota = `death` (arrodillado) **2.1x mas grande** que el cuerpo normal.
- **Bugs**: (1) escala por pose (1.3); (2) caminar congelado; (3) restos de recorte de otros cuadros en los PNG: `kick.png` trae una pierna suelta abajo a la izquierda, `hurt.png` y `ready.png` un trozo de bota en el borde derecho, `idle4.png` un pico oscuro en la bota (se ve en `states/clone_walk_cmp.png`); (4) `punch.png` tiene el puno cortado por el borde derecho; (5) sin efectos procedurales (1.2).
- **Falta y como lograrlo sin arte**: caminar = alternar `idle3`/`idle4` (piernas abiertas) con `idle1` + T1; derribo = `hurt` + T4 hasta tumbarlo -> `death` (arrodillado) como "en el suelo"; levantarse = `death` -> `ready` (T8); golpe 2 = `punch` desplazado +10 px y rotado 4 grados; golpe 3 = `dash` (salto con estela) usado como puno volador + T3; uppercut = `kick` rotado; victoria = `ready` + aura (no hay pose propia). Limpiar los restos de recorte con un script de borrado de islas (no es arte nuevo). Arte nuevo para: caminar real, caer de espaldas, bloqueo.
- **Moveset**: rival espejo de Rayden: `J,J,J` jab(punch)-cruzado(punch+T2)-puno volador(dash+T3); `K` patada (`kick`); `J,J,K` termina en patada; `L` orbe oscuro (`release_orb`) con FX morado (T9 recolor); dash+J = `dash` + estela.

### 2.2 BRAKK — id 2
- **Tipo**: atlas de manifiesto `brakk_v2` (`assets/bosses/brakk/brakk_v2_atlas_clean.png`, 12x9 celdas 256x176, 104 con contenido). Ruta `animator` con perfil de ataque (`AttackAnimation.h`, skin 2). Las poses sueltas de `assets/bosses/brakk/*.png` NO se usan para el jugador (solo para el jefe antiguo).
- **Clips**: 29. El jugador usa 19 (`idle, walk, dash, block, hit_high, hit_low, airborne, knockdown, getup, defeat, recovery, punch1-3, kick, energy, dash_attack, rage_attack, finisher, transform`). **Nunca usa**: `run, charge, chain, chain_throw, smash, fury, explosive, hit, victory` (son del jefe). 54 cuadros usados, **50 cuadros sin usar** (7, 10, 12, 15, 18-22, 28, 32-35, 37, 38, 40, 45, 47-57, 62, 63, 67-76, 81-84, 87, 89, 90, 100, 103); muchos son fragmentos de cadena cortados en dos celdas, pero hay poses completas utiles (10 cadena sobre la cabeza, 22, 47, 50-57 golpe al suelo con cadena, 67-76 latigazos y punetazos, 100).
- **Estados** (`states/s2_small.png`): reposo 8 cuadros bien. Caminar 5 cuadros bien pero con **rectangulos marrones de fondo** (restos de mate) en los cuadros 8-9. Dash = 2 cuadros de carrera. Golpe 1/2 = latigazo de cadena corto (bien), golpe 3 = golpe al suelo (bien). Patada = lanzar cadena (no es patada). Energia = lanzar cadena con sierra. Bloqueo bien. Golpe recibido 1 cuadro. Derribo/levantarse bien (2 cuadros). Derrota bien pero el ultimo cuadro trae una **linea suelta arriba**.
- **Bugs**: (1) mate marron en `walk`, `charge`, `dash_attack`, `kick`; (2) `rage_attack` cuadro 88 = solo el arco de cadena, **el cuerpo de Brakk desaparece** (`states/s2_small.png`, habilidad 3); `explosive` y `rage_attack` con cuadros cortados en el borde de celda; (3) linea suelta en el cuadro 102 (derrota); (4) `hit_high`/`hit_low` de 1 cuadro.
- **Falta y como lograrlo sin arte**: usar los clips del jefe que ya existen: `chain` (8 cuadros) como golpe 4, `smash` (7) como remate, `chain_throw` como energia, `fury` como entrada de furia, `explosive` como super, `victory` en la victoria, `run` al correr. Golpe recibido de 2 cuadros con `hit` (93,94). Patada real = no existe (es un luchador de cadena): reemplazar por embestida de hombro (`charge` + T3). Arte nuevo: patada, agarre.
- **Moveset** (bruto de cadena, lento y de alcance): `J,J,J,J` latigazo-latigazo-cadena giratoria(`chain`)-golpe al suelo(`smash`); `K` embestida de hombro (`charge`, armadura); `L` cadena con sierra (`chain_throw`); `adelante+L` jalon (cuadros 36-46); furia = `fury` + `explosive` como super de area; remate `smash` con temblor T10.

### 2.3 GRINDER — id 3
- **Tipo**: poses sueltas `assets/bosses/grinder/` (7 PNG de pixel art diminuto, 30-75 px), `DrawSpriteUniform` escala 3 (uniforme: el tamano se mantiene bien).
- **Poses**: `idle, hurt, death, saw, slam, ram, overdrive`. Se usan las 7. Solo hay 1 reposo.
- **Estados** (`states/s3_small.png`): reposo = 1 imagen fija (sin respiracion). **Caminar = la misma imagen fija, patina**. Dash = `ram` bien. Golpe 1/2/3 = `saw` los tres identicos. Patada = `slam`. Energia y furia = `overdrive`. Bloqueo = `idle`. Golpe/aire/derribo = `hurt` de pie. Derrota = `death` bien.
- **Bugs**: (1) congelado en reposo y al caminar; (2) golpes identicos; (3) arte de resolucion muy inferior al resto del reparto (37x57 ampliado x3, pixeles enormes junto a sprites HD); (4) nunca cae al suelo.
- **Falta sin arte**: respiracion y paso = T1 sobre `idle` (escala Y +-2 %, rebote, vaiven) — al ser una sola pose el paso sera "pesado"; golpes distintos = `saw` con T2 de intensidad creciente y estela de chispas (T9) y `saw` rotado 8 grados en el 2; derribo = `hurt` + T4 -> `death`; levantarse = `death` -> `slam` (agachado) -> `idle`; bloqueo = `idle` agachado (escala Y 0.94) + escudo. Arte nuevo necesario si se quiere que case con los HD (redibujar en alta resolucion) y para caminar.
- **Moveset** (maquina pesada con sierra): `J,J,J` sierra-sierra-sierra larga con chispas multi-golpe; `K` golpe al suelo (`slam`) con onda; `L`/furia `overdrive` como aura que dana alrededor; dash = embestida `ram` con armadura.

### 2.4 TITAN-X — id 4  (la queja del usuario: "camina de espaldas y le faltan golpes")
- **Tipo**: poses sueltas `assets/bosses/titanx/` (12 PNG), `DrawBossPose` altura 170, `facesRightByDefault = false`.
- **Poses y hacia donde miran de verdad** (`img/titanx_detail.png`, `img/poses_titanx.png`):
  - `idle1, idle2, idle3, idle4`: miran a la **DERECHA**.
  - `lean`: DERECHA (inclinado hacia delante). `uppercut`: DERECHA (puno arriba).
  - `shoot_orb.png`: DERECHA, y en realidad es un **puno recto** (brazo extendido), no un disparo.
  - `recoil.png`: IZQUIERDA, y en realidad es el **disparo del orbe verde** (no un retroceso). Nombres cruzados.
  - `punch1.png` (130 px) y `punch2.png` (77 px): son las **dos mitades de UNA misma pose** mal recortada (`titanx_punch_joined.png`): `punch1` tiene piernas y torso sin cabeza mas un puno suelto flotando; `punch2` es la tira con la cabeza. Juntas forman una carga de puno (brazo atras) mirando a la derecha.
  - `slam`: de frente (golpe al suelo con ondas verdes) y trae un trozo de puno de otro cuadro en el borde izquierdo. `death`: tumbado.
- **Por que camina de espaldas**: casi todo el set mira a la derecha pero la ficha dice `false`; `flip = facing==Right` -> al mirar a la derecha se espeja y queda mirando a la izquierda, y al reves. Ademas no hay pose de caminar: `Walk` cae en el ciclo `idle1-4` a 9 fps. Resultado: avanza de espaldas, sin mover las piernas, en las dos direcciones (`states/s4_small.png`, cuadros "walk-> t1..t3" y "walk<-"). En el VS (`vs_titanx/titanx_2.png`) el jugador le da la espalda al jefe. El mismo supuesto ("mira a la izquierda") esta en `Boss.cpp:259-261` y `Stage3Game.cpp:569` (y `Stage4Game.cpp:448` para el Mejorado), asi que el JEFE Titan-X tambien sale al reves.
- **Uso actual por estado**: reposo/caminar/bloqueo = idle1-4 (espejado mal). Dash = `lean` (espejado mal: dashea de espaldas; la estela azul sale por delante). Golpe 1, 2, 3, habilidades 2-5, dash-ataque y remate = `punch1` (media pose sin cabeza, con puno flotante, y 1.19x mas grande); como `punch1` mira a la izquierda, el espejo la deja bien orientada por casualidad. Furia + golpe = `slam`. Patada = `uppercut` (espejado mal). Energia = `shoot_orb.png` (que es un puno, espejado mal: "dispara" hacia atras y el orbe de carga se dibuja en su espalda). Golpe recibido, aire, derribo y levantarse = `recoil.png` = **la pose de disparar el orbe** (parece que ataca cuando le pegan) y nunca cae. Derrota = `death` 1.36x mas grande.
- **Poses sin usar**: `punch2` (nunca la elige `CharacterPose`; solo el jefe para `GuardCounter`). Las demas se usan pero casi todas en el estado equivocado.
- **Bugs concretos**: (1) orientacion invertida en idle/caminar/dash/patada/energia; (2) caminar congelado; (3) golpes 1-3 y habilidades identicos y con media pose cortada; (4) golpe recibido = pose de ataque; (5) no cae, no se levanta; (6) escala distinta por pose; (7) alcance de golpe de Rayden en un cuerpo el doble de grande; (8) orbe de carga en la espalda.
- **Arreglo sin arte nuevo** (orden recomendado):
  1. `facesRightByDefault = true` + tabla por pose: `recoil.png` es la unica que mira a la izquierda (se dibuja pre-espejada). Renombrar logicamente: `recoil.png` -> "disparo", `shoot_orb.png` -> "puno recto".
  2. Escala uniforme (1.3): `scale = 170/259`.
  3. Reconstruir el puno: dibujar `punch1` y a su derecha `punch2` (desfase 130 px de origen) = "carga de puno" completa; borrar el puno flotante con un recorte del rectangulo `x<35` de `punch1`.
  4. Caminar = `idle3` (zancada) <-> `idle1` <-> `idle4` <-> `idle2` al ritmo de `walkPhase` + T1 con rebote PESADO (bajar 4-5 px y temblor de camara leve en cada pisada) + inclinacion 3 grados hacia delante; correr/dash = `lean` + T5 estela verde.
  5. Golpes distintos: Golpe 1 = carga (`punch1+punch2`) -> puno recto (`shoot_orb.png`) con T2/T3; Golpe 2 = `shoot_orb.png` desplazado y rotado +6 grados (cruzado) con estela; Golpe 3 = `uppercut` con T6 pequeno salto; Patada/pisoton = `slam` con onda (ya dibujada) + temblor T10; Energia = `idle2` (carga, con brillo verde T9) -> `recoil.png` (disparo); Furia = `slam` repetido 3 veces (golpes al suelo) con zoom.
  6. Golpe recibido = `idle4` rotado -7 grados + empujon (T2 inverso) + tinte; derribo = T4 sobre `lean` hasta `death`; levantarse = `death` -> `slam` (agachado) -> `idle1`; bloqueo = `idle1` con escala Y 0.96 + brazos... no hay -> escudo verde.
  7. Cajas de golpe x1.6 para su tamano y orbe de carga en la mano real (desfase por pose).
  - Arte nuevo necesario para: ciclo de caminar real, bloqueo, patada real, pose de victoria.
- **Moveset propuesto** (prototipo mutante, lento y demoledor): `J,J,J` carga-puno recto / cruzado / uppercut lanzador; `K` pisoton (`slam`) con onda que derriba; `J,J,K` termina en pisoton; `L` orbe verde (`idle2`->`recoil`); `adelante+J` embestida (`lean` + T3 larga, armadura); furia: triple golpe al suelo; remate: uppercut + disparo a quemarropa.

### 2.5 TITAN-X MEJORADO — id 5
- **Tipo**: poses sueltas `assets/bosses/titanx_mejorado/` (11 PNG), `DrawBossPose` altura 235, `facesRightByDefault = false`.
- **Orientacion real** (`img/poses_titanx_mejorado.png`, `img/titanxm_idle.png`): `idle1, idle3, idle4, cannon_aim, charge_orb, rage_aura` miran a la DERECHA; `idle2` casi de frente; `shoot_orb` mira a la IZQUIERDA; `dash` y `slam` de frente; `death` tumbado. Igual que Titan-X: la ficha esta al reves para la mayoria.
- **Uso actual**: reposo/caminar/bloqueo = idle1-4 (espejado mal = **camina de espaldas**, y como `idle2` mira de frente/izquierda el ciclo parpadea de orientacion). Dash = `dash` (de frente, bien). Golpe 1-3 y habilidades = `charge_orb` (espejado mal: el canon apunta hacia atras). Patada = `slam`. Energia = `shoot_orb` (espejado = bien). Furia + ataque = `rage_aura`. **Golpe recibido/aire/derribo = `cannon_aim`** (apunta el canon cuando le pegan, y al reves). Derrota = `death` **x2.25 de tamano** (desborda; `states/s5_small.png`).
- **Poses sin usar**: ninguna; pero `cannon_aim` esta en el estado equivocado. `charge_orb` y `cannon_aim` tienen el canon cortado en el borde derecho del PNG; `rage_aura` trae un trozo de otro cuadro abajo a la derecha; `dash` cortado a la izquierda.
- **Bugs**: (1) orientacion; (2) caminar congelado; (3) escala por pose (la derrota gigante es el peor caso de todo el reparto); (4) golpe recibido con pose de ataque; (5) golpes identicos; (6) recortes.
- **Sin arte nuevo**: `facesRightByDefault = true` y `shoot_orb` pre-espejada; escala uniforme `235/284`; caminar = `idle3` <-> `idle4` <-> `idle1` + T1 muy pesado; golpe 1 = `cannon_aim` (apuntar) -> golpe 2 = `charge_orb` (rafaga corta, multi-golpe) -> golpe 3 = `rage_aura` (embestida de hombro, T3); patada = `slam` con onda; energia = `charge_orb` (carga) -> `shoot_orb` (disparo); dash = `dash` (aura) + T5; golpe recibido = `idle2` rotado + empujon; derribo T4 -> `death`; levantarse = `death` -> `slam` -> `idle1`.
- **Moveset** (forma final con canon): `J,J,J` apuntar-rafaga-embestida; `K` onda de choque (`slam`); `L` canonazo cargado (mantener L carga mas, escala del orbe); `adelante+L` rafaga continua multi-golpe (`charge_orb` con parpadeo); furia `rage_aura` (aura que dana) + super = `slam` + `shoot_orb` encadenados.

### 2.6 MAX — id 6
- **Tipo**: atlas de manifiesto `max` (`assets/characters/max/max_atlas.png`, 14x22 celdas 270x175, 304 con contenido) con forma transformada `max_white`. Ruta `animator` con perfil de ataque (skin 6) y efectos propios. Es un personaje distinto de RAYDEN (id 0).
- **Clips**: 25. Cuadros usados 112 de 304: **192 cuadros sin usar** (`img/max_unused_a.png`, `img/max_unused_b.png`): saltos y volteretas 21-38, combos de punos/patadas y patadas media luna 51-87, 90-91, 98-101, golpes agachado 112-125, pilares y rayos de energia 128-140, otro set grande (encapuchado, golpes, caida, levantarse, proyectil) 144-205, set pequeno con media luna 218-242, giros con media luna azul, rayo y **secuencia de derrota con sangre** 286-303.
- **`max_white` NUNCA se usa (bug)**: el atlas no declara `"clips"` y no existe el juego de clips `max_white` en `data/sprite_manifest.json` -> `InitFromManifest("max_white")` falla en `FindClips` -> `UpdateTransformForm` no cambia de atlas. La transformacion de MAX solo pone el aura dorada sobre el MAX normal (`states/s6_small.png`, "transformed idle"). Los 240 cuadros de `max_white_atlas_clean.png` (forma con llamas azules, misma disposicion que `max`) estan muertos. Arreglo: `"clips": "max"` en el atlas `max_white` (mismo orden de cuadros) o un juego `max_white`. Ojo: en la celda 142 de `max_white` hay un texto incrustado ("3MATE"/"SMAZE") que habria que borrar.
- **Estados** (`states/s6_small.png`): reposo/caminar/golpe 1-2/bloqueo/golpe recibido/derrota usan el set PEQUENO (chaqueta negra, camisa blanca, mechon blanco, ~60 px). Golpe 3, patada, energia, dash, dash-ataque, furia, remate y el 1er cuadro de derribo usan el set GRANDE (camiseta sin mangas, cinta azul, llamas, ~1.6x mas grande). **Cada combo cambia de ropa y de tamano a mitad**. `getup` = pequeno-pequeno-pequeno-GRANDE-GRANDE-pequeno-pequeno. `knockdown` empieza con un cuadro gigante de explosion (277) y uno de patada giratoria (278): parece que ataca al caer. `defeat` termina sentado (106) en vez de tumbado. `victory` = 243 (pequeno) + 244 (grande con aura). Varias celdas del set grande estan cortadas en recto (aura/llamas con borde cuadrado en `energy`, `dash`).
- **Bugs**: (1) `max_white` sin clips; (2) mezcla de dos disenos y dos escalas dentro de los clips; (3) derribo con cuadros de ataque; (4) derrota que termina sentado; (5) llamas cortadas por la celda.
- **Sin arte nuevo**: rehacer los clips con UN solo set coherente (el pequeno tiene de todo: 21-38 saltos, 51-87 combos, 112-125 agachado, 218-242 media luna, 286-303 giros y derrota con sangre 298-302) y dejar el grande solo para super/transformacion; o escalar el set grande x0.62 en sus clips (`durations` + factor por clip) para igualar tamanos. Derrota = 298-302. Derribo = 102-104 sin 277/278. Salto = 21-24 + 39-42 (`landing` ya existe) con T6.
- **Moveset** (artista marcial de energia azul): `J,J,J,J` jab-cruzado (43-50) - gancho (51-56) - media luna ascendente (57-60); `K,K,K` patada baja-media-giratoria (61-80); `abajo+K` barrida (112-118); `adelante+K` patada voladora (28-33); `L` proyectil (176-180 ya existe); `abajo,adelante+L` rayo (138-140, 293-294); furia = transformacion `max_white` real + super (286-297); salto + patada aerea (21-38).

### 2.7 KF HEROE (PELO BLANCO) — id 7 (kfRoster 0)
- **Tipo**: KF por piezas desde `apk_reference/king_fighter_iii/bin/animation.bin` (sprite 0), 458 cuadros armados en atlas. Dibujo `animator.Draw` sin efectos procedurales (el arte ya trae su movimiento). Transforma en KF HEROE TRANSFORMADO (kfRoster 11).
- **Clips**: 57, **23 habilidades** en 5 paginas (SOMBRAS, ESTALLIDO, PILAR, LLAMARADA, FENIX, TORNADO, BARRIDA, CIRCULO FUEGO, ERUPCION, MEDIA LUNA, TAJO, INFIERNO, SUPER, DESPERTAR, VOLTERETA, GIRO, RODADA, PATADA AEREA, CAIDA, TORBELLINO, MORTAL, CONTRA, SALTO TIGRE). Atlas armado solo con cuadros usados (no sobran).
- **Estados** (`states/s7_small.png`, `crops/heroe_p*.png`): reposo 4 cuadros bien; caminar = accion 1 (8 cuadros, parece trote) bien; dash 12 cuadros con chispas bien; golpes 1-3 y patada de 7/15/15/20 cuadros, muy buenos; energia = especial (18 cuadros, ruedas de fuego). Bloqueo = 1 cuadro (accion 15) = `recovery`. Golpe recibido 5 cuadros con chispa (se corta a los 0.13 s del estado `Hit`). Aire = accion 14 (lanzado+cae+tumbado, 9 cuadros) y despues `knockdown` (15 cuadros) vuelve a mostrar una caida: **cae dos veces**. Derrota termina **arrodillado**, no tumbado.
- **Duplicados**: `energy = special = finisher`; `dash_attack = dash` (el ataque en carrera es solo la animacion de dash); `rage_attack = super`; `hit_high = hit_low`.
- **Falta sin arte**: `dash_attack` = usar `ab14` VOLTERETA o `ab17` PATADA AEREA; `finisher` = `ab11` INFIERNO o `ab4` FENIX en vez de repetir el especial; golpe bajo = cuadros finales de `ab6` BARRIDA; derrota = T4 al ultimo cuadro; victoria = `transform` (accion 31, aura) o `ab13` DESPERTAR; salto = `ab14`/`ab20`.
- **Moveset**: `J,J,J,J` atk1-atk2-atk3-atk4; `abajo+K` BARRIDA; `adelante+K` PATADA AEREA; `L` LLAMARADA; `abajo,adelante+J` MEDIA LUNA; dash+J VOLTERETA; remate INFIERNO; furia SUPER; contraataque = `ab21` CONTRA al bloquear en el ultimo instante.

### 2.8 KF HEROINA (PELIRROJA) — id 8 (kfRoster 1, sprite 2)
- Tipo y dibujo como el 7. 45 clips, 421 cuadros, **11 habilidades** (SOMBRAS, ESTALLIDO, PILAR, LLAMARADA, FENIX, ERUPCION, SUPER, VOLTERETA, GIRO, RODADA, PATADA AEREA). Transforma en kfRoster 12.
- Estados: muy completos (golpes 11/13/22/20 cuadros). Bloqueo 1 cuadro. Golpe recibido 5. Aire+derribo con doble caida como el 7. Derrota 16 cuadros.
- Duplicados: `energy = special = finisher`, `dash_attack = dash`, `rage_attack = super`, `hit_high = hit_low`.
- Sin arte: `dash_attack` = RODADA o GIRO; `finisher` = FENIX; golpe bajo = RODADA; victoria = `transform`.
- **Moveset** (agil, patadas): `K,K,K` atk2-atk4-PATADA AEREA; `J,J,J` atk1-atk3-ESTALLIDO; dash+J RODADA; `abajo+K` GIRO; `L` LLAMARADA; furia SUPER.

### 2.9 KF MATON — id 9 (kfRoster 2, sprite 6, enemigo)
- 35 clips, 63 cuadros, 3 "habilidades" (SOMBRAS = **su animacion de levantarse**, TECNICA = atk1, TECNICA = atk2). Plantilla de enemigo (`kEnemyClips`).
- Estados (`states/s9_small.png`, `crops/maton_p0.png`): reposo bien; caminar bien; dash = `run` = `walk` (mismo clip). Golpe 1 = atk1 = salto con agarre que **termina tirado en el suelo** (raro para un jab). Golpe 2 = atk2 (golpe con el puno, bien). **Golpe 3, patada, furia = 1 cuadro de 0.05 s** (la accion no existe: no se ve nada). **Energia/especial/remate = 1 cuadro**. Bloqueo = `idle`. Transformar = no tiene `transform` ni `victory` -> 1 cuadro. Golpe recibido 4 (alto) / 5 (bajo, distinto) bien. Aire = salpicadura de sangre + cae (11 cuadros) bien. Derrota bien.
- Sin arte: golpe 3 = atk2 con T2 mas fuerte y avance; patada = atk1 sin la parte del suelo (cortar el clip en el cuadro 7); energia = carga de atk1 (cuadros 3-6, el salto) como "golpe al suelo"; habilidad 1 = quitar `getup`. Faltan patada y bloqueo reales.
- **Moveset** (matón pesado): `J,J,J` atk2-atk2(T2)-cabezazo (atk1 cuadros 0-6); `K` pisoton (atk1 completo con onda); `L` embestida (`run` + T3 + caja de golpe).

### 2.10 KF NAVAJERA — id 10 (kfRoster 3, sprite 10)
- 37 clips, 76 cuadros, 5 habilidades (SOMBRAS = levantarse + 4 TECNICA que repiten atk1/atk2).
- Solo tiene **2 ataques reales**: atk1 (6 cuadros) = atk4 = patada = furia; atk2 (12) = atk3. Energia/especial/remate = 1 cuadro. Bloqueo = idle. `dash_attack` = `run`.
- Sin arte: alternar atk1/atk2 con velocidades distintas (atk2 al 130 %) y estela T5 de cuchillo; patada = atk1 + T3 largo; energia = lanzar navaja: atk2 + proyectil de FX (T9).
- **Moveset** (rapida, cuchillo): `J,J,J,J` atk1-atk2-atk1(rapido)-atk2(estocada con T3); dash+J estocada en carrera; `L` navaja lanzada.

### 2.11 KF SOLDADO — id 11 (kfRoster 4, sprite 11)
- 38 clips, 76 cuadros, 6 habilidades (SOMBRAS = levantarse; TECNICA x5: atk1, atk2, atk3, atk2, skill5 = 4 cuadros 0.2 s de otra accion).
- Ataques reales: atk1 (9), atk2 = atk4 = patada = furia (7), atk3 (9). Energia/remate = 1 cuadro. Bloqueo = idle.
- Sin arte: 3 golpes reales para `J,J,J`; patada = atk2 con T2; energia = `skill5` (accion 21+, 4 cuadros) ampliada.
- **Moveset** (militar): `J,J,J` atk1-atk3-atk2; `K` culatazo (atk2+T3); `L` granada (skill5 + FX).

### 2.12 KF RUBIA — id 12 (kfRoster 5, sprite 13)
- 38 clips, 60 cuadros, 6 habilidades (SOMBRAS = levantarse, FENIX = `skill5` 6 cuadros 1.95 s, resto = atk1/atk2/atk3).
- Ataques reales: atk1 (7) y atk3 (7) distintos; atk2 = atk4 = patada = furia (5). Energia/remate = 1 cuadro. Bloqueo = idle.
- **Moveset** (acrobata): `J,J` atk1-atk3; `K` atk2; `L` FENIX (skill5); dash+J atk3 + T3.

### 2.13 KF GORRA — id 13 (kfRoster 6, sprite 14)
- 38 clips, 69 cuadros, 6 habilidades (SOMBRAS = levantarse; skill5 = 7 cuadros 1.25 s).
- Ataques: atk1 = atk3 (6), atk2 (7), atk4 = patada = furia (6). Energia/remate = 1 cuadro. Bloqueo = idle. `walk` propio (bien).
- **Moveset** (callejero): `J,J,K` atk1-atk2-atk4; `L` = skill5; dash+J atk2 + T3.

### 2.14 KF PELEADOR — id 14 (kfRoster 7, sprite 16)
- 38 clips, 96 cuadros, 6 habilidades (SOMBRAS = levantarse). Tiene **especial real** (20 cuadros 1.65 s) = energia = remate = skill5.
- Ataques: atk1 (11), atk2 (10), atk3 = atk4 = patada = furia (15). Bloqueo = idle.
- **Moveset** (boxeador): `J,J,J` atk1-atk2-atk3; `K` atk4 con T2; `L` especial; remate = especial + T10.

### 2.15 KF CUCHILLERO — id 15 (kfRoster 8, sprite 18)
- 37 clips, 86 cuadros, 5 habilidades (SOMBRAS = levantarse). Ataques: atk1 (8), atk2 = atk4 = patada = furia (7), atk3 (9). Energia/remate = 1 cuadro. Bloqueo = idle.
- **Moveset**: `J,J,J` atk1-atk3-atk2; `L` lanzar cuchillo (atk2 + FX); dash+J atk3 + T3.

### 2.16 KF JEFE GARRA — id 16 (kfRoster 9, sprite 12, escala 1.08)
- 37 clips, 101 cuadros, 5 TECNICA (= atk1..atk4 y especial). **4 ataques reales y largos** (9/22/15/14 cuadros) + especial 8. `getup` = 1 cuadro (no se levanta), `knockdown` 4 cuadros, aire 4, bloqueo = idle.
- Sin arte: levantarse = `knockdown` al reves (T8).
- **Moveset** (jefe de garras): `J,J,J` atk1-atk2-atk3; `K` atk4; `L` especial; furia = atk2 + atk3 encadenados.

### 2.17 KF BUFONA — id 17 (kfRoster 10, sprite 15)
- 38 clips, 90 cuadros, 6 habilidades (BARRIDA, TECNICA...). 4 ataques reales (12/13/7/14) + especial 13. `getup` 1 cuadro, `knockdown` 3, aire 3, bloqueo = idle.
- Sin arte: levantarse = T8 de `knockdown`; derribo = T4.
- **Moveset** (payasa acrobata): `J,J,J` atk1-atk2-atk3; `K` atk4; `L` especial; dash+J BARRIDA.

### 2.18 KF HEROE TRANSFORMADO — id 18 (kfRoster 11, sprite 1)
- Elegible suelto Y como forma del 7. 37 clips, 144 cuadros, 4 habilidades (COMBO 3, VOLTERETA, ESPECIAL, TECNICA).
- **`rage_attack` = `idle`** (no tiene super: la furia no hace nada visible). `getup` 1 cuadro. `knockdown` solo 4 cuadros (0.3 s, no llega a tumbarse del todo). Bloqueo = recovery (4 cuadros). `dash_attack` = `dash`.
- Sin arte: furia = atk3 (25 cuadros) + atk4 encadenados; levantarse = T8.
- **Moveset** (forma de fuego): `J,J,J,J` atk1-atk4; `L` especial (30 cuadros); furia = atk3+atk4; dash+J = skill5 (otra accion de 7 cuadros).

### 2.19 KF HEROINA TRANSFORMADA — id 19 (kfRoster 12, sprite 3)
- 37 clips, 79 cuadros, 4 habilidades (VOLTERETA, COMBO 3, VOLTERETA, ESPECIAL: dos repetidas).
- **El mas roto de los KF**: `hit = hit_high = hit_low = defeat = idle` (al recibir golpe o morir sigue en reposo); `knockdown = air = airborne` = 1 cuadro; `block = getup = super = rage_attack = recovery` = 1 cuadro. Solo los 4 ataques (13/16/24/30) y el especial son buenos.
- Sin arte: golpe recibido = `idle` + retroceso, temblor y tinte (codigo procedural para KF, hoy desactivado); derrota/derribo = T4 sobre el ultimo cuadro de `atk3`; furia = atk3+atk4.
- **Moveset** (lanza y rayos): `J,J,J` atk1-atk2-atk3; `K` atk4; `L` especial; furia atk3+atk4.

### 2.20 RAYDER CLON BETA — id 20 (kfRoster 13, morado, Scale2x)
- Copia exacta del heroe KF (id 7) con piezas recoloreadas a morado y cuadros x2 (escala 0.75). Mismos 57 clips, 458 cuadros y 23 habilidades; mismos problemas (doble caida, derrota arrodillado, `dash_attack = dash`, `finisher = special`). Transforma en kfRoster 14.
- **Moveset**: el del 7, con nombres propios del clon; diferencia visual = estela morada T5.

### 2.21 RAYDER CLON — id 21 (kfRoster 15, rojo, despiece propio x3)
- Heroe KF armado con el despiece del clon (`assets/characters/rayder/kf_clone/rig_rojo.*`) a 3x, poderes en rojo. 57 clips, 23 habilidades, iguales al 7. Transforma en kfRoster 16.
- **Bug visual**: el cuerpo es casi negro sobre negro (`states/s21_small.png`): silueta sin lectura de brazos/piernas en los golpes rapidos. Sin arte: borde/contorno claro de 1 px (T9) o subir brillo del despiece.
- **Moveset**: el del 7 con FX rojos.

### 2.22 RONIN — id 22 (kfRoster 32, por piezas, `data/kratos/ronin.txt`)
- **Tipo**: por piezas (`LoadBetaCharacter("ronin")`), 150 cuadros todos usados, 31 clips, 5 habilidades (ONDA DE KI, ESPADA RAPIDA, TORMENTA, RAFAGA, RODILLAZO). Archivo GENERADO por `tools/ronin/build_ronin.py` ("no editar a mano"). Mira a la derecha (correcto).
- **Estados** (`states/s22_small.png`, `crops/ronin_p*.png`): reposo 6 bien. **Caminar = correr = 6 cuadros de guardia arrastrando los pies** (no camina de verdad). Dash 3 bien. Golpes 1/2 (5 cuadros) bien, golpe 3 (3) bien, patada (5) bien, dash-ataque (5) bien, remate (5) bien, energia/especial = ONDA DE KI (27) bien, furia = super (34) bien, salto `jump` (5) existe pero nunca se usa, victoria (3) solo como transformacion.
- **Falta arte** (declarado en el propio archivo): golpe recibido = 1 cuadro de guardia erguida (no parece golpe), `hit_low` 1 cuadro, derribo/derrota/aire = 1 cuadro agachado: **nunca cae al suelo**, levantarse 3 cuadros desde agachado. Bloqueo 1 cuadro.
- Sin arte: golpe recibido = cuadro de guardia rotado -10 grados + empujon + tinte; derribo/derrota = T4 sobre el cuadro de `knockdown` hasta 85 grados (tumbado de lado) y sombra estirada; aire = `jump` cuadro 3 (patada en el aire) rotado; caminar = alternar los 2 cuadros de `idle` con zancada mas la mitad delantera de `dash`. Nota: para RONIN el codigo de `PlayerDraw` no aplica efectos (es KF) -> hay que habilitar un modo "procedural ligero" por personaje.
- **Moveset** (artista marcial de ki): `J,J,J` punch1-punch2-punch3; `J,J,K` termina en patada; `K,K` kick + RODILLAZO; dash+J dash_attack; `adelante+K` patada voladora (`jump` + T6); `L` ONDA DE KI; `abajo,adelante+J` RAFAGA; remate ESPADA RAPIDA; furia TORMENTA/super.

### 2.23 a 2.26 GUERRERO (Kratos) — ids 23-26 (ESPADAS, CESTUS, CADENA DEL RAYO, GARRAS); BETA
- **Tipo**: BETA por piezas (`data/kratos/kratos_{espadas,cesto,rayo,garras}.txt`, generados por `GowApk/tools/beta/build_beta_characters.py`; los usa tambien GowApk). 263-272 cuadros, todos usados, 35 clips, 5 habilidades (HABILIDAD A, B, C, COMBO 1, COMBO 2). Cambia de arma con Q dentro del grupo de 4 (`WeaponGroupStart`).
- **Estados** (`states/s23_small.png`, `crops/kratos_p*.png`): reposo 13 bien; **caminar = correr** (16 cuadros de carrera a toda velocidad usados para caminar: corre en el sitio a velocidad de paso, patina); dash 12 bien; golpes 1/2 (7/6) cadena de espadas bien; golpe 3 = dash_attack (22, gran barrido) bien; patada = remate (14) bien; energia (22) bien; especial (46) y super/furia (76 cuadros, **5.6 s**, fuego a pantalla completa) — muy largos para un boton de furia. Bloqueo = 1 cuadro de 0.03 s. **Golpe recibido = aire = airborne = recovery** (6 cuadros): flinch tras cada golpe propio (1.6) y vuela con la pose de "golpe de pie". Derribo (8) y levantarse (12) bien. Derrota 18 bien. Victoria 12 (de pie quieto) sin usar.
- Por arma: Cestus golpes mas cortos (atk2 = 5 cuadros 0.23 s), dash menos alto; Rayo y Garras iguales en estructura a Espadas.
- Sin arte: caminar = mismo clip `run` a 45 % de velocidad (escala de `durations`) o usar los primeros 4 cuadros de `idle` + T1; recovery = ultimo cuadro del ataque o `idle` (cambiar en el generador o en `BetaCharacter.cpp`); aire = `knockdown` cuadros 0-2; bloqueo = cuadro de bloqueo mantenido (hoy dura 0.03 s pero es en bucle; bien) + inclinacion; victoria al ganar.
- **Moveset** (por arma): Espadas: `J,J,J` atk1-atk2-atk3 (barrido), `K` atk4, `L` energia, `adelante+J` HABILIDAD A; Cestus: puno pesado, `J,J,K` con T10 extra; Rayo: alcance largo, `L` cadena de rayos; Garras: almas, multi-golpe. Furia (76 cuadros) mejor como super con coste de toda la barra; la furia normal = COMBO 1.

### 2.27 SOLDADO ESQUELETO — id 27 (BETA enemigo, `GowApk/data/characters/esqueleto.txt`)
- 51 cuadros, 31 clips, 3 habilidades (ATAQUE, ATAQUE 2, ATAQUE A DISTANCIA). **Un solo ataque** de 17 cuadros (1.37 s, tajo con estela morada) usado para punch1 = punch2 = punch3 = kick = dash_attack = finisher = rage_attack = special = super. Energia = 3 cuadros (lanzar). Bloqueo = idle. Recovery = hit (8). Derrota 3. `intro` y `projectile` sin usar.
- Sin arte: dividir el tajo de 17 cuadros en 3 golpes (cuadros 0-6, 6-11, 11-16) para `J,J,J`; patada = tajo al reves (T8) o con T7 giro; energia con el clip `projectile` como proyectil.
- **Moveset** (espadachin no-muerto): `J,J,J` tres tajos partidos; `K` tajo giratorio (T7); `L` lanzar hueso (`energy` + `projectile`).

### 2.28 BESTIA EXCAVADORA — id 28 (BETA, `excavador.txt`)
- 65 cuadros, 31 clips, 3 habilidades. Igual patron que el esqueleto: un ataque para todo, energia propia, recovery = hit, bloqueo = idle.
- **Moveset** (topo): partir su ataque en 3; `L` excavar/emerger (energia); dash = `run` con polvo FX.

### 2.29 MOMIA CON ESCUDO — id 29 (BETA, `momia_escudo.txt`)
- 49 cuadros, 31 clips, 3 habilidades. Un ataque para todo, energia propia, recovery = hit, **bloqueo = idle aunque lleva escudo**.
- Sin arte: bloqueo = primer cuadro del ataque (escudo delante) mantenido.
- **Moveset** (tanque): `J,J` escudazo partido; `K` empujon con escudo (T3); bloqueo con escudo real.

### 2.30 BESTIA ELEFANTE — id 30 (BETA, `bestia_elefante.txt`)
- 71 cuadros, 30 clips. Ataque de 36 cuadros (2.43 s!) para punch1-3/kick; `dash_attack = energy = finisher = rage = special = super` = 3 cuadros. `air` 1 cuadro. Bloqueo = idle = victoria. Recovery = hit.
- Sin arte: partir el ataque de 36 cuadros en 3-4 golpes; energia de 3 cuadros = embestida con T3.
- **Moveset** (bestia pesada): `J,J,J` partido; `K` pisoton (T10); dash+J embestida.

### 2.31 AVE SANADORA — id 31 (BETA, `ave_sanadora.txt`)
- 68 cuadros, 30 clips. Ataque de 33 cuadros para todo, especial 2 cuadros, `air` 1, recovery = hit, bloqueo = idle = victoria.
- **Moveset** (voladora): `J,J` picotazo partido; `L` = especial ampliado con FX de curacion (`fx_curacion.txt` ya existe en GowApk); se cura al usar la furia.

### 2.32 CENTAURO / 2.33 CENTAURO ROJO — ids 32, 33 (BETA, `centauro.txt`, `centauro_rojo.txt`)
- 98 cuadros, 30 clips. Ataque de 25 cuadros para punch1-3/kick; especial 22 para dash_attack/energy/finisher/rage/special/super. **Recovery = hit = air** (10). Bloqueo = idle = victoria. Derrota 3. Los cuadros traen un **circulo magico azul permanente** bajo los cascos (arte del jefe) que queda raro en el jugador; es enorme (desborda las celdas).
- Sin arte: partir el ataque de 25 en 3; carga de lanza = `run` + T3 + caja.
- **Moveset** (lancero): `J,J,J` estocadas partidas; dash+J carga de lanza; `L` especial.

### 2.34 BRUTO DE LA BOLA / 2.35 BRUTO DE LA BOLA ROJO — ids 34, 35 (BETA, `bruto_cadena*.txt`)
- 116/114 cuadros, 31 clips. Ataque de 24 cuadros (2 s) para todos los ataques incluidos especial/super; energia propia de 51 cuadros (2.7 s); recovery = hit = air (13); bloqueo = idle = victoria.
- **Moveset** (bola y cadena): `J,J` mitades del ataque; `K` golpe de bola al suelo (T10); `L` energia larga como super.

### 2.36 MEDUSA / 2.37 MEDUSA ROJA — ids 36, 37 (BETA, `medusa*.txt`)
- 65/68 cuadros, 30 clips, 2 habilidades. 2 ataques: atk1 = atk2 (14) y atk3 = atk4 = patada = energia = especial = super (15). `air` 1 cuadro, recovery = hit, bloqueo = idle = victoria. Sin pose de petrificar propia.
- **Moveset** (serpiente): `J,J` coletazo; `K` ataque 2; `L` mirada petrificante = ataque 2 + tinte gris al rival (T9).

### 2.38 a 2.41 RONIN-KRATOS — ids 38-41 (ESPADAS, CESTUS, RAYO, GARRAS); BETA
- **Tipo**: copia de los `kratos_*.txt` con piezas del cuerpo de RONIN (`tools/ronin_kratos/rk_build.py` -> `data/kratos/ronin_kratos_*.txt`), escala 0.96. Mismos clips, tiempos, cajas y sonidos que Kratos (`dump.txt`: identica estructura).
- **Estados** (`states/s38_small.png`): mismos que Kratos: caminar = correr, recovery = hit (flinch tras cada golpe), aire = hit, bloqueo 1 cuadro, furia de 5.6 s. Visualmente bien armado.
- **Moveset**: el de Kratos por arma, con el estilo de Ronin (agregar la ONDA DE KI de Ronin como `L` en las 4 armas seria un buen diferenciador, reutilizando sus cuadros si se incluyen sus imagenes en el `.txt`).

---

## 3. Bloques de trabajo (para repartir sin pisarse archivos)

Regla: cada archivo tiene UN dueno. Donde dos bloques necesitan el mismo archivo se indica quien lo toca y en que orden.

### Bloque 0 — Nucleo de combate (UN implementador, va PRIMERO; los demas dependen de sus ganchos)
- Archivos: `src/game/Player.h`, `src/game/Player.cpp`, `src/game/player/PlayerCombat.cpp`, `src/game/player/PlayerCommon.h`, `src/game/combat/AttackData.{h,cpp}`, `src/game/VSMode.cpp` (pose de victoria al ganar).
- Tareas: estado `Victory` (y opcional `Jump` usando `position.z`); tabla de combo por personaje (cadenas de nombres de clip, p.ej. 4 golpes, `abajo+K`, `adelante+J`) con fallback a lo actual; `recovery` que use `idle`/ultimo cuadro cuando el clip `recovery` == `hit`; hook `CharacterHasClip`/`PlayIfExists`; habilidades de los no-KF configurables por personaje en vez de `kOurSkills` fijo.

### Bloque 1 — Poses sueltas: CLON ANTIGUO (1), GRINDER (3), TITAN-X (4), TITAN-X MEJORADO (5)
- Archivos: `src/game/CharacterVisual.cpp` (`CharacterPose` y las filas 1, 3, 4, 5 de `kCharacters`), `src/game/CharacterVisual.h` (campos nuevos: tabla de poses por personaje con `miraDerecha`, `dx`, escala uniforme), `src/rendering/BossSprite.h` (nueva `DrawBossPoseEx` con rotacion/ancla/escala uniforme), `src/rendering/AssetManager.cpp` (lista de `LoadBossPoseSet` si se agregan claves compuestas, p.ej. el puno unido de Titan-X).
- `src/game/player/PlayerDraw.cpp`: SOLO la rama `if (skin != 0 && cv.folder != nullptr && cv.atlasId == nullptr)` / `if (altTex.id != 0)`. Recomendado: mover esa rama a un archivo nuevo `src/game/player/PlayerPoseDraw.cpp` (llamado desde una linea en `PlayerDraw.cpp`) para no chocar con el Bloque 2.
- Opcional, coordinar: `src/game/combat/Boss.cpp` y `src/game/Stage3Game.cpp`/`Stage4Game.cpp` tienen el mismo error de orientacion de Titan-X/Titan-X Mejorado como JEFES (mismos PNG).
- Limpieza de PNG (restos de recorte en `assets/bosses/rayder_clone/*.png`, `titanx/slam.png`, `titanx_mejorado/rage_aura.png`): script nuevo en `tools/`, no arte nuevo.

### Bloque 2 — Atlas del manifiesto: RAYDEN (0, solo agregar), BRAKK (2), MAX (6)
- Archivos: `data/sprite_manifest.json` (clips nuevos; `"clips": "max"` o juego `max_white`; clips de Brakk sin mate), `src/game/combat/AttackAnimation.h` (perfiles skin 2 y 6, y agregar skin 0 si se quiere), `src/game/player/PlayerDraw.cpp` SOLO la parte `animator` no-KF (switch de `sx/sy/angle`, estelas, root motion de `UpdateMotionFeel`). Limpieza de `assets/bosses/brakk/brakk_v2_atlas_clean.png` (mate marron, linea suelta) y `assets/characters/max/max_white_atlas_clean.png` (texto en celda 142) con scripts de `tools/` (`clean_sprite_cutouts.py`, `fix_white_matte.py` ya existen).
- RAYDEN: no modificar `assets/characters/rayden_128.png` ni sus 19 clips; solo clips nuevos con nombres nuevos.

### Bloque 3 — KF de la APK: ids 7-21
- Archivos: `src/game/lab/KfReference.cpp` y `KfReference.h` (exclusivos de este bloque). Tareas: mapear `dash_attack`, `finisher`, `rage_attack`, `block` a acciones/habilidades reales distintas; quitar `getup` de la lista de habilidades de enemigos (accion 20); no rellenar con `idle` en silencio (generar `hit` procedural, derribo = ultimo cuadro rotado); evitar la doble caida (`airborne` -> `knockdown`); arreglar la HEROINA TRANSFORMADA (19).
- Si necesitan efectos procedurales en el dibujo KF (hoy `animator.Draw` sin nada), pedir al Bloque 2 un flag `cv.proceduralLight` o hacerlo en un archivo nuevo; no editar `PlayerDraw.cpp` en paralelo.

### Bloque 4 — RONIN (22)
- Archivos: `tools/ronin/build_ronin.py` -> regenerar `data/kratos/ronin.txt` (no editar el `.txt` a mano). Derivados procedurales (caer, golpe recibido) se pueden generar en el script como cuadros con piezas rotadas (`rot90`/posiciones), sin arte nuevo.

### Bloque 5 — BETA por piezas: GUERRERO (23-26), enemigos (27-37), RONIN-KRATOS (38-41)
- Archivos: `GowApk/tools/beta/build_beta_characters.py` (genera `data/kratos/kratos_*.txt` y `GowApk/data/characters/*.txt`), `tools/ronin_kratos/rk_build.py` (genera `data/kratos/ronin_kratos_*.txt`, hay que regenerarlo DESPUES de cambiar los de Kratos), `src/game/beta/BetaCharacter.cpp` (si se prefiere derivar clips al cargar: partir ataques largos, `recovery` != `hit`, caminar = run lento).
- OJO: estos `.txt` los usa tambien GowApk; cualquier cambio de clips existentes afecta a ese juego. Preferible AGREGAR clips nuevos (p.ej. `df_punch2`) o derivarlos en `BetaCharacter.cpp` solo para District Fury.
- Es un unico bloque porque Kratos y enemigos salen del MISMO generador; si se quiere partir en dos personas, una hace generador y otra `BetaCharacter.cpp`.

### Orden y choques
1. Bloque 0 primero (ganchos de combo/estado). 2. Bloques 1, 2, 3, 4, 5 en paralelo. Choque unico real: `PlayerDraw.cpp` entre 1 y 2 -> resuelto moviendo la rama de poses a `PlayerPoseDraw.cpp` (Bloque 1) antes de empezar. `CharacterVisual.cpp` solo lo toca el Bloque 1 (si otro bloque necesita un campo nuevo en `CharacterVisual.h`, se lo pide al Bloque 1 o al 0).

---

## 4. Top 10 de problemas mas graves

1. **TITAN-X camina, dashea, patea y dispara de espaldas**: `facesRightByDefault=false` pero sus poses miran a la derecha; tampoco tiene pose de caminar (repite el reposo a 9 fps). Su golpe es media pose sin cabeza (`punch1.png` y `punch2.png` son las dos mitades de una pose cortada) y al recibir golpe muestra la pose de disparo (`recoil.png` y `shoot_orb.png` tienen los nombres cruzados). El jefe Titan-X sale igual de girado (`Boss.cpp`, `Stage3Game.cpp`).
2. **TITAN-X MEJORADO**: el mismo error de orientacion; su derrota se dibuja **2.25x mas grande** y al recibir golpe apunta el canon.
3. **`DrawBossPose` reescala cada pose a la misma altura**: tamanos que saltan en CLON, TITAN-X y TITAN-X MEJORADO (derrotas gigantes, golpes mas grandes que el cuerpo).
4. **Los 4 personajes de poses sueltas no tienen nada procedural**: caminar congelado y patinando (Grinder ni siquiera respira), los 3 golpes del combo son la misma pose, nunca caen al suelo ni se levantan.
5. **MAX**: su forma transformada `max_white` (240 cuadros) nunca se carga porque el atlas no tiene juego de clips; y sus clips mezclan dos disenos y dos escalas (chaqueta pequena / camiseta grande), asi que cambia de ropa y de tamano a mitad de combo. 192 cuadros de `max` sin usar (saltos, combos, barridas, media luna, derrota).
6. **BETA (Kratos, Ronin-Kratos y los 11 enemigos)**: `recovery` = `hit`, asi que hacen un gesto de dolor tras CADA golpe propio; Kratos vuela por los aires con la pose de golpe de pie; caminar = correr.
7. **Enemigos BETA con un solo ataque para todo** (esqueleto, momia, excavadora, elefante, ave, centauros, brutos, medusas): golpe 1 = 2 = 3 = patada = especial = super.
8. **Enemigos KF (MATON, NAVAJERA, SOLDADO, RUBIA, GORRA, CUCHILLERO...)**: energia/especial/remate de 1 cuadro de 0.05 s (no se ve nada), varios golpes de 1 cuadro (MATON), bloqueo = reposo, y la habilidad "SOMBRAS" es en realidad su animacion de levantarse.
9. **KF HEROINA TRANSFORMADA**: golpe recibido y derrota = reposo, y derribo/furia/bloqueo de 1 cuadro. KF HEROE TRANSFORMADO: la furia es el reposo. En los heroes KF se ve la caida dos veces (`airborne` + `knockdown`) y la derrota termina arrodillado.
10. **Sin variedad de movimiento en todo el reparto**: no hay salto, victoria, giro, barrida ni combos de mas de 3 golpes (la FSM y la cadena de combo son iguales para los 42), y las habilidades 1-5 de los ids 0-6 repiten los golpes normales. Hay arte de sobra sin usar (clips de jefe de Brakk, 50 cuadros de Brakk, 192 de MAX, `jump`/`victory`/`intro` de los de piezas). RONIN tambien necesita arte o procedimiento para recibir golpe, caer y perder.
