# Sonidos de RAYDER

Paquete de 13 efectos de sonido nuevos para el personaje **Rayder**, en
`assets/audio/sfx/rayder/`.

Todos son **WAV estéreo, 44100 Hz, 16 bits**, igual de cortos y con el mismo
volumen percibido que los sonidos que ya tenía el juego.

Se generan por código (no son grabaciones ni audio de ninguna IA) con:

```
python3 -I tools/audio/genera_sonidos_rayder.py assets/audio/sfx/rayder
```

El generador usa una **semilla fija** (`SEMILLA = 20261021`), así que
ejecutarlo otra vez produce exactamente los mismos archivos. Para otra tanda
distinta basta cambiar la semilla o los parámetros de cada receta.

---

## 1. Tabla de sonidos

Las tres capas de cada sonido son siempre las mismas tres ideas:

- **Transitorio**: el chasquido inicial (ruido muy corto, filtrado en agudos).
  Es lo que hace que suene a impacto y no a zumbido.
- **Cuerpo**: la parte con tono, con la frecuencia cayendo de agudo a grave.
  Esa caída es lo que el oído lee como "peso" o "fuerza".
- **Cola**: una reverberación corta (convolución con ruido que decae). Da el
  aire alrededor del golpe. Es distinta en el canal izquierdo y en el derecho,
  y eso es lo que hace que el sonido se sienta ancho.

| Archivo | Cuándo suena en el juego | Cómo está hecho (capas) | Duración |
|---|---|---|---|
| `golpe_1.wav` | Primer puñetazo del combo | Transitorio 1.8–9.5 kHz + cuerpo 205→86 Hz + capa de "tela" 350–2400 Hz + cola 0.10 s | 0.131 s |
| `golpe_2.wav` | Segundo puñetazo del combo | Igual, cuerpo más agudo (230→97 Hz), cola 0.10 s más brillante | 0.140 s |
| `golpe_3.wav` | Tercer puñetazo del combo | Igual, cuerpo más grave (185→78 Hz), cola 0.11 s más oscura | 0.147 s |
| `patada_1.wav` | Patada (variante 1) | Silbido de aire al arranque + transitorio 0.9–6.5 kHz + cuerpo triangular 128→49 Hz + cola 0.12 s | 0.163 s |
| `patada_2.wav` | Patada (variante 2) | Igual, cuerpo 142→54 Hz | 0.170 s |
| `patada_3.wav` | Patada (variante 3) | Igual, cuerpo 116→44 Hz, cola algo más larga | 0.183 s |
| `impacto_fuerte.wav` | Último golpe del combo / remate | Transitorio ancho 0.7–8 kHz + cuerpo 150→48 Hz + subgrave 80→34 Hz + crujido 1.2–5 kHz + cola 0.20 s | 0.300 s |
| `silbido.wav` | El brazo cortando el aire, **sin impacto** | Sin transitorio: solo ruido con el filtro barriendo 500→3200→700 Hz, envolvente en campana + cola 0.09 s | 0.200 s |
| `carga_poder.wav` | Rayder acumula energía antes de lanzar | Tono que **sube** 120→780 Hz con armónicos + trémolo que se acelera (7→42 Hz) + chispas 2.5–11 kHz que van entrando + cola 0.16 s | 0.850 s |
| `disparo_poder.wav` | Lanzar el proyectil | Chasquido de salida 1.5–11 kHz + cuerpo sierra 900→160 Hz filtrado + estela de ruido 1.2→5.2 kHz + cola 0.15 s | 0.280 s |
| `impacto_poder.wav` | El proyectil pega en el enemigo | Estallido 0.4–9 kHz + cuerpo 190→45 Hz + capa eléctrica (modulación en anillo 620×173 Hz) + cola 0.24 s | 0.400 s |
| `furia.wav` | Activar la FURIA | Impacto de arranque + acorde grave ascendente (70→185 Hz con quinta y octava) + rugido de ruido que se abre 150→1800 Hz + cola 0.26 s | 0.950 s |
| `esquiva.wav` | Desplazamiento rápido / dash | Barrido de aire descendente 2800→600 Hz + empuje tonal 320→70 Hz + roce 3–9 kHz + cola 0.11 s | 0.240 s |

Las tres variantes de golpe y de patada existen para que **encadenar un combo
no suene tres veces idéntico**. Lo correcto es rotarlas (0, 1, 2, 0, 1, 2…) o
elegirlas al azar.

---

## 2. Comprobación de los archivos

