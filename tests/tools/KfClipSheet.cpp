// Herramienta (F1.6): hoja de clips de un personaje KF con el numero de cuadro
// y su duracion, para elegir subrangos (from/to) y comprobar clips derivados.
// Uso (desde la raiz del repo): kf_clip_sheet <dir_salida> <skin> [clip ...]
// Sin clips: todos los del personaje. Escribe <dir>/clips_<skin>_<n>.png.
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>
using namespace district_fury;
int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "uso: kf_clip_sheet <dir> <skin> [clip...]\n"); return 2; }
    const std::string out = argv[1];
    const int skin = std::atoi(argv[2]);
    std::filesystem::create_directories(out);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(640, 360, "clips");
    AssetManager::Get().LoadAll();
    Player p;
    p.ApplyCharacter(skin);
    p.Update(0.0f);
    const Animator& a = p.animator;
    std::vector<std::string> clips;
    for (int i = 3; i < argc; ++i) clips.push_back(argv[i]);
    if (clips.empty()) for (const auto& kv : a.namedClips) clips.push_back(kv.first);
    std::sort(clips.begin(), clips.end());
    const float scale = GetCharacterVisual(skin).kfRoster >= 0 ? 1.0f : 0.8f;
    const int cellW = 120, rowH = 130, cols = 20, perSheet = 14;
    for (size_t s0 = 0, n = 0; s0 < clips.size(); s0 += perSheet, ++n) {
        const size_t cnt = std::min<size_t>(perSheet, clips.size() - s0);
        const int W = 110 + cols * cellW, H = 30 + (int)cnt * rowH;
        RenderTexture2D rt = LoadRenderTexture(W, H);
        BeginTextureMode(rt);
        ClearBackground({34, 38, 48, 255});
        DrawText(TextFormat("%d %s", skin, GetCharacterVisual(skin).name), 8, 6, 18, {255, 220, 90, 255});
        for (size_t r = 0; r < cnt; ++r) {
            const std::string& nm = clips[s0 + r];
            auto it = a.namedClips.find(nm);
            const float y0 = 30.0f + r * rowH;
            DrawLine(0, (int)y0, W, (int)y0, {60, 66, 80, 255});
            DrawText(nm.c_str(), 6, (int)y0 + 4, 14, {190, 220, 235, 255});
            if (it == a.namedClips.end()) { DrawText("(no existe)", 6, (int)y0 + 22, 12, RED); continue; }
            const AnimationClip& c = it->second;
            float tot = 0;
            for (float d : c.durations) tot += d;
            DrawText(TextFormat("n=%d %.2fs", (int)c.frames.size(), tot), 6, (int)y0 + 22, 12, {160, 160, 170, 255});
            for (size_t k = 0; k < c.frames.size() && (int)k < cols; ++k) {
                const float cx = 110.0f + k * cellW + cellW * 0.5f;
                a.DrawFrame(c.frames[k], {cx, y0 + rowH - 14.0f}, scale, false);
                DrawText(TextFormat("%d", (int)k), (int)(cx - cellW * 0.45f), (int)y0 + 4, 12, YELLOW);
                if (k < c.durations.size()) DrawText(TextFormat("%.2f", c.durations[k]), (int)(cx + 8), (int)y0 + 4, 10, GRAY);
            }
        }
        EndTextureMode();
        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);
        ExportImage(img, (out + "/clips_" + std::to_string(skin) + "_" + std::to_string(n) + ".png").c_str());
        UnloadImage(img);
        UnloadRenderTexture(rt);
    }
    CloseWindow();
    return 0;
}
