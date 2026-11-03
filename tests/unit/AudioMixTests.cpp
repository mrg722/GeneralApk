// F1.10: motor de mezcla y director de audio, sin dispositivo de audio.
// Prueba la logica pura (SfxBank.h: voces, prioridad, antirrafaga, variantes,
// pan, ducking, limitador, normalizacion), la persistencia de OPCIONES, la tabla
// evento -> sonido (todo evento suena o es "mudo intencional") y una sesion real
// del Stage 1 jugada por el bot, contando eventos y voces simultaneas.
#include "audio/AudioDirector.h"
#include "audio/AudioSettings.h"
#include "audio/AudioSystem.h"
#include "audio/SfxBank.h"
#include "game/CharacterVisual.h"
#include "game/combat/AttackData.h"
#include "game/events/GameEvents.h"
#include "game/Stage1StoryGame.h"
#include "../Stage1Bot.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace district_fury;

namespace {

// 20 Play del mismo golpe en 10 ms -> 1 sonido.
void TestBurst() {
    BurstGuard burst;
    VoiceLimiter lim;
    int sounded = 0;
    for (int i = 0; i < 20; ++i) {
        const double t = 1.0 + i * 0.0005;   // 20 disparos en 10 ms
        if (burst.Allow(7, t) && lim.Acquire(7, kPrioLight, t) >= 0) ++sounded;
    }
    assert(sounded == 1);
    assert(burst.Allow(7, 1.0 + 0.031));   // pasada la ventana de 30 ms vuelve a sonar
    assert(burst.Allow(8, 1.0));           // otra clave no se bloquea
}

// 16 golpes simultaneos -> 12 voces y los de prioridad 4 sobreviven.
void TestLimiter() {
    for (int order = 0; order < 3; ++order) {
        VoiceLimiter lim;
        int prio[16];
        for (int i = 0; i < 16; ++i) prio[i] = kPrioLight;
        // 4 de prioridad maxima: al principio, al final o intercalados.
        for (int k = 0; k < 4; ++k) prio[order == 0 ? k : order == 1 ? 12 + k : k * 4 + 1] = kPrioTop;
        for (int i = 0; i < 16; ++i) lim.Acquire(100 + i, prio[i], 2.0 + i * 1e-4);
        assert(lim.Active() == kMaxSfxVoices);
        int top = 0;
        for (int s = 0; s < kMaxSfxVoices; ++s) top += lim.Slot(s).priority == kPrioTop ? 1 : 0;
        assert(top == 4);
    }
    // Lleno de prioridad 4: un golpe ligero se descarta.
    VoiceLimiter full;
    for (int i = 0; i < kMaxSfxVoices; ++i) assert(full.Acquire(i, kPrioTop, i * 0.01) >= 0);
    assert(full.Acquire(99, kPrioLight, 1.0) == -1);
    bool stolen = false;
    const int s = full.Acquire(98, kPrioTop, 1.0, &stolen);
    assert(s == 0 && stolen);   // entre iguales se roba la mas vieja
    full.Release(3);
    assert(full.Active() == kMaxSfxVoices - 1);
}

// Variantes: nunca repite la anterior; alias libre o el mas viejo.
void TestVariants() {
    MixRng rng;
    for (int count = 2; count <= 4; ++count) {
        int last = -1, seen[4] = {};
        for (int i = 0; i < 2000; ++i) {
            const int v = PickVariant(count, last, rng);
            assert(v >= 0 && v < count && v != last);
            ++seen[v];
            last = v;
        }
        for (int v = 0; v < count; ++v) assert(seen[v] > 2000 / count / 2);   // todas salen
    }
    assert(PickVariant(1, 0, rng) == 0);
    const bool playing[4] = {true, false, true, true};
    const double start[4] = {1.0, 0.5, 0.2, 0.9};
    assert(PickAlias(playing, start, 4) == 1);
    const bool allBusy[4] = {true, true, true, true};
    assert(PickAlias(allBusy, start, 4) == 2);
}

// Pan dentro de [0.15, 0.85] para cualquier X; centro = 0.5.
void TestPan() {
    for (float x = -5000.0f; x <= 10000.0f; x += 37.0f) {
        const float p = PanForX(x, 1200.0f);
        assert(p >= 0.15f - 1e-6f && p <= 0.85f + 1e-6f);
    }
    assert(std::fabs(PanForX(1200.0f, 1200.0f) - 0.5f) < 1e-6f);
    assert(PanForX(1500.0f, 1200.0f) > 0.5f && PanForX(900.0f, 1200.0f) < 0.5f);
}

// Ducking: baja y vuelve a 1.0 en < 0.6 s (el mas largo de la tabla: -9 dB, 0.4 s).
void TestDucker() {
    const float depth[] = {6.0f, 7.0f, 8.0f, 9.0f};
    const float hold[] = {0.25f, 0.3f, 0.3f, 0.4f};
    for (int k = 0; k < 4; ++k) {
        Ducker d;
        d.Trigger(depth[k], hold[k]);
        assert(std::fabs(d.Gain() - DbToLinear(-depth[k])) < 1e-4f);
        float t = 0.0f;
        while (d.Gain() < 1.0f && t < 2.0f) { d.Update(1.0f / 60.0f); t += 1.0f / 60.0f; }
        assert(d.Gain() == 1.0f);
        assert(t < 0.6f);
    }
    Ducker d;
    d.Trigger(6.0f, 0.1f);
    d.Trigger(9.0f, 0.3f);   // se queda con la mas profunda y la espera mas larga
    assert(std::fabs(d.Gain() - DbToLinear(-9.0f)) < 1e-4f);
}

void TestLimiterAndNormalize() {
    assert(SoftClip(0.0f) == 0.0f);
    assert(std::fabs(SoftClip(1.0f) - 1.0f) < 1e-5f);
    for (float x = -8.0f; x <= 8.0f; x += 0.01f) assert(std::fabs(SoftClip(x)) <= 1.0f);
    float buf[6] = {3.0f, -3.0f, 0.5f, -0.5f, 0.0f, 1.0f};
    SoftClipBuffer(buf, 6);
    for (float v : buf) assert(std::fabs(v) <= 1.0f);
    // Seno de amplitud 1: RMS -3.01 dBFS.
    static float sine[44100];
    for (int i = 0; i < 44100; ++i) sine[i] = std::sin(2.0f * 3.14159265f * 440.0f * i / 44100.0f);
    const float st = St50Db(sine, 44100, 1, 44100);
    assert(std::fabs(st + 3.01f) < 0.1f);
    assert(std::fabs(NormalizeGainDb(st, -10.0f) + 6.99f) < 0.1f);
    assert(NormalizeGainDb(-60.0f, -10.0f) == 12.0f);   // acotado
    assert(NormalizeGainDb(-180.0f, -10.0f) == 0.0f);   // silencio: no se toca
    assert(TargetSt50Db(SoundCat::Hit) == -10.0f && TargetSt50Db(SoundCat::Ui) == -16.0f &&
           TargetSt50Db(SoundCat::Ambient) == -28.0f);
    assert(VoicesFor(SoundCat::Hit) == 4 && VoicesFor(SoundCat::Ui) == 2 && VoicesFor(SoundCat::Jingle) == 1);
}

void TestSettings() {
    using namespace audio_settings;
    Values v;
    v.music = 70; v.sfx = 40; v.ambient = 0; v.muted = true;
    const Values r = Parse(Serialize(v));
    assert(r.music == 70 && r.sfx == 40 && r.ambient == 0 && r.muted);
    assert(Snap(-5) == 0 && Snap(104) == 100 && Snap(64) == 60 && Snap(65) == 70);
    const Values d = Parse("basura\nmusic=33\n");
    assert(d.music == 30 && d.sfx == 100 && d.ambient == 100 && !d.muted);
    // Sin Init los buses quedan en 1 (contrato F0.1) y no se escribe nada a disco.
    AudioSystem& a = AudioSystem::Get();
    assert(a.BusVolume(Bus::Music) == 1.0f && a.EffectiveBus(Bus::Ui) == 1.0f);
    a.SetBusVolume(Bus::Sfx, 0.5f);
    assert(a.EffectiveBus(Bus::Ui) == 0.5f && a.EffectiveBus(Bus::Music) == 1.0f);
    a.SetBusVolume(Bus::Sfx, 1.0f);
}

void TestArchetypes() {
    auto find = [](const char* name) {
        for (int i = 0; i < CharacterCount(); ++i)
            if (std::strcmp(GetCharacterVisual(i).name, name) == 0) return i;
        return -1;
    };
    assert(ArchetypeForSkin(0) == Archetype::Brawler);   // RAYDEN (ORIGINAL)
    assert(ArchetypeForSkin(6) == Archetype::Max);       // MAX (otro personaje)
    assert(ArchetypeForSkin(22) == Archetype::Ronin);
    assert(ArchetypeForSkin(-1) == Archetype::Brawler);
    struct { const char* name; Archetype a; } kCases[] = {
        {"BRAKK", Archetype::Brawler}, {"CLON ANTIGUO (CHAQUETA)", Archetype::Brawler},
        {"KF MATON", Archetype::Brawler}, {"KF RUBIA", Archetype::Agil}, {"KF BUFONA", Archetype::Agil},
        {"KF HEROINA TRANSFORMADA", Archetype::Agil}, {"KF HEROE (PELO BLANCO)", Archetype::Agil},
        {"RAYDER CLON BETA", Archetype::Agil}, {"KF NAVAJERA", Archetype::Cuchillas},
        {"KF JEFE GARRA", Archetype::Cuchillas}, {"GUERRERO (ESPADAS DEL CAOS)", Archetype::Cuchillas},
        {"RONIN-KRATOS (GARRAS DE HADES)", Archetype::Cuchillas}, {"GRINDER", Archetype::Pesado},
        {"TITAN-X", Archetype::Pesado}, {"TITAN-X MEJORADO", Archetype::Pesado}, {"KF SOLDADO", Archetype::Pesado},
        {"CENTAURO ROJO", Archetype::Pesado}, {"GUERRERO (CESTUS DE NEMEA)", Archetype::Pesado},
        {"GUERRERO (CADENA DEL RAYO)", Archetype::Electrico}, {"RONIN-KRATOS (CADENA DEL RAYO)", Archetype::Electrico},
    };
    for (const auto& c : kCases) {
        const int s = find(c.name);
        assert(s >= 0);
        assert(ArchetypeForSkin(s) == c.a);
    }
    assert(std::strcmp(ArchetypeFolder(Archetype::Agil), "agil") == 0 && !ArchetypeFolder(Archetype::Max));
}

// Toda combinacion de evento suena (con respaldo legado) o tiene motivo de silencio.
void TestTable() {
    const HitKind kinds[] = {HitKind::Light, HitKind::Medium, HitKind::Heavy, HitKind::Kick, HitKind::Slash,
                             HitKind::Energy, HitKind::Block, HitKind::GuardBreak, HitKind::Prop};
    int checked = 0;
    for (int t = 0; t < static_cast<int>(GameEventType::Count); ++t)
        for (int skin = -1; skin < CharacterCount(); ++skin)
            for (int fe = 0; fe < 2; ++fe)
                for (HitKind k : kinds)
                    for (int heavy = 0; heavy < 2; ++heavy) {
                        GameEvent e;
                        e.type = static_cast<GameEventType>(t);
                        e.skin = skin; e.fromEnemy = fe != 0; e.kind = k; e.heavy = heavy != 0;
                        e.weight = heavy ? 3 : 1; e.x = 900.0f; e.y = 500.0f;
                        e.attack = static_cast<int>(AttackId::Punch2);
                        e.value = t == static_cast<int>(GameEventType::ArenaUnlock) ? (heavy ? -1 : 2) : 5;
                        SoundCue c;
                        const bool sounds = CueFor(e, c);
                        assert(sounds == (MuteReason(e) == nullptr));
                        if (!sounds) continue;
                        ++checked;
                        assert(c.fallback != Sfx::Count);   // si falta el banco nuevo, suena el WAV legado
                        assert(c.name || c.packName || c.cat == SoundCat::Ui);   // UI de menu: WAV ui_* actual
                        assert(c.priority >= kPrioUi && c.priority <= kPrioTop);
                        assert(c.pitch > 0.5f && c.pitch < 2.0f && c.pitchJitter <= (c.cat == SoundCat::Step ? 0.08f : 0.06f));
                        if (c.cat == SoundCat::Hit && c.priority >= kPrioHeavy && c.duckDb > 0.0f) assert(c.pitchJitter <= 0.04f);
                        if (c.cat == SoundCat::Ui || c.cat == SoundCat::Jingle) assert(c.pitchJitter == 0.0f);
                        if (c.duckDb > 0.0f) assert(c.duckDb >= 6.0f && c.duckDb <= 9.0f &&
                                                    c.duckHold >= 0.25f && c.duckHold <= 0.4f);
                    }
    assert(checked > 1000);

    // Casos concretos de la tabla.
    GameEvent e;
    SoundCue c;
    e.type = GameEventType::HitLanded; e.skin = 0; e.kind = HitKind::Light;
    assert(CueFor(e, c) && std::strcmp(c.name, "hit_light") == 0 && c.fallback == Sfx::Punch && !c.pack);
    e.skin = 6; e.attack = static_cast<int>(AttackId::Punch2);   // MAX: pack propio
    assert(CueFor(e, c) && std::strcmp(c.pack, "rayder_v2") == 0 && std::strcmp(c.packName, "golpe_2") == 0 &&
           c.fallback == Sfx::RayderGolpe2);
    e.skin = 22; e.kind = HitKind::Kick;                         // RONIN
    assert(CueFor(e, c) && std::strcmp(c.pack, "ronin_v2") == 0 && std::strcmp(c.packName, "patada_1") == 0);
    GameEvent d; d.type = GameEventType::Dash; d.skin = 22;
    assert(CueFor(d, c) && std::strcmp(c.packName, "espada_desenvaine") == 0 && c.packPitch == 1.3f);
    GameEvent h; h.type = GameEventType::PlayerHurt;
    assert(CueFor(h, c) && std::strcmp(c.name, "player_hurt") == 0 && c.duckDb > 0);
    h.type = GameEventType::PlayerKnockdown;
    assert(!CueFor(h, c) && MuteReason(h));
    h.type = GameEventType::GameOver;
    assert(CueFor(h, c) && std::strcmp(c.name, "game_over") == 0 && c.fallback == Sfx::GameOver);
    h.type = GameEventType::EnemyKilled;
    assert(CueFor(h, c) && std::strcmp(c.name, "ko_enemy") == 0 && c.priority == kPrioTop);
    h.type = GameEventType::ComboMilestone; h.value = 20;
    assert(CueFor(h, c) && std::strcmp(c.name, "combo_20") == 0);
    assert(RainForMode(GameMode::Menu) && RainForMode(GameMode::Stage1) && !RainForMode(GameMode::Stage2) &&
           !RainForMode(GameMode::VS));
}

// Sesion real: el bot juega el Stage 1 y cada frame se drena la cola por el
// director (como en el juego) con voces simuladas por duracion de categoria.
struct SessionReport {
    int events[static_cast<int>(GameEventType::Count)] = {};
    int cued[static_cast<int>(GameEventType::Count)] = {};
    int muted[static_cast<int>(GameEventType::Count)] = {};
    int started = 0, burst = 0, dropped = 0, maxVoices = 0;
};

float SimDuration(SoundCat c) {
    switch (c) {
        case SoundCat::Hit: return 0.28f;
        case SoundCat::Move: return 0.20f;
        case SoundCat::Step: return 0.08f;
        case SoundCat::Ui: return 0.12f;
        case SoundCat::Jingle: return 2.0f;
        case SoundCat::Voice: return 0.8f;
        case SoundCat::Ambient: return 0.0f;
    }
    return 0.3f;
}

void PlaySession(StoryDifficulty diff, bool invulnerable, SessionReport& rep) {
    ClearGameEvents();
    Stage1StoryGame game;
    game.SetSavePath("audio_mix_tests_save.dat");
    game.StartRunForTest(diff);
    Player& p = game.PlayerRef();
    PlayerInput in;
    p.scriptedInput = &in;
    p.debugInvulnerable = invulnerable;
    Stage1Bot bot;
    bot.invulnerable = invulnerable;
    VoiceLimiter lim;
    BurstGuard burst;
    double voiceEnd[kMaxSfxVoices] = {};
    const float dt = 1.0f / 60.0f;
    for (int f = 0; f < 900 * 60; ++f) {
        const StoryFlow flow = game.Flow();
        if (flow == StoryFlow::StageClear || flow == StoryFlow::GameOver) break;
        bot.Think(game, in);
        game.Update(dt);
        const double now = f * dt;
        for (int s = 0; s < kMaxSfxVoices; ++s)
            if (lim.Slot(s).used && voiceEnd[s] <= now) lim.Release(s);
        DrainGameEvents([&](const GameEvent& e) {
            const int t = static_cast<int>(e.type);
            ++rep.events[t];
            SoundCue c;
            if (!CueFor(e, c)) {
                assert(MuteReason(e));   // solo silencios intencionales
                ++rep.muted[t];
                return;
            }
            ++rep.cued[t];
            const int key = t * 64 + (c.name ? static_cast<int>(std::strlen(c.name)) : 0) % 64;
            if (!burst.Allow(key % BurstGuard::kMaxKeys, now)) { ++rep.burst; return; }
            const int s = lim.Acquire(key, c.priority, now);
            if (s < 0) { ++rep.dropped; return; }
            voiceEnd[s] = now + SimDuration(c.cat);
            ++rep.started;
            rep.maxVoices = std::max(rep.maxVoices, lim.Active());
        });
        assert(lim.Active() <= kMaxSfxVoices);
    }
    assert(DroppedGameEvents() == 0);
}

void TestSession() {
    SessionReport rep;
    PlaySession(StoryDifficulty::Normal, true, rep);    // llega al jefe y termina el nivel
    PlaySession(StoryDifficulty::Normal, false, rep);   // el bot muere: dano real al jugador
    std::fprintf(stderr, "%-18s %8s %8s %8s\n", "evento", "eventos", "suenan", "mudos");
    int combat = 0;
    for (int t = 0; t < static_cast<int>(GameEventType::Count); ++t) {
        if (!rep.events[t]) continue;
        std::fprintf(stderr, "%-18s %8d %8d %8d\n", GameEventName(static_cast<GameEventType>(t)), rep.events[t],
                     rep.cued[t], rep.muted[t]);
        combat += rep.events[t];
    }
    std::fprintf(stderr, "sonidos=%d antirrafaga=%d descartados_por_limite=%d max_voces=%d\n", rep.started,
                 rep.burst, rep.dropped, rep.maxVoices);
    assert(combat > 100);
    assert(rep.maxVoices <= kMaxSfxVoices);
    // Lo que se da y lo que se recibe suena siempre.
    assert(rep.cued[static_cast<int>(GameEventType::HitLanded)] > 0);
    assert(rep.cued[static_cast<int>(GameEventType::PlayerHurt)] > 0);
    assert(rep.cued[static_cast<int>(GameEventType::EnemyKilled)] > 0);
    assert(rep.cued[static_cast<int>(GameEventType::Swing)] > 0);
}

}  // namespace

int main() {
    TestBurst();
    TestLimiter();
    TestVariants();
    TestPan();
    TestDucker();
    TestLimiterAndNormalize();
    TestSettings();
    TestArchetypes();
    TestTable();
    TestSession();
    std::remove("audio_mix_tests_save.dat");
    std::puts("audio_mix_tests OK");
    return 0;
}
