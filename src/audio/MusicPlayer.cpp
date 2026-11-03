// Reproductor de musica (ver MusicPlayer.h). F1.12: logica pura de temas, capas y
// stingers + backend de streams de raylib (solo con dispositivo de audio).
#include "audio/MusicPlayer.h"
#include "core/Platform.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace district_fury {
namespace {
constexpr float kHalfPi = 1.57079632679f;

struct ThemeDef {
    const char* name;
    const char* layer0;
    const char* layer1;
};

const ThemeDef kThemes[] = {
    {"silencio", nullptr, nullptr},
    {"menu", "menu.ogg", nullptr},
    {"stage1", "stage1_calma.ogg", "stage1_combate.ogg"},
    {"stage2", "stage2_calma.ogg", "stage2_combate.ogg"},
    {"stage3", "stage3_calma.ogg", "stage3_combate.ogg"},
    {"stage4", "stage4_calma.ogg", "stage4_combate.ogg"},
    {"stage5", "stage5_calma.ogg", "stage5_combate.ogg"},
    {"jefe", "boss_a.ogg", "boss_b.ogg"},
    {"vs", "vs.ogg", nullptr},
};
static_assert(sizeof(kThemes) / sizeof(kThemes[0]) == static_cast<std::size_t>(MusicTheme::Count),
              "una entrada por MusicTheme");

const ThemeDef& Def(MusicTheme t) {
    const int i = static_cast<int>(t);
    return kThemes[(i >= 0 && i < static_cast<int>(MusicTheme::Count)) ? i : 0];
}

bool IsStageTheme(MusicTheme t) {
    return t == MusicTheme::Stage1 || t == MusicTheme::Stage2 || t == MusicTheme::Stage3 ||
           t == MusicTheme::Stage4 || t == MusicTheme::Stage5;
}

float Approach(float v, float target, float step) {
    if (v < target) return std::min(target, v + step);
    return std::max(target, v - step);
}

bool EnvDebugLog() {
    const char* v = std::getenv("DF_MUSICA_LOG");
    return v && *v && std::strcmp(v, "0") != 0;
}
}  // namespace

// --- backend raylib ----------------------------------------------------------

struct MusicPlayer::Stream {
    const char* file = nullptr;
    Music music{};
    bool loop = true;
    bool started = false;
};

MusicPlayer& MusicPlayer::Get() {
    static MusicPlayer instance;
    static bool envChecked = false;
    if (!envChecked) {
        envChecked = true;
        if (EnvDebugLog()) instance.debugLog = true;
    }
    return instance;
}

// --- tablas -------------------------------------------------------------------

MusicTheme MusicPlayer::ThemeForMode(GameMode mode) {
    switch (mode) {
        case GameMode::Menu:
        case GameMode::CharacterSelect:
        case GameMode::Options:
        case GameMode::Reward: return MusicTheme::Menu;
        case GameMode::Stage1: return MusicTheme::Stage1;
        case GameMode::Stage2: return MusicTheme::Stage2;
        case GameMode::Stage3: return MusicTheme::Stage3;
        case GameMode::Stage4: return MusicTheme::Stage4;
        case GameMode::Stage5: return MusicTheme::Stage5;
        case GameMode::Boss: return MusicTheme::Boss;
        case GameMode::VS: return MusicTheme::VS;
        case GameMode::Boot:
        case GameMode::Beta: return MusicTheme::None;   // BETA tiene su propio audio (F1.13)
    }
    return MusicTheme::None;
}

const char* MusicPlayer::ThemeName(MusicTheme t) { return Def(t).name; }

const char* MusicPlayer::ThemeFile(MusicTheme t, int layer) {
    if (layer == 0) return Def(t).layer0;
    if (layer == 1) return Def(t).layer1;
    return nullptr;
}

int MusicPlayer::ThemeLayers(MusicTheme t) { return Def(t).layer0 ? (Def(t).layer1 ? 2 : 1) : 0; }

const char* MusicPlayer::StingerFile(MusicStinger s) {
    switch (s) {
        case MusicStinger::Victoria: return "victoria.ogg";
        case MusicStinger::Derrota: return "derrota.ogg";
        case MusicStinger::None: break;
    }
    return nullptr;
}

float MusicPlayer::StingerSeconds(MusicStinger s) { return s == MusicStinger::None ? 0.0f : 4.6f; }

const char* MusicPlayer::AmbientFileForMode(GameMode mode) {
    switch (mode) {
        case GameMode::Menu:
        case GameMode::Stage1: return "assets/audio/sfx/world_rain_loop.wav";
        case GameMode::Stage2: return "assets/audio/ambient/amb_industrial.ogg";
        case GameMode::Stage3: return "assets/audio/ambient/amb_puerto.ogg";
        case GameMode::Stage4: return "assets/audio/ambient/amb_azotea.ogg";
        case GameMode::Stage5: return "assets/audio/ambient/amb_guarida.ogg";
        default: return nullptr;
    }
}

