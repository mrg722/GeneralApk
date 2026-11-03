# Auditoria de AUDIO — District Fury (rama ccr-077ad675-rlrg28, commit 1567205)

Solo lectura. No se modifico nada del repo. Scripts de analisis usados:
`/tmp/claude-0/-home-user/05b0c2b9-43b9-5c8e-82ee-61da3b1aff5a/scratchpad/wavstat.py` y `wav2.py`.

---

## 0. Resumen ejecutivo (lo que mas duele, por orden)

1. **Los golpes genericos son ruido blanco sin cuerpo.** Los WAV del "starter pack" (`combat_*`, `move_*`, `fury_*`) tienen centroide espectral de ~13 kHz y **menos del 1 % de energia entre 500 Hz y 3 kHz**: solo un subgrave (<120 Hz) mas siseo. En el altavoz de un movil, que no reproduce casi nada por debajo de ~250 Hz, suenan como un "tsss" plano. Esos archivos los usan Rayden (skin 0), todos los KF, Kratos y el resto del roster, y ademas los usa el lado ENEMIGO.
2. **Los paquetes Rayder/MAX y Ronin tienen el problema contrario:** son graves y no tienen chasquido. Los golpes llevan el 96-98 % de su energia entre 120 y 500 Hz. `rayder/patada_1` (87 % por debajo de 120 Hz), `rayder/patada_3` (90 %), `rayder/impacto_fuerte` (73 %), `ronin/aterrizaje` (97 %) y `ronin/impacto_fuerte` (63 %) **casi no se oiran en un altavoz de movil**.
3. **Sonido doble en cada golpe que conecta** (en el Stage 1): `PlayerDraw.cpp` reproduce el impacto del personaje y `CombatWorld.cpp:300` reproduce ademas `Hit`/`HeavyHit` generico. En MAX/Ronin se mezcla el golpe propio con el siseo generico, y si el golpe mata se suma `EnemyDeath`.
4. **Los Stages 2-5 y el modo VS no llaman nunca a AudioSystem.** La muerte de un enemigo no suena, el golpe en el jefe tampoco, y **si un enemigo te pega con menos de 28 de dano no suena nada** (`Player::TakeDamage`, en su rama normal, no reproduce ningun sonido).
5. **No hay limite de voces, ni variacion, ni prioridad, ni mezcla.** Hay un solo `Sound` por Sfx. `PlaySound` reinicia la reproduccion en curso y corta su cola. No se usa nunca `SetSoundPitch` ni `SetSoundPan`. No hay buses (SFX/UI/musica/ambiente) ni ducking.
6. **No hay musica en el juego principal.** Solo suenan la lluvia en bucle de 2 s (en TODOS los modos, menus incluidos) y el jingle de arranque.
7. **El modo BETA (GowApk) tiene un problema de copyright y esta roto en parte.** Su musica (`menu.ogg`, `gate1music.ogg`, `gate2music.ogg`, `gameCG.ogg`, `music_00x.ogg`) y sus SFX son audio extraido del juego original del que viene `nuevosSprites`. Eso **choca con la regla del usuario** (nada de musica con copyright). Ademas, 17 de sus .ogg son stubs mudos de 23 ms, incluidos `sound24`, `sound44` y `sound45`, que el codigo usa. Los sonidos de Kratos (`sound34-38`, `61-65`, `70`, `73-75`) estan en `assets/kratos/audio/`, pero BetaMode los busca en `GowApk/assets/audio/`, **asi que no suenan**.

---

## 1. `src/audio/AudioSystem.h/.cpp` — como esta montado

- `enum class Sfx` tiene 61 entradas (incluidas Rayder ×13, Ronin ×11 e IntroArranque) y un `std::array<Sound, Count>` con un `Sound` por slot.
- **Respaldo sintetizado** (`BuildSound`): una senoidal con un segundo armonico a ×2.73 cuando `noise=true` (no es ruido de verdad), envolvente lineal y 23 sonidos. Todos suenan a "beep". Solo se oyen si falta el WAV. Rayder y Ronin no tienen respaldo, asi que sin WAV se quedan mudos.
- **Tabla `files[]`**: carga `assets/audio/sfx/<name>.wav` y llama a `SetSoundVolume` una vez con un volumen fijo de entre 0.5 y 0.95. Puntos concretos:
  - `fury_charge.wav` se carga **dos veces** (`EnergyCharge` y `FuryCharge`) y `combat_ko.wav` tambien dos veces (`EnemyDeath` y `GameOver`). Son copias duplicadas en RAM y el **GameOver suena igual que la muerte de un enemigo cualquiera**.
  - `ui_open.wav` y `world_city_industrial.wav` **no se cargan nunca**.
  - Los Sfx que estan cargados pero no se reproducen en ningun sitio son `Jump`, `BossAttack`, `BossPhase`, `StageClear`, `RoninSalto` y `RoninAterrizaje`, y en la practica tambien `UiConfirm`/`UiCancel` fuera de la pausa del Stage 1.
  - `StageClear` usa `energy_pickup.wav`, pero los pickups reales usan `Sfx::Ui` (`ui_navigate`).
- **`Play(Sfx)`**: `if (!ready || muted || betaAudio) return; PlaySound(sound)`. No hay pitch, ni pan, ni alias, ni cooldown, ni conteo de voces.
- **Modo BETA**: `SetBetaAudio(true)` (lo llama `BetaMode.cpp:136`) silencia TODO `Play()` y pausa la lluvia. BetaMode reproduce lo suyo con `::PlaySound` y su propia cache.
- **Ambiente**: `world_rain_loop.wav` (2.0 s, mono, RMS -32 dBFS) va como `Music` en bucle a 0.22 y se actualiza cada frame desde `main.cpp:138`. Suena en menus, en el modo VS y en todos los stages por igual. Un bucle de 2 s se nota que se repite. La union del bucle esta limpia (salto de 0.0). No existe silenciado independiente para musica/SFX: solo hay `SetMuted` global.
- raylib es la **5.5**, que trae `LoadSoundAlias`, `SetSoundPitch`, `SetSoundPan` y `AttachAudioMixedProcessor`. Todo lo que propongo abajo se puede hacer **sin dependencias nuevas**.

---

## 2. Mapa de llamadas: que suena en cada accion

### 2.1 Lado jugador — ataque (`src/game/player/PlayerCombat.cpp` y `PlayerDraw.cpp`)

