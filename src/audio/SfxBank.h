// Banco de sonidos de combate: LOGICA PURA de mezcla (F1.10).
//
// Todo lo de aqui es testeable sin dispositivo de audio (tests/unit/AudioMixTests.cpp):
//  - VoiceLimiter: limite global de 12 SFX simultaneos con prioridad (roba la voz
//    de menor prioridad / mas vieja o descarta la nueva).
//  - BurstGuard: antirrafaga (el mismo sonido < 30 ms no se repite).
//  - PickVariant / PickAlias: variante aleatoria sin repetir la ultima y alias
//    libre (o el mas viejo) dentro del pool de voces de un archivo.
//  - PanForX: paneo por la X del evento respecto al centro de la camara.
//  - Ducker: atenuacion temporal de musica/ambiente bajo golpes fuertes.
//  - SoftClip: limitador suave del bus maestro (tanh).
//  - St50Db / NormalizeGainDb: normalizacion al cargar por categoria.
// AudioSystem envuelve esto con raylib (LoadSoundAlias, SetSoundPitch...).
// Los archivos los genera tools/audio/genera_banco_combate.py (F1.11): todo
// sintetizado por codigo, nada con copyright.
#pragma once
#include <cstdint>

namespace district_fury {

// Prioridades (0 UI/ambiente, 1 movimiento/pasos, 2 golpe ligero,
// 3 golpe pesado/dano al jugador, 4 furia/KO/jefe).
enum SfxPriority : int { kPrioUi = 0, kPrioMove = 1, kPrioLight = 2, kPrioHeavy = 3, kPrioTop = 4 };

// Categoria de un sonido: decide su normalizacion al cargar y su pool de voces.
enum class SoundCat { Hit, Move, Step, Ui, Jingle, Voice, Ambient };

constexpr int kMaxSfxVoices = 12;       // limite global de SFX a la vez
constexpr double kBurstWindow = 0.030;  // antirrafaga: 30 ms

// Voces del pool por archivo (alias de raylib): 4 golpes/impactos, 2 UI, 1 jingles.
int VoicesFor(SoundCat cat);
// Objetivo de normalizacion (st50, dBFS) por categoria: golpes -10, UI -16,
// ambiente -28; movimiento -16, pasos -24 (suenan bajo), jingles -12, voz -12.
float TargetSt50Db(SoundCat cat);

struct VoiceSlot {
    bool used = false;
    int key = -1;          // sonido (grupo) que ocupa la voz
    int priority = 0;
    double start = 0.0;    // instante en que empezo
};

// Limite global de voces. AudioSystem libera (Release) las que ya terminaron
// antes de pedir una nueva con Acquire.
class VoiceLimiter {
public:
    // Devuelve la voz a usar (libre o robada; `stolen` = habia otra sonando
    // que hay que parar) o -1 si el nuevo sonido se descarta.
    // Regla: con todas ocupadas se roba la de MENOR prioridad (la mas vieja entre
    // iguales) si la nueva tiene prioridad >= que ella; si no, se descarta la nueva.
    int Acquire(int key, int priority, double now, bool* stolen = nullptr);
    void Release(int slot);
    int Active() const;
    const VoiceSlot& Slot(int i) const { return slots[i]; }
    void Clear();

private:
    VoiceSlot slots[kMaxSfxVoices];
};

// Antirrafaga por clave (grupo/Sfx). Admite hasta kMaxKeys claves distintas.
class BurstGuard {
public:
    static constexpr int kMaxKeys = 1024;
    // true si el sonido puede sonar ahora (y registra el disparo).
    bool Allow(int key, double now, double window = kBurstWindow);
    void Clear();

private:
    double last[kMaxKeys] = {};
    bool seen[kMaxKeys] = {};
};

// Generador pseudoaleatorio pequeno y determinista (xorshift32).
struct MixRng {
    uint32_t s = 0x9E3779B9u;
    uint32_t Next();
    float Uniform(float lo, float hi);   // [lo, hi)
};

// Variante en [0, count) distinta de `last` (si hay mas de una).
int PickVariant(int count, int last, MixRng& rng);
// Alias a usar dentro del pool de un archivo: el primero libre o, si todos
// suenan, el que empezo antes.
int PickAlias(const bool* playing, const double* start, int count);

// Pan [0..1] (0.5 = centro) por la X del evento: 0.5 + clamp((x - centroCamara)/1280, -0.35, 0.35).
// Es la formula del plan con camX = borde izquierdo (centro = camX + 640).
float PanForX(float x, float cameraCenterX);

float DbToLinear(float db);
float LinearToDb(float g);

// Ducking: Trigger(dB, hold) baja la ganancia a -dB durante `hold` segundos y
// luego se recupera de forma exponencial (tau 35 ms; vuelve a 1.0 exacto).
class Ducker {
public:
    void Trigger(float depthDb, float holdSec);
    void Update(float dt);
    float Gain() const { return gain; }
    void Reset() { gain = 1.0f; hold = 0.0f; }

private:
    float gain = 1.0f;
    float hold = 0.0f;
};

// Limitador suave del bus maestro: tanh(1.2 x) / tanh(1.2), acotado a [-1, 1].
float SoftClip(float x);
// Procesa un bloque intercalado (el que entrega AttachAudioMixedProcessor).
// Sin asignaciones ni locks: apto para el hilo de audio de Android.
void SoftClipBuffer(float* samples, unsigned int count);

// RMS maximo en ventanas de 50 ms (st50), en dBFS, de muestras float intercaladas.
float St50Db(const float* samples, unsigned int frames, int channels, int sampleRate);
// Ganancia (dB) que lleva `st50Db` al objetivo, acotada a [-18, +12] dB.
float NormalizeGainDb(float st50Db, float targetDb);

}  // namespace district_fury
