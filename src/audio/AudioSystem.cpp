// Motor de mezcla (ver AudioSystem.h).
#include "audio/AudioSystem.h"
#include "audio/AudioDirector.h"
#include "audio/AudioSettings.h"
#include "audio/MusicPlayer.h"
#include "core/Platform.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace district_fury {
namespace {
constexpr float kPi = 3.14159265359f;
constexpr int kSampleRate = 44100;
constexpr const char* kSfxRoot = "assets/audio/sfx/";

std::size_t Index(Sfx sfx) { return static_cast<std::size_t>(sfx); }

bool ReadAsset(const std::string& rel, std::vector<unsigned char>& bytes) {
    for (const std::string& p : {rel, "../" + rel, "../../" + rel})
        if (platform::AssetExists(p) && platform::LoadBinaryFile(p, bytes) && !bytes.empty()) return true;
    return false;
}

std::string FindAssetPath(const std::string& rel) {
    for (const std::string& p : {rel, "../" + rel, "../../" + rel})
        if (platform::AssetExists(p)) return p;
    return {};
}

bool StartsWith(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

// Limitador suave del bus maestro (hilo de audio: sin asignaciones ni locks).
void MasterLimiter(void* buffer, unsigned int frames) {
    SoftClipBuffer(static_cast<float*>(buffer), frames * 2u);   // salida estereo f32 de raylib
}
}  // namespace

SoundCat CategoryForName(const std::string& base) {
    if (StartsWith(base, "ui_") || StartsWith(base, "pickup_") || StartsWith(base, "combo_")) return SoundCat::Ui;
    if (StartsWith(base, "step")) return SoundCat::Step;
    if (StartsWith(base, "whoosh") || StartsWith(base, "jump") || StartsWith(base, "land_light") ||
        StartsWith(base, "telegraph") || StartsWith(base, "silbido") || StartsWith(base, "esquiva") ||
        StartsWith(base, "salto") || StartsWith(base, "aterrizaje") || StartsWith(base, "espada_desenvaine") ||
        StartsWith(base, "energy_charge") || StartsWith(base, "carga_poder"))
        return SoundCat::Move;
    if (base == "game_over" || base == "stage_clear" || StartsWith(base, "intro")) return SoundCat::Jingle;
    if (StartsWith(base, "rage") || base == "transform" || StartsWith(base, "boss_phase") || base == "furia")
        return SoundCat::Voice;
    return SoundCat::Hit;
}

SoundCat CategoryForSfx(Sfx sfx) {
    switch (sfx) {
        case Sfx::EnergyCharge: case Sfx::Dash: case Sfx::Whoosh: case Sfx::Jump: case Sfx::Land:
        case Sfx::RayderSilbido: case Sfx::RayderCargaPoder: case Sfx::RayderEsquiva:
        case Sfx::RoninEspadaDesenvaine: case Sfx::RoninSalto: case Sfx::RoninAterrizaje:
            return SoundCat::Move;
        case Sfx::Rage: case Sfx::FuryCharge: case Sfx::BossPhase: case Sfx::RayderFuria:
            return SoundCat::Voice;
        case Sfx::Ui: case Sfx::UiConfirm: case Sfx::UiCancel: case Sfx::UiPause: case Sfx::UiOpen:
            return SoundCat::Ui;
        case Sfx::StageClear: case Sfx::GameOver: case Sfx::IntroArranque:
            return SoundCat::Jingle;
        default:
            return SoundCat::Hit;
    }
}

AudioSystem& AudioSystem::Get() {
    static AudioSystem instance;
    return instance;
}

int AudioSystem::LoadClipFromWave(const std::string& key, Wave w, SoundCat cat, float extraGain) {
    if (w.frameCount == 0) return -1;
    auto it = clipByFile.find(key);
    if (it != clipByFile.end()) return it->second;
    float st = -180.0f;
    if (float* s = LoadWaveSamples(w)) {
        st = St50Db(s, w.frameCount, static_cast<int>(w.channels), static_cast<int>(w.sampleRate));
        UnloadWaveSamples(s);
    }
    Clip c;
    c.voice[0] = LoadSoundFromWave(w);
    if (c.voice[0].frameCount == 0) return -1;
    c.voices = std::clamp(VoicesFor(cat), 1, kMaxAlias);
    for (int i = 1; i < c.voices; ++i) c.voice[i] = LoadSoundAlias(c.voice[0]);
    c.norm = DbToLinear(NormalizeGainDb(st, TargetSt50Db(cat))) * extraGain;
    clips.push_back(c);
    const int idx = static_cast<int>(clips.size()) - 1;
    clipByFile[key] = idx;
    return idx;
}

int AudioSystem::LoadClip(const std::string& file, SoundCat cat) {
    auto it = clipByFile.find(file);
    if (it != clipByFile.end()) return it->second;
    std::vector<unsigned char> bytes;
    if (!ReadAsset(file, bytes)) return -1;
    Wave w = LoadWaveFromMemory(".wav", bytes.data(), static_cast<int>(bytes.size()));
    const int idx = LoadClipFromWave(file, w, cat, 1.0f);
    if (w.data) UnloadWave(w);
    return idx;
}

// Grupo "carpeta/nombre": nombre.wav o nombre_1.wav..nombre_8.wav (variantes).
int AudioSystem::FindGroup(const std::string& path, SoundCat cat) {
    auto it = groupByPath.find(path);
    if (it != groupByPath.end()) return it->second;
    int result = -1;
    if (ready) {
        Group g;
        const std::string base = std::string(kSfxRoot) + path;
        const int single = LoadClip(base + ".wav", cat);
        if (single >= 0) g.clips.push_back(single);
        else
            for (int i = 1; i <= 8; ++i) {
                const int c = LoadClip(base + "_" + std::to_string(i) + ".wav", cat);
                if (c < 0) break;
                g.clips.push_back(c);
            }
        if (!g.clips.empty()) {
            groups.push_back(g);
            result = static_cast<int>(groups.size()) - 1;
        }
    }
    groupByPath[path] = result;
    return result;
}

bool AudioSystem::HasGroup(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    return FindGroup(path, CategoryForName(slash == std::string::npos ? path : path.substr(slash + 1))) >= 0;
}

void AudioSystem::Init() {
    if (ready) return;
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        deviceOwned = IsAudioDeviceReady();
    }
    if (!IsAudioDeviceReady()) return;
    ready = true;
    clips.reserve(512);
    legacyGroup.fill(-1);