| Accion | Rayden (skin 0), KF, Kratos, resto | MAX (skin 6, `IsRayder()`) | RONIN (`IsRonin()`) |
|---|---|---|---|
| Lanzar golpe/patada normal (`BeginAttack`, l.188-211) | `Whoosh` (move_whoosh) | `RayderSilbido` | `Whoosh` |
| Lanzar EnergyWave | `EnergyCharge` (fury_charge) | `RayderCargaPoder` | `RoninEspadaDesenvaine` |
| Lanzar RageAttack | `Rage` (fury_release) | `RayderFuria` | `RoninEspadaTajo` |
| Lanzar Finisher | `FuryCharge` (fury_charge, el **mismo** WAV que EnergyWave) | `RayderDisparoPoder` | `RoninEspadaTajo` (el **mismo** que RageAttack) |
| Habilidad por clip (`BeginClipAttack`, l.97) | `EnergyShot`, o `FuryCharge` en la ultima habilidad | **generico** (EnergyShot/FuryCharge) | **generico** |
| Impacto al conectar un golpe ligero (PlayerDraw l.87-110) | `Punch` (combat_punch_light) | `RayderGolpe1/2/3` rotando por `comboStep%3` | `RoninGolpe1/2/3` rotando |
| Impacto de patada ligera | `Kick` | `RayderPatada1/2/3` | `RoninPatada1` (siempre la misma) |
| Impacto pesado | `HeavyHit` | `RayderImpactoFuerte` | `RoninImpactoFuerte` / `RoninPatada2` |
| Impacto de habilidad | `EnergyImpact` | `RayderImpactoPoder` | `RoninEspadaImpacto` |
| Dash (l.264) | `Dash` | `RayderEsquiva` | `Dash` generico |
| Activar furia (l.283) | `Rage` | `RayderFuria` | `Rage` generico |
| Transformarse (Player.cpp:197) | `Rage` | `RayderFuria` | — |
| Salto | **nada** (`Sfx::Jump` nunca se llama) | nada | nada (`RoninSalto` no se usa) |
| Aterrizar (PlayerDraw l.125) | `Land` | `Land` | `Land` (`RoninAterrizaje` no se usa) |
| Pasos al caminar | nada | nada | nada |
| Derrota (Player.cpp:137) | `GameOver` = combat_ko | igual | igual |

Problemas que salen de esta tabla:
- **Rayden (skin 0), que es el protagonista, no tiene identidad sonora:** todo lo suyo es generico.
- Los personajes Kratos y RONIN-KRATOS (`data/kratos/*.txt`) traen hasta 78 lineas `clipsound` cada uno. `BetaCharacter.cpp` las lee, pero **el juego principal nunca las reproduce** (solo lo hace `BetaMode::PollSounds`), asi que en campana y VS suenan genericos. Ademas `IsRonin()` compara con el nombre exacto "RONIN", por lo que los RONIN-KRATOS caen en el caso generico.
- Para MAX, la habilidad por clip y la transformacion mezclan sonidos genericos con los suyos.
- El aterrizaje tras un knockdown usa el mismo `Land` que el aterrizaje de un salto.
- `PlayerDraw` solo reproduce el impacto en el **flanco** `hasHit && !prevHasHit`. Los KF con varios golpes por clip suenan una vez si `hasHit` no vuelve a false entre golpes; conviene verificarlo en partida.

### 2.2 Lado combate (`src/game/combat/CombatWorld.cpp`, solo el Stage 1)

| Linea | Evento | Sfx | Comentario |
|---|---|---|---|
| 102 | `SpawnProjectile`, de jugador **o de enemigo** | `EnergyShot` | El disparo enemigo suena igual que el del jugador. Ademas, en la EnergyWave del jugador suena **junto** al `EnergyCharge` del lanzamiento. |
| 267 | Golpe del jugador bloqueado por el enemigo | `Hit` | Se suma al impacto del personaje en PlayerDraw, que no sabe que el golpe fue bloqueado y suena como un golpe limpio. |
| 291 | Enemigo muere por golpe | `EnemyDeath` | — |
| 300 | Golpe del jugador conecta | `Hit` / `HeavyHit` | **Doble** junto con PlayerDraw. |
| 329 | Golpe en el jefe | `Hit` / `HeavyHit` | Tambien doble. |
| 356 | Enemigo pega al jugador | `Hit` si bloqueaba, `HeavyHit` si no | **Cualquier** golpe enemigo sin bloquear suena "pesado". |
| 383 | Proyectil enemigo pega al jugador | `EnergyImpact` | El mismo sonido que el poder del jugador. |
| 400 / 406 | Proyectil del jugador mata / pega | `EnemyDeath` + `EnergyImpact` | Con proyectiles que atraviesan, puede dispararse N veces en el mismo frame. |
| 453 | Onda de energia pega | `EnergyImpact` | Lo mismo: N veces por frame si hay varios enemigos. |
| 481 | Golpe a un objeto destructible | `Hit` | Tambien pone `player.hasHit`, asi que suena el impacto de personaje mas `Hit`. **Golpear un barril suena como golpear a una persona.** |
| 574 | Recoger un pickup | `Ui` | Usa `ui_navigate`. `energy_pickup.wav` existe y no se usa. |

### 2.3 Jugador recibiendo dano (`src/game/Player.cpp` 440-497)

- Bloqueo: `Block`, y luego CombatWorld anade `Hit`, asi que suenan **dos**. En la rotura de guardia suenan `Block` + `HeavyHit` + `Hit` (tres).
- Golpe durante una habilidad: `Hit`. Golpe fuerte (dano >= 28): `HeavyHit`, y en el Stage 1 CombatWorld anade otro `HeavyHit` (el mismo `Sound` se reinicia).
- **Golpe normal (dano < 28): no hay sonido en Player.** En el Stage 1 lo cubre CombatWorld con `HeavyHit`. **En los Stages 2-5 y en VS ese golpe es mudo.**
- Enemigo con la guardia rota (`StreetEnemy.cpp:262`): `HeavyHit`, el mismo sonido que cualquier golpe fuerte.

### 2.4 Stages, menus y otros

- `Stage1StoryGame.cpp` 232-277: menu de pausa con `Ui`, `UiCancel`, `UiConfirm` y `UiPause`. Es **el unico menu con sonido**.
- `Stage1Waves.cpp` 34/54: inicio de oleada y desbloqueo de la arena con `Ui`, el mismo "tic" de navegar.
- **Sin sonido alguno:** el menu principal, la seleccion de personaje, OPCIONES, la pantalla de recompensa entre stages, los menus del modo VS, los Stages 2-5 (ni UI ni combate de lado enemigo), el jefe (`Boss.cpp` no tiene audio; `BossAttack` y `BossPhase` existen pero no se usan), la victoria / stage superado (`StageClear` no se usa), los hitos de combo, los avisos de "telegraph" de ataque enemigo y los whooshes de ataque enemigo.
- `main.cpp:78`: `IntroArranque` suena una vez al arrancar.

