// Herramienta (no es parte de ctest): juega el Nivel 1 con el bot en una
// ventana real y guarda capturas en los eventos clave. Ejecutar desde la raiz
// del repositorio para que encuentre assets/:
//   ./build/stage1_visual_check [carpeta_salida]
#include "Stage1Bot.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace district_fury;

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : ".";
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "District Fury - Stage1 visual check");
    SetTargetFPS(0);
    AssetManager::Get().LoadAll();

    Stage1StoryGame game;
    game.SetSavePath(out + "/visual_check_save.dat");
    game.StartRunForTest(StoryDifficulty::Normal);
    PlayerInput in;
    game.PlayerRef().scriptedInput = &in;
    game.PlayerRef().debugInvulnerable = true;
    Stage1Bot bot;
    bot.invulnerable = true;

    int shots = 0, goShots = 0;
    bool wasLocked = false, sawBoss = false, sawSubBoss = false, sawAir = false;
    auto Shot = [&](const char* name) {
        // raylib guarda la captura en el directorio de trabajo con el nombre de
        // archivo; luego se mueve a la carpeta pedida.
        const std::string file = std::to_string(++shots) + "_" + name + ".png";
        TakeScreenshot(file.c_str());
        const std::string path = out + "/" + file;
        if (out != "." && std::rename(file.c_str(), path.c_str()) != 0) std::printf("no se pudo mover %s\n", file.c_str());
        std::printf("captura: %s\n", path.c_str());
    };
    for (int frame = 0; frame < 60 * 600 && !WindowShouldClose(); ++frame) {
        const bool running = bot.Think(game, in);
        game.Update(1.0f / 60.0f);
        BeginDrawing();
        ClearBackground(BLACK);
        game.Draw();
        EndDrawing();
        const ArenaDirector& a = game.Arena();
        if (a.Locked() && !wasLocked && shots < 3) Shot("bloqueo_oleada");
        wasLocked = a.Locked();
        if (a.GoVisible() && a.GoTimer() < 2.9f && goShots < 2) { Shot("go"); ++goShots; }
        if (game.Flow() == StoryFlow::SubBossIntro && !sawSubBoss) { sawSubBoss = true; Shot("guardian"); }
        for (const StreetEnemy& e : game.Enemies())
            if (!sawAir && e.state == StreetEnemyState::Airborne && e.position.z > 40.f) { sawAir = true; Shot("enemigo_en_el_aire"); }
        if (game.Flow() == StoryFlow::Boss && !sawBoss && game.Boss().hp < game.Boss().maxHp * 0.8f) { sawBoss = true; Shot("brakk"); }
        if (!running) { Shot(game.Flow() == StoryFlow::StageClear ? "nivel_completado" : "game_over"); break; }
    }
    AssetManager::Get().UnloadAll();
    CloseWindow();
    return 0;
}
