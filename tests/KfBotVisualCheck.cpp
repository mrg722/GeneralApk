// Herramienta (no es parte de ctest): verifica el bot de referencia KF del
// Laboratorio. Genera (1) una hoja con todos sus clips armados desde las piezas
// de la APK y (2) capturas de una pelea real en el Modo VS.
//   ./build/kf_bot_visual_check [carpeta_salida]
#include "game/VSMode.h"
#include "game/lab/KfReference.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace district_fury;

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : ".";
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "KF bot visual check");
    AssetManager::Get().LoadAll();
    int shots = 0;
    auto Capture = [&](const char* name) {
        const std::string file = std::to_string(++shots) + "_" + name + ".png";
        TakeScreenshot(file.c_str());
        if (out != ".") std::rename(file.c_str(), (out + "/" + file).c_str());
        std::printf("captura: %s/%s\n", out.c_str(), file.c_str());
    };

    // Todos los personajes del roster: carga y hoja con su reposo.
    BeginDrawing();
    ClearBackground({40, 44, 54, 255});
    for (int i = 0; i < KfRosterCount(); ++i) {
        const KfReference& k = GetKfCharacter(i);
        std::printf("roster %2d %-24s %s frames=%zu clips=%zu\n", i, KfRoster(i).name, k.loaded ? "OK" : k.error.c_str(), k.templ.frames.size(), k.clipNames.size());
        if (!k.loaded) continue;
        const float x = 70.f + (i % 6) * 200.f, y = 300.f + (i / 6) * 330.f;
        Animator a = k.templ; a.PlayNamed("idle"); a.Draw({x, y}, 2.4f, false);
        Animator b = k.templ; b.PlayNamed("atk2"); b.Update(0.25f); b.Draw({x + 100.f, y}, 2.4f, false);
        DrawText(KfRoster(i).name, (int)x - 40, (int)y + 10, 12, {255, 220, 90, 255});
    }
    EndDrawing();
    Capture("roster_kf");
    const KfReference& ref = GetKfReference();
    std::printf("referencia: %s, %zu clips, %zu frames\n", ref.loaded ? "cargada" : ref.error.c_str(), ref.clipNames.size(), ref.templ.frames.size());
    if (!ref.loaded) { CloseWindow(); return 1; }

    // (1) Hoja de clips: cada fila = un clip, sus frames en orden.
    BeginDrawing();
    ClearBackground({40, 44, 54, 255});
    int row = 0;
    for (const std::string& name : ref.clipNames) {
        const AnimationClip& c = ref.templ.namedClips.at(name);
        DrawText(name.c_str(), 6, 8 + row * 47, 10, {255, 220, 90, 255});
        for (size_t k = 0; k < c.frames.size() && k < 22; ++k) {
            Animator a = ref.templ;
            a.currentFrame = c.frames[k];
            a.Draw({80.f + k * 54.f, 44.f + row * 47.f}, 0.62f, false);
        }
        ++row;
    }
    EndDrawing();
    Capture("clips_bot_kf");

    // (2) Pelea en el Laboratorio: Rayden (entrada inyectada) vs bot KF.
    VSMode vs;
    vs.Init();
    vs.StartLabForTest(true, StreetEnemyType::UrbanNinja, argc > 2 ? std::atoi(argv[2]) : 0, argc > 3 ? std::atoi(argv[3]) : 0);
    PlayerInput in;
    vs.PlayerRef().scriptedInput = &in;
    vs.PlayerRef().debugInvulnerable = false;
    const char* seen[] = {"walk", "atk1", "atk2", "hit", "air", "special", "defeat"};
    bool got[7] = {};
    for (int f = 0; f < 60 * 90 && !WindowShouldClose(); ++f) {
        in = PlayerInput{};
        const auto& es = vs.Enemies();
        if (!es.empty()) {
            const StreetEnemy& e = es[0];
            const Player& p = vs.PlayerRef();
            const float dx = e.position.x - p.position.x, dy = e.position.y - p.position.y;
            if (std::fabs(dy) > 12) in.moveY = dy > 0 ? 1.f : -1.f;
            if (std::fabs(dx) > 95) in.moveX = dx > 0 ? 1.f : -1.f;
            else if ((f / 8) % 5 != 4) { if (f % 8 == 0) in.punch = (f / 8) % 4 != 3; if (f % 8 == 0 && (f / 8) % 4 == 3) in.kick = true; }
        }
        vs.Update(1.0f / 60.0f);
        BeginDrawing(); ClearBackground(BLACK); vs.Draw(); EndDrawing();
        if (!es.empty())
            for (int k = 0; k < 7; ++k)
                if (!got[k] && es[0].skinClip == seen[k] && es[0].skinAnimator.clipFrameIndex >= 1) { got[k] = true; Capture((std::string("pelea_") + seen[k]).c_str()); }
        if (!es.empty() && es[0].IsDefeated() && got[6]) break;
    }
    CloseWindow();
    return 0;
}