### 2.5 Disparos dobles: resumen

1. Golpe del jugador que conecta (Stage 1): impacto de personaje + `Hit`/`HeavyHit` (+ `EnemyDeath`).
2. Golpe del jugador bloqueado: impacto de personaje (que suena como golpe limpio) + `Hit`.
3. Golpe a un destructible: impacto de personaje + `Hit`.
4. El jugador bloquea: `Block` + `Hit`. La rotura de guardia lleva 3 sonidos.
5. Knockdown del jugador: `HeavyHit` ×2.
6. EnergyWave: `EnergyCharge` (lanzar) + `EnergyShot` (spawn) en el mismo frame.
7. Proyectiles que atraviesan / onda contra varios enemigos: `EnergyImpact` y `EnemyDeath` N veces en el mismo frame, reiniciando el mismo `Sound`, lo que produce un "tartamudeo" y corta la cola.
8. BETA: `data/kratos/*.txt` repite cada `clipsound` (por ejemplo `punch1 4 sound73` ×2), asi que `PollSounds` lo dispara dos veces en el mismo frame. Es inofensivo por el reinicio, pero sobra.

---

## 3. Analisis de los WAV (`assets/audio/sfx/`)

Columnas: dur = duracion; pk = pico en dBFS; rms = RMS total; st50 = RMS maximo en una ventana de 50 ms (aprox. "volumen de golpe"); clip = muestras con |x| >= 0.999; lead = silencio inicial por debajo de -40 dB; cen = centroide espectral. Bandas: % de energia por debajo de 120 Hz, 120-500 Hz, 500 Hz-3 kHz y por encima de 3 kHz. Todos son 44100 Hz y 16 bits.

### 3.1 Starter pack (mono)

| Archivo | dur | pk | rms | st50 | clip | lead | cen | <120 / 120-500 / 0.5-3k / >3k |
|---|---|---|---|---|---|---|---|---|
| combat_block | 70 ms | -2.7 | -12.0 | -10.8 | 0 | 0.1 | 13.0k | 0 / 31 / 6 / 64 |
| combat_damage | 130 | -0.6 | -11.0 | -8.8 | 0 | 0 | 13.8k | 15 / 0 / 0 / 84 |
| combat_impact_heavy | 200 | **0.0** | -8.0 | -5.8 | **226** | 0.1 | 13.5k | 27 / 2 / 0 / 71 |
| combat_kick | 140 | 0.0 | -10.1 | -8.0 | 7 | 0.1 | 13.6k | 21 / 2 / 0 / 77 |
| combat_ko | 330 | 0.0 | -12.1 | -8.4 | 6 | 0.1 | 13.4k | 48 / 0 / 0 / 52 |
| combat_punch_heavy | 160 | 0.0 | -8.9 | -6.5 | **79** | 0 | 13.6k | 23 / 2 / 0 / 75 |
| combat_punch_light | 100 | -1.7 | -12.2 | -10.1 | 0 | 0.1 | 13.6k | 16 / 1 / 0 / 84 |
| energy_pickup | 170 | -9.1 | -18.0 | -15.9 | 0 | 0.3 | 683 | 0 / 24 / 76 / 0 |
| fury_charge | 550 | **-13.6** | -21.3 | -20.3 | 0 | 0.5 | 4.7k | 1 / 27 / 67 / 6 |
| fury_electric | 240 | -5.4 | -15.1 | -13.5 | 0 | 0.1 | 11.8k | 5 / 11 / 13 / 71 |
| fury_power_impact | 240 | 0.0 | -9.1 | -7.2 | **72** | 0 | 13.7k | 24 / 0 / 2 / 74 |
| fury_release | 220 | -0.8 | -12.3 | -9.8 | 0 | 0.1 | 12.8k | 12 / 4 / 6 / 78 |
| move_dash | 220 | -1.7 | -11.7 | -10.0 | 0 | 0 | 13.2k | 4 / 3 / 1 / 93 |
| move_jump | 160 | **-17.1** | -21.3 | -20.0 | 0 | 0.5 | 382 | 0 / 86 / 14 / 0 |
| move_land | 120 | -1.5 | -11.5 | -9.6 | 0 | 0 | 13.7k | 17 / 0 / 0 / 82 |
| move_whoosh | 180 | -3.1 | -13.3 | -11.6 | 0 | 0 | 13.4k | 2 / 3 / 1 / 95 |
| ui_cancel | 100 | -13.2 | -17.4 | -16.1 | 0 | 0.3 | 225 | 1 / 99 / 0 / 0 |
| ui_confirm | 130 | -7.1 | -16.1 | -14.2 | 0 | 0.2 | 685 | 0 / 1 / 99 / 0 |
| ui_navigate | 55 | -13.2 | -14.5 | -14.1 | 0 | 0.2 | 6.7k | 0 / 0 / 90 / 10 |
| ui_open (sin uso) | 115 | -9.1 | -17.8 | -16.0 | 0 | 0.3 | 448 | 0 / 31 / 69 / 0 |
| ui_pause | 120 | -8.9 | -17.4 | -15.3 | 0 | 0.3 | 381 | 0 / 100 / 0 / 0 |
| world_city_industrial (sin uso) | 2500 | -18.0 | -24.1 | -23.9 | 0 | 0.1 | 9.9k | 99 / 0 / 0 / 1 |
| world_metal_clang | 200 | -5.3 | -15.9 | -13.4 | 0 | 0 | 10.7k | 0 / 37 / 35 / 28 |
| world_rain_loop | 2000 | -13.2 | -32.2 | -28.5 | 0 | 4.3 | 11.0k | 1 / 2 / 12 / 86 |
| intro_arranque (estereo) | 3900 | -1.1 | -14.7 | -6.6 | 0 | **300** (intencional) | 3.0k | 33 / 56 / 10 / 2 |

### 3.2 Rayder / MAX (estereo, generados por `tools/audio/genera_sonidos_rayder.py`)

