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
    GameOver
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

private:
    AudioSystem() = default;
    ~AudioSystem() = default;
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    std::array<Sound, 15> sounds{};
    bool ready = false;
    bool deviceOwned = false;
    bool muted = false;

    void BuildSound(Sfx sfx, float frequency, float duration, float volume, bool noise = false);
};

}
