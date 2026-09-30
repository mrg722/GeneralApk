#include "ui/TouchControls.h"
#include "core/InputMap.h"
#include "game/Player.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace district_fury {
namespace touch {
namespace {

struct Button {
    Vector2 center;
    float radius;
    int key;
    const char* label;
    Color ring;
};

bool gEnabled = false;
float gScale = 1.0f, gOffX = 0.0f, gOffY = 0.0f;
std::vector<Vector2> gInjected;

// Joystick (izquierda).
constexpr Vector2 kStick{175.0f, 560.0f};
constexpr float kStickRadius = 105.0f;
constexpr float kStickZone = 300.0f;      // radio de captura del toque
Vector2 gKnob = kStick;

const Button kCombat[] = {
    {{1165.0f, 612.0f}, 64.0f, KEY_J, "GOLPE", {225, 55, 45, 255}},
    {{1040.0f, 660.0f}, 44.0f, KEY_K, "PATADA", {230, 140, 40, 255}},
    {{1052.0f, 548.0f}, 40.0f, input::kVirtualSpecialWave, "ONDA", {60, 170, 255, 255}},
    {{1160.0f, 488.0f}, 40.0f, input::kVirtualSpecialRise, "GANCHO", {150, 110, 255, 255}},
    {{930.0f, 668.0f}, 36.0f, KEY_B, "BLOQ", {90, 200, 230, 255}},
    {{930.0f, 585.0f}, 34.0f, KEY_LEFT_SHIFT, "DASH", {200, 200, 210, 255}},
    {{1245.0f, 395.0f}, 30.0f, KEY_SPACE, "FURIA", {255, 190, 60, 255}},
    {{1245.0f, 150.0f}, 26.0f, KEY_ESCAPE, "PAUSA", {170, 170, 180, 255}},
};
// Menus: cruceta a la derecha (no tapa las listas, que estan a la izquierda).
const Button kMenu[] = {
    {{1180.0f, 612.0f}, 54.0f, KEY_ENTER, "OK", {90, 210, 120, 255}},
    {{1200.0f, 470.0f}, 36.0f, KEY_ESCAPE, "ATRAS", {200, 90, 90, 255}},
    {{1010.0f, 530.0f}, 34.0f, KEY_UP, "^", {200, 200, 210, 255}},
    {{1010.0f, 670.0f}, 34.0f, KEY_DOWN, "v", {200, 200, 210, 255}},
    {{940.0f, 600.0f}, 34.0f, KEY_LEFT, "<", {200, 200, 210, 255}},
    {{1080.0f, 600.0f}, 34.0f, KEY_RIGHT, ">", {200, 200, 210, 255}},
};
const Button kEnd[] = {
    {{1180.0f, 612.0f}, 54.0f, KEY_ENTER, "OK", {90, 210, 120, 255}},
    {{1050.0f, 660.0f}, 42.0f, KEY_R, "REINTENTAR", {230, 170, 60, 255}},
    {{1050.0f, 545.0f}, 38.0f, KEY_Q, "MENU", {200, 90, 90, 255}},
};
const Button kReward[] = {
    {{520.0f, 640.0f}, 44.0f, KEY_ONE, "1", {90, 210, 120, 255}},
    {{640.0f, 640.0f}, 44.0f, KEY_TWO, "2", {90, 170, 255, 255}},
    {{760.0f, 640.0f}, 44.0f, KEY_THREE, "3", {230, 170, 60, 255}},
};

template <size_t N>
const Button* Set(const Button (&arr)[N], size_t& n) { n = N; return arr; }

const Button* ButtonsFor(Context c, size_t& n) {
    switch (c) {
        case Context::Combat:    return Set(kCombat, n);
        case Context::EndScreen: return Set(kEnd, n);
        case Context::Reward:    return Set(kReward, n);
        default:                 return Set(kMenu, n);
    }
}

bool UsesStick(Context c) { return c == Context::Combat; }

std::vector<Vector2> Touches() {
    std::vector<Vector2> pts = gInjected;
    const int count = GetTouchPointCount();
    for (int i = 0; i < count; ++i) {
        const Vector2 p = GetTouchPosition(i);
        pts.push_back({(p.x - gOffX) / gScale, (p.y - gOffY) / gScale});
    }
    return pts;
}

void ApplyStick(Vector2 p, Context c) {
    Vector2 d{p.x - kStick.x, p.y - kStick.y};
    const float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len > kStickRadius) { d.x *= kStickRadius / len; d.y *= kStickRadius / len; }
    gKnob = {kStick.x + d.x, kStick.y + d.y};
    if (len < 22.0f) return;                              // zona muerta
    const float nx = d.x / std::max(len, 1.0f), ny = d.y / std::max(len, 1.0f);
    const bool right = nx > 0.38f, left = nx < -0.38f, down = ny > 0.38f, up = ny < -0.38f;
    // Combate: WASD (movimiento y comandos). Menus: tambien flechas.
    input::SetVirtual(KEY_D, right); input::SetVirtual(KEY_A, left);
    input::SetVirtual(KEY_S, down); input::SetVirtual(KEY_W, up);
    if (c != Context::Combat) {
        input::SetVirtual(KEY_RIGHT, right); input::SetVirtual(KEY_LEFT, left);
        input::SetVirtual(KEY_DOWN, down); input::SetVirtual(KEY_UP, up);
    }
}

}  // namespace