    // Respaldo sintetizado (si falta el WAV del paquete del usuario).
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
    BuildSound(Sfx::UiOpen, 680.0f, 0.08f, 0.16f);

    // Paquete del usuario: si el WAV existe, reemplaza al sintetizado. Cada
    // archivo se carga UNA vez (fury_charge lo comparten EnergyCharge y
    // FuryCharge). GameOver ya no reusa combat_ko (sonaba igual que matar a un
    // enemigo): usa combat/game_over (F1.11) o su respaldo sintetizado propio.
    // Los volumenes a mano se sustituyen por la normalizacion st50 por categoria.
    struct File { Sfx sfx; const char* name; };
    const File files[] = {
        {Sfx::Punch, "combat_punch_light"}, {Sfx::Kick, "combat_kick"},
        {Sfx::Hit, "combat_damage"}, {Sfx::HeavyHit, "combat_impact_heavy"},
        {Sfx::EnergyCharge, "fury_charge"}, {Sfx::EnergyShot, "fury_electric"},
        {Sfx::EnergyImpact, "fury_power_impact"}, {Sfx::EnemyDeath, "combat_ko"},
        {Sfx::BossAttack, "combat_punch_heavy"}, {Sfx::BossPhase, "world_metal_clang"},
        {Sfx::Dash, "move_dash"}, {Sfx::Rage, "fury_release"}, {Sfx::Ui, "ui_navigate"},
        {Sfx::StageClear, "energy_pickup"},
        {Sfx::Block, "combat_block"}, {Sfx::Whoosh, "move_whoosh"}, {Sfx::Land, "move_land"},
        {Sfx::Jump, "move_jump"}, {Sfx::FuryCharge, "fury_charge"},
        {Sfx::UiConfirm, "ui_confirm"}, {Sfx::UiCancel, "ui_cancel"}, {Sfx::UiPause, "ui_pause"},
        {Sfx::UiOpen, "ui_open"},
        {Sfx::RayderGolpe1, "rayder/golpe_1"}, {Sfx::RayderGolpe2, "rayder/golpe_2"},
        {Sfx::RayderGolpe3, "rayder/golpe_3"}, {Sfx::RayderPatada1, "rayder/patada_1"},
        {Sfx::RayderPatada2, "rayder/patada_2"}, {Sfx::RayderPatada3, "rayder/patada_3"},
        {Sfx::RayderImpactoFuerte, "rayder/impacto_fuerte"}, {Sfx::RayderSilbido, "rayder/silbido"},
        {Sfx::RayderCargaPoder, "rayder/carga_poder"}, {Sfx::RayderDisparoPoder, "rayder/disparo_poder"},
        {Sfx::RayderImpactoPoder, "rayder/impacto_poder"}, {Sfx::RayderFuria, "rayder/furia"},
        {Sfx::RayderEsquiva, "rayder/esquiva"},
        {Sfx::RoninGolpe1, "ronin/golpe_1"}, {Sfx::RoninGolpe2, "ronin/golpe_2"},
        {Sfx::RoninGolpe3, "ronin/golpe_3"}, {Sfx::RoninPatada1, "ronin/patada_1"},
        {Sfx::RoninPatada2, "ronin/patada_2"}, {Sfx::RoninImpactoFuerte, "ronin/impacto_fuerte"},
        {Sfx::RoninEspadaDesenvaine, "ronin/espada_desenvaine"}, {Sfx::RoninEspadaTajo, "ronin/espada_tajo"},
        {Sfx::RoninEspadaImpacto, "ronin/espada_impacto"}, {Sfx::RoninSalto, "ronin/salto"},
        {Sfx::RoninAterrizaje, "ronin/aterrizaje"},
        // Jingle de arranque: TEMPORAL/placeholder (docs/INTRO_ARRANQUE.md).
        {Sfx::IntroArranque, "intro_arranque"},
    };
    for (const File& f : files) {
        const int c = LoadClip(std::string(kSfxRoot) + f.name + ".wav", CategoryForSfx(f.sfx));
        if (c < 0) continue;
        Group g;
        g.clips.push_back(c);
        groups.push_back(g);
        legacyGroup[Index(f.sfx)] = static_cast<int>(groups.size()) - 1;
    }

