#include "game/beta/BetaHud.h"
#include "game/Player.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace district_fury {
namespace beta_hud {
namespace {

Texture2D Ui(const char* name) {
    return AssetManager::Get().GetTextureByPath(std::string("assets/beta/ui/") + name + ".png");
}

void Blit(const Texture2D& t, Rectangle src, Rectangle dst, Color tint = WHITE) {
    if (t.id) DrawTexturePro(t, src, dst, {0, 0}, 0, tint);
}

}  // namespace

void DrawDigits(const char* sheet, int cells, int zeroCell, int value, Vector2 pos, float h, Color tint) {
    const Texture2D t = Ui(sheet);
    const std::string s = std::to_string(std::max(0, value));
    if (!t.id) { DrawText(s.c_str(), (int)pos.x, (int)(pos.y - h / 2), (int)h, tint); return; }
    const float cw = (float)t.width / cells;
    const float k = h / t.height;
    const float w = cw * k * 0.8f;   // las cifras se solapan un poco, como en el original
    float x = pos.x - w * s.size() / 2.0f;
    for (char c : s) {
        const int cell = zeroCell + (c - '0');
        Blit(t, {cell * cw, 0, cw, (float)t.height}, {x - (cw * k - w) / 2, pos.y - h / 2, cw * k, h}, tint);
        x += w;
    }
}

void DrawPlayer(const Player& p, bool heroPortrait, const char* name) {
    const float k = 1.6f, ox = 8.0f, oy = 6.0f;
    const Texture2D frame = Ui("barra_espada");
    // Retrato en el circulo (2,8)-(62,68) del marco.
    const Rectangle circle{ox + 2 * k, oy + 8 * k, 60 * k, 60 * k};
    if (heroPortrait) {
        const Texture2D face = Ui("retrato_guerrero");
        Blit(frame, {0, 0, (float)frame.width, (float)frame.height}, {ox, oy, frame.width * k, frame.height * k});
        Blit(face, {0, 0, (float)face.width, (float)face.height}, {circle.x + 4, circle.y + 2, circle.width - 6, circle.height - 4});
    } else {
        Blit(frame, {0, 0, (float)frame.width, (float)frame.height}, {ox, oy, frame.width * k, frame.height * k});
        p.DrawPortrait(circle);
    }
    // Vida (roja) y magia (azul) en las ranuras del marco.
    const Texture2D red = Ui("barra_roja"), blue = Ui("barra_azul");
    const float hp = std::clamp((float)p.hp / std::max(1, p.maxHp), 0.0f, 1.0f);
    const float sp = std::clamp((float)p.sp / std::max(1, p.maxSp), 0.0f, 1.0f);
    Blit(red, {0, 0, red.width * hp, (float)red.height}, {ox + 72 * k, oy + 25 * k, 93 * k * hp, 8 * k});
    Blit(blue, {0, 0, blue.width * sp, (float)blue.height}, {ox + 72 * k, oy + 36 * k, 78 * k * sp, 8 * k});
    // Furia: brillo en la bola roja del marco cuando esta lista.
    if (p.rage >= p.maxRage || p.isRageMode)
        DrawCircleV({ox + 52 * k, oy + 57 * k}, 9 * k * (0.8f + 0.2f * std::sin((float)GetTime() * 8)), {255, 200, 60, 120});
    DrawText(name, (int)(ox + 74 * k), (int)(oy + 49 * k), 14, {255, 230, 170, 255});
    DrawText(TextFormat("%d/%d", std::max(0, p.hp), p.maxHp), (int)(ox + 74 * k), (int)(oy + 6 * k), 12, {255, 255, 255, 230});
}

void DrawBossBar(const char* name, int hp, int maxHp) {
    const Texture2D frame = Ui("jefe_marco");
    const char* layers[3] = {"jefe_rojo", "jefe_amarillo", "jefe_verde"};
    const float k = 2.2f;
    const float w = frame.id ? frame.width * k : 710.0f, x = (1280.0f - w) / 2, y = 16.0f;
    // 3 capas: se vacia la verde, luego la amarilla y al final la roja.
    const float per = std::max(1.0f, maxHp / 3.0f);
    const float left = (float)std::max(0, hp);
    int layer = std::min(2, (int)(left / per - 0.0001f));
    if (left <= 0) layer = 0;
    const float frac = left <= 0 ? 0.0f : (left - layer * per) / per;
    const Rectangle slot{x + 44 * k, y + 6 * k, 236 * k, 15 * k};
    Blit(frame, {0, 0, (float)frame.width, (float)frame.height}, {x, y, w, frame.height * k});
    if (layer > 0) {   // la capa de abajo se ve completa detras
        const Texture2D under = Ui(layers[layer - 1]);
        Blit(under, {0, 0, (float)under.width, (float)under.height}, slot);
    }
    const Texture2D cur = Ui(layers[layer]);
    Blit(cur, {0, 0, cur.width * frac, (float)cur.height}, {slot.x, slot.y, slot.width * frac, slot.height});
    DrawText(TextFormat("JEFE  %s", name), (int)slot.x, (int)(y + frame.height * k + 4), 18, {255, 220, 150, 255});
    // "x3", "x2"... capas que quedan.
    const char* left_ = TextFormat("x%d", layer + 1);
    DrawText(left_, (int)(slot.x + slot.width - MeasureText(left_, 24)), (int)(y + frame.height * k + 2), 24, {255, 200, 60, 255});
}

void DrawEnemyBar(Vector2 c, float ratio) {
    const Texture2D groove = Ui("enemigo_fondo"), bar = Ui("enemigo_vida");
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    if (!groove.id) {
        DrawRectangle((int)c.x - 40, (int)c.y, 80, 6, {30, 10, 10, 200});
        DrawRectangle((int)c.x - 40, (int)c.y, (int)(80 * ratio), 6, {230, 60, 50, 255});
        return;
    }
    const float x = c.x - groove.width / 2.0f;
    Blit(groove, {0, 0, (float)groove.width, (float)groove.height}, {x, c.y, (float)groove.width, (float)groove.height});
    Blit(bar, {0, 0, bar.width * ratio, (float)bar.height}, {x + 4, c.y + 2.5f, bar.width * ratio, (float)bar.height});
}

void DrawCombo(int hits, float alpha) {
    if (hits < 2 || alpha <= 0) return;
    const unsigned char a = (unsigned char)(255 * std::clamp(alpha, 0.0f, 1.0f));
    const Texture2D blood = Ui("sangre_combo"), label = Ui("hits");
    Blit(blood, {0, 0, (float)blood.width, (float)blood.height}, {960, 200, 300, 95}, {255, 255, 255, (unsigned char)(a * 0.85f)});
    // Dajishuzi: 12 celdas, el 0 en la tercera.
    DrawDigits("numeros_golpe", 12, 2, hits, {1060, 246}, 70, {255, 255, 255, a});
    Blit(label, {0, 0, (float)label.width, (float)label.height}, {1130, 222, label.width * 1.2f, label.height * 1.2f}, {255, 255, 255, a});
}

void DrawPotions(int red, int blue, bool touch) {
    if (touch) {   // junto a los botones tactiles (TouchControls kBeta)
        DrawText(TextFormat("%d", red), 585, 676, 20, {255, 110, 90, 255});
        DrawText(TextFormat("%d", blue), 715, 676, 20, {120, 180, 255, 255});
        return;
    }
    const Texture2D r = Ui("pocion_roja"), b = Ui("pocion_azul");
    Blit(r, {0, 0, (float)r.width, (float)r.height}, {520, 668, 40, 43});
    DrawText(TextFormat("Z  %d", red), 562, 682, 16, {255, 120, 100, 255});
    Blit(b, {0, 0, (float)b.width, (float)b.height}, {620, 668, 40, 43});
    DrawText(TextFormat("X  %d", blue), 662, 682, 16, {120, 180, 255, 255});
}

}  // namespace beta_hud
}  // namespace district_fury
