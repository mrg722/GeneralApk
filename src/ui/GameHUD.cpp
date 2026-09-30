#include "ui/GameHUD.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cstdio>

namespace district_fury {
namespace ui {

namespace {
float SafeRatio(int value, int maxValue) {
    if (maxValue <= 0) return 0.0f;
    return std::clamp(static_cast<float>(value) / static_cast<float>(maxValue), 0.0f, 1.0f);
}
}  // namespace

void DrawPlayerVitals(const PlayerVitals& v) {
    DrawRectangle(v.x, v.y, v.width, v.panelHeight, {5, 8, 11, 235});

    // Retrato de Rayden — mismo patron ya usado y probado en VSMode::DrawHud
    // (textura "rayden_clean", recorte 96x96, destino 70x70).
    const int portraitX = v.x + 10;
    const int portraitY = v.y + 10;
    const Texture2D portrait = AssetManager::Get().GetTexture("rayden_clean");
    if (portrait.id) {
        DrawTexturePro(portrait, {0, 0, 96, 96},
                       {static_cast<float>(portraitX), static_cast<float>(portraitY), 70, 70},
                       {0, 0}, 0, WHITE);
    }
    DrawRectangleLines(portraitX, portraitY, 70, 70, {70, 200, 235, 160});

    const int textX = portraitX + 82;
    const int barWidth = std::max(120, v.width - (textX - v.x) - 20);

    DrawText(v.title, textX, v.y + 8, 18, {185, 220, 255, 255});

    char buf[64];
    std::snprintf(buf, sizeof(buf), "VIDA %d/%d", v.hp, v.maxHp);
    DrawText(buf, textX, v.y + 34, 14, WHITE);
    DrawRectangle(textX + 78, v.y + 35, barWidth - 78, 11, {25, 25, 30, 255});
    DrawRectangle(textX + 78, v.y + 35,
                  static_cast<int>((barWidth - 78) * SafeRatio(v.hp, v.maxHp)), 11,
                  {45, 170, 255, 255});

    std::snprintf(buf, sizeof(buf), "ESCUDO %d%%", static_cast<int>(100.0f * SafeRatio(v.shield, v.maxShield)));
    DrawText(buf, textX, v.y + 52, 13, {100, 220, 255, 255});
    DrawRectangle(textX + 78, v.y + 53, barWidth - 118, 7, {25, 25, 30, 255});
    DrawRectangle(textX + 78, v.y + 53,
                  static_cast<int>((barWidth - 118) * SafeRatio(v.shield, v.maxShield)), 7,
                  {70, 200, 255, 255});

    std::snprintf(buf, sizeof(buf), "ENERGIA %d   FURIA %d%%", v.sp,
                  static_cast<int>(100.0f * SafeRatio(v.rage, v.maxRage)));
    DrawText(buf, textX, v.y + 70, 13, v.isRageMode ? Color{90, 215, 255, 255} : WHITE);

    if (v.combo > 1) {
        std::snprintf(buf, sizeof(buf), "COMBO x%d", v.combo);
        DrawText(buf, v.x + v.width - 130, v.y + 8, 20, {255, 190, 65, 255});
    }
}

}  // namespace ui
}  // namespace district_fury
