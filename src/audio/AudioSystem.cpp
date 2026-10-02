#include "audio/AudioSystem.h"
#include <algorithm>
#include <cmath>
#include "core/Platform.h"
#include <string>
#include <vector>

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
    BuildSound(Sfx::Block, 400.0f, 0.06f, 0.2f, true);
    BuildSound(Sfx::Whoosh, 900.0f, 0.06f, 0.08f);
    BuildSound(Sfx::Land, 70.0f, 0.08f, 0.2f, true);
    BuildSound(Sfx::Jump, 300.0f, 0.08f, 0.12f);
    BuildSound(Sfx::FuryCharge, 220.0f, 0.4f, 0.25f);
    BuildSound(Sfx::UiConfirm, 760.0f, 0.07f, 0.16f);
    BuildSound(Sfx::UiCancel, 380.0f, 0.07f, 0.16f);
    BuildSound(Sfx::UiPause, 500.0f, 0.08f, 0.16f);

    // Paquete del usuario: si el WAV existe, reemplaza al sonido sintetizado.
    struct File { Sfx sfx; const char* name; float volume; };
    const File files[] = {
        {Sfx::Punch, "combat_punch_light", 0.8f}, {Sfx::Kick, "combat_kick", 0.85f},
        {Sfx::Hit, "combat_damage", 0.8f}, {Sfx::HeavyHit, "combat_impact_heavy", 0.9f},
        {Sfx::EnergyCharge, "fury_charge", 0.7f}, {Sfx::EnergyShot, "fury_electric", 0.75f},
        {Sfx::EnergyImpact, "fury_power_impact", 0.85f}, {Sfx::EnemyDeath, "combat_ko", 0.8f},
        {Sfx::BossAttack, "combat_punch_heavy", 0.9f}, {Sfx::BossPhase, "world_metal_clang", 0.8f},
        {Sfx::Dash, "move_dash", 0.7f}, {Sfx::Rage, "fury_release", 0.85f}, {Sfx::Ui, "ui_navigate", 0.6f},
        {Sfx::StageClear, "energy_pickup", 0.8f}, {Sfx::GameOver, "combat_ko", 0.9f},
        {Sfx::Block, "combat_block", 0.8f}, {Sfx::Whoosh, "move_whoosh", 0.5f}, {Sfx::Land, "move_land", 0.6f},
        {Sfx::Jump, "move_jump", 0.6f}, {Sfx::FuryCharge, "fury_charge", 0.8f},
        {Sfx::UiConfirm, "ui_confirm", 0.6f}, {Sfx::UiCancel, "ui_cancel", 0.6f}, {Sfx::UiPause, "ui_pause", 0.6f},
    };
    for (const File& f : files) {
        std::vector<unsigned char> bytes;
        const std::string path = std::string("assets/audio/sfx/") + f.name + ".wav";
        bool ok = false;
        for (const std::string& p : {path, "../" + path, "../../" + path})
            if (platform::AssetExists(p) && platform::LoadBinaryFile(p, bytes)) { ok = true; break; }
        if (!ok || bytes.empty()) continue;
        Wave w = LoadWaveFromMemory(".wav", bytes.data(), static_cast<int>(bytes.size()));
        if (w.frameCount == 0) continue;
        Sound snd = LoadSoundFromWave(w);
        UnloadWave(w);
        if (snd.frameCount == 0) continue;
        SetSoundVolume(snd, f.volume);
        Sound& slot = sounds[Index(f.sfx)];
        if (slot.frameCount > 0) UnloadSound(slot);
        slot = snd;
    }
    // Ambiente: lluvia en bucle, bajo.
    for (const std::string& p : {std::string("assets/audio/sfx/world_rain_loop.wav"), std::string("../assets/audio/sfx/world_rain_loop.wav"),
                                 std::string("../../assets/audio/sfx/world_rain_loop.wav")}) {
        if (!platform::AssetExists(p)) continue;
        ambient = LoadMusicStream(p.c_str());
        if (ambient.frameCount > 0) {
            ambient.looping = true;
            SetMusicVolume(ambient, 0.22f);
            PlayMusicStream(ambient);
            ambientReady = true;
        }
        break;
    }
    ready = true;
}

void AudioSystem::UpdateAmbient() {
    if (!ambientReady) return;
    if (muted || betaAudio) { if (IsMusicStreamPlaying(ambient)) PauseMusicStream(ambient); return; }
    if (!IsMusicStreamPlaying(ambient)) ResumeMusicStream(ambient);
    UpdateMusicStream(ambient);
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
    if (!ready || muted || betaAudio) return;
    Sound& sound = sounds[Index(sfx)];
    if (sound.frameCount > 0) PlaySound(sound);
}

void AudioSystem::Shutdown() {
    if (!ready && !deviceOwned) return;
    for (Sound& sound : sounds) {
        if (sound.frameCount > 0) UnloadSound(sound);
    }
    if (ambientReady) { UnloadMusicStream(ambient); ambientReady = false; }
    ready = false;
    if (deviceOwned && IsAudioDeviceReady()) CloseAudioDevice();
    deviceOwned = false;
}

}
