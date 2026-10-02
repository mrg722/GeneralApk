# District Fury: estado del proyecto (v0.21-lab)

## Lo que llevamos ✅

| Área | Estado |
|---|---|
| **Motor de combate** | FSM completa (reposo, caminar, dash, ataque, bloqueo, golpe, rotura de guardia, en el aire, derribo, recuperación). Buffer de entrada de 15 frames, cadena de golpes, cancelación por especial, comandos ↓↘→ y →↓↘, hitstop y sacudida suave. |
| **Habilidades** | 6 por personaje, con 15 s de espera y un reloj en el botón. La 6 es TRANSFORMAR (12 s con más daño y velocidad y cambio de forma). |
| **Rayden Cruz** | Jugable, 22 frames, con fluidez añadida (respiración, balanceo, avance al golpear y estelas). |
| **Rayder** | Usa todas las hojas recibidas: estilo KF, combate 50, avanzada 36, carga de transformación 12 y forma de pelo blanco completa. Aparece de espaldas en la elección. |
| **Rayden clon (rojo)** | Igual que antes (poses del jefe). |
| **Rayder clon (rojo) — NUEVO** | La fórmula del héroe KF con un despiece propio generado con Canva (8 cabezas, 6 torsos, 12 piezas de brazo, 10 de pierna y otras 17 base). Cada una de las 208 piezas del KF se etiquetó a mano y recibe la pieza del clon de la misma parte, a 3x de resolución. Tiene todas las habilidades del KF y los poderes en rojo (`tools/build_kf_clone_rig_canva.py`). Modo VS: personaje "RAYDER CLON". |
| **Todas las habilidades KF** | Los personajes KF ya no tienen solo 5 habilidades: el héroe tiene 23 (fuego, súper, despertar, acrobacias), repartidas en páginas de 5 en los botones 1–5. El botón PAG (TAB) cambia de página y cada habilidad tiene su propia espera de 15 s. |
| **Rama mejora-de-diseno (GPT)** | Integrada: secuencias del Rayden clon, tiempos y avance de los ataques sincronizados con AttackData para Rayder y Brakk, superposición de depuración F3 y prueba AttackAnimationTimingTests. |
| **Rayder clon BETA (prueba, morado)** | **Rompecabezas sobre una copia del héroe KF de pelo blanco**, con todos sus cuadros, habilidades, súper y transformación. Cada una de las 208 piezas de su cuerpo se clasificó en cabeza, torso, chaqueta, manga, antebrazo, puño, muslo, pierna o bota. En cada pieza se pegó la misma parte recortada de la figura del Rayder clon, girada y escalada para seguir su forma y con las sombras y el contorno del KF (`tools/build_kf_clone_pieces.py` → `assets/characters/rayder/kf_clone/img_1.png`). Los rojos pasaron a morado claro eléctrico, la piel quedó intacta y los poderes son morados. La calidad se subió a 2x con Scale2x. El héroe KF original no se toca. |
| **Laboratorio KF** | 13 personajes con sus colores originales. El héroe y la heroína se transforman en sus formas reales (sprites 1 y 3). |
| **Brakk** | Hoja mejorada en el Nivel 1 y jugable en VS. |
| **Nivel 1** | Completable: 4 escenarios, oleadas, "GO >>", guardianes y Brakk. |
| **Modo VS** | Escenario BETA, 20 personajes y rival KF. |
| **Controles táctiles** | Joystick y botones con tu arte, en español. Opción de ANCHO DE PANTALLA. |
| **Efectos** | Tus dos hojas de efectos azules: impacto, escudo, proyectil, estallido y estela. |
| **Audio** | Tu paquete de 24 sonidos: golpes al conectar, silbido al lanzar, bloqueo, dash, aterrizaje, poderes, transformación, KO, menús y lluvia de ambiente. |
| **Android** | APK firmado (arm64, Android 7 o superior), ícono DF e instrucciones adb. |

## Escenarios marcados BETA (fondo provisional con su código)

E1.S2, E1.S3, E1.S4 · E2.S3, E2.S4 · E3.S1 a E3.S4 · E4.S1 a E4.S4 · E5.S1 a E5.S4.

Para cambiar uno, pon el fondo definitivo en `assets/backgrounds/hd/stageN_scenarioMM.png` (1600×720 o más). `tools/build_beta_backgrounds.py` no sobrescribe los fondos reales.

## Lo que nos falta ⏳

| # | Falta | Quién / qué |
|---|---|---|
| 1 | Hoja estilo KF completa de **Rayden Cruz** | Usuario (arte) |
| 2 | Hoja del **Rayder de pelo blanco** con una figura por casilla y sin texto | Usuario (arte) |
| 3 | **Brakk** sobre fondo transparente | Usuario (arte) |
| 4 | **Enemigos comunes**: caminar, en el aire y levantarse | Usuario (arte) |
| 5 | **17 fondos HD** (los BETA de arriba) | Usuario (arte) |
| 6 | **Música** de menú, combate y jefe | Usuario (audio) |
| 7 | **Jefes** Grinder, Titan-X y Titan-X mejorado con hoja completa | Usuario (arte) |
| 8 | Probar en el **Samsung A57** y enviar `adb logcat` si algo falla | Usuario (prueba) |
| 9 | Ajustar la dificultad de Brakk (un bot que no es invulnerable pierde en la fase 3) | Equipo (código), tras tu prueba |
| 10 | Quitar el laboratorio KF cuando ya no sirva de referencia | Equipo (código), ver `ESTRUCTURA_CODIGO.md` |