Después de generarlos se volvió a abrir cada `.wav` y se midió. Resultado:
**los 13 archivos correctos, ningún problema**.

Qué se comprobó en cada uno: que sea estéreo, 44100 Hz y 16 bits; que no esté
en silencio; que no sature (pico y muestras recortadas); que dure lo esperado;
y que los dos canales no sean idénticos (si lo fueran, no habría anchura).

| Archivo | Canales | Frec. | Bits | Duración | Pico | RMS | Correlación L/R | Estado |
|---|---|---|---|---|---|---|---|---|
| `golpe_1` | 2 | 44100 | 16 | 0.131 s | −3.00 dB | −11.26 dB | 0.933 | OK |
| `golpe_2` | 2 | 44100 | 16 | 0.140 s | −3.00 dB | −11.05 dB | 0.899 | OK |
| `golpe_3` | 2 | 44100 | 16 | 0.147 s | −3.00 dB | −11.34 dB | 0.938 | OK |
| `patada_1` | 2 | 44100 | 16 | 0.163 s | −3.00 dB | −10.46 dB | 0.971 | OK |
| `patada_2` | 2 | 44100 | 16 | 0.170 s | −3.00 dB | −11.38 dB | 0.954 | OK |
| `patada_3` | 2 | 44100 | 16 | 0.183 s | −3.00 dB | −10.76 dB | 0.974 | OK |
| `impacto_fuerte` | 2 | 44100 | 16 | 0.300 s | −3.00 dB | −12.11 dB | 0.934 | OK |
| `silbido` | 2 | 44100 | 16 | 0.200 s | −3.00 dB | −12.93 dB | 0.622 | OK |
| `carga_poder` | 2 | 44100 | 16 | 0.850 s | −3.00 dB | −13.97 dB | 0.263 | OK |
| `disparo_poder` | 2 | 44100 | 16 | 0.280 s | −3.00 dB | −15.53 dB | 0.555 | OK |
| `impacto_poder` | 2 | 44100 | 16 | 0.400 s | −3.00 dB | −14.78 dB | 0.789 | OK |
| `furia` | 2 | 44100 | 16 | 0.950 s | −3.00 dB | −13.94 dB | 0.844 | OK |
| `esquiva` | 2 | 44100 | 16 | 0.240 s | −3.00 dB | −15.04 dB | 0.649 | OK |

Notas de la comprobación:

- **Margen**: todos tienen el pico exactamente en **−3.00 dBFS**. Ninguna
  muestra llega al máximo, o sea que no hay recorte digital.
- **Volumen**: los sonidos de combate ya existentes del juego andan entre
  −8 y −13 dB RMS. Los nuevos quedan entre −10 y −15 dB RMS, así que pegan con
  el resto. En la primera tanda salieron unos 5 dB más bajos (los golpes
  sintetizados tienen un pico altísimo y muy corto); se corrigió metiendo una
  saturación suave (`tanh`) antes de normalizar, que es el parámetro `empuje`
  de cada receta en el generador.
- **Duración**: todas entre 0.13 s y 0.95 s, dentro de lo pedido, y ninguna es
  más larga que su equivalente antiguo.
- **Compatibilidad con mono**: se sumaron los dos canales para ver si se
  cancelaban. La pérdida máxima es de −2.0 dB (en `carga_poder`), y en los
  golpes es menor de −0.25 dB. O sea que en un altavoz mono (móvil) siguen
  sonando bien, no se anulan.
- **Componente continua (DC)**: por debajo de 0.005 en todos. Sin problema.

La comprobación se hizo abriendo los `.wav` con el módulo `wave` de Python y
midiendo con numpy. **No se escucharon**: esta máquina no tiene salida de
audio, así que la verificación es por medidas, no de oído. Conviene darles una
escucha antes de dejarlos fijos.

---

## 3. Cómo conectarlo en el juego

> No se tocó nada de `src/`. Esto son instrucciones para hacer el cableado.

### 3.1. Cargar los nuevos sonidos

En `src/audio/AudioSystem.h`, añadir las entradas al `enum class Sfx`, antes
de `Count`:

```cpp
    // Rayder
    RayderGolpe1, RayderGolpe2, RayderGolpe3,
    RayderPatada1, RayderPatada2, RayderPatada3,
    RayderImpactoFuerte, RayderSilbido,
    RayderCargaPoder, RayderDisparoPoder, RayderImpactoPoder,
    RayderFuria, RayderEsquiva,
    Count
```