    // Banco nuevo (F1.11): se precarga el contrato para no cargar en pleno combate.
    // Lo que falte queda marcado como ausente y se cae al WAV legado.
    static const char* kContract[] = {
        "hit_light", "hit_medium", "hit_heavy", "kick", "slash", "energy", "block", "guard_break",
        "player_hurt", "whoosh_light", "whoosh_heavy", "jump", "land_light", "land_knockdown", "step",
        "step_wet", "ko_enemy", "game_over", "stage_clear", "telegraph", "boss_hit", "boss_phase",
        "pickup_health", "pickup_energy", "combo_10", "combo_20", "combo_30", "prop_wood", "prop_metal",
        "energy_charge", "energy_shot", "energy_impact", "rage_start", "transform", "ui_select_char", "ui_deny",
    };
    static const char* kArchFolders[] = {"agil", "cuchillas", "pesado", "electrico"};
    static const char* kRayder[] = {"golpe_1", "golpe_2", "golpe_3", "patada_1", "patada_2", "patada_3",
                                    "impacto_fuerte", "silbido", "carga_poder", "disparo_poder",
                                    "impacto_poder", "furia", "esquiva"};
    static const char* kRonin[] = {"golpe_1", "golpe_2", "golpe_3", "patada_1", "patada_2", "impacto_fuerte",
                                   "espada_desenvaine", "espada_tajo", "espada_impacto", "salto", "aterrizaje"};
    for (const char* n : kContract) {
        HasGroup(std::string("combat/") + n);
        for (const char* a : kArchFolders) HasGroup(std::string("arquetipos/") + a + "/" + n);
    }
    for (const char* n : kRayder) HasGroup(std::string("rayder_v2/") + n);
    for (const char* n : kRonin) HasGroup(std::string("ronin_v2/") + n);

    // Ambiente: lluvia en bucle, normalizada a -28 dBFS st50 (solo Menu y Stage1).
    const std::string rain = FindAssetPath(std::string(kSfxRoot) + "world_rain_loop.wav");
    if (!rain.empty()) {
        Wave w = LoadWave(rain.c_str());
        if (w.frameCount > 0) {
            if (float* s = LoadWaveSamples(w)) {
                const float st = St50Db(s, w.frameCount, static_cast<int>(w.channels), static_cast<int>(w.sampleRate));
                ambientNorm = DbToLinear(NormalizeGainDb(st, TargetSt50Db(SoundCat::Ambient)));
                UnloadWaveSamples(s);
            }
            UnloadWave(w);
        }
        ambient = LoadMusicStream(rain.c_str());
        if (ambient.frameCount > 0) {
            ambient.looping = true;
            SetMusicVolume(ambient, 0.0f);
            PlayMusicStream(ambient);
            ambientReady = true;
        }
    }

