# District Fury: estado del proyecto (v0.17-lab)

## Lo que llevamos ✅

| Área | Estado |
|---|---|
| **Motor de combate** | FSM completa (reposo, caminar, dash, ataque, bloqueo, golpe, rotura de guardia, en el aire, derribo, recuperación). Buffer de entrada de 15 frames, cadena de golpes, cancelación por especial, comandos ↓↘→ y →↓↘, hitstop y sacudida suave. |
| **Habilidades** | 6 por personaje, con 15 s de espera y un reloj en el botón. La 6 es TRANSFORMAR (12 s con más daño y velocidad y cambio de forma). |
| **Rayden Cruz** | Jugable, 22 frames, con fluidez añadida (respiración, balanceo, avance al golpear y estelas). |
| **Rayder** | Usa todas las hojas recibidas: estilo KF, combate 50, avanzada 36, carga de transformación 12 y forma de pelo blanco completa. Aparece de espaldas en la elección. |
| **Rayden clon (rojo)** | Igual que antes (poses del jefe). |
| **Rayder clon BETA (prueba, morado)** | Se recortaron las 23 animaciones de tu hoja del clon (idle, caminar, correr, dash, saltar, golpes, combos, patada, onda oscura, rage, teletransporte, clones, ataque aéreo, finisher, daño y muerte) y se pintaron de morado claro eléctrico, con la piel intacta. Luego se montaron cuadro a cuadro sobre una copia de los pasos y tiempos del héroe KF: hereda su cadencia, sus 5 habilidades, el súper y la transformación. El héroe KF original no se toca. Herramienta: `tools/build_rayder_clone_beta.py`. |
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
