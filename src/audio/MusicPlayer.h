// Reproductor de musica original adaptativa por modo (contrato F0.1, implementado
// en F1.12). Toda la musica es propia: la compone y sintetiza por algoritmo
// tools/audio/genera_musica.py (assets/audio/music/*.ogg, ver LEEME_musica.txt).
//
// Recibe del AudioDirector los eventos ModeEnter, ArenaLock/ArenaUnlock,
// BossPhase, StageClear, Victory y GameOver:
//   - ModeEnter elige el tema del modo (crossfade de 1 s entre temas).
//   - Cada stage tiene dos capas sincronizadas (calma / combate) que suenan a la
//     vez; ArenaLock sube la de combate y ArenaUnlock vuelve a calma (rampa de
//     potencia constante de 1 s). El jefe tiene capas A/B: BossPhase >= 2 -> B.
//   - StageClear / Victory -> stinger "victoria"; GameOver -> "derrota"; despues,
//     silencio (o el tema del modo al que se haya entrado mientras sonaba).
// Volumen final de cada stream = ganancia de la logica x SetVolume (bus musica x
// master, lo da F1.10) x SetDuck. Como mucho 2 streams activos + 1 en fundido.
//
// La logica (estado, rampas, voces deseadas) es pura y se prueba sin dispositivo
// (tests/unit/MusicPlayerTests.cpp); los streams de raylib solo se abren si hay
// dispositivo de audio (IsAudioDeviceReady) y se cargan al empezar cada tema.
// Log de depuracion "MUSICA: <tema>": variable de entorno DF_MUSICA_LOG=1 o
// SetDebugLog(true).
#pragma once
#include "game/events/GameEvents.h"
#include <string>
#include <vector>

namespace district_fury {

enum class MusicTheme { None, Menu, Stage1, Stage2, Stage3, Stage4, Stage5, Boss, VS, Count };
enum class MusicStinger { None, Victoria, Derrota };

// Una voz que deberia sonar ahora (lo que la logica pide al backend).
struct MusicVoice {
    const char* file = nullptr;   // nombre dentro de assets/audio/music/
    float gain = 0.0f;            // ganancia de la logica (sin bus ni duck)
    bool loop = true;
    bool fadingOut = false;       // voz saliente (tema anterior en fundido)
};

class MusicPlayer {
public:
    static MusicPlayer& Get();
    void OnEvent(const GameEvent& e);
    void Update(float dt);
    void SetDuck(float gain);     // atenuacion temporal (0..1) bajo SFX fuertes
    void SetVolume(float v);      // volumen del bus de musica (0..1)

    // --- agregado en F1.12 -------------------------------------------------
    void PlayMusic(MusicTheme theme);         // cambia de tema (crossfade 1 s)
    void PlayStinger(MusicStinger s);         // victoria / derrota (sin bucle)
    void Stop();                              // fundido a silencio (1 s)
    void Reset();                             // estado inicial inmediato (tests)
    void Shutdown();                          // descarga los streams (antes de CloseAudioDevice)
    void SetDebugLog(bool on) { debugLog = on; }

    MusicTheme CurrentTheme() const { return theme; }
    MusicStinger CurrentStinger() const { return stinger; }
    float LayerMix() const { return layerMix; }          // 0 = calma/A, 1 = combate/B
    float LayerTarget() const { return layerTarget; }
    float ThemeFade() const { return themeFade; }
    float Volume() const { return volume; }
    float Duck() const { return duck; }
    std::vector<MusicVoice> Voices() const;   // voces deseadas ahora (<= 3)
    int ActiveStreams() const;                // streams de raylib abiertos (0 sin dispositivo)

    // Tablas (puras).
    static MusicTheme ThemeForMode(GameMode mode);
    static const char* ThemeName(MusicTheme theme);
    static const char* ThemeFile(MusicTheme theme, int layer);   // nullptr si no hay esa capa
    static int ThemeLayers(MusicTheme theme);
    static const char* StingerFile(MusicStinger s);
    static float StingerSeconds(MusicStinger s);                 // duracion nominal
    // Ambiente sugerido por modo (lo reproduce F1.10/AudioSystem; nullptr = sin ambiente).
    static const char* AmbientFileForMode(GameMode mode);
    // Ruta existente de assets/audio/music/<file> ("" si no esta en disco/APK).
    static std::string ResolveMusicPath(const char* file);

    static constexpr float kCrossfadeSeconds = 1.0f;
    static constexpr float kLayerRampSeconds = 1.0f;

private:
    MusicPlayer() = default;
    void Log(const char* what, const char* detail) const;
    void BeginOutgoing();
    void SyncBackend();

    MusicTheme theme = MusicTheme::None;
    float themeFade = 0.0f;       // 0..1 entrada del tema actual
    float layerMix = 0.0f;
    float layerTarget = 0.0f;
    // Voz saliente (una sola: la capa dominante del tema anterior).
    const char* outFile = nullptr;
    float outGain = 0.0f;         // ganancia al empezar el fundido
    float outFade = 0.0f;         // 1 -> 0
    // Stinger.
    MusicStinger stinger = MusicStinger::None;
    float stingerElapsed = 0.0f;
    float stingerLength = 0.0f;
    MusicTheme pendingTheme = MusicTheme::None;   // tema a seguir tras el stinger
    bool silencedByStinger = false;
    GameMode lastMode = GameMode::Boot;

    float volume = 0.7f;
    float duck = 1.0f;
    bool debugLog = false;

    struct Stream;                // backend raylib (MusicPlayer.cpp)
    std::vector<Stream*> streams;
};

}  // namespace district_fury