| Archivo | dur | pk | rms | st50 | lead | cen | <120 / 120-500 / 0.5-3k / >3k |
|---|---|---|---|---|---|---|---|
| golpe_1 | 131 | -3.3 | -11.4 | -7.6 | 0.4 | 1.9k | 1 / 97 / 2 / 0 |
| golpe_2 | 140 | -3.1 | -11.3 | -7.4 | 0.4 | 2.2k | 1 / 98 / 2 / 0 |
| golpe_3 | 147 | -3.3 | -11.5 | -7.4 | 0.5 | 2.4k | 2 / 96 / 1 / 0 |
| patada_1 | 163 | -3.3 | -10.5 | -6.6 | 0.4 | 1.8k | **88** / 12 / 0 / 0 |
| patada_2 | 170 | -3.3 | -11.5 | -7.0 | 0.5 | 1.9k | 44 / 55 / 1 / 0 |
| patada_3 | 183 | -3.5 | -10.8 | -6.7 | 0.4 | 2.5k | **90** / 8 / 1 / 1 |
| impacto_fuerte | 300 | -3.8 | -12.3 | -7.5 | 0.4 | 1.8k | **73** / 26 / 0 / 1 |
| impacto_poder | 400 | -4.2 | -15.3 | -9.0 | 0.5 | 4.6k | 7 / 81 / 7 / 5 |
| silbido | 200 | -3.1 | -13.8 | -10.3 | **7.2** | 4.1k | 0 / 0 / 66 / 34 |
| carga_poder | 850 | -3.7 | -16.0 | -13.1 | 1.4 | 4.6k | 0 / 57 / 28 / 16 |
| disparo_poder | 280 | -3.3 | -16.6 | -10.5 | 0.4 | 3.7k | 0 / 1 / 88 / 12 |
| furia | 950 | -4.1 | -14.3 | -8.5 | 0.5 | 1.7k | 17 / 81 / 1 / 0 |
| esquiva | 240 | -3.3 | -15.9 | -10.0 | 0.5 | 4.0k | 0 / 44 / 36 / 20 |

### 3.3 Ronin (estereo, `tools/audio/genera_sonidos_ronin.py`)

| Archivo | dur | pk | rms | st50 | lead | cen | <120 / 120-500 / 0.5-3k / >3k |
|---|---|---|---|---|---|---|---|
| golpe_1 | 88 | -3.0 | -10.9 | -8.5 | 0.4 | 3.8k | 0 / 98 / 1 / 1 |
| golpe_2 | 99 | -3.3 | -12.6 | -9.7 | 0.4 | 3.1k | 0 / 97 / 2 / 1 |
| golpe_3 | 105 | -3.0 | -12.3 | -9.2 | 0.4 | 3.1k | 0 / 99 / 1 / 1 |
| patada_1 | 139 | -3.3 | -10.5 | -7.1 | 0.5 | 2.7k | 44 / 54 / 1 / 1 |
| patada_2 | 215 | -3.3 | -12.0 | -6.8 | 0.4 | 2.5k | **68** / 23 / 7 / 2 |
| impacto_fuerte | 260 | -3.6 | -12.7 | -7.3 | 0.4 | 2.2k | **63** / 36 / 1 / 0 |
| espada_desenvaine | 300 | -3.6 | -15.7 | -9.6 | 0.4 | 5.5k | 0 / 0 / 15 / 85 |
| espada_tajo | 320 | -3.4 | -14.4 | -10.8 | 1.6 | 4.9k | 0 / 1 / 52 / 47 |
| espada_impacto | 340 | -3.9 | -16.0 | -9.7 | 0.5 | 5.3k | 8 / 75 / 7 / 11 |
| salto (sin uso) | 110 | -3.1 | -13.8 | -10.6 | 0.5 | 2.6k | 0 / 78 / 14 / 7 |
| aterrizaje (sin uso) | 160 | -3.4 | -10.9 | -6.9 | 0.4 | 1.4k | **97** / 2 / 1 / 0 |

### 3.4 Conclusiones del analisis

- **Silencio inicial:** ningun SFX de combate tiene un retraso que se note (todos por debajo de 2 ms, y silbido 7 ms, aceptable). El unico retraso grande es intencional: los 300 ms del intro. En BETA, varios .ogg de Kratos tienen **40-100 ms** de silencio al inicio (sound34 104 ms, sound61/62 ~50 ms, sound64 62 ms), lo que retrasa el golpe 3-6 frames.
- **Clipping:** en el starter pack recortan `combat_impact_heavy` (226 muestras), `combat_punch_heavy` (79), `fury_power_impact` (72), `combat_kick` (7) y `combat_ko` (6). Los paquetes Rayder y Ronin estan limpios y normalizados a -3 dBFS. Los .ogg de BETA/Kratos estan masivamente recortados: sound75 36 022 muestras, sound63 10 073, sound37 7 183, sound64 6 360, sound26 3 065.
- **Diferencias de nivel:** entre el sonido mas alto y el mas bajo hay unos 14 dB de "st50" (impact_heavy -5.8 frente a fury_charge -20.3). `move_jump` (-20) y `fury_charge` (-20) quedan **enterrados** frente a los golpes. Encima, en la tabla `files[]`, `EnergyCharge` lleva 0.7 (-3 dB), lo que lo hunde aun mas. Los paquetes Rayder y Ronin estan unos 2-3 dB por encima del starter pack en golpes ligeros (st50 -7.5 frente a -10). Al cambiar de personaje cambia el volumen percibido.
- **Similitud:** el "starter pack" tiene un espectro casi identico entre sonidos (similitud de espectro de 0.75-0.83 entre `punch_light`/`damage`, `impact_heavy`/`power_impact`, `dash`/`whoosh`, `electric`/`release`). Por eso **todo suena a lo mismo**.
- **DC offset** de hasta 0.0074 (`move_dash`). Es despreciable, pero conviene quitarlo con un filtro paso alto de 20 Hz si se regeneran.

---

## 4. Musica