En `src/audio/AudioSystem.cpp`, dentro de `Init()`, la tabla `files` ya hace
exactamente lo que hace falta (busca el `.wav`, lo carga y reemplaza al sonido
sintetizado). Solo hay que añadir las filas, pero **con la subcarpeta en el
nombre**, porque la ruta se arma como `"assets/audio/sfx/" + f.name + ".wav"`:

```cpp
{Sfx::RayderGolpe1,       "rayder/golpe_1",        0.85f},
{Sfx::RayderGolpe2,       "rayder/golpe_2",        0.85f},
{Sfx::RayderGolpe3,       "rayder/golpe_3",        0.85f},
{Sfx::RayderPatada1,      "rayder/patada_1",       0.88f},
{Sfx::RayderPatada2,      "rayder/patada_2",       0.88f},
{Sfx::RayderPatada3,      "rayder/patada_3",       0.88f},
{Sfx::RayderImpactoFuerte,"rayder/impacto_fuerte", 0.95f},
{Sfx::RayderSilbido,      "rayder/silbido",        0.55f},
{Sfx::RayderCargaPoder,   "rayder/carga_poder",    0.75f},
{Sfx::RayderDisparoPoder, "rayder/disparo_poder",  0.80f},
{Sfx::RayderImpactoPoder, "rayder/impacto_poder",  0.90f},
{Sfx::RayderFuria,        "rayder/furia",          0.85f},
{Sfx::RayderEsquiva,      "rayder/esquiva",        0.70f},
```

Esas entradas **no** necesitan un `BuildSound(...)` previo: si el `.wav` no
estuviera, el hueco simplemente queda vacío y `Play()` no hace nada (ya
comprueba `sound.frameCount > 0`).

Ojo con una cosa: la tabla actual pone el volumen con `SetSoundVolume(snd, f.volume)`
**una sola vez al cargar**. Si luego se va a cambiar el volumen por distancia
(apartado 3.3), ese valor pasa a ser el "volumen base" y hay que guardarlo
aparte para multiplicarlo, no perderlo.

### 3.2. Rotar las variantes del combo

Para que el combo no repita el mismo golpe, llevar un contador en quien dispara
el ataque:

```cpp
// En el jugador / en el sistema de combate
int pasoCombo = 0;  // 0, 1, 2

void GolpeDeRayder() {
    static const Sfx variantes[3] = {
        Sfx::RayderGolpe1, Sfx::RayderGolpe2, Sfx::RayderGolpe3
    };
    AudioSystem::Get().Play(variantes[pasoCombo % 3]);
    ++pasoCombo;
}
```

Lo mismo para las patadas. El tercer golpe del combo (el remate) usa
`Sfx::RayderImpactoFuerte` en vez de `golpe_3`.

### 3.3. Que suene a izquierda o derecha según dónde esté el enemigo

raylib tiene tres funciones para esto, y hay que llamarlas **justo antes** de
`PlaySound`:

| Función raylib | Qué hace | Rango |
|---|---|---|
| `SetSoundPan(Sound, float)` | Mueve el sonido entre altavoz izquierdo y derecho | 0.0 a 1.0; **0.5 = centro** |
| `SetSoundVolume(Sound, float)` | Volumen | 0.0 a 1.0 |
| `SetSoundPitch(Sound, float)` | Tono (y con él, la velocidad) | 1.0 = normal |

La propuesta es añadir un `Play` con posición al `AudioSystem`:

```cpp
// AudioSystem.h  (declaración)
//   mundoX    : posición X del que produce el sonido (coordenadas de mundo)
//   camaraX   : X del centro de la cámara
//   anchoVista: ancho visible en píxeles de mundo
void PlayEn(Sfx sfx, float mundoX, float camaraX, float anchoVista);
```

```cpp
// AudioSystem.cpp  (implementación)
void AudioSystem::PlayEn(Sfx sfx, float mundoX, float camaraX, float anchoVista) {
    if (!ready || muted || betaAudio) return;
    Sound& sound = sounds[Index(sfx)];
    if (sound.frameCount == 0) return;

    // 1) Posición relativa a la cámara: -1 (borde izquierdo) .. +1 (derecho)
    const float mitad = std::max(1.0f, anchoVista * 0.5f);
    float rel = std::clamp((mundoX - camaraX) / mitad, -1.0f, 1.0f);

    // 2) Paneo. 0.5 es el centro. El 0.35 es cuánto se abre como máximo:
    //    dejarlo por debajo de 0.5 evita que un golpe desaparezca de un oído.
    const float apertura = 0.35f;
    SetSoundPan(sound, 0.5f + rel * apertura);

    // 3) Volumen por distancia: lo que pasa en el centro de pantalla suena
    //    entero; en los bordes baja hasta el 55%.
    const float volumenBase = 0.85f;   // el que tenía la tabla de Init()
    SetSoundVolume(sound, volumenBase * (1.0f - 0.45f * std::fabs(rel)));

    // 4) Variación de tono: ±6%. Es lo que más ayuda a que un combo rápido
    //    no suene a repetición mecánica.
    SetSoundPitch(sound, 0.94f + static_cast<float>(GetRandomValue(0, 120)) * 0.001f);

    PlaySound(sound);
}
```

