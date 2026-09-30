#pragma once
#include "raylib.h"
#include <algorithm>
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

// DF-014: fondo panoramico SIN mosaico (antes se repetia y se veian costuras).
// Se escala a alto 720 y se desplaza dentro de la imagen: t=0 muestra el borde
// izquierdo y t=1 el derecho. Si la imagen es mas angosta que la pantalla, se
// estira a 1280 de ancho.
inline void DrawPannedBackdrop(Texture2D tex, float cameraX, float t, Color tint = WHITE) {
    if (tex.id == 0 || tex.width <= 0 || tex.height <= 0) return;
    const float scale = 720.0f / static_cast<float>(tex.height);
    const float w = std::max(1280.0f, static_cast<float>(tex.width) * scale);
    const float tc = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    const float left = cameraX - 640.0f - tc * (w - 1280.0f);
    const Rectangle src{0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)};
    DrawTexturePro(tex, src, {left, 0.0f, w, 720.0f}, {0.0f, 0.0f}, 0.0f, tint);
}

// Firma historica de los stages 2-5: ahora panea a lo largo de todo el stage.
inline void DrawScenarioBackdrop(Texture2D tex, float cameraX, float parallax = 0.35f, Color tint = WHITE) {
    (void)parallax;
    DrawPannedBackdrop(tex, cameraX, (cameraX - 640.0f) / (6000.0f - 1280.0f), tint);
}

}  // namespace district_fury