- **Juego principal: no hay musica.** Solo lluvia (2 s en bucle) e `intro_arranque.wav`. Segun `docs/INTRO_ARRANQUE.md`, el intro es una composicion original sintetizada (`tools/audio/genera_intro_ronin.py`) inspirada en la energia de un jingle, sin transcribir Dragon Ball. **Cumple la regla del usuario**, pero esta marcado como TEMPORAL.
- `assets/audio/sfx/world_city_industrial.wav` (2.5 s, 99 % por debajo de 120 Hz) existe y no se usa.
- **BETA / GowApk:** `BetaMode::PlayMusic` usa `menu.ogg` (35 s), `gate1music.ogg` (28 s), `gate2music.ogg` (30 s), `gameCG.ogg` (17 s), `music_001.ogg` (35 s), `music_002.ogg` (8 s, victoria) y `music_003.ogg` (7 s, derrota), a volumen fijo 0.55. Los stages de `GowApk/data/stages/*.txt` eligen entre gate1 y gate2. **Es musica del juego original de donde salen los sprites de `nuevosSprites`** (`GowApk/docs/AUDITORIA_BETA.md`: "musica y efectos .ogg ... INTEGRADO"). **Va contra la regla de "nada de musica con copyright"** y hay que sustituirla.
- `apk_reference/king_fighter_iii/bin/music/` (bg0-3.mid y wavs de KOF III) tambien es material con copyright. El codigo no lo usa para audio (solo lee `animation.bin`), pero **no deberia ir dentro del APK**. La carpeta `build-android/.../assets/apk_reference` del snapshot solo contiene `animation.bin`, que es lo correcto.

---

## 5. Modo BETA / GowApk — audio

- `GowApk/src/BetaMode.cpp`: `PlaySound(name)` carga `GowApk/assets/audio/<name>.ogg` en una cache y reproduce sin pitch ni volumen. `PollSounds` reproduce las lineas `clipsound <clip> <frame> <sound>` de cada personaje al cambiar de cuadro, y `sound hurt/die/dash` al perder vida o morir.
- **Stubs mudos:** 17 archivos `.ogg` identicos (md5 `7ddad058...`, 2381 bytes, 23 ms, 8 kHz): `sound8, 22, 23, 24, 28, 30, 41, 44, 45, 49, 50, 53, 54, 57, 58, 71, 72`. El codigo usa `sound24` (inicio de stage / confirmar, ×3), `sound44` (×2) y `sound45`, y **los tres son mudos**.
- **Ruta equivocada:** los sonidos que piden los personajes Kratos / RONIN-KRATOS (`data/kratos/*.txt`: `sound34, 36, 37, 38, 61-65, 70, 73-75`, y `sound3`/`sound4` para dash) **solo estan en `assets/kratos/audio/`**, y `BetaMode.cpp:521` busca en `GowApk/assets/audio/`. Si el heroe de BETA es Kratos, sus golpes, su dash, su dolor y su muerte **no suenan**. `BetaMode.cpp:1042 PlaySound("sound61")` tampoco suena. Ningun sitio de `src/` ni de `GowApk/src` referencia `assets/kratos/audio`: son archivos huerfanos.
- **Android:** el snapshot `build-android/district/stage/assets/` incluye `assets/kratos/audio` pero **no `GowApk/assets/audio`**. Si el empaquetado actual es igual, BETA en Android no tiene ni musica ni SFX. Pendiente de confirmar en el script de empaquetado.
- `clipsound` aparece duplicado en cada linea de `data/kratos/*.txt` y en algunas habilidades se dispara hasta 8 veces el mismo sonido (`ab1` con `sound64` en 7 cuadros).

---

## 6. Problemas de mezcla (motor)

| Problema | Efecto | Donde |
|---|---|---|
| Una sola instancia por Sfx | Si suena el mismo sonido dos veces seguidas (combo rapido, varios enemigos), el segundo **corta** al primero. No hay superposicion natural. | `AudioSystem::Play` |
| Sin pitch/volumen aleatorio | 3 golpes iguales seguidos suenan clonados. "Fatiga de oido" en Rayden y los genericos, que solo tienen 1 WAV por tipo. | `Play` |
| Sin limite ni prioridad | En una oleada grande pueden sonar a la vez EnergyImpact ×N + EnemyDeath ×N + Hit, todo al mismo nivel, y suma clipping en el mixer de miniaudio. | — |
| Sin buses | No se puede bajar la musica/ambiente frente a los SFX, ni hacer ducking en golpes pesados o en la furia. | — |
| Sin cooldown por Sfx | Varios disparos en el mismo frame (proyectiles que atraviesan, onda de energia). | `CombatWorld` 400-453 |
| Sin pan | Todo centrado. En un beat'em up horizontal, el pan segun X da lectura espacial gratis. | — |
| Ambiente unico | La lluvia suena en menus, en VS y en todos los stages. | `UpdateAmbient` |
| Silencio solo global | OPCIONES solo tiene mute, sin volumen de musica/SFX. | `SetMuted` |

---

## 7. PROPUESTA DE "SUPER MEJORA" (priorizada)

Principio de reparto para no chocar con quien edita el combate: **casi todo va dentro de `src/audio/`, que es territorio exclusivo del implementador de audio**. En el combate solo se cambian **lineas sueltas `Play(...)`** por llamadas a eventos, sin tocar la logica. Los cambios en archivos de combate se agrupan en un unico commit pequeno al final (P2), coordinado con quien edite esos archivos.

### P0 — Motor de mezcla (solo `src/audio/AudioSystem.h/.cpp`, cero conflictos)