void SetEnabled(bool enabled) { gEnabled = enabled; }
bool Enabled() { return gEnabled; }
void SetScreenTransform(float scale, float offsetX, float offsetY) {
    gScale = scale > 0.0f ? scale : 1.0f; gOffX = offsetX; gOffY = offsetY;
}
void InjectTouchForTest(Vector2 p) { gInjected.push_back(p); }

void Update(Context context) {
    input::ClearNext();
    gKnob = kStick;
    if (gEnabled) {
        size_t n = 0;
        const Button* buttons = ButtonsFor(context, n);
        for (const Vector2& p : Touches()) {
            bool used = false;
            for (size_t i = 0; i < n && !used; ++i) {
                const float dx = p.x - buttons[i].center.x, dy = p.y - buttons[i].center.y;
                const float hit = buttons[i].radius * 1.25f;  // dedos: margen generoso
                if (dx * dx + dy * dy <= hit * hit) { input::SetVirtual(buttons[i].key, true); used = true; }
            }
            if (!used && UsesStick(context)) {
                const float dx = p.x - kStick.x, dy = p.y - kStick.y;
                if (dx * dx + dy * dy <= kStickZone * kStickZone && p.x < 520.0f) ApplyStick(p, context);
            }
        }
    }
    // Boton "atras" de Android = ESC (pausa / volver).
    if (IsKeyDown(KEY_BACK)) input::SetVirtual(KEY_ESCAPE, true);
    gInjected.clear();
    input::Commit();
}

void Draw(Context context, const Player* player) {
    if (!gEnabled) return;
    if (UsesStick(context)) {
        DrawCircleV(kStick, kStickRadius + 14.0f, {20, 10, 12, 90});
        DrawCircleLinesV(kStick, kStickRadius + 14.0f, {220, 60, 50, 170});
        DrawCircleLinesV(kStick, kStickRadius - 30.0f, {220, 60, 50, 90});
        for (int i = 0; i < 4; ++i) {   // chevrones de direccion
            const float a = i * 1.5708f;
            const Vector2 tip{kStick.x + std::cos(a) * (kStickRadius + 2.0f), kStick.y + std::sin(a) * (kStickRadius + 2.0f)};
            const Vector2 l{kStick.x + std::cos(a - 0.18f) * (kStickRadius - 20.0f), kStick.y + std::sin(a - 0.18f) * (kStickRadius - 20.0f)};
            const Vector2 r{kStick.x + std::cos(a + 0.18f) * (kStickRadius - 20.0f), kStick.y + std::sin(a + 0.18f) * (kStickRadius - 20.0f)};
            DrawTriangle(tip, l, r, {235, 120, 110, 150});
            DrawTriangle(tip, r, l, {235, 120, 110, 150});
        }
        DrawCircleV(gKnob, 42.0f, {60, 60, 66, 200});
        DrawCircleLinesV(gKnob, 42.0f, {240, 240, 245, 200});
    }
    size_t n = 0;
    const Button* buttons = ButtonsFor(context, n);
    for (size_t i = 0; i < n; ++i) {
        const Button& b = buttons[i];
        const bool held = input::Down(b.key);
        bool dim = false, glow = false;
        if (player && b.key == input::kVirtualSpecialWave) dim = player->sp < GetAttack(AttackId::EnergyWave).spCost;
        if (player && b.key == KEY_SPACE) { glow = player->rage >= player->maxRage; dim = !glow && !player->isRageMode; }
        const unsigned char a = dim ? 90 : 215;
        DrawCircleV(b.center, b.radius, held ? Color{70, 70, 78, 235} : Color{28, 26, 30, a});
        DrawCircleLinesV(b.center, b.radius, {b.ring.r, b.ring.g, b.ring.b, a});
        DrawCircleLinesV(b.center, b.radius - 3.0f, {b.ring.r, b.ring.g, b.ring.b, (unsigned char)(a / 2)});
        if (glow) DrawCircleLinesV(b.center, b.radius + 5.0f + 3.0f * std::sin((float)GetTime() * 8.0f), {255, 210, 90, 220});
        const char c0 = b.label[0];
        if (b.label[1] == '\0' && (c0 == '^' || c0 == 'v' || c0 == '<' || c0 == '>')) {
            // Flechas de la cruceta: triangulo dibujado (la fuente no trae flechas).
            const float r = b.radius * 0.45f;
            const Vector2 c = b.center;
            Vector2 p1, p2, p3;
            if (c0 == '^') { p1 = {c.x, c.y - r}; p2 = {c.x - r, c.y + r * 0.7f}; p3 = {c.x + r, c.y + r * 0.7f}; }
            else if (c0 == 'v') { p1 = {c.x, c.y + r}; p2 = {c.x + r, c.y - r * 0.7f}; p3 = {c.x - r, c.y - r * 0.7f}; }
            else if (c0 == '<') { p1 = {c.x - r, c.y}; p2 = {c.x + r * 0.7f, c.y + r}; p3 = {c.x + r * 0.7f, c.y - r}; }
            else { p1 = {c.x + r, c.y}; p2 = {c.x - r * 0.7f, c.y - r}; p3 = {c.x - r * 0.7f, c.y + r}; }
            DrawTriangle(p1, p2, p3, {245, 240, 235, a});
            DrawTriangle(p1, p3, p2, {245, 240, 235, a});
            continue;
        }
        const int fs = b.radius >= 55.0f ? 20 : (b.radius >= 40.0f ? 14 : 11);
        const int w = MeasureText(b.label, fs);
        DrawText(b.label, (int)(b.center.x - w / 2.0f), (int)(b.center.y - fs / 2.0f), fs, {245, 240, 235, a});
    }
}

}  // namespace touch
}  // namespace district_fury
