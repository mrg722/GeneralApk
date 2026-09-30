// Recorre el Nivel 1 (Stage1StoryGame) de principio a fin con un bot que usa
// la misma entrada que el teclado (PlayerInput). Verifica que el nivel se puede
// terminar: 4 escenarios x 2 oleadas con bloqueo de camara y "GO >>",
// 4 guardianes y el jefe Brakk.
#include "game/Stage1StoryGame.h"
#include "Stage1Bot.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;

struct Result {
    bool cleared = false;
    bool died = false;
    int frames = 0;
    int locks = 0;
    int goShows = 0;
    int hpLeft = 0;
    int maxScenario = 0;
};

Result Play(StoryDifficulty difficulty, bool invulnerable, int maxSeconds) {
    Stage1StoryGame game;
    game.SetSavePath("stage1_playthrough_test_save.dat");
    game.StartRunForTest(difficulty);
    Player& p = game.PlayerRef();
    PlayerInput in;
    p.scriptedInput = &in;
    p.debugInvulnerable = invulnerable;

    Result r;
    bool wasLocked = false, goWasOn = false;
    Stage1Bot bot;
    bot.invulnerable = invulnerable;
    for (r.frames = 0; r.frames < maxSeconds * 60; ++r.frames) {
        const StoryFlow flow = game.Flow();
        if (flow == StoryFlow::StageClear) { r.cleared = true; break; }
        if (flow == StoryFlow::GameOver) { r.died = true; break; }
        bot.Think(game, in);
        game.Update(kDt);

        const bool locked = game.Arena().Locked();
        if (locked && !wasLocked) ++r.locks;
        wasLocked = locked;
        const bool goOn = game.Arena().GoTimer() > 0.0f;
        if (goOn && !goWasOn) ++r.goShows;
        goWasOn = goOn;
        r.maxScenario = std::max(r.maxScenario, game.Scenario());
        if (std::getenv("DF_TRACE") && r.frames % 300 == 0) {
            int alive = 0, pending = 0;
            for (const StreetEnemy& e : game.Enemies()) { if (e.active && !e.IsDefeated()) ++alive; if (!e.active && !e.IsDefeated()) ++pending; }
            std::fprintf(stderr, "t=%5.1f flow=%d esc=%d px=%.0f py=%.0f cam=%.0f lock=%d vivos=%d pendientes=%d hp=%d st=%d boss=%d\n",
                r.frames / 60.f, (int)game.Flow(), game.Scenario(), p.position.x, p.position.y, game.CameraX(),
                game.Arena().Locked(), alive, pending, p.hp, (int)p.state, game.Boss().hp);
        }
        // La camara nunca sale de los limites del nivel.
        assert(game.CameraX() >= 640.f - 0.01f && game.CameraX() <= 5360.f + 0.01f);
    }
    r.hpLeft = p.hp;
    return r;
}

void Report(const char* name, const Result& r) {
    std::fprintf(stderr, "%-26s cleared=%d died=%d time=%5.1fs escenario=%d bloqueos=%d GO=%d hp=%d\n",
                name, r.cleared, r.died, r.frames / 60.0f, r.maxScenario, r.locks, r.goShows, r.hpLeft);
}
}  // namespace

int main() {
    // 1) Validacion estructural: el nivel se puede completar (jugador invulnerable).
    const Result god = Play(StoryDifficulty::Normal, true, 900);
    Report("Normal (invulnerable)", god);
    assert(god.cleared);
    assert(god.maxScenario == 4);
    assert(god.locks >= 8 + 4);   // 2 oleadas + guardian por escenario
    assert(god.goShows >= 8);     // "GO >>" tras cada oleada limpia

    // 2) Partidas reales del bot (informativas: miden dificultad, no bloquean).
    Report("Facil (bot)", Play(StoryDifficulty::Easy, false, 900));
    Report("Normal (bot)", Play(StoryDifficulty::Normal, false, 900));
    std::puts("stage1_playthrough_test OK");
    return 0;
}