1. **Pool de voces por Sfx con `LoadSoundAlias`** (raylib 5.5): N alias por sonido (4 para golpes e impactos, 2 para UI, 1 para jingles). `Play` busca un alias libre (`!IsSoundPlaying`). Si no hay ninguno, roba el mas antiguo. Ya no se cortan las colas.
2. **Limitador global de voces:** como mucho unas 12 SFX simultaneas. Cada Sfx lleva una **prioridad** (0 UI/ambiente, 1 movimiento, 2 golpe ligero, 3 golpe pesado / dano al jugador, 4 furia / KO / jefe). Si el limite esta lleno, se descarta el nuevo de menor prioridad o se roba la voz de menor prioridad.
3. **Antirrafaga:** si el mismo Sfx ya se disparo hace menos de 30 ms, no se vuelve a disparar. Resuelve el N-por-frame de los proyectiles y la onda, y el `Block`+`Hit`.
4. **Variacion por reproduccion:** cada Sfx tiene `pitchJitter` (por ejemplo ±6 % en golpes, ±3 % en impactos pesados, 0 en UI y jingles) y `volJitter` (±1.5 dB). Se aplica con `SetSoundPitch` / `SetSoundVolume` sobre el alias antes de `PlaySound`.
5. **Variantes:** `Play(Sfx)` elige al azar entre las variantes cargadas del grupo, sin repetir la ultima (round-robin aleatorio). Basta con que la tabla `files[]` acepte una lista de archivos por Sfx (`combat/punch_light_1..4`).
6. **Buses y volumen por categoria:** se multiplica `vol(tabla) * bus[categoria] * master`, con categorias SFX, UI, Voz/Furia, Musica y Ambiente. Se exponen `SetBusVolume(Bus, float)` para OPCIONES.
7. **Ducking:** `Duck(dB, ms)`. Un golpe pesado, un KO, la furia o un golpe al jugador bajan la musica y el ambiente 6-9 dB durante 250-400 ms, con recuperacion exponencial. Se aplica en `UpdateAmbient` (que pasa a llamarse `Update(dt)` sin cambiar la firma de main, o se mantiene el nombre) sobre `SetMusicVolume`.
8. **Pan opcional:** `Play(Sfx, float worldX)` hace `pan = 0.5 + clamp((x - camX - 640) / 1280, -0.35, 0.35)`. Es una sobrecarga, asi que las llamadas actuales siguen compilando.
9. **Limitador suave en el bus maestro:** `AttachAudioMixedProcessor` con un soft clip `tanh(x * 1.2) / tanh(1.2)`, o un limitador de pico con release de 50 ms. Evita el clipping cuando suenan muchas voces a la vez. Debe ser barato porque corre en el hilo de audio de Android.
10. **Normalizar en la carga:** se calcula el RMS de cada WAV en `Init` y se ajusta la ganancia a un objetivo por categoria (golpes -10 dBFS st50, UI -16, ambiente -28) en vez de los volumenes a mano de `files[]`. Corrige los 14 dB de diferencia sin regenerar archivos.
11. **Cargar `fury_charge` y `combat_ko` una sola vez** (alias en lugar de duplicados).

### P1 — Banco de sonidos nuevo por sintesis (`tools/audio/genera_banco_combate.py` nuevo + `assets/audio/sfx/combat/`, `assets/audio/sfx/arquetipos/*`)

Se sigue el mismo estilo que `genera_sonidos_rayder.py`: semilla fija, numpy, salida WAV de 16 bits a 44.1 kHz, **mono** (el pan lo hace el motor; ocupa la mitad), pico a -3 dBFS, paso alto a 20 Hz y fundido de 2 ms al final. **Regla para movil:** cada golpe debe tener energia real entre **300 Hz y 4 kHz** (cuerpo + chasquido). El subgrave es un extra, no la base.

Recetas (capas → envolvente → mezcla):

| Tipo | Capas | Envolvente / sweep | Duracion |
|---|---|---|---|
| **Ligero** (jab), 4 variantes | (a) chasquido: ruido blanco con paso banda 2-6 kHz, 4 ms; (b) cuerpo: seno + triangulo 420→180 Hz; (c) "carne": ruido rosa con paso banda 300-1200 Hz y decaimiento de 40 ms; (d) un poco de distorsion `tanh(2.5x)` | ataque de 0.5 ms; cuerpo exponencial con tau 35 ms; pitch exponencial en 30 ms | 90-120 ms |
| **Medio** (cross / gancho), 4 variantes | lo mismo + (e) golpe grave: seno 160→70 Hz con tau 60 ms; chasquido algo mas ancho, 1.5-5 kHz | sweep en 45 ms; carne con tau 60 ms | 130-170 ms |
| **Pesado** (remate), 3 variantes | chasquido ancho 0.8-8 kHz + cuerpo 260→60 Hz + subgrave 90→40 Hz + "crunch" (ruido granulado a 30-60 granos/s con paso banda 1-3 kHz) + cola corta de reverb (convolucion con ruido de 120 ms) + saturacion | sweep en 80 ms; cola de 150 ms | 250-320 ms |
| **Patada**, 3 variantes | pre-whoosh de 40 ms (ruido con paso banda barriendo 800→2500 Hz) + impacto "medio" con el cuerpo 1/3 mas grave + "tela" (ruido de alta frecuencia con paso banda 3-7 kHz y tau 20 ms) | pre-whoosh que crece y luego impacto | 160-200 ms |
| **Corte** (cuchillas, espada, garras), 3 variantes | swish (ruido con paso banda barriendo 6→2 kHz, Q alto) + ring metalico (3-4 parciales inarmonicas, 1.9k/3.1k/4.7k/6.2k, tau 80-150 ms) + carne 300-900 Hz | swish en 60 ms, ring con decaimiento | 200-300 ms |
| **Energia** (rayo, fuego), 3 variantes | zap: sierra 1200→200 Hz con FM rapida (modulador de 60-90 Hz con indice alto) + crackle (impulsos aleatorios de Poisson filtrados 2-9 kHz) + golpe grave 150→50 Hz | zap en 120 ms, crackle de 200 ms | 250-400 ms |
| **Bloqueo**, 3 variantes | "tok" de madera/cuero: seno 900 Hz + 1.4 kHz con tau 15 ms + ruido con paso banda 1-3 kHz de 10 ms + un ring corto de 2.2 kHz | muy corto, sin subgrave | 70-90 ms |
| **Rotura de guardia** | 3 "tok" de bloqueo en cascada (0, 25, 55 ms) que bajan de pitch + golpe pesado + vidrio (ruido con paso alto a 5 kHz y granulos) | — | 400 ms |
| **Dano al jugador** (distinto del dano al enemigo) | cuerpo medio + un "thud" grave mas sordo (paso bajo a 2 kHz) para que el jugador distinga **recibir** de **dar** | — | 150 ms |
| **Whoosh por peso**, 3 tamanos | ruido rosa con paso banda cuyo centro hace la curva 600→2400→900 Hz; ligero 120 ms, pesado 250 ms con Q bajo | envolvente en campana | 120-250 ms |
| **Salto** | whoosh corto ascendente 500→1500 Hz + ropa (ruido con paso banda 2-5 kHz) | 100 ms | — |
| **Aterrizaje ligero / tras knockdown** | ligero: "tap" 200-800 Hz + polvo (ruido con paso alto 3 kHz y tau 60 ms). Knockdown: cuerpo 180→60 Hz + rebote (repeticion a -8 dB a los 90 ms) + polvo | — | 120 / 300 ms |
| **Pasos** (opcional), 4 variantes | tap con paso banda 150-600 Hz + roce 2-4 kHz, a -14 dB y en charco cuando llueve (ruido con paso banda 1-3 kHz con un barrido rapido) | se disparan en `walkPhase` | 60 ms |
| **KO enemigo** | golpe pesado + "cuerpo al suelo" con retraso de 180 ms (aterrizaje de knockdown) + un swell de 2 notas a -10 dB | — | 600 ms |
| **Game over** (distinto del KO) | acorde menor descendente con sierras desafinadas filtradas 220→110 Hz y una cola de 1.5 s | — | 2 s |
| **Stage clear / victoria** | arpegio mayor rapido (1-3-5-8) con pulso + campana FM y cola | — | 1.5 s |
| **Telegraph de enemigo** | "shing" de tension: seno de 2.5 kHz con vibrato rapido y fade-in de 150 ms, a -12 dB. Avisa del ataque enemigo y ayuda mucho al gameplay | — | 200 ms |
| **Jefe: golpe y cambio de fase** | golpe: pesado con el pitch al 70 % + sub. Fase: "rugido" (ruido marron con paso banda 100-600 Hz y AM a 30 Hz) + impacto + ring metalico (reutiliza `world_metal_clang` a pitch 0.6) | — | 0.6 / 1.2 s |
| **Pickup** | ya existe `energy_pickup.wav`: hay que usarlo; vida = dos notas ascendentes; energia = brillo FM | — | — |
| **Hitos de combo** (10/20/30) | ping FM que sube un semitono por hito | — | 200 ms |
| **UI** | `ui_open` existe y no se usa. Hace falta un "select de personaje" (golpe + metal) y una "denegacion" (dos tonos bajos) | — | — |