    AttachAudioMixedProcessor(MasterLimiter);
    limiterAttached = true;

    audio_settings::Load();   // OPCIONES guardadas (musica/efectos/ambiente/mute)
    persist = true;
}

void AudioSystem::SetBusVolume(Bus b, float v) {
    const float c = std::clamp(v, 0.0f, 1.0f);
    float& slot = bus[static_cast<int>(b)];
    if (slot == c) return;
    slot = c;
    if (persist) audio_settings::Save();
}

void AudioSystem::SetMuted(bool value) {
    if (muted == value) return;
    muted = value;
    if (persist) audio_settings::Save();
}

float AudioSystem::EffectiveBus(Bus b) const {
    const float master = bus[static_cast<int>(Bus::Master)];
    switch (b) {
        case Bus::Sfx:     return bus[static_cast<int>(Bus::Sfx)] * master;
        case Bus::Ui:
        case Bus::Voice:   return bus[static_cast<int>(b)] * bus[static_cast<int>(Bus::Sfx)] * master;
        case Bus::Music:
        case Bus::Ambient: return bus[static_cast<int>(b)] * master;
        case Bus::Master:  return master;
    }
    return master;
}

void AudioSystem::UpdateAmbient() {
    const float dt = GetFrameTime();
    AudioDirectorUpdate(dt);   // eventos de juego -> sonidos / musica
    ducker.Update(dt);
    UpdateMusicAndAmbient(dt);
}

void AudioSystem::UpdateMusicAndAmbient(float dt) {
    const bool silent = muted || betaAudio;
    // Musica (F1.12): volumen del bus x master, ducking y su propio Update.
    MusicPlayer::Get().SetVolume(silent ? 0.0f : EffectiveBus(Bus::Music));
    MusicPlayer::Get().SetDuck(ducker.Gain());
    MusicPlayer::Get().Update(dt);
    if (!ambientReady) return;
    const float target = (!silent && rainWanted) ? 1.0f : 0.0f;
    const float step = dt * 2.0f;   // rampa de 0.5 s al entrar/salir
    rainGain = rainGain < target ? std::min(target, rainGain + step) : std::max(target, rainGain - step);
    if (rainGain <= 0.0f) {
        if (IsMusicStreamPlaying(ambient)) PauseMusicStream(ambient);
        return;
    }
    if (!IsMusicStreamPlaying(ambient)) ResumeMusicStream(ambient);
    SetMusicVolume(ambient, std::min(2.0f, ambientNorm * EffectiveBus(Bus::Ambient) * ducker.Gain() * rainGain));
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
    const int c = LoadClipFromWave("synth:" + std::to_string(Index(sfx)), wave, CategoryForSfx(sfx), 1.0f);
    UnloadWave(wave);
    if (c < 0) return;
    Group g;
    g.clips.push_back(c);
    groups.push_back(g);
    legacyGroup[Index(sfx)] = static_cast<int>(groups.size()) - 1;
}

void AudioSystem::ReleaseFinishedVoices() {
    for (int i = 0; i < kMaxSfxVoices; ++i) {
        if (!limiter.Slot(i).used) continue;
        const Clip& c = clips[static_cast<std::size_t>(slotClip[i])];
        if (!IsSoundPlaying(c.voice[slotAlias[i]])) limiter.Release(i);
    }
}

