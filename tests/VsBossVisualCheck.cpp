// Herramienta (no es parte de ctest): pelea contra cada jefe del Modo VS y
// guarda capturas en reposo y atacando, para revisar como se ven.
//   ./build/vs_boss_visual_check [carpeta_salida] [personaje=0]
#include "game/CharacterVisual.h"
#include "game/VSMode.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace district_fury;

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : ".";
    const int character = argc > 2 ? std::atoi(argv[2]) : 0;
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "vs boss visual check");
    SetTargetFPS(0);
    AssetManager::Get().LoadAll();
    const char* names[] = {"brakk", "grinder", "titanx", "titanx_mejorado", "rayder_clone"};
    for (int b = 0; b < 5; ++b) {
        VSMode vs;
        vs.Init();
        vs.StartBossForTest(b, character);
        Player& p = vs.PlayerRef();
        p.debugInvulnerable = true;
        PlayerInput in;
        p.scriptedInput = &in;
        int shot = 0;
        for (int f = 0; f < 60 * 9; ++f) {
            vs.Update(1.0f / 60.0f);
            if (f % 90 == 45) {
                BeginDrawing();
                ClearBackground(BLACK);
                vs.Draw();
                EndDrawing();
                const std::string file = std::string(names[b]) + "_" + std::to_string(shot++) + ".png";
                TakeScreenshot(file.c_str());
                if (out != ".") std::rename(file.c_str(), (out + "/" + file).c_str());
            }
        }
        std::printf("%s: vida %d/%d fase %d\n", names[b], vs.BossRef().GetHp(), vs.BossRef().GetMaxHp(), vs.BossRef().GetPhase());
    }
    CloseWindow();
    return 0;
}
