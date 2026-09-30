#pragma once
#include "raylib.h"
#include <cmath>

// DF-013.2 (19-09) — fondo de escenario con parallax.
//
// Los 20 fondos entregados son 256x144 y escalan x5 exacto a 1280x720, asi
// que con filtro POINT quedan nitidos (pixel art, sin interpolar). Se dibujan
// repetidos en horizontal y desplazados a una fraccion de la camara, de modo
// que el fondo acompana al jugador sin viajar a su misma velocidad.
//
// Si la textura no existe, no dibuja nada: cada stage conserva su fondo
// procedural de respaldo y nada se rompe.
namespace district_fury {

inline void DrawScenarioBackdrop(Texture2D tex, float cameraX, float parallax = 0.35f, Color tint = WHITE) {
    if (tex.id == 0 || tex.width <= 0) return;
    const float scale = 720.0f / static_cast<float>(tex.height);
    const float w = static_cast<float>(tex.width) * scale;
    const float offset = std::fmod(cameraX * parallax, w);
    const float left = cameraX - 640.0f - offset;
    const Rectangle src{0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)};
    for (int i = 0; i < 3; ++i) {
        const Rectangle dst{left + w * static_cast<float>(i), 0.0f, w, 720.0f};
        DrawTexturePro(tex, src, dst, {0.0f, 0.0f}, 0.0f, tint);
    }
}

}  // namespace district_fury