std::string MusicPlayer::ResolveMusicPath(const char* file) {
    if (!file) return "";
    for (const char* prefix : {"assets/audio/music/", "../assets/audio/music/", "../../assets/audio/music/"}) {
        const std::string p = std::string(prefix) + file;
        if (platform::AssetExists(p)) return p;
    }
    return "";
}

// --- logica -------------------------------------------------------------------

void MusicPlayer::Log(const char* what, const char* detail) const {
    if (!debugLog) return;
    std::fprintf(stderr, "MUSICA: %s%s%s\n", what, detail ? " " : "", detail ? detail : "");
}

void MusicPlayer::BeginOutgoing() {
    // El tema actual pasa a voz saliente: solo su capa dominante (limite de
    // streams); una saliente anterior se corta.
    if (theme == MusicTheme::None) return;
    const float in = std::sin(themeFade * kHalfPi);
    const bool b = ThemeLayers(theme) == 2 && layerMix > 0.5f;
    const float lg = ThemeLayers(theme) == 2 ? (b ? std::sin(layerMix * kHalfPi) : std::cos(layerMix * kHalfPi)) : 1.0f;
    outFile = ThemeFile(theme, b ? 1 : 0);
    outGain = in * lg;
    outFade = 1.0f;
}

void MusicPlayer::PlayMusic(MusicTheme t) {
    silencedByStinger = false;
    if (stinger != MusicStinger::None) {   // tras el stinger
        pendingTheme = t;
        return;
    }
    if (t == theme) return;
    BeginOutgoing();
    theme = t;
    themeFade = 0.0f;
    layerMix = layerTarget = 0.0f;
    Log(t == MusicTheme::None ? "silencio" : ThemeName(t), nullptr);
}

void MusicPlayer::Stop() { PlayMusic(MusicTheme::None); }

void MusicPlayer::PlayStinger(MusicStinger s) {
    if (s == MusicStinger::None || stinger == s) return;
    if (stinger == MusicStinger::None) BeginOutgoing();
    theme = MusicTheme::None;
    themeFade = 0.0f;
    layerMix = layerTarget = 0.0f;
    stinger = s;
    stingerElapsed = 0.0f;
    stingerLength = StingerSeconds(s);
    pendingTheme = MusicTheme::None;
    silencedByStinger = false;
    Log("stinger", s == MusicStinger::Victoria ? "victoria" : "derrota");
}

void MusicPlayer::Reset() {
    Shutdown();
    theme = MusicTheme::None;
    themeFade = layerMix = layerTarget = 0.0f;
    outFile = nullptr;
    outGain = outFade = 0.0f;
    stinger = MusicStinger::None;
    stingerElapsed = stingerLength = 0.0f;
    pendingTheme = MusicTheme::None;
    silencedByStinger = false;
    lastMode = GameMode::Boot;
    duck = 1.0f;
}

void MusicPlayer::OnEvent(const GameEvent& e) {
    switch (e.type) {
        case GameEventType::ModeEnter:
            lastMode = static_cast<GameMode>(e.value);
            PlayMusic(ThemeForMode(lastMode));
            break;
        case GameEventType::ArenaLock:
        case GameEventType::ArenaUnlock: {
            const bool lock = e.type == GameEventType::ArenaLock;
            // Reintento tras la derrota: el modo no cambia, pero vuelve el combate.
            if (lock && theme == MusicTheme::None && silencedByStinger && IsStageTheme(ThemeForMode(lastMode))) {
                PlayMusic(ThemeForMode(lastMode));
                layerMix = layerTarget = 1.0f;
                Log("capa", "combate");
                break;
            }
            if (!IsStageTheme(theme)) break;   // en el jefe/menu/VS no se cambia de capa
            const float target = lock ? 1.0f : 0.0f;
            if (target != layerTarget) {
                layerTarget = target;
                Log("capa", lock ? "combate" : "calma");
            }
            break;
        }
        case GameEventType::BossPhase: {
            const float target = e.value >= 2 ? 1.0f : 0.0f;
            if (stinger != MusicStinger::None) break;
            if (theme != MusicTheme::Boss) {
                PlayMusic(MusicTheme::Boss);
                layerMix = layerTarget = target;
            } else if (target != layerTarget) {
                layerTarget = target;
            }
            Log("capa", target > 0.5f ? "jefe B" : "jefe A");
            break;
        }
        case GameEventType::StageClear:
        case GameEventType::Victory: PlayStinger(MusicStinger::Victoria); break;
        case GameEventType::GameOver: PlayStinger(MusicStinger::Derrota); break;
        default: break;
    }
}

