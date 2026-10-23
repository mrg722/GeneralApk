# Jingle de arranque (`intro_arranque.wav`)

## Que es

Un jingle de **~3.9 segundos**, generado 100% por codigo (sintesis
matematica: osciladores, filtros y reverberacion por convolucion), que
suena **una sola vez** al abrir District Fury.

## Aclaracion legal explicita

El pedido original fue "que suene como la intro de Dragon Ball Z / Budokai
Tenkaichi". Esa musica es propiedad de Toei Animation / Bird Studio / Bandai
Namco. **Este archivo NO transcribe esa melodia ni ninguna otra obra
existente, no samplea ni copia audio real de ningun juego o pelicula, y no
fue generado con una IA de audio.** Es una composicion **original**, nueva,
hecha para capturar la *energia/vibra* generica de un jingle de arranque de
juego de pelea dramatico (golpe de entrada con "bronces" sinteticos y
percusion tipo taiko/toms, un barrido ascendente de energia tipo "carga de
poder", e impacto final brillante con cola larga) sin citar la pieza
real. Es inspiracion en la energia del genero, no una reproduccion.

## Tecnica de sintesis

Script: `tools/audio/genera_intro_ronin.py` (misma familia de tecnicas que
`tools/audio/genera_sonidos_rayder.py`: capas, envolventes, reverberacion
por convolucion, semilla fija para reproducibilidad — nada grabado, nada de
IA de audio).

Estructura:

- **0.00-0.30 s** — silencio / respiracion tensa (ruido grave muy bajo).
- **0.30-1.00 s** — golpe de entrada: "acorde de bronces" sintetico grave
  (varios osciladores de sierra en relacion de octava/quinta que suben de
  tono) + percusion tipo taiko/toms + un stinger brillante encima.
- **1.00-2.60 s** — barrido ascendente de energia ("carga de poder": tono
  subiendo con armonicos, tremolo que se acelera, chispas de ruido agudo —
  misma receta que `carga_poder()` de Rayder) con percusion acelerando
  debajo.
- **2.60-4.20 s (bloque final, suma ~3.90 s totales)** — impacto final
  grande (transitorio + cuerpo grave doble + brillo metalico) con una cola
  de reverberacion larga (~1.3 s) que se apaga.

Formato: WAV estereo, 44100 Hz, 16 bits. Semilla fija (`SEMILLA = 20261023`)
para que el resultado sea reproducible. Pico normalizado a **-1 dBFS** (mas
fuerte que los efectos normales del juego, que rondan -12 dB RMS, porque
este suena una sola vez al arrancar) sin saturar — resultado medido:
pico -1.00 dBFS, RMS -14.42 dBFS.

Para regenerarlo:

```
python3 -I tools/audio/genera_intro_ronin.py assets/audio/sfx
```

## Donde esta cableado

- `src/audio/AudioSystem.h`: entrada `Sfx::IntroArranque` en el enum.
- `src/audio/AudioSystem.cpp`: entrada `{Sfx::IntroArranque, "intro_arranque", 0.9f}`
  en la tabla `files[]` de `Init()`.
- `src/main.cpp`: justo despues de `AudioSystem::Get().Init()`, se llama
  `AudioSystem::Get().Play(Sfx::IntroArranque);` **una sola vez**, al
  arrancar `main()` (no esta en ningun loop ni se repite al volver al menu).

## Estado

**Placeholder temporal.** Esta marcado como tal con un comentario en
`src/main.cpp` y `src/audio/AudioSystem.cpp`: pendiente de reemplazar por
musica definitiva.
