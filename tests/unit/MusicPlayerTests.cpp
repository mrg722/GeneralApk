// F1.12: maquina de estados del reproductor de musica (logica pura, sin
// dispositivo de audio) y existencia de los archivos que resuelve.
#include "audio/MusicPlayer.h"
#include "core/Platform.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace district_fury;

namespace {

GameEvent Ev(GameEventType t, int value = 0) {
    GameEvent e;
    e.type = t;
    e.value = value;
    return e;
}

void Mode(MusicPlayer& m, GameMode mode) { m.OnEvent(Ev(GameEventType::ModeEnter, static_cast<int>(mode))); }

void Run(MusicPlayer& m, float seconds, float dt = 1.0f / 60.0f) {
    for (float t = 0; t < seconds - 1e-6f; t += dt) m.Update(dt);
}

float GainOf(const MusicPlayer& m, const char* file) {
    float g = -1.0f;
    for (const MusicVoice& v : m.Voices())
        if (std::strcmp(v.file, file) == 0) g = v.gain;
    return g;
}

bool Near(float a, float b, float eps = 0.02f) { return std::fabs(a - b) <= eps; }

void TestFilesExist() {
    for (int t = 1; t < static_cast<int>(MusicTheme::Count); ++t) {
        const MusicTheme th = static_cast<MusicTheme>(t);
        assert(MusicPlayer::ThemeLayers(th) >= 1);
        for (int l = 0; l < MusicPlayer::ThemeLayers(th); ++l) {
            const char* f = MusicPlayer::ThemeFile(th, l);
            const std::string p = MusicPlayer::ResolveMusicPath(f);
            if (p.empty()) std::printf("falta assets/audio/music/%s\n", f);
            assert(!p.empty());
        }
    }
    for (MusicStinger s : {MusicStinger::Victoria, MusicStinger::Derrota})
        assert(!MusicPlayer::ResolveMusicPath(MusicPlayer::StingerFile(s)).empty());
    for (int m = 0; m <= static_cast<int>(GameMode::Beta); ++m) {
        const char* a = MusicPlayer::AmbientFileForMode(static_cast<GameMode>(m));
        if (a) assert(platform::AssetExists(a));
    }
    // Stages y jefe tienen 2 capas; menu y VS una.
    assert(MusicPlayer::ThemeLayers(MusicTheme::Stage1) == 2 && MusicPlayer::ThemeLayers(MusicTheme::Boss) == 2);
    assert(MusicPlayer::ThemeLayers(MusicTheme::Menu) == 1 && MusicPlayer::ThemeLayers(MusicTheme::VS) == 1);
    assert(MusicPlayer::ThemeLayers(MusicTheme::None) == 0);
}

void TestModeMapping() {
    assert(MusicPlayer::ThemeForMode(GameMode::Menu) == MusicTheme::Menu);
    assert(MusicPlayer::ThemeForMode(GameMode::Reward) == MusicTheme::Menu);
    assert(MusicPlayer::ThemeForMode(GameMode::Stage1) == MusicTheme::Stage1);
    assert(MusicPlayer::ThemeForMode(GameMode::Stage5) == MusicTheme::Stage5);
    assert(MusicPlayer::ThemeForMode(GameMode::Boss) == MusicTheme::Boss);
    assert(MusicPlayer::ThemeForMode(GameMode::VS) == MusicTheme::VS);
    assert(MusicPlayer::ThemeForMode(GameMode::Beta) == MusicTheme::None);
    assert(MusicPlayer::ThemeForMode(GameMode::Boot) == MusicTheme::None);
}

void TestModeEnterAndCrossfade() {
    MusicPlayer& m = MusicPlayer::Get();
    m.Reset();
    Mode(m, GameMode::Menu);
    assert(m.CurrentTheme() == MusicTheme::Menu);
    assert(m.Voices().size() == 1);
    Run(m, 0.5f);
    assert(GainOf(m, "menu.ogg") > 0.5f && GainOf(m, "menu.ogg") < 0.8f);   // a mitad del fundido
    Run(m, 0.6f);
    assert(Near(GainOf(m, "menu.ogg"), 1.0f));
    // Menu -> Stage1: el menu sale en 1 s mientras entran las dos capas.
    Mode(m, GameMode::Stage1);
    assert(m.CurrentTheme() == MusicTheme::Stage1);
    std::vector<MusicVoice> v = m.Voices();
    assert(v.size() == 3);   // 2 capas + 1 saliente (limite de streams)
    Run(m, 0.5f);
    assert(GainOf(m, "menu.ogg") > 0.0f && GainOf(m, "stage1_calma.ogg") > 0.0f);
    Run(m, 0.6f);
    assert(GainOf(m, "menu.ogg") < 0.0f);   // ya no esta
    assert(Near(GainOf(m, "stage1_calma.ogg"), 1.0f));
    assert(Near(GainOf(m, "stage1_combate.ogg"), 0.0f));
    assert(m.Voices().size() == 2);
    // Mismo tema otra vez: no reinicia.
    Mode(m, GameMode::Stage1);
    assert(Near(m.ThemeFade(), 1.0f));
}

void TestArenaLayers() {
    MusicPlayer& m = MusicPlayer::Get();
    m.Reset();
    Mode(m, GameMode::Stage2);
    Run(m, 1.2f);
    m.OnEvent(Ev(GameEventType::ArenaLock, -1));
    assert(Near(m.LayerTarget(), 1.0f));
    Run(m, 0.5f);
    assert(m.LayerMix() > 0.4f && m.LayerMix() < 0.6f);            // rampa de 1 s
    const float a = GainOf(m, "stage2_calma.ogg"), b = GainOf(m, "stage2_combate.ogg");
    assert(Near(a * a + b * b, 1.0f, 0.03f));                        // potencia constante
    Run(m, 0.6f);
    assert(Near(m.LayerMix(), 1.0f));
    assert(Near(GainOf(m, "stage2_combate.ogg"), 1.0f) && Near(GainOf(m, "stage2_calma.ogg"), 0.0f));
    m.OnEvent(Ev(GameEventType::ArenaUnlock, -1));
    Run(m, 1.05f);
    assert(Near(m.LayerMix(), 0.0f));
    assert(Near(GainOf(m, "stage2_calma.ogg"), 1.0f));
    // En el menu ArenaLock no hace nada.
    Mode(m, GameMode::Menu);
    m.OnEvent(Ev(GameEventType::ArenaLock, 1));
    assert(Near(m.LayerTarget(), 0.0f));
}

void TestBossLayers() {
    MusicPlayer& m = MusicPlayer::Get();
    m.Reset();
    Mode(m, GameMode::Stage3);
    Run(m, 1.2f);
    m.OnEvent(Ev(GameEventType::ArenaLock, -1));
    Run(m, 1.2f);
    // Fase 1 (o ModeEnter Boss) -> tema del jefe, capa A.
    m.OnEvent(Ev(GameEventType::BossPhase, 1));
    assert(m.CurrentTheme() == MusicTheme::Boss && Near(m.LayerTarget(), 0.0f));
    assert(m.Voices().size() <= 3);
    Run(m, 1.1f);
    assert(Near(GainOf(m, "boss_a.ogg"), 1.0f) && GainOf(m, "stage3_combate.ogg") < 0.0f);
    m.OnEvent(Ev(GameEventType::ArenaUnlock, -1));   // en el jefe no cambia de capa
    assert(Near(m.LayerTarget(), 0.0f));
    m.OnEvent(Ev(GameEventType::BossPhase, 2));       // entra la capa B
    assert(Near(m.LayerTarget(), 1.0f));
    Run(m, 1.05f);
    assert(Near(GainOf(m, "boss_b.ogg"), 1.0f));
    m.OnEvent(Ev(GameEventType::BossPhase, 3));
    assert(Near(m.LayerTarget(), 1.0f));
    // Sin evento de inicio del jefe: la primera fase >= 2 arranca el tema directo en B.
    m.Reset();
    Mode(m, GameMode::Stage5);
    Run(m, 1.2f);
    m.OnEvent(Ev(GameEventType::BossPhase, 2));
    assert(m.CurrentTheme() == MusicTheme::Boss && Near(m.LayerMix(), 1.0f));
    // ModeEnter Boss
    m.Reset();
    Mode(m, GameMode::Boss);
    assert(m.CurrentTheme() == MusicTheme::Boss && Near(m.LayerTarget(), 0.0f));
}

void TestStingers() {
    MusicPlayer& m = MusicPlayer::Get();
    m.Reset();
    Mode(m, GameMode::Stage4);
    Run(m, 1.2f);
    m.OnEvent(Ev(GameEventType::StageClear, 4));
    assert(m.CurrentStinger() == MusicStinger::Victoria);
    assert(m.CurrentTheme() == MusicTheme::None);
    assert(m.Voices().size() == 2);                    // stinger + capa saliente
    assert(Near(GainOf(m, "victoria.ogg"), 1.0f));
    Run(m, 1.1f);
    assert(m.Voices().size() == 1);
    m.OnEvent(Ev(GameEventType::Victory, 0));          // repetido: no reinicia
    Run(m, MusicPlayer::StingerSeconds(MusicStinger::Victoria));
    assert(m.CurrentStinger() == MusicStinger::None);
    assert(m.Voices().empty());                        // luego silencio
    // Derrota y reintento: el siguiente ArenaLock vuelve a la musica del stage.
    m.Reset();
    Mode(m, GameMode::Stage1);
    Run(m, 1.2f);
    m.OnEvent(Ev(GameEventType::GameOver, static_cast<int>(GameMode::Stage1)));
    assert(m.CurrentStinger() == MusicStinger::Derrota && GainOf(m, "derrota.ogg") > 0.99f);
    Run(m, 5.0f);
    assert(m.Voices().empty());
    m.OnEvent(Ev(GameEventType::ArenaLock, 1));
    assert(m.CurrentTheme() == MusicTheme::Stage1 && Near(m.LayerMix(), 1.0f));
    // ModeEnter durante el stinger: el tema nuevo entra al terminar.
    m.Reset();
    Mode(m, GameMode::VS);
    Run(m, 1.2f);
    m.OnEvent(Ev(GameEventType::Victory, static_cast<int>(GameMode::VS)));
    Mode(m, GameMode::Menu);
    assert(m.CurrentStinger() == MusicStinger::Victoria && m.CurrentTheme() == MusicTheme::None);
    Run(m, 4.7f);
    assert(m.CurrentTheme() == MusicTheme::Menu);
}

void TestStreamLimit() {
    // Cambios rapidos de tema: nunca mas de 2 capas + 1 saliente.
    MusicPlayer& m = MusicPlayer::Get();
    m.Reset();
    const GameMode seq[] = {GameMode::Stage1, GameMode::Stage2, GameMode::Boss, GameMode::Menu, GameMode::Stage5};
    for (GameMode g : seq) {
        Mode(m, g);
        m.OnEvent(Ev(GameEventType::ArenaLock, -1));
        Run(m, 0.3f);
        assert(m.Voices().size() <= 3);
    }
    m.OnEvent(Ev(GameEventType::GameOver, 0));
    assert(m.Voices().size() <= 2);
    assert(m.ActiveStreams() == 0);   // sin dispositivo no se abre nada
}

void TestVolume() {
    MusicPlayer& m = MusicPlayer::Get();
    m.Reset();
    m.SetVolume(1.5f);
    assert(Near(m.Volume(), 1.0f));
    m.SetDuck(0.4f);
    assert(Near(m.Duck(), 0.4f));
    m.SetDuck(-1.0f);
    assert(Near(m.Duck(), 0.0f));
    m.SetDuck(1.0f);
    m.SetVolume(0.7f);
}

}  // namespace

int main() {
    TestFilesExist();
    TestModeMapping();
    TestModeEnterAndCrossfade();
    TestArenaLayers();
    TestBossLayers();
    TestStingers();
    TestStreamLimit();
    TestVolume();
    MusicPlayer::Get().Reset();
    std::printf("MusicPlayerTests OK\n");
    return 0;
}
