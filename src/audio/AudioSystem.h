#pragma once
#include "raylib.h"
#include <array>

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
    Count
};

class AudioSystem {
public:
    static AudioSystem& Get();
    void Init();
    void Shutdown();
    void Play(Sfx sfx);
    bool IsReady() const { return ready; }

    // DF-013: usado por la pantalla de OPCIONES del menu principal.
    void SetMuted(bool value) { muted = value; }
    bool IsMuted() const { return muted; }
    // Modo BETA: usa los sonidos originales de nuevosSprites, asi que apaga los
    // efectos genericos del juego y la lluvia ambiente mientras esta activo.
    void SetBetaAudio(bool value) { betaAudio = value; }
    bool BetaAudio() const { return betaAudio; }
    // Ambiente de lluvia en bucle (se llama cada frame desde main).
    void UpdateAmbient();

private:
    AudioSystem() = default;
    ~AudioSystem() = default;
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    std::array<Sound, static_cast<std::size_t>(Sfx::Count)> sounds{};
    Music ambient{};
    bool ambientReady = false;
    bool ready = false;
    bool deviceOwned = false;
    bool muted = false;
    bool betaAudio = false;

    void BuildSound(Sfx sfx, float frequency, float duration, float volume, bool noise = false);
};

}
