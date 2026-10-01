// Herramienta (no es parte de ctest): genera docs/personajes_jugables/ con una
// hoja por personaje jugable: cada fila es un clip (reposo, golpes, habilidades,
// transformacion...) dibujado cuadro a cuadro tal como lo usa el juego.
//   ./build/roster_sheets [carpeta_salida]
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using namespace district_fury;

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : "docs/personajes_jugables";
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(640, 360, "roster sheets");
    AssetManager::Get().LoadAll();
    std::FILE* index = std::fopen((out + "/LISTA.md").c_str(), "w");
    if (index) std::fprintf(index, "# Personajes jugables\n\n| # | Personaje | Clips | Hoja |\n|---|---|---|---|\n");
    for (int id = 0; id < CharacterCount(); ++id) {
        Player p;
        p.ApplyCharacter(id);
        p.Update(0.0f);
        const Animator& a = p.animator;
        const CharacterVisual& cv = GetCharacterVisual(id);
        std::vector<std::string> clips;
        for (const auto& kv : a.namedClips) clips.push_back(kv.first);
        std::sort(clips.begin(), clips.end());
        std::string file = std::to_string(id) + "_" + cv.name + ".png";
        for (char& c : file) if (c == ' ' || c == '(' || c == ')' || c == '/') c = '_';
        const bool poses = id != 0 && cv.folder != nullptr && cv.atlasId == nullptr;
        if (poses || a.texture.id == 0 || clips.empty()) {
            if (index) std::fprintf(index, "| %d | %s | poses sueltas (assets/bosses/%s) | - |\n", id, cv.name, cv.folder ? cv.folder : "-");
            continue;
        }
        const bool kf = cv.kfRoster >= 0;
        const float scale = kf ? 1.4f : 0.8f;
        const int cellW = kf ? 150 : 140, rowH = kf ? 150 : 130, maxCols = 16;
        int cols = 1;
        for (const auto& n : clips) {
            const AnimationClip& c = a.namedClips.at(n);
            const int count = c.frames.empty() ? c.endFrame - c.startFrame + 1 : (int)c.frames.size();
            cols = std::max(cols, std::min(maxCols, count));
        }
        const int W = 130 + cols * cellW, H = 40 + (int)clips.size() * rowH;
        RenderTexture2D rt = LoadRenderTexture(W, H);
        BeginTextureMode(rt);
        ClearBackground({34, 38, 48, 255});
        DrawText(TextFormat("%s  (%zu clips)", cv.name, clips.size()), 10, 10, 20, {255, 220, 90, 255});
        for (size_t r = 0; r < clips.size(); ++r) {
            const AnimationClip& c = a.namedClips.at(clips[r]);
            std::vector<int> fr = c.frames;
            if (fr.empty()) for (int f = c.startFrame; f <= c.endFrame; ++f) fr.push_back(f);
            const float y = 40.0f + r * rowH + rowH - 12.0f;
            DrawText(clips[r].c_str(), 8, (int)(y - rowH / 2), 14, {190, 220, 235, 255});
            DrawLine(0, (int)(40 + r * rowH), W, (int)(40 + r * rowH), {60, 66, 80, 255});
            for (size_t k = 0; k < fr.size() && (int)k < maxCols; ++k)
                a.DrawFrame(fr[k], {130.0f + k * cellW + cellW * 0.5f, y}, scale, false);
        }
        EndTextureMode();
        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);
        ExportImage(img, (out + "/" + file).c_str());
        UnloadImage(img);
        UnloadRenderTexture(rt);
        std::string list;
        for (const auto& n : clips) list += (list.empty() ? "" : ", ") + n;
        if (index) std::fprintf(index, "| %d | %s | %s | [%s](%s) |\n", id, cv.name, list.c_str(), file.c_str(), file.c_str());
        std::printf("%s: %zu clips\n", cv.name, clips.size());
    }
    if (index) std::fclose(index);
    CloseWindow();
    return 0;
}
