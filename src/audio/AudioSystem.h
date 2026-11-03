// Motor de mezcla de District Fury (F1.10).
//
// - Pool de voces por archivo (LoadSoundAlias): 4 golpes/impactos, 2 UI, 1 jingles.
// - Limite global de 12 SFX con prioridad, antirrafaga de 30 ms, pitch/volumen
//   aleatorio y variantes sin repetir la ultima (logica pura en SfxBank.h).
// - Buses (Master, Sfx, Ui, Voice, Music, Ambient): vol = tabla x bus x master.
//   El deslizador EFECTOS de OPCIONES es Bus::Sfx y escala tambien Ui y Voice.
// - Ducking de musica/ambiente, paneo por X, limitador suave en el bus maestro
//   (AttachAudioMixedProcessor) y normalizacion st50 por categoria al cargar.
// - Sonidos por NOMBRE (contrato con F1.11: assets/audio/sfx/combat/...,
//   arquetipos/<a>/..., rayder_v2/..., ronin_v2/...) con caida al WAV legado
//   (enum Sfx) si el archivo nuevo no existe: nunca silencio, nunca crash.
// El combate no llama aqui: emite GameEvent y AudioDirector decide (SoundCue).
#pragma once
#include "raylib.h"
#include "audio/SfxBank.h"
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace district_fury {

enum class Sfx {
    Punch,
    Kick,
    Hit,
    HeavyHit,
    EnergyCharge,
    EnergyShot,
    EnergyImpact,
    EnemyDeath,
    BossAttack,
    BossPhase,
    Dash,
    Rage,
    Ui,
    StageClear,
    GameOver,
    // Paquete de audio del usuario (assets/audio/sfx/)
    Block,
    Whoosh,
    Land,
    Jump,
    FuryCharge,
    UiConfirm,
    UiCancel,
    UiPause,
    // Rayder: golpes, patadas y poderes propios (docs/SONIDO_RAYDER.md).
    RayderGolpe1, RayderGolpe2, RayderGolpe3,
    RayderPatada1, RayderPatada2, RayderPatada3,
    RayderImpactoFuerte, RayderSilbido,
    RayderCargaPoder, RayderDisparoPoder, RayderImpactoPoder,
    RayderFuria, RayderEsquiva,
    // Ronin: punetazos, patadas y habilidad de espada propios (artista
    // marcial a manos limpias + un tajo de espada), mismo patron que Rayder.
    RoninGolpe1, RoninGolpe2, RoninGolpe3,
    RoninPatada1, RoninPatada2,
    RoninImpactoFuerte,
    RoninEspadaDesenvaine, RoninEspadaTajo, RoninEspadaImpacto,
    RoninSalto, RoninAterrizaje,
    // Jingle de arranque del juego (docs/INTRO_ARRANQUE.md).
    // TEMPORAL/placeholder: pendiente de reemplazar por musica definitiva.
    IntroArranque,
    // F1.10: ui_open.wav (antes nunca se cargaba).
    UiOpen,
    Count
};

// Buses de mezcla (contrato F0.1).
enum class Bus { Master, Sfx, Ui, Voice, Music, Ambient };
constexpr int kBusCount = 6;

// Arquetipo sonoro de un personaje (ver AudioDirector: ArchetypeForSkin).
enum class Archetype { Brawler, Agil, Cuchillas, Pesado, Electrico, Max, Ronin };

// Lo que el director pide sonar para un evento. Orden de resolucion:
//  1) pack propio "<pack>/<packName>" (MAX rayder_v2, RONIN ronin_v2) con packPitch;
//  2) "arquetipos/<arquetipo>/<name>" (el archivo ya trae su tono: pitch 1);
//  3) "combat/<alt o name>" con `pitch`;
//  4) WAV legado `fallback` (con packPitch si era un pack, si no `pitch`).
struct SoundCue {
    const char* name = nullptr;
    const char* alt = nullptr;
    const char* pack = nullptr;
    const char* packName = nullptr;
    Archetype arch = Archetype::Brawler;
    Sfx fallback = Sfx::Count;
    SoundCat cat = SoundCat::Hit;
    Bus bus = Bus::Sfx;
    int priority = kPrioLight;
    float pitch = 1.0f;
    float packPitch = 1.0f;
    float pitchJitter = 0.0f;    // +-fraccion (0.06 = +-6 %)
    float volJitterDb = 0.0f;    // +-dB
    float gainDb = 0.0f;
    float duckDb = 0.0f;         // > 0: atenua musica/ambiente
    float duckHold = 0.0f;
    bool hasX = false;           // paneo por la X del evento
    float x = 0.0f;
};

// Como se resolvio un cue (para el registro de depuracion y los tests).
enum class CueSource { None, Pack, Archetype, Generic, Legacy };

class AudioSystem {
public:
    static AudioSystem& Get();
    void Init();
    void Shutdown();
    // Sonido legado por enum (main: jingle de arranque). Pasa por el mismo motor.
    void Play(Sfx sfx);
    // Sonido pedido por el director. Devuelve de donde salio (None = no sono).
    CueSource PlayCue(const SoundCue& cue);
    bool IsReady() const { return ready; }

    // Volumen del bus (0..1); OPCIONES, BetaMode y MusicPlayer lo leen.
    float BusVolume(Bus b) const { return bus[static_cast<int>(b)]; }
    void SetBusVolume(Bus b, float v);
    // Ganancia final de un bus de SFX (Ui/Voice tambien pasan por Sfx) x master.
    float EffectiveBus(Bus b) const;

    // DF-013: usado por la pantalla de OPCIONES del menu principal (se guarda).
    void SetMuted(bool value);
    bool IsMuted() const { return muted; }
    // Modo BETA: usa los sonidos originales de nuevosSprites, asi que apaga los
    // efectos genericos del juego y la lluvia ambiente mientras esta activo.
    void SetBetaAudio(bool value) { betaAudio = value; }
    bool BetaAudio() const { return betaAudio; }
    // Cada frame desde main: director de eventos, ducking, musica y ambiente.
    void UpdateAmbient();

    // Paneo: centro de la camara del stage (si main lo conoce) o, si no, la X
    // del jugador (la anota el director con sus eventos).
    void SetListenerX(float cameraCenterX) { listenerX = cameraCenterX; listenerFromCamera = true; }
    void ClearListener() { listenerFromCamera = false; }
    void NotePlayerX(float x) { if (!listenerFromCamera) listenerX = x; }
    // Lluvia: solo en Menu y Stage1 (lo decide el director con ModeEnter).
    void SetRainWanted(bool v) { rainWanted = v; }
    bool RainWanted() const { return rainWanted; }
    void Duck(float depthDb, float holdSec) { ducker.Trigger(depthDb, holdSec); }
    float DuckGain() const { return ducker.Gain(); }

    // Estadisticas (DF_AUDIO_LOG=1 las imprime al salir).
    int ActiveVoices() const { return limiter.Active(); }
    int MaxVoicesSeen() const { return maxVoices; }
    int SoundsStarted() const { return started; }
    int DroppedByBurst() const { return droppedBurst; }
    int DroppedByLimit() const { return droppedLimit; }
    // true si existe (o se pudo cargar) el grupo "carpeta/nombre".
    bool HasGroup(const std::string& path);

private:
    AudioSystem() = default;
    ~AudioSystem() = default;
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    static constexpr int kMaxAlias = 4;
    struct Clip {
        Sound voice[kMaxAlias]{};   // voice[0] = sonido base; el resto alias
        double start[kMaxAlias]{};
        int voices = 0;
        float norm = 1.0f;          // ganancia de normalizacion st50
    };
    struct Group {
        std::vector<int> clips;
        int lastVariant = -1;
    };

    std::vector<Clip> clips;
    std::vector<Group> groups;
    std::unordered_map<std::string, int> groupByPath;   // -1 = no existe
    std::unordered_map<std::string, int> clipByFile;    // cada archivo se carga una vez
    std::array<int, static_cast<std::size_t>(Sfx::Count)> legacyGroup{};
    int slotClip[kMaxSfxVoices]{};                        // voz global -> clip y alias que suena
    int slotAlias[kMaxSfxVoices]{};

    VoiceLimiter limiter;
    BurstGuard burst;
    MixRng rng;
    Ducker ducker;
    float bus[kBusCount] = {1, 1, 1, 1, 1, 1};
    float listenerX = 640.0f;
    bool listenerFromCamera = false;

    Music ambient{};
    float ambientNorm = 0.22f;
    float rainGain = 1.0f;      // rampa de entrada/salida de la lluvia
    bool rainWanted = true;
    bool ambientReady = false;
    bool ready = false;
    bool deviceOwned = false;
    bool muted = false;
    bool betaAudio = false;
    bool persist = false;       // OPCIONES: guardar al cambiar (solo tras Init)
    bool limiterAttached = false;

    int maxVoices = 0, started = 0, droppedBurst = 0, droppedLimit = 0;

    int LoadClip(const std::string& file, SoundCat cat);
    int LoadClipFromWave(const std::string& key, Wave w, SoundCat cat, float extraGain);
    int FindGroup(const std::string& path, SoundCat cat);
    void BuildSound(Sfx sfx, float frequency, float duration, float volume, bool noise = false);
    bool PlayGroup(int group, int key, const SoundCue& cue, float pitch);
    void ReleaseFinishedVoices();
    void UpdateMusicAndAmbient(float dt);
};

// Categoria de un nombre del contrato (normalizacion y pool de voces).
SoundCat CategoryForName(const std::string& base);
SoundCat CategoryForSfx(Sfx sfx);

}  // namespace district_fury
