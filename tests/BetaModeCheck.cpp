// Herramienta (no es parte de ctest): recorre el MODO BETA como un jugador.
// Carga los 15 personajes BETA y los 6 escenarios, juega las oleadas de cada
// escenario (el heroe busca al enemigo mas cercano y ataca) y un 1 VS 1 sin
// invulnerabilidad. Guarda capturas y falla si algo no carga o no avanza.
//   ./build/beta_mode_check [carpeta_salida]
#include "core/InputMap.h"
#include "game/CharacterVisual.h"
#include "game/beta/BetaMode.h"
#include "game/VSMode.h"
#include "ui/TouchControls.h"
#include "game/lab/KfReference.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace district_fury;

namespace {
std::string gOut = ".";
void Shot(const std::string& name) {
    const std::string file = name + ".png";
    TakeScreenshot(file.c_str());
    if (gOut != ".") std::rename(file.c_str(), (gOut + "/" + file).c_str());
}
void Frame(const BetaMode& g) { BeginDrawing(); ClearBackground(BLACK); g.Draw(); EndDrawing(); }
void Press(BetaMode& g, int key) {
    input::ClearNext(); input::SetVirtual(key, true); input::Commit();
    g.Update(1.0f / 60.0f);
    input::ClearNext(); input::Commit();
    g.Update(1.0f / 60.0f);
}

// Bot simple: camina hacia el enemigo vivo mas cercano y combina golpes y habilidades.
PlayerInput Bot(const BetaMode& g, const Player& p, int f) {
    PlayerInput in;
    const auto enemies = g.EnemiesForTest();
    if (enemies.empty()) {
        in.moveX = 1.0f;
        if (g.BossActive() && std::fabs(g.BossX() - p.position.x) < 260.0f) {
            in.moveX = 0.0f;
            in.punch = (f % 14) == 0;
            if ((f % 150) == 75) in.skill = (f / 150) % 3;
        }
        return in;
    }
    const Player* t = enemies[0];
    for (const Player* e : enemies)
        if (std::fabs(e->position.x - p.position.x) < std::fabs(t->position.x - p.position.x)) t = e;
    const float dx = t->position.x - p.position.x, dy = t->position.y - p.position.y;
    if (std::fabs(dx) > 110.0f) in.moveX = dx > 0 ? 1.0f : -1.0f;
    if (std::fabs(dy) > 14.0f) in.moveY = dy > 0 ? 1.0f : -1.0f;
    if (std::fabs(dx) < 200.0f && std::fabs(dy) < 30.0f) {
        if (in.moveX == 0.0f && (dx > 0) != (p.facing == Facing::Right)) in.moveX = dx > 0 ? 0.3f : -0.3f;
        in.punch = (f % 14) == 0;
        if ((f % 150) == 75) in.skill = (f / 150) % 3;
    }
    return in;
}

int Play(BetaMode& g, const std::string& tag, float seconds, bool invulnerable) {
    PlayerInput in;
    Player& p = g.PlayerRef();
    p.debugInvulnerable = invulnerable;
    p.scriptedInput = &in;
    const int frames = (int)(seconds * 60);
    int f = 0;
    for (; f < frames && !g.Won() && !g.Lost(); ++f) {
        in = Bot(g, g.PlayerRef(), f);
        input::ClearNext();
        // Remate: pulsa el boton a los 0.3 s de que aparece (como un jugador).
        static int waitFrames = 0;
        waitFrames = g.QteWaiting() ? waitFrames + 1 : 0;
        if (waitFrames == 18) input::SetVirtual(KEY_J, true);
        input::Commit();
        g.Update(1.0f / 60.0f);
        if (g.QteActive()) {
            static int qteShots = 0;
            if ((f % 90) == 0 && qteShots < 40) { Frame(g); Shot(tag + "_remate_" + std::to_string(qteShots++)); }
        }
        if (f % 420 == 200) { Frame(g); Shot(tag + "_" + std::to_string(f / 420)); }
    }
    Frame(g);
    Shot(tag + "_fin");
    std::printf("%-22s oleada=%d enemigos=%d vida=%d/%d x=%.0f cam=%.0f %s (%.1fs)\n", tag.c_str(), g.WaveIndex(),
                g.AliveEnemies(), p.hp, p.maxHp, p.position.x, g.CameraX(),
                g.Won() ? "VICTORIA" : g.Lost() ? "DERROTA" : "sin terminar", f / 60.0f);
    p.scriptedInput = nullptr;
    return g.Won() || g.Lost() ? 0 : 1;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc > 1) gOut = argv[1];
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "beta mode check");
    SetRandomSeed(20261002);
    SetTargetFPS(0);
    AssetManager::Get().LoadAll();
    int problems = 0;
    // 1) Los 15 personajes cargan, con clips y habilidades.
    for (int i = 0; i < 15; ++i) {
        const CharacterVisual& cv = GetCharacterVisual(FirstBetaCharacter() + i);
        const KfReference& r = GetKfCharacter(cv.kfRoster);
        std::printf("personaje %-30s %s clips=%zu habilidades=%zu sonidos=%zu\n", cv.name, r.loaded ? "OK" : r.error.c_str(),
                    r.clipNames.size(), r.abilityClips.size(), r.sounds.size());
        if (!r.loaded || !cv.beta) ++problems;
    }
    BetaMode g;
    g.Init();
    for (int c = 0; c < 4; ++c) { Frame(g); Shot("beta_sel_" + std::to_string(c)); Press(g, KEY_RIGHT); }
    const bool soloJefes = std::getenv("BETA_SOLO_JEFES") != nullptr;
    // 2) Oleadas en los 6 escenarios con las 4 armas del heroe.
    for (int s = 0; s < (soloJefes ? 0 : 6); ++s) {
        g.StartForTest(s % 4, s, false);
        if (!g.Stage().Loaded()) { std::printf("escenario %d NO CARGA\n", s); ++problems; continue; }
        problems += Play(g, "beta_oleadas_" + g.Stage().id, 240.0f, true);
        if (!g.Won()) ++problems;
    }
    // 3) 1 VS 1 real (sin invulnerabilidad): heroe contra medusa y un enemigo como jugador.
    g.StartForTest(0, 3, true, 13);
    Play(g, "beta_duelo_medusa", 120.0f, false);
    g.StartForTest(9, 1, true, 1);
    Play(g, "beta_duelo_centauro_vs_cestus", 120.0f, false);
    // 4) Jefes Spine en vivo (Poseidon, tentaculos, Titan).
    for (int b = 0; b < 3; ++b) {
        g.StartBossForTest(b, b == 2 ? 3 : b, b);
        if (!g.BossActive()) { std::printf("jefe %d NO CARGA\n", b); ++problems; continue; }
        problems += Play(g, std::string("beta_jefe_") + g.BossName(), 180.0f, true);
        if (!g.Won()) ++problems;
    }
    // 7) Controles tactiles originales (captura) con el guerrero.
    {
        touch::SetEnabled(true);
        g.StartForTest(0, 0, false);
        for (int f = 0; f < 120; ++f) { input::ClearNext(); input::Commit(); g.Update(1.0f / 60.0f); }
        BeginDrawing(); ClearBackground(BLACK); g.Draw(); touch::Draw(touch::Context::Beta, &g.PlayerRef()); EndDrawing();
        Shot("tactil_beta");
        touch::SetEnabled(false);
    }
    // 6) HISTORIA completa: tarjetas, 3 capitulos, remates y vuelta al menu.
    {
        g.StartStoryForTest(0);
        PlayerInput in;
        int f = 0, shots = 0, lastChapter = -1;
        for (; f < 60 * 900 && !g.ShouldExit(); ++f) {
            input::ClearNext();
            static int wait = 0;
            wait = g.QteWaiting() ? wait + 1 : 0;
            if (g.InStoryCard() && f % 90 == 60) input::SetVirtual(KEY_ENTER, true);
            if ((g.Won() || g.Lost()) && f % 60 == 30) input::SetVirtual(KEY_ENTER, true);
            if (wait == 18) input::SetVirtual(KEY_J, true);
            input::Commit();
            if (!g.InStoryCard() && !g.Won() && !g.Lost()) {
                Player& p = g.PlayerRef();
                p.debugInvulnerable = true;
                p.scriptedInput = &in;
                in = Bot(g, p, f);
            }
            g.Update(1.0f / 60.0f);
            if (g.StoryChapter() != lastChapter || (f % 1200 == 600 && shots < 30)) {
                lastChapter = g.StoryChapter();
                Frame(g);
                Shot("historia_" + std::to_string(shots++));
            }
        }
        std::printf("HISTORIA: capitulo final %d, vuelta al menu %s (%.0fs)\n", g.StoryChapter(), g.ShouldExit() ? "si" : "no", f / 60.0f);
        if (!g.ShouldExit()) ++problems;
        g.ClearExit();
    }
    // 5) Guerrero en el Modo VS: como jugador (cambia de arma con Q) contra el guerrero IA.
    {
        SetRandomSeed(7);
        VSMode vs;
        vs.Init();
        vs.StartRivalForTest(FirstBetaCharacter(), FirstBetaCharacter());
        Player& me = vs.PlayerRef();
        PlayerInput pin;
        me.scriptedInput = &pin;
        const int rivalStart = vs.RivalRef().hp;
        int weapons = 0, rivalWeapons = 0, lastRivalSkin = vs.RivalRef().skin;
        bool hurt = false;
        for (int f = 0; f < 60 * 30; ++f) {
            pin = PlayerInput{};
            const float dx = vs.RivalRef().position.x - me.position.x;
            pin.moveX = std::fabs(dx) > 120 ? (dx > 0 ? 1.0f : -1.0f) : 0.0f;
            pin.punch = (f % 16) == 0;
            input::ClearNext();
            if (f % 400 == 200) { input::SetVirtual(KEY_Q, true); ++weapons; }
            input::Commit();
            vs.Update(1.0f / 60.0f);
            if (vs.RivalRef().skin != lastRivalSkin) { ++rivalWeapons; lastRivalSkin = vs.RivalRef().skin; }
            if (me.hp < me.maxHp) hurt = true;
            if (f % 450 == 225) { BeginDrawing(); ClearBackground(BLACK); vs.Draw(); EndDrawing(); Shot("vs_guerrero_" + std::to_string(f / 450)); }
        }
        std::printf("VS guerrero: arma final %s, cambios jugador %d, cambios rival %d, rival %d->%d, jugador herido %s\n",
                    GetCharacterVisual(me.skin).name, weapons, rivalWeapons, rivalStart, vs.RivalRef().hp, hurt ? "si" : "no");
        if (vs.RivalRef().hp >= rivalStart || !hurt || rivalWeapons == 0) ++problems;
        me.scriptedInput = nullptr;
    }
    g.Shutdown();
    CloseWindow();
    std::printf("problemas: %d\n", problems);
    return problems == 0 ? 0 : 1;
}
