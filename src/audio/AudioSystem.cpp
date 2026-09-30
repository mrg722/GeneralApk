#include "audio/AudioSystem.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {
constexpr float kPi = 3.14159265359f;
constexpr int kSampleRate = 44100;

std::size_t Index(Sfx sfx) { return static_cast<std::size_t>(sfx); }
}

AudioSystem& AudioSystem::Get() {
    static AudioSystem instance;
    return instance;
}

void AudioSystem::Init() {
    if (ready) return;
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        deviceOwned = IsAudioDeviceReady();
    }
    if (!IsAudioDeviceReady()) return;

    BuildSound(Sfx::Punch, 190.0f, 0.075f, 0.26f);
    BuildSound(Sfx::Kick, 120.0f, 0.105f, 0.32f);
    BuildSound(Sfx::Hit, 155.0f, 0.07f, 0.28f, true);
    BuildSound(Sfx::HeavyHit, 82.0f, 0.14f, 0.38f, true);
    BuildSound(Sfx::EnergyCharge, 330.0f, 0.22f, 0.20f);
    BuildSound(Sfx::EnergyShot, 520.0f, 0.13f, 0.25f);
    BuildSound(Sfx::EnergyImpact, 250.0f, 0.18f, 0.34f, true);
    BuildSound(Sfx::EnemyDeath, 92.0f, 0.16f, 0.30f, true);
    BuildSound(Sfx::BossAttack, 68.0f, 0.22f, 0.36f, true);
    BuildSound(Sfx::BossPhase, 440.0f, 0.42f, 0.30f);
    BuildSound(Sfx::Dash, 720.0f, 0.10f, 0.18f);
    BuildSound(Sfx::Rage, 180.0f, 0.40f, 0.32f);
    BuildSound(Sfx::Ui, 620.0f, 0.06f, 0.16f);
    BuildSound(Sfx::StageClear, 520.0f, 0.35f, 0.24f);
    BuildSound(Sfx::GameOver, 105.0f, 0.42f, 0.26f);
    ready = true;
}

void AudioSystem::BuildSound(Sfx sfx, float frequency, float duration, float volume, bool noise) {
    const int frames = std::max(1, static_cast<int>(duration * kSampleRate));
    auto* data = static_cast<float*>(MemAlloc(static_cast<unsigned int>(frames * sizeof(float))));
    if (!data) return;

    const std::size_t total = static_cast<std::size_t>(frames);
    for (std::size_t i = 0; i < total; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float envelope = std::min(1.0f, t * 55.0f) * std::max(0.0f, 1.0f - t / duration);
        const float carrier = std::sin(2.0f * kPi * frequency * t);
        const float grit = std::sin(2.0f * kPi * (frequency * 2.73f) * t) * 0.35f;
        const float sample = noise ? (carrier * 0.55f + grit) : carrier;
        data[i] = sample * envelope * volume;
    }

    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(frames);
    wave.sampleRate = kSampleRate;
    wave.sampleSize = 32;
    wave.channels = 1;
    wave.data = data;
    sounds[Index(sfx)] = LoadSoundFromWave(wave);
    UnloadWave(wave);
}

void AudioSystem::Play(Sfx sfx) {
    if (!ready || muted) return;
    Sound& sound = sounds[Index(sfx)];
    if (sound.frameCount > 0) PlaySound(sound);
}

void AudioSystem::Shutdown() {
    if (!ready && !deviceOwned) return;
    for (Sound& sound : sounds) {
        if (sound.frameCount > 0) UnloadSound(sound);
    }
    ready = false;
    if (deviceOwned && IsAudioDeviceReady()) CloseAudioDevice();
    deviceOwned = false;
}

}