**Mezclar a partir de lo existente (sin regenerar):**
- Golpe ligero generico = `combat_punch_light` + `rayder/golpe_N`, cada uno a -3 dB. El primero aporta los agudos y el segundo el medio-grave, asi que juntos rellenan el espectro. Se puede hacer offline en el script o en el motor con capas (`Play` de 2 Sfx con un desfase de 0 ms).
- Arreglar los graves de Rayder y Ronin para el movil: una pasada offline con realce de +4 dB en 1.5-3 kHz, un chasquido sintetico de 3 ms anadido al inicio y saturacion armonica (`tanh`) para que el sub genere armonicos audibles (efecto "bass enhancer"). **Si se regeneran los WAV de Rayder/Ronin con sus scripts, hay que tocar `tools/audio/genera_sonidos_*.py`** (anadir capa "crack 2-5 kHz" y saturacion). No hace falta tocar el C++.
- Quitar el clipping del starter pack: `combat_impact_heavy`, `combat_punch_heavy` y `fury_power_impact` re-exportados a -1 dBFS (o sustituidos por el banco nuevo).

**Bancos por arquetipo** (asignacion por personaje en una tabla, sin tocar el combate):

| Arquetipo | Personajes | Impactos | Lanzamiento | Especial |
|---|---|---|---|---|
| Puños (brawler) | Rayden (0), KF MATON, KF PELEADOR, KF GORRA, BRAKK | ligero/medio/pesado genericos nuevos | whoosh por peso | energia |
| Agil (patadas) | KF RUBIA, KF BUFONA, KF HEROINA TRANSFORMADA | ligero + patada con el pitch al 112 % | whoosh ligero | energia brillante |
| Cuchillas | KF NAVAJERA, KF CUCHILLERO, KF JEFE GARRA | corte | swish | corte + energia |
| Pesado | GRINDER, KF SOLDADO, bestias, brutos | pesado con el pitch al 85 % | whoosh pesado | pesado + sub |
| MAX | ya tiene pack propio (+ arreglo de medios) | rayder/* | silbido | rayder/* |
| RONIN | ya tiene pack propio (+ usar `salto`/`aterrizaje`, y dash con `espada_desenvaine` a pitch 1.3 o un whoosh nuevo) | ronin/* | whoosh | espada |
| Kratos / RONIN-KRATOS | por arma: espadas = corte, cestus = pesado, rayo = energia, garras = corte oscuro | segun el arma | swish | segun el arma |

Implementacion: nuevo `src/audio/SfxBank.h/.cpp` con `enum class HitKind {Light, Medium, Heavy, Kick, Slash, Energy, Block, GuardBreak}` y `enum class SoundArchetype`, mas `SoundArchetype ArchetypeFor(skin)` en una tabla basada en el nombre de `CharacterVisual`, como ya hacen `IsRayder` e `IsRonin`. **El archetipo se resuelve dentro de `src/audio/`; el combate no necesita saberlo.**

### P2 — Conectar eventos (cambios de una linea en los archivos de combate, en un solo commit coordinado)

API nueva en `AudioSystem`: `PlayHit(HitKind, int skin, float x)`, `PlaySwing(weight, skin)`, `PlayPlayerHurt(bool heavy)`, `PlayBlock()`, `PlayGuardBreak()`, `PlayKO(float x)`, `PlayUi(UiSfx)`, `PlayJingle(Jingle)`. **Regla: un unico dueno por evento.**

| Archivo | Cambio | Riesgo de conflicto |
|---|---|---|
| `src/game/player/PlayerDraw.cpp` 87-110, 125 | Sustituir el arbol de `if IsRayder/IsRonin` por `PlayHit(kind, skin, x)`. Es el **unico** dueno del impacto del jugador. Aterrizaje: diferenciar un salto de un knockdown. | Medio (bloque compacto de unas 20 lineas) |
| `src/game/combat/CombatWorld.cpp` 267, 300, 329 | **Quitar** el `Play(Hit/HeavyHit)` del impacto del jugador (lo cubre PlayerDraw). En 267 (bloqueado), en vez de quitarlo, cambiarlo a `PlayBlock()` y avisar a PlayerDraw (por ejemplo `player.lastHitGuarded`) para que no reproduzca un golpe limpio. | Bajo (lineas sueltas) |
| `src/game/combat/CombatWorld.cpp` 356 | Quitar, porque el dueno pasa a ser `Player::TakeDamage`. | Bajo |
| `src/game/combat/CombatWorld.cpp` 102 | Distinguir el proyectil enemigo (`fromEnemy`) del propio. | Bajo |
| `src/game/combat/CombatWorld.cpp` 400-453 | Lo cubre el antirrafaga del motor, sin cambios; opcionalmente pasar `x` para el pan. | Nulo |
| `src/game/combat/CombatWorld.cpp` 481 | Destructible: `PlayHit(HitKind::Prop)`, que suena a madera/metal (reutiliza `world_metal_clang`). | Bajo |
| `src/game/combat/CombatWorld.cpp` 574 | Pickup: `energy_pickup`. | Bajo |
| `src/game/Player.cpp` 450-497 | `TakeDamage` pasa a ser el dueno unico del dano al jugador: bloqueo, rotura de guardia, dano normal (**anadir el que falta** en la rama < 28) y knockdown. | Medio (archivo de combate del jugador) |
| `src/game/Player.cpp` 137 | GameOver: jingle nuevo, no `combat_ko`. | Bajo |
| `src/game/player/PlayerCombat.cpp` 97, 188-211, 264, 283 | `PlaySwing(weight, skin)` y `PlaySpecial(id, skin)`. Ronin pasa a tener dash y furia propios. Se anade el salto donde se entra en `Airborne` por salto. | Medio |
| `src/game/Stage2Game.cpp`…`Stage5Game.cpp`, `src/game/VSMode.cpp` | Anadir `PlayKO` al matar, golpe al jefe, UI y musica por stage. **Hoy no tienen audio.** El dano al jugador ya queda cubierto por `TakeDamage`. | Medio (cada stage tiene su resolver) |
| `src/game/StreetEnemy.cpp` 262 | `PlayGuardBreak()`. Opcional: telegraph cuando `telegraphing` pasa a true. | Bajo |
| `src/game/combat/Boss.cpp` | Cambio de fase con `BossPhase` y ataque con `BossAttack` (ya existen en el enum). | Bajo |
| Menus (`main.cpp` y donde esten el menu principal, la seleccion, OPCIONES y la recompensa) | `PlayUi(Nav/Confirm/Cancel/Open)`. Sliders de volumen por bus en OPCIONES. | Bajo |

### P3 — Musica original sintetizada por modo (viable, 100 % propia)

- Generador nuevo `tools/audio/genera_musica.py` (numpy y semilla fija, igual que el intro). **Se pre-renderiza a OGG** con `ffmpeg -c:a libvorbis -q:a 3`; ffmpeg con libvorbis esta disponible en el entorno. raylib decodifica OGG con stb_vorbis. Un minuto en estereo a q3 ocupa unos 0.7 MB. Se descarta la sintesis en tiempo real por el coste de CPU en Android.
- Formato: **bucles de 32-64 compases** con el punto de union en el limite de compas, y mismo tempo y tonalidad en las 2 capas para hacer **musica adaptativa** (capa "calma" y capa "combate", con crossfade cuando la arena se bloquea).
- Piezas sugeridas (composiciones nuevas, sin citar obras existentes):
  - **Menu**: synthwave/darksynth en La menor a 96 BPM, pad de sierras desafinadas con paso bajo lento, arpegio de 16avos y bombo suave. La lluvia queda por debajo.
  - **Stage 1 (calles con lluvia)**: 120 BPM, bajo de sierra en semicorcheas, caja con reverb, lead de onda cuadrada con portamento. Capa de combate: hi-hats + bajo saturado.
  - **Stages 2-5**: misma paleta con otra tonalidad y otra textura: industrial (usar `world_city_industrial` como drone), puerto, azotea y guarida. Variar el tempo entre 110 y 140.
  - **Jefe**: 140 BPM, percusion tipo taiko (reutilizar la receta del intro), ostinato de bajo cromatico y stab de "bronces" de sierra. La capa 2 entra al cambiar de fase.
  - **VS**: tema corto y energico, 150 BPM.
  - **Victoria / derrota**: stingers de 3-5 s (como `StageClear` / `GameOver`).
  - **BETA**: reemplazo 1:1 de `menu.ogg`, `gate1music.ogg`, `gate2music.ogg`, `gameCG.ogg` y `music_001-003.ogg` por piezas propias de estilo "epico griego": modo frigio, drones de quinta, percusion tribal, coro sintetizado (formantes "aah" con 3 filtros de paso banda sobre una sierra). **Esto elimina el problema de copyright** sin tocar el codigo de BetaMode (mismos nombres de archivo). Lo mismo para los SFX `soundNN.ogg` de BETA: hay que regenerarlos con el banco P1 y los mismos nombres.
- Motor: `src/audio/MusicPlayer.h/.cpp` nuevo (o dentro de AudioSystem) con `PlayMusic(MusicId)`, crossfade de 1 s, bus de musica, ducking (P0.7) y ambiente por stage (la lluvia solo en el Stage 1 y en el menu). El modo BETA pasaria a usar este reproductor en vez del suyo (`BetaMode::PlayMusic`).

### P4 — Arreglos de BETA (solo `GowApk/src/BetaMode.cpp` + datos)

1. `BetaMode::PlaySound`: probar primero `GowApk/assets/audio/` y luego `assets/kratos/audio/`. **Arregla los sonidos de Kratos en BETA.** Es una sola linea en `FindAsset` o un segundo intento.
2. Sustituir los 17 stubs mudos (sobre todo `sound24`, `44` y `45`, que se usan) por sonidos del banco P1.
3. Quitar los `clipsound` duplicados de `data/kratos/*.txt`. Hay que hacerlo en el script que los genera (`tools/ronin_kratos/rk_build.py` y el builder de Kratos), no a mano.
4. Recortar los 40-100 ms de silencio inicial de `sound34`, `61`, `62`, `64` y `65`, y bajar el clipping (normalizar a -1 dBFS). O, mejor, sustituirlos todos (prioridad de copyright).
5. Respetar los buses y el mute del motor (BetaMode ya mira `IsMuted`; anadir el volumen de bus).
6. Verificar que `GowApk/assets/audio` (o su reemplazo) entra en el APK de Android.

### Orden recomendado para el implementador

1. **P0** completo (solo `src/audio/`). Mejora inmediata de variacion, superposicion y nivel sin tocar el juego.
2. **P1**: generar el banco + arreglo de medios de Rayder/Ronin (solo `tools/audio/` y `assets/audio/`).
3. **P2**: un commit pequeno de "dueno unico por evento" coordinado con quien edite el combate: PlayerDraw, CombatWorld, Player, PlayerCombat, y luego Stage2-5/VS/Boss en otro commit.
4. **P3**: musica (tools + assets + `src/audio/MusicPlayer.*` + una llamada por modo/stage).
5. **P4**: BETA. El reemplazo de musica y SFX con copyright es tan urgente como P0 por la regla del usuario.

Archivos que NO deberia tocar el implementador de audio salvo en P2: `src/game/combat/*`, `src/game/player/*`, `src/game/Player.cpp`, `src/game/Stage*Game.cpp`, `src/game/VSMode.cpp`. Fuera de esa fase, todo cabe en `src/audio/`, `tools/audio/`, `assets/audio/`, `GowApk/assets/audio/` y `GowApk/src/BetaMode.cpp`.