Formato del arte: PNG con transparencia, una fila por movimiento, figuras separadas, sin números tocando el dibujo, mirando a la derecha y con los pies alineados.

## Auditoria 1x1 (v0.23)

Herramientas nuevas (no son parte de ctest, se corren a mano):
- `build/all_modes_check [carpeta]`: Modo VS como un jugador (22 personajes, pelea con cada uno),
  los 17 rivales KF del laboratorio y los 5 stages de Historia con Rayden y Rayder. Capturas de cada uno.
- `build/vs_boss_visual_check [carpeta] [personaje]`: los 5 jefes del Modo VS.

Corregido en esta auditoria:
- Jefe Brakk del Modo VS con la hoja mejorada (antes poses viejas pixeladas).
- Fases de los jefes del VS invertidas (vida llena = FASE 3).
- Jefe Rayder clon (VS y Stage 5) con el diseno final rojo sin mangas; logica de ataques de GPT intacta.
- Stage 2 y Stage 3: un relleno tapaba el fondo HD; Stage 3 tenia la camara al reves (al avanzar,
  jugador y fondo salian de la pantalla).
- Stages 2-5 decian "RAYDEN CRUZ" con cualquier personaje.
- Barra del jefe tapaba el panel del jugador en los 5 stages.
- F3 (depuracion de GPT) cambiaba al Stage 3 en Historia: ahora es F9.
- Rayden Cruz volvio a avanzar al golpear; tests con assert activos en Release.
- "RAYDEN CLON" (clon viejo con chaqueta) renombrado "CLON ANTIGUO (CHAQUETA)"; el VS muestra n/22.

## Escenarios (v0.25) — regla acordada

- Los escenarios de buena calidad no se tocan: stage1_scenario01 (Barrio Bajo), stage2_scenario01
  (tuberias) y stage2_scenario02 (fundicion), con su arte HD en assets/backgrounds/hd/.
- Los escenarios pixelados, rotos, faltantes, incompletos o danados usan el fondo BETA con su
  codigo en neon (E<stage>.S<escenario>) en assets/backgrounds/hd/ (tools/build_beta_backgrounds.py).
  Cuando llegue el arte definitivo basta con reemplazar ese archivo.
- Stages 4 y 5 ya no dibujan postes y franjas de respaldo encima del fondo; Stage 3 tenia la camara al reves.
- Modo VS: STAGE + ESCENARIO eligen uno de los 20 escenarios (con el mismo criterio de arriba).

## Rival (IA) en el Modo VS (v0.25)

- Campo nuevo "RIVAL (IA)": cualquiera de los 22 personajes (KF heroe y heroina, sus formas
  transformadas, Rayder, Rayder clon, Brakk...) como oponente 1 vs 1 manejado por la maquina
  (src/game/RivalAI.cpp). Es un Player completo: mismos movimientos, habilidades por paginas,
  transformacion, bloqueo y dash que cuando lo controla el jugador.
- Verificado con all_modes_check: cada rival usa habilidades, golpea y recibe golpes.

## MODO BETA (nuevosSprites)

Opcion **MODO BETA** del menu principal (debajo de MODO VS). Es una experiencia aparte
con el contenido de `nuevosSprites.zip`; no cambia Historia ni VS (los personajes BETA
no aparecen en VS).

- **Personajes (15):** heroe con 4 armas (Espadas del Caos, Cestus de Nemea, Cadena del
  Rayo, Garras de Hades) y 11 enemigos (esqueleto, excavadora, momia con escudo,
  elefante, ave sanadora, centauro, bruto de la bola, medusa y sus variantes rojas).
  Cada cuadro se arma con las piezas originales (sin escalar ni deformar, 1.25 en
  pantalla igual que el mapa), con sus cajas de cuerpo y de ataque originales.
- **Escenarios (6):** cubierta del barco, montana nevada, cueva helada, camino y arena
  del volcan, paso de la montana. Mapas de tiles originales + parallax + clima; la
  franja caminable sale de la capa de colision.
- **Modos:** OLEADAS (avanzar por el escenario, 4-5 oleadas con enemigo final) y
  1 VS 1 contra cualquier personaje BETA manejado por RivalAI.
- **Sonido:** sonidos por cuadro de golpes y habilidades, herido/muerte por personaje,
  UI y musica (menu, barco, volcan, victoria, derrota) del paquete original.
- **Efectos:** chispas de impacto, humo de muerte, almas; lluvia/rayos, nieve y ceniza.
- Datos: `data/beta/` (generados por `tools/beta/*.py`), arte/sonido: `assets/beta/`.
- Auditoria completa archivo por archivo: `docs/beta/AUDITORIA_BETA.md`.
- Verificacion: `./build/beta_mode_check` (carga los 15, gana las oleadas de los 6
  escenarios y dos 1 VS 1 sin invulnerabilidad).
- Pendiente: jefes Titan, Poseidon y monstruo de tentaculos (arte en Spine 2.1).
