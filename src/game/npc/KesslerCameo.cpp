#include "game/npc/KesslerCameo.h"
#include "rendering/AssetManager.h"
#include "rendering/BossSprite.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {
constexpr float kWalkSpeed = 110.0f;
constexpr float kLineDuration = 2.4f;
constexpr float kSpriteScale = 1.5f;  // ~124px: misma escala humana que Rayden (hoja "ESCALAS")

void DrawFallbackKessler(float x, float y, bool facingRight, float bob) {
    const float top = y - 118.0f + bob;
    DrawEllipse((int)x, (int)y, 26, 6, {0, 0, 0, 140});
    DrawRectangle((int)(x - 11), (int)(y - 44), 9, 44, {24, 24, 28, 255});
    DrawRectangle((int)(x + 2), (int)(y - 44), 9, 44, {24, 24, 28, 255});
    DrawRectangle((int)(x - 20), (int)(top + 26), 40, 60, {222, 224, 228, 255});
    DrawRectangle((int)(x - 7), (int)(top + 26), 14, 34, {30, 30, 34, 255});
    DrawCircle((int)x, (int)(top + 13), 12, {214, 180, 158, 255});
    DrawRectangle((int)(x - 12), (int)(top - 1), 24, 8, {170, 172, 178, 255});
    const float gx = facingRight ? x + 2 : x - 10;
    DrawRectangleLines((int)gx, (int)(top + 10), 8, 4, {20, 20, 20, 255});
}
}  // namespace

void KesslerCameo::Start(float enterFromX, float stand, float exit, float ground,
                         const std::array<const char*, 3>& l) {
    x = enterFromX; standX = stand; exitX = exit; groundY = ground;
    lines = l; line = 0; timer = 0; walkTime = 0;
    facingRight = standX > x;
    state = State::Entering;
}

void KesslerCameo::Skip() {
    if (state == State::Entering || state == State::Talking) { state = State::Leaving; facingRight = exitX > x; }
}

void KesslerCameo::Update(float dt) {
    switch (state) {
        case State::Entering: {
            walkTime += dt;
            const float dir = standX > x ? 1.0f : -1.0f;
            x += dir * kWalkSpeed * dt;
            if ((dir > 0 && x >= standX) || (dir < 0 && x <= standX)) { x = standX; state = State::Talking; timer = 0; }
            break;
        }
        case State::Talking:
            timer += dt;
            if (timer >= kLineDuration) {
                timer = 0; ++line;
                if (line >= (int)lines.size() || lines[(size_t)line] == nullptr) { state = State::Leaving; facingRight = exitX > x; }
            }
            break;
        case State::Leaving: {
            walkTime += dt;
            const float dir = exitX > x ? 1.0f : -1.0f;
            x += dir * kWalkSpeed * 1.2f * dt;
            if ((dir > 0 && x >= exitX) || (dir < 0 && x <= exitX)) state = State::Done;
            break;
        }
        default: break;
    }
}

void KesslerCameo::DrawWorld() const {
    if (!IsActive()) return;
    const bool walking = state == State::Entering || state == State::Leaving;
    auto& am = AssetManager::Get();
    Texture2D tex = am.GetTexture(walking ? ((int)(walkTime * 6.0f) % 2 == 0 ? "kessler_walk1" : "kessler_walk2") : "kessler_idle");
    if (tex.id == 0) tex = am.GetTexture("kessler_idle");
    if (tex.id != 0) {
        // Los PNG esperados miran a la derecha.
        DrawSpriteUniform(tex, {x, groundY}, kSpriteScale, !facingRight, WHITE);
        return;
    }
    const float bob = walking ? std::sin(walkTime * 12.0f) * 1.5f : 0.0f;
    DrawFallbackKessler(x, groundY, facingRight, bob);
}

void KesslerCameo::DrawDialogue() const {
    if (state != State::Talking || line >= (int)lines.size() || lines[(size_t)line] == nullptr) return;
    DrawRectangle(190, 540, 900, 110, {6, 10, 16, 235});
    DrawRectangleLines(190, 540, 900, 110, {90, 200, 140, 200});
    DrawRectangle(190, 540, 6, 110, {90, 230, 150, 230});
    DrawText("DR. KESSLER", 214, 552, 18, {150, 240, 180, 255});
    DrawText(lines[(size_t)line], 214, 584, 20, {230, 235, 240, 255});
    DrawText("ENTER — SALTAR", 950, 626, 12, {150, 165, 175, 220});
}

}  // namespace district_fury
