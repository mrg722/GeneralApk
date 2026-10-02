#include "game/beta/BetaStage.h"
#include "core/Platform.h"
#include "game/Types.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace district_fury {

bool BetaStage::Load(const std::string& stageId) {
    *this = BetaStage{};
    std::string text;
    const std::string rel = "data/beta/stages/" + stageId + ".txt";
    for (const std::string& p : {rel, "../" + rel, "../../" + rel})
        if (platform::LoadTextFile(p, text)) break;
    if (text.empty()) return false;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "stage") ls >> id;
        else if (tag == "name") std::getline(ls >> std::ws, name);
        else if (tag == "size") ls >> width >> height;
        else if (tag == "walk") ls >> walkY0 >> walkY1 >> walkX0 >> walkX1;
        else if (tag == "music") ls >> music;
        else if (tag == "weather") ls >> weather;
        else if (tag == "sky") {
            int r = 0, g = 0, b = 0;
            ls >> r >> g >> b;
            sky = Color{(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
        } else if (tag == "chunk") {
            BetaStageChunk c;
            ls >> c.file >> c.x;
            c.tex = AssetManager::Get().GetTextureByPath("assets/beta/stages/" + c.file);
            chunks.push_back(c);
        } else if (tag == "layer") {
            // El nombre puede no tener espacios (el generador los quita).
            BetaStageLayer l;
            int rep = 1;
            ls >> l.file >> l.parallax >> l.y >> rep >> l.gap;
            l.repeat = rep != 0;
            l.tex = AssetManager::Get().GetTextureByPath("assets/beta/stages/" + l.file);
            layers.push_back(l);
        }
    }
    if (width <= 0 || height <= 0 || chunks.empty()) return false;
    // La franja caminable del mapa (walkY0..walkY1) cae sobre el carril del juego.
    mapTop = kLaneMinY - walkY0 * kScale;
    const float mapBottom = mapTop + height * kScale;
    cameraDy = 720.0f - mapBottom;
    loaded = true;
    return true;
}

void BetaStage::DrawBack(float cameraX) const {
    DrawRectangle(0, 0, 1280, 720, sky);
    const float top = mapTop + cameraDy;
    for (const BetaStageLayer& l : layers) {
        if (!l.tex.id) continue;
        const float w = l.tex.width * kScale, h = l.tex.height * kScale;
        const float step = w + l.gap * kScale;
        // El fondo lejano (primera capa) cubre desde el borde de la pantalla
        // aunque el mapa sea mas bajo que 720 (arenas de 528 px).
        const float y = (&l == &layers.front() ? std::min(0.0f, top) : top) + l.y * kScale;
        float x0 = -cameraX * l.parallax;
        if (!l.repeat) {
            DrawTexturePro(l.tex, {0, 0, (float)l.tex.width, (float)l.tex.height}, {x0, y, w, h}, {0, 0}, 0, WHITE);
            continue;
        }
        x0 = std::fmod(x0, step);
        if (x0 > 0) x0 -= step;
        for (float x = x0; x < 1280.0f; x += step)
            DrawTexturePro(l.tex, {0, 0, (float)l.tex.width, (float)l.tex.height}, {x, y, w, h}, {0, 0}, 0, WHITE);
    }
    for (const BetaStageChunk& c : chunks) {
        if (!c.tex.id) continue;
        const float x = c.x * kScale - cameraX;
        const float w = c.tex.width * kScale;
        if (x > 1280.0f || x + w < 0.0f) continue;
        DrawTexturePro(c.tex, {0, 0, (float)c.tex.width, (float)c.tex.height}, {x, top, w, c.tex.height * kScale},
                       {0, 0}, 0, WHITE);
    }
}

void BetaStage::DrawWeather(float cameraX, float time) const {
    // Clima ligero sobre la escena (el original lo hace con particulas; aqui se
    // usa un patron determinista para no depender del frame rate).
    if (weather == "lluvia") {
        for (int i = 0; i < 90; ++i) {
            const float sx = std::fmod(i * 137.0f - cameraX * 1.1f + time * 260.0f, 1400.0f);
            const float sy = std::fmod(i * 61.0f + time * 900.0f, 760.0f) - 40.0f;
            const float x = sx < 0 ? sx + 1400.0f : sx;
            DrawLineEx({x - 60, sy}, {x - 70, sy + 34}, 1.5f, {190, 205, 230, 90});
        }
        // Relampago ocasional (fx_barco a2..a7 son los destellos del original).
        const float flash = std::fmod(time, 9.0f);
        if (flash < 0.12f || (flash > 0.22f && flash < 0.3f)) DrawRectangle(0, 0, 1280, 720, {220, 230, 255, 70});
    } else if (weather == "nieve") {
        for (int i = 0; i < 110; ++i) {
            const float drift = std::sin(time * 1.3f + i) * 18.0f;
            float x = std::fmod(i * 97.0f - cameraX * 0.9f + drift + time * 25.0f, 1300.0f);
            if (x < 0) x += 1300.0f;
            const float y = std::fmod(i * 53.0f + time * (60.0f + (i % 5) * 18.0f), 740.0f) - 20.0f;
            DrawCircleV({x - 10, y}, 1.5f + (i % 3), {245, 248, 255, 200});
        }
    } else if (weather == "ceniza") {
        for (int i = 0; i < 60; ++i) {
            float x = std::fmod(i * 113.0f - cameraX * 0.8f + std::sin(time + i) * 30.0f, 1300.0f);
            if (x < 0) x += 1300.0f;
            const float y = 740.0f - std::fmod(i * 47.0f + time * (35.0f + (i % 4) * 12.0f), 760.0f);
            DrawCircleV({x - 10, y}, 1.5f + (i % 2), {255, (unsigned char)(140 + (i % 3) * 30), 60, 170});
        }
    }
}

}  // namespace district_fury