void MusicPlayer::Update(float dt) {
    if (dt < 0.0f) dt = 0.0f;
    themeFade = Approach(themeFade, theme == MusicTheme::None ? 0.0f : 1.0f, dt / kCrossfadeSeconds);
    layerMix = Approach(layerMix, layerTarget, dt / kLayerRampSeconds);
    if (outFile) {
        outFade = Approach(outFade, 0.0f, dt / kCrossfadeSeconds);
        if (outFade <= 0.0f) outFile = nullptr;
    }
    if (stinger != MusicStinger::None) {
        stingerElapsed += dt;
        if (stingerElapsed >= stingerLength) {
            stinger = MusicStinger::None;
            const MusicTheme next = pendingTheme;
            pendingTheme = MusicTheme::None;
            if (next != MusicTheme::None) {
                PlayMusic(next);
            } else {
                silencedByStinger = true;
                Log("silencio", nullptr);
            }
        }
    }
    SyncBackend();
}

void MusicPlayer::SetDuck(float gain) { duck = std::clamp(gain, 0.0f, 1.0f); }
void MusicPlayer::SetVolume(float v) { volume = std::clamp(v, 0.0f, 1.0f); }

std::vector<MusicVoice> MusicPlayer::Voices() const {
    std::vector<MusicVoice> out;
    const int layers = ThemeLayers(theme);
    const float in = std::sin(themeFade * kHalfPi);
    if (layers == 1) {
        out.push_back({ThemeFile(theme, 0), in, true, false});
    } else if (layers == 2) {
        out.push_back({ThemeFile(theme, 0), in * std::cos(layerMix * kHalfPi), true, false});
        out.push_back({ThemeFile(theme, 1), in * std::sin(layerMix * kHalfPi), true, false});
    }
    if (stinger != MusicStinger::None) out.push_back({StingerFile(stinger), 1.0f, false, false});
    if (outFile) {
        bool dup = false;   // la saliente puede ser el mismo archivo que una entrante
        for (const MusicVoice& v : out) dup = dup || std::strcmp(v.file, outFile) == 0;
        if (!dup) out.push_back({outFile, outGain * std::cos((1.0f - outFade) * kHalfPi), true, true});
    }
    return out;
}

int MusicPlayer::ActiveStreams() const { return static_cast<int>(streams.size()); }

void MusicPlayer::Shutdown() {
    for (Stream* s : streams) {
        if (IsAudioDeviceReady()) {
            StopMusicStream(s->music);
            UnloadMusicStream(s->music);
        }
        delete s;
    }
    streams.clear();
}

void MusicPlayer::SyncBackend() {
    if (!IsAudioDeviceReady()) {
        if (!streams.empty()) {   // el dispositivo se cerro: soltar sin llamar a raylib
            for (Stream* s : streams) delete s;
            streams.clear();
        }
        return;
    }
    const std::vector<MusicVoice> want = Voices();
    // 1) cerrar streams que ya no se piden
    for (std::size_t i = 0; i < streams.size();) {
        bool keep = false;
        for (const MusicVoice& v : want) keep = keep || std::strcmp(v.file, streams[i]->file) == 0;
        if (!keep) {
            StopMusicStream(streams[i]->music);
            UnloadMusicStream(streams[i]->music);
            delete streams[i];
            streams.erase(streams.begin() + static_cast<long>(i));
        } else {
            ++i;
        }
    }
    // 2) abrir los nuevos (las capas de un tema se abren en el mismo frame y
    //    arrancan juntas: quedan sincronizadas porque duran lo mismo)
    std::vector<Stream*> fresh;
    for (const MusicVoice& v : want) {
        bool have = false;
        for (Stream* s : streams) have = have || std::strcmp(v.file, s->file) == 0;
        if (have) continue;
        const std::string path = ResolveMusicPath(v.file);
        if (path.empty()) continue;   // sin archivo: silencio, nunca crash
        Music m = LoadMusicStream(path.c_str());
        if (m.frameCount == 0) continue;
        Stream* s = new Stream();
        s->file = v.file;
        s->music = m;
        s->loop = v.loop;
        s->music.looping = v.loop;
        streams.push_back(s);
        fresh.push_back(s);
        if (!v.loop && stinger != MusicStinger::None && std::strcmp(v.file, StingerFile(stinger)) == 0) {
            const float len = GetMusicTimeLength(m);
            if (len > 0.1f) stingerLength = len;
        }
    }
    // 3) volumen y avance de cada stream
    for (Stream* s : streams) {
        float g = 0.0f;
        for (const MusicVoice& v : want)
            if (std::strcmp(v.file, s->file) == 0) g = v.gain;
        SetMusicVolume(s->music, std::clamp(g * volume * duck, 0.0f, 1.0f));
    }
    for (Stream* s : fresh) {
        PlayMusicStream(s->music);
        s->started = true;
    }
    for (Stream* s : streams) UpdateMusicStream(s->music);
}

}  // namespace district_fury