Y en el combate, en lugar de `Play(Sfx::RayderGolpe1)`:

```cpp
AudioSystem::Get().PlayEn(Sfx::RayderGolpe1,
                          enemigo.posicion.x,          // dónde pega
                          camara.target.x,             // centro de cámara
                          GetScreenWidth() / camara.zoom);
```

**Dos avisos importantes antes de dar esto por bueno:**

1. **El sentido del paneo hay que comprobarlo de oído.** En raylib 5.5
   `SetSoundPan` convierte el valor con `pan * -2.0f + 1.0f` antes de pasarlo a
   miniaudio, con lo que **0.0 suena a la derecha y 1.0 a la izquierda**, al
   revés de lo que uno esperaría. Si al probarlo está invertido, cambiar el
   signo: `SetSoundPan(sound, 0.5f - rel * apertura);`.
2. **El paneo sobre archivos estéreo es "balance", no reposicionado.**
   miniaudio, con una fuente estéreo, lo que hace al panear es bajarle el
   volumen a un canal. Suena bien y sirve perfectamente para situar el golpe,
   pero no "mueve" el contenido de un lado al otro. Si se quisiera un paneo
   más marcado, la opción sería generar los mismos sonidos en mono (en el
   generador, quedarse con `mono` sin pasar por `a_estereo`) y dejar que raylib
   haga todo el posicionamiento. Los estéreo que hay ahora traen ya su propia
   anchura, que es lo que se pidió.

### 3.4. Golpes que se solapan

`PlaySound` sobre el mismo `Sound` **reinicia** la reproducción: si dos golpes
caen muy juntos, el segundo corta al primero. Si molesta, raylib 5.5 tiene
`LoadSoundAlias(Sound)`, que crea una copia que comparte los datos (no gasta
memoria extra) y se puede reproducir en paralelo. Lo típico es tener 3 o 4
alias por sonido de golpe y usarlos por turnos. Hay que descargarlos con
`UnloadSoundAlias` en `Shutdown()`, **no** con `UnloadSound`.

---

## 4. Qué es real y qué no

Esto hay que decirlo claro para que nadie espere lo que no es:

**Lo que sí hay:**

- Sonido **estéreo** con anchura. Cada canal lleva una cola de reverberación
  distinta y hay un microretardo de menos de 1 ms en un lado. El oído lo
  interpreta como amplitud, y el sonido no se siente pegado al centro.
- **Posición en el eje izquierda–derecha** con `SetSoundPan`.
- **Volumen según la distancia** con `SetSoundVolume`.
- **Variación de tono** con `SetSoundPitch`.
- Una **cola de reverberación corta** horneada dentro de cada archivo.

**Lo que NO hay:**

- **No es audio 3D.** No hay HRTF, ni filtrado según la forma de la oreja, ni
  retardo interaural, ni nada que permita distinguir si algo está delante o
  detrás, arriba o abajo. raylib no lo trae.
- **No hay profundidad real** (eje Z). Lo que se simula como "lejos" es
  simplemente bajar el volumen; no se filtran los agudos ni se añade más
  reverberación con la distancia, aunque eso último sí se podría hacer a mano.
- **La reverberación no es dinámica.** Está grabada dentro del archivo, es
  siempre la misma y no cambia según el escenario (un callejón y una plaza
  sonarían igual).

En resumen: cuando en este documento se dice **"envolvente"**, significa
*estéreo con posición, distancia y variación de tono*. Nada más. Es lo máximo
honesto que se puede hacer con raylib sin meter una librería de audio aparte
(tipo FMOD, Wwise o OpenAL Soft con HRTF).

---

## 5. Archivos

| Qué | Dónde |
|---|---|
| Generador | `tools/audio/genera_sonidos_rayder.py` |
| Sonidos | `assets/audio/sfx/rayder/*.wav` (13 archivos) |
| Este documento | `docs/SONIDO_RAYDER.md` |
