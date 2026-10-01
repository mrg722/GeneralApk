#include "ui/TouchControls.h"
#include "core/InputMap.h"
#include "game/Player.h"
#include "rendering/AssetManager.h"
#include <string>
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
    const char* art = nullptr;   // assets/ui/touch/<art>.png (arte del usuario)
    bool artHasText = false;     // el arte ya trae su texto (GOLPE, DASH, BLOQ)
};

bool gEnabled = false;
float gScaleX = 1.0f, gScaleY = 1.0f, gOffX = 0.0f, gOffY = 0.0f;
std::vector<Vector2> gInjected;

// Joystick (izquierda).
constexpr Vector2 kStick{175.0f, 560.0f};
constexpr float kStickRadius = 105.0f;
constexpr float kStickZone = 300.0f;      // radio de captura del toque
Vector2 gKnob = kStick;

// Combate estilo King Fighter: GOLPE grande abajo a la derecha y las 6
// habilidades en arco alrededor (cada una con su espera de 15 s).
const Button kCombat[] = {
    {{1170.0f, 615.0f}, 62.0f, KEY_J, "GOLPE", {225, 55, 45, 255}, "golpe", true},
    {{1057.0f, 574.0f}, 40.0f, KEY_K, "PATADA", {230, 140, 40, 255}, "estallido_rojo"},
    {{961.0f, 633.0f}, 30.0f, input::kVirtualSkill0 + 0, "1", {60, 170, 255, 255}, "espiral_azul"},
    {{965.0f, 568.0f}, 30.0f, input::kVirtualSkill0 + 1, "2", {150, 110, 255, 255}, "puno_azul"},
    {{990.0f, 507.0f}, 30.0f, input::kVirtualSkill0 + 2, "3", {60, 220, 160, 255}, "mira_verde"},
    {{1032.0f, 456.0f}, 30.0f, input::kVirtualSkill0 + 3, "4", {255, 120, 70, 255}, "corredor_azul"},
    {{1088.0f, 422.0f}, 30.0f, input::kVirtualSkill0 + 4, "5", {255, 70, 120, 255}, "calavera_verde"},
    {{1152.0f, 406.0f}, 30.0f, input::kVirtualSkill0 + 5, "6", {255, 205, 70, 255}, "estallido_verde"},
    {{1245.0f, 485.0f}, 28.0f, input::kVirtualSpecialWave, "ESPECIAL", {60, 170, 255, 255}, "puno_verde"},
    {{1222.0f, 405.0f}, 22.0f, input::kVirtualSkillPage, "PAG", {200, 200, 210, 255}},
    {{860.0f, 668.0f}, 36.0f, KEY_B, "BLOQ", {90, 200, 230, 255}, "bloq", true},
    {{865.0f, 578.0f}, 32.0f, KEY_LEFT_SHIFT, "DASH", {200, 200, 210, 255}, "dash", true},
    {{1240.0f, 330.0f}, 30.0f, KEY_SPACE, "FURIA", {255, 190, 60, 255}, "mano_roja"},
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

Texture2D Art(const char* name) {
    if (!name || !IsWindowReady()) return Texture2D{};
    return AssetManager::Get().GetTextureByPath(std::string("assets/ui/touch/") + name + ".png");
}

// Dibuja el arte centrado en el boton (diametro = 2 * radio * k).
bool DrawArt(const char* name, Vector2 c, float r, Color tint, float k = 1.12f) {
    const Texture2D t = Art(name);
    if (t.id == 0) return false;
    const float d = 2.0f * r * k;
    DrawTexturePro(t, {0, 0, (float)t.width, (float)t.height}, {c.x - d / 2, c.y - d / 2, d, d}, {0, 0}, 0, tint);
    return true;
}

std::vector<Vector2> Touches() {
    std::vector<Vector2> pts = gInjected;
    const int count = GetTouchPointCount();
    for (int i = 0; i < count; ++i) {
        const Vector2 p = GetTouchPosition(i);
        pts.push_back({(p.x - gOffX) / gScaleX, (p.y - gOffY) / gScaleY});
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
void SetScreenTransform(float scale, float offsetX, float offsetY) { SetScreenTransform(scale, scale, offsetX, offsetY); }
void SetScreenTransform(float scaleX, float scaleY, float offsetX, float offsetY) {
    gScaleX = scaleX > 0.0f ? scaleX : 1.0f; gScaleY = scaleY > 0.0f ? scaleY : 1.0f; gOffX = offsetX; gOffY = offsetY;
}
void InjectTouchForTest(Vector2 p) { gInjected.push_back(p); }

void Update(Context context) {
    input::ClearNext();
    gKnob = kStick;
    if (gEnabled) {
        size_t n = 0;
        const Button* buttons = ButtonsFor(context, n);
        for (const Vector2& p : Touches()) {
            // El boton mas cercano dentro de su zona (dedos: margen generoso).
            int best = -1;
            float bestD = 1e9f;
            for (size_t i = 0; i < n; ++i) {
                const float dx = p.x - buttons[i].center.x, dy = p.y - buttons[i].center.y;
                const float hit = buttons[i].radius * 1.25f;
                const float d2 = dx * dx + dy * dy;
                if (d2 <= hit * hit && d2 < bestD) { bestD = d2; best = static_cast<int>(i); }
            }
            const bool used = best >= 0;
            if (used) input::SetVirtual(buttons[best].key, true);
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
        if (DrawArt("palanca_base", kStick, kStickRadius + 14.0f, {255, 255, 255, 215}, 1.0f)) {
            DrawArt("palanca_perilla", gKnob, 46.0f, WHITE, 1.0f);
        } else {
            DrawCircleV(kStick, kStickRadius + 14.0f, {20, 10, 12, 90});
            DrawCircleLinesV(kStick, kStickRadius + 14.0f, {220, 60, 50, 170});
            DrawCircleV(gKnob, 42.0f, {60, 60, 66, 200});
            DrawCircleLinesV(gKnob, 42.0f, {240, 240, 245, 200});
        }
    }
    size_t n = 0;
    const Button* buttons = ButtonsFor(context, n);
    for (size_t i = 0; i < n; ++i) {
        const Button& b = buttons[i];
        if (b.key == input::kVirtualSkillPage && (!player || player->SkillPageCount() <= 1)) continue;
        const bool held = input::Down(b.key);
        bool dim = false, glow = false;
        if (player && b.key == input::kVirtualSpecialWave) dim = player->sp < GetAttack(AttackId::EnergyWave).spCost;
        if (player && b.key == KEY_SPACE) { glow = player->rage >= player->maxRage; dim = !glow && !player->isRageMode; }
        const unsigned char a = dim ? 90 : 215;
        const int skillIdx = b.key - input::kVirtualSkill0;
        const bool cooling = player && skillIdx >= 0 && skillIdx < input::kSkillKeys && !player->SkillReady(skillIdx);
        const unsigned char lum = held ? 255 : (dim || cooling) ? 95 : 235;
        const bool art = DrawArt(b.art, b.center, b.radius, {lum, lum, lum, 255}, held ? 1.05f : 1.12f);
        if (!art) {
            DrawCircleV(b.center, b.radius, held ? Color{70, 70, 78, 235} : Color{28, 26, 30, a});
            DrawCircleLinesV(b.center, b.radius, {b.ring.r, b.ring.g, b.ring.b, a});
            DrawCircleLinesV(b.center, b.radius - 3.0f, {b.ring.r, b.ring.g, b.ring.b, (unsigned char)(a / 2)});
        }
        if (glow) DrawCircleLinesV(b.center, b.radius + 5.0f + 3.0f * std::sin((float)GetTime() * 8.0f), {255, 210, 90, 220});
        const int skill = b.key - input::kVirtualSkill0;
        if (skill >= 0 && skill < input::kSkillKeys) {
            // Habilidad: nombre del personaje y espera de 15 s como reloj que se vacia.
            if (player && !player->SkillReady(skill)) {
                const float left = std::min(1.0f, player->SkillCooldownLeft(skill) / Player::kSkillCooldown);
                DrawCircleSector(b.center, b.radius - 2.0f, -90.0f, -90.0f + 360.0f * left, 32, {0, 0, 0, 170});
                const char* secs = player->SkillCooldownLeft(skill) > Player::kSkillCooldown ? "-" : TextFormat("%d", (int)std::ceil(player->SkillCooldownLeft(skill)));
                DrawText(secs, (int)(b.center.x - MeasureText(secs, 20) / 2.0f), (int)(b.center.y - 10), 20, {235, 235, 240, 230});
            } else if (player) {
                DrawCircleLinesV(b.center, b.radius + 3.0f + 2.0f * std::sin((float)GetTime() * 6.0f + skill), {b.ring.r, b.ring.g, b.ring.b, 160});
            }
            const char* name = player ? player->SkillName(skill) : b.label;
            const int fs = 10;
            DrawText(name, (int)(b.center.x - MeasureText(name, fs) / 2.0f), (int)(b.center.y + b.radius + 3.0f), fs, {245, 240, 235, 230});
            const char* num = b.label;
            if (!art && (!player || player->SkillReady(skill)))
                DrawText(num, (int)(b.center.x - MeasureText(num, 22) / 2.0f), (int)(b.center.y - 11), 22, {b.ring.r, b.ring.g, b.ring.b, 240});
            continue;
        }
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
        if (art && b.artHasText) continue;   // GOLPE / DASH / BLOQ: el arte ya lo dice
        if (b.key == input::kVirtualSkillPage) {   // pagina de habilidades: "1/5"
            const int pages = player ? player->SkillPageCount() : 1;
            if (pages <= 1) continue;   // sin paginas: no se muestra (no hay boton inutil)
            const char* t = TextFormat("%d/%d", player ? player->skillPage % pages + 1 : 1, pages);
            DrawText("PAG", (int)(b.center.x - MeasureText("PAG", 10) / 2.0f), (int)(b.center.y - 11), 10, {245, 240, 235, a});
            DrawText(t, (int)(b.center.x - MeasureText(t, 10) / 2.0f), (int)(b.center.y + 1), 10, {255, 214, 72, a});
            continue;
        }
        const int fs = b.radius >= 55.0f ? 20 : (b.radius >= 40.0f ? 14 : 11);
        const int w = MeasureText(b.label, fs);
        if (art) {   // icono: etiqueta en espanol debajo
            DrawText(b.label, (int)(b.center.x - MeasureText(b.label, 10) / 2.0f), (int)(b.center.y + b.radius + 4.0f), 10, {245, 240, 235, 235});
            continue;
        }
        DrawText(b.label, (int)(b.center.x - w / 2.0f), (int)(b.center.y - fs / 2.0f), fs, {245, 240, 235, a});
    }
}

}  // namespace touch
}  // namespace district_fury