bool AudioSystem::PlayGroup(int group, int key, const SoundCue& cue, float pitch) {
    const double now = GetTime();
    if (!burst.Allow(key, now)) { ++droppedBurst; return false; }
    Group& g = groups[static_cast<std::size_t>(group)];
    const int v = PickVariant(static_cast<int>(g.clips.size()), g.lastVariant, rng);
    g.lastVariant = v;
    const int ci = g.clips[static_cast<std::size_t>(v)];
    Clip& c = clips[static_cast<std::size_t>(ci)];

    ReleaseFinishedVoices();
    bool stolen = false;
    const int slot = limiter.Acquire(key, cue.priority, now, &stolen);
    if (slot < 0) { ++droppedLimit; return false; }
    if (stolen) StopSound(clips[static_cast<std::size_t>(slotClip[slot])].voice[slotAlias[slot]]);

    bool playing[kMaxAlias] = {};
    for (int i = 0; i < c.voices; ++i) playing[i] = IsSoundPlaying(c.voice[i]);
    const int a = PickAlias(playing, c.start, c.voices);
    // Si se reusa un alias que aun suena, su voz global anterior queda libre.
    for (int j = 0; j < kMaxSfxVoices; ++j)
        if (j != slot && limiter.Slot(j).used && slotClip[j] == ci && slotAlias[j] == a) limiter.Release(j);
    c.start[a] = now;

    Sound& s = c.voice[a];
    const float p = std::clamp(pitch * (1.0f + rng.Uniform(-cue.pitchJitter, cue.pitchJitter)), 0.25f, 4.0f);
    const float db = cue.gainDb + rng.Uniform(-cue.volJitterDb, cue.volJitterDb);
    const float vol = std::clamp(c.norm * DbToLinear(db) * EffectiveBus(cue.bus), 0.0f, 2.0f);
    SetSoundVolume(s, vol);
    SetSoundPitch(s, p);
    SetSoundPan(s, cue.hasX ? PanForX(cue.x, listenerX) : 0.5f);
    PlaySound(s);
    slotClip[slot] = ci;
    slotAlias[slot] = a;
    ++started;
    maxVoices = std::max(maxVoices, limiter.Active());
    if (cue.duckDb > 0.0f) ducker.Trigger(cue.duckDb, cue.duckHold);
    return true;
}

CueSource AudioSystem::PlayCue(const SoundCue& cue) {
    if (!ready || muted || betaAudio) return CueSource::None;
    if (cue.pack && cue.packName) {
        const int g = FindGroup(std::string(cue.pack) + "/" + cue.packName, cue.cat);
        if (g >= 0) return PlayGroup(g, g, cue, cue.packPitch) ? CueSource::Pack : CueSource::None;
    }
    if (cue.name) {
        if (const char* folder = ArchetypeFolder(cue.arch)) {
            const int g = FindGroup(std::string("arquetipos/") + folder + "/" + cue.name, cue.cat);
            if (g >= 0) return PlayGroup(g, g, cue, 1.0f) ? CueSource::Archetype : CueSource::None;
        }
        const int g = FindGroup(std::string("combat/") + (cue.alt ? cue.alt : cue.name), cue.cat);
        if (g >= 0) return PlayGroup(g, g, cue, cue.pitch) ? CueSource::Generic : CueSource::None;
    }
    if (cue.fallback != Sfx::Count) {
        const int g = legacyGroup[Index(cue.fallback)];
        if (g >= 0)
            return PlayGroup(g, g, cue, cue.pack ? cue.packPitch : cue.pitch) ? CueSource::Legacy : CueSource::None;
    }
    return CueSource::None;
}

void AudioSystem::Play(Sfx sfx) {
    SoundCue cue;
    cue.fallback = sfx;
    cue.cat = CategoryForSfx(sfx);
    cue.bus = cue.cat == SoundCat::Ui ? Bus::Ui : Bus::Sfx;
    cue.priority = cue.cat == SoundCat::Jingle ? kPrioTop : kPrioLight;
    PlayCue(cue);
}

void AudioSystem::Shutdown() {
    if (!ready && !deviceOwned) return;
    if (std::getenv("DF_AUDIO_LOG")) {
        AudioDirectorPrintStats();
        std::fprintf(stderr, "AUDIO: sonidos=%d max_voces=%d descartados_rafaga=%d descartados_limite=%d\n",
                     started, maxVoices, droppedBurst, droppedLimit);
    }
    if (limiterAttached) { DetachAudioMixedProcessor(MasterLimiter); limiterAttached = false; }
    for (Clip& c : clips) {
        for (int i = c.voices - 1; i >= 1; --i) UnloadSoundAlias(c.voice[i]);
        if (c.voice[0].frameCount > 0) UnloadSound(c.voice[0]);
    }
    clips.clear();
    groups.clear();
    groupByPath.clear();
    clipByFile.clear();
    limiter.Clear();
    MusicPlayer::Get().Shutdown();   // sus streams se descargan antes de cerrar el dispositivo
    if (ambientReady) { UnloadMusicStream(ambient); ambientReady = false; }
    ready = false;
    persist = false;
    if (deviceOwned && IsAudioDeviceReady()) CloseAudioDevice();
    deviceOwned = false;
}

}  // namespace district_fury
