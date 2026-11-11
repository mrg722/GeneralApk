#include "ui/GameHUD.h"
#include "rendering/AssetManager.h"
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/AttackData.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

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
    if (v.player) {
        // Retrato del personaje en uso (su propio sprite, cabeza y torso).
        DrawRectangle(portraitX, portraitY, 70, 70, {18, 28, 40, 255});
        BeginScissorMode(portraitX, portraitY, 70, 70);
        v.player->DrawPortrait({static_cast<float>(portraitX), static_cast<float>(portraitY), 70, 70});
        EndScissorMode();
    } else {
        const Texture2D portrait = AssetManager::Get().GetTexture("rayden_clean");
        const bool isRayden = v.title && std::strncmp(v.title, "RAYDEN", 6) == 0;
        if (portrait.id && isRayden) {
            DrawTexturePro(portrait, {0, 0, 96, 96},
                           {static_cast<float>(portraitX), static_cast<float>(portraitY), 70, 70},
                           {0, 0}, 0, WHITE);
        } else if (v.title && v.title[0]) {
            DrawRectangle(portraitX, portraitY, 70, 70, {18, 28, 40, 255});
            const char initial[2] = {v.title[0], 0};
            DrawText(initial, portraitX + 35 - MeasureText(initial, 48) / 2, portraitY + 11, 48,
                     {185, 220, 255, 255});
        }
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

// ---------------------------------------------------------------------------
// F2.3 - HUD de combate
// ---------------------------------------------------------------------------

float ComboDamageScale(int hits) {
    return std::max(0.35f, 1.0f - 0.08f * static_cast<float>(std::max(0, hits - 3)));
}

Color ComboScaleColor(float scale) {
    // 1.0 blanco, 0.8 amarillo, 0.6 naranja, 0.35 rojo.
    auto lerp = [](Color a, Color b, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return Color{static_cast<unsigned char>(a.r + (b.r - a.r) * t), static_cast<unsigned char>(a.g + (b.g - a.g) * t),
                     static_cast<unsigned char>(a.b + (b.b - a.b) * t), 255};
    };
    const Color white{245, 245, 250, 255}, yellow{255, 220, 70, 255}, orange{255, 140, 45, 255}, red{255, 60, 55, 255};
    if (scale >= 0.8f) return lerp(yellow, white, (scale - 0.8f) / 0.2f);
    if (scale >= 0.6f) return lerp(orange, yellow, (scale - 0.6f) / 0.2f);
    return lerp(red, orange, (scale - 0.35f) / 0.25f);
}

namespace {
constexpr float kComboHold = 1.4f;      // s sin golpear y el contador se apaga
constexpr float kSpecialShow = 0.8f;    // s que se ve el nombre del especial
constexpr float kFloatLife = 0.85f;

Color HudFade(Color c, float a) {
    c.a = static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
    return c;
}

void ShadowText(const char* text, int x, int y, int size, Color c) {
    ::DrawText(text, x + 2, y + 2, size, HudFade({0, 0, 0, 255}, c.a / 255.0f * 0.85f));
    ::DrawText(text, x, y, size, c);
}

int WidthOf(const char* text, int size) {
    return IsWindowReady() ? MeasureText(text, size) : static_cast<int>(std::strlen(text)) * size * 6 / 10;
}
}  // namespace

CombatHud& Combat() {
    static CombatHud hud;
    return hud;
}

void CombatHud::Reset() {
    floaters_.clear();
    notices_.clear();
    seen_.clear();
    hits_ = comboDamage_ = 0;
    stageCombo_ = -1;
    comboT_ = shownT_ = pulse_ = 0;
    special_.clear();
    specialT_ = 0;
    lastAttack_ = 0;
    lastState_ = -1;
    lastSkill_ = -1;
    lastJuggle_ = 0;
}

void CombatHud::Notice(const char* text, Color color, float seconds, int fontSize) {
    if (!text || !text[0]) return;
    for (NoticeItem& n : notices_)
        if (n.text == text) { n.t = 0; n.life = seconds; n.color = color; return; }   // el mismo aviso se renueva
    NoticeItem n;
    n.text = text; n.color = color; n.life = seconds; n.size = fontSize;
    notices_.insert(notices_.begin(), n);
    if (notices_.size() > 3) notices_.pop_back();
}

void CombatHud::AddHit(int damage, Vector3D at, bool counter) {
    if (damage <= 0) return;
    if (comboT_ <= 0.0f) { hits_ = 0; comboDamage_ = 0; lastJuggle_ = 0; }
    ++hits_;
    comboDamage_ += damage;
    comboT_ = kComboHold;
    pulse_ = 0.16f;
    Floater f;
    f.text = std::to_string(damage);
    f.at = at;
    f.at.z += 96.0f;
    f.at.x += static_cast<float>((hits_ * 37) % 31 - 15);   // no se apilan en el mismo punto
    f.life = kFloatLife;
    f.size = 20 + std::min(damage, 40) / 2;
    f.color = counter ? Color{255, 220, 60, 255} : damage >= 25 ? Color{255, 150, 60, 255} : Color{255, 255, 255, 255};
    floaters_.push_back(f);
    if (floaters_.size() > 24) floaters_.erase(floaters_.begin());
    if (counter) Notice("CONTRA!", {255, 215, 60, 255}, 0.9f, 34);
}

void CombatHud::Observe(int key, int hp, Vector3D pos, bool startup, bool /*airborne*/, int airHits) {
    Seen* s = nullptr;
    for (Seen& v : seen_) if (v.key == key) { s = &v; break; }
    if (!s) {
        seen_.push_back({key, hp, startup, true});
        return;
    }
    if (s->valid && s->hp > hp && s->hp - hp < 100000) {
        const int dmg = s->hp - std::max(0, hp);
        if (dmg > 0) {
            AddHit(dmg, pos, s->startup);
            if (airHits >= 3 && airHits > lastJuggle_) {
                lastJuggle_ = airHits;
                char buf[24];
                std::snprintf(buf, sizeof(buf), "JUGGLE x%d", airHits);
                Notice(buf, ComboScaleColor(ComboDamageScale(airHits)), 0.9f, 28);
            }
        }
    }
    s->hp = hp;
    s->startup = startup;
    s->valid = true;
}

void CombatHud::ObserveTarget(int key, int hp, Vector3D pos, bool inStartup) {
    Observe(key, hp, pos, inStartup, false, 0);
}

void CombatHud::Update(float dt, const Player& player, const std::vector<StreetEnemy>& enemies, int stageCombo) {
    dt = std::clamp(dt, 0.0f, 0.1f);
    stageCombo_ = stageCombo;
    // Reloj de los elementos vivos.
    for (Floater& f : floaters_) f.t += dt;
    floaters_.erase(std::remove_if(floaters_.begin(), floaters_.end(), [](const Floater& f) { return f.t >= f.life; }),
                    floaters_.end());
    for (NoticeItem& n : notices_) n.t += dt;
    notices_.erase(std::remove_if(notices_.begin(), notices_.end(), [](const NoticeItem& n) { return n.t >= n.life; }),
                   notices_.end());
    if (comboT_ > 0.0f) comboT_ = std::max(0.0f, comboT_ - dt);
    if (pulse_ > 0.0f) pulse_ = std::max(0.0f, pulse_ - dt);
    if (specialT_ > 0.0f) specialT_ = std::max(0.0f, specialT_ - dt);
    shownT_ += dt;

    // Si el jugador cae, el contador se apaga pronto.
    if (player.state == PlayerState::Defeat) comboT_ = std::min(comboT_, 0.4f);

    // Golpes: cada caida de vida de un enemigo entre dos Update.
    static constexpr int kBossKeyBase = 1000;
    const int n = static_cast<int>(enemies.size());
    int known = 0;
    for (const Seen& v : seen_) if (v.key < kBossKeyBase) ++known;
    if (known > n) {   // se retiro un enemigo: los indices se corrieron; se vuelve a leer sin contar golpes
        seen_.erase(std::remove_if(seen_.begin(), seen_.end(), [](const Seen& v) { return v.key < kBossKeyBase; }),
                    seen_.end());
    }
    for (int i = 0; i < n; ++i) {
        const StreetEnemy& e = enemies[static_cast<size_t>(i)];
        if (!e.active) { for (Seen& v : seen_) if (v.key == i) v.valid = false; continue; }
        const bool air = e.state == StreetEnemyState::Launched || e.state == StreetEnemyState::WallBounce;
        Observe(i, e.hp, e.position, e.InAttackStartup(), air, e.airHits);
    }

    // Nombre del especial al lanzarlo (0.8 s).
    const int cur = static_cast<int>(player.currentAttack);
    const int st = static_cast<int>(player.state);
    const bool attacking = player.state == PlayerState::Attack;
    const bool fresh = attacking && (lastState_ != static_cast<int>(PlayerState::Attack) || cur != lastAttack_ ||
                                     player.activeSkill != lastSkill_);
    if (fresh) {
        const AttackId id = player.currentAttack;
        const char* name = nullptr;
        if (id == AttackId::Skill && player.activeSkill >= 0) name = player.SkillName(player.activeSkill);
        else if (AttackIsSpecialMove(id) || AttackIsSuper(id) || (player.activeSkill >= 0 && id != AttackId::None))
            name = player.CurrentAttackDef().name;
        if (name && name[0] && name[0] != '-') { special_ = name; specialT_ = kSpecialShow; }
    }
    lastAttack_ = cur;
    lastState_ = st;
    lastSkill_ = player.activeSkill;
}

void CombatHud::DrawWorld() const {
    for (const Floater& f : floaters_) {
        const float k = f.t / f.life;
        const float rise = 46.0f * (1.0f - (1.0f - k) * (1.0f - k));   // sube y frena
        const float pop = k < 0.12f ? 1.0f + 0.5f * (1.0f - k / 0.12f) : 1.0f;
        const int size = static_cast<int>(f.size * pop);
        const float a = k < 0.6f ? 1.0f : 1.0f - (k - 0.6f) / 0.4f;
        const int w = WidthOf(f.text.c_str(), size);
        const int x = static_cast<int>(f.at.x) - w / 2;
        const int y = static_cast<int>(f.at.y - f.at.z - rise) - size / 2;
        ShadowText(f.text.c_str(), x, y, size, HudFade(f.color, a));
    }
}

void CombatHud::DrawScreen() const {
    // Contador de golpes (derecha): N GOLPES, dano acumulado y escalado.
    const int n = stageCombo_ > 1 ? std::max(stageCombo_, hits_) : hits_;
    if (n >= 2 && comboT_ > 0.0f) {
        const float scale = ComboDamageScale(n);
        const Color col = ComboScaleColor(scale);
        const float a = comboT_ < 0.4f ? comboT_ / 0.4f : 1.0f;
        const int big = 44 + (pulse_ > 0.0f ? static_cast<int>(10.0f * pulse_ / 0.16f) : 0);
        const char* num = TextFormat("%d", n);
        const int xr = 1250;
        const int numW = WidthOf(num, big);
        ShadowText("GOLPES", xr - WidthOf("GOLPES", 22), 214, 22, HudFade(col, a));
        ShadowText(num, xr - WidthOf("GOLPES", 22) - 10 - numW, 188 + (54 - big) / 2 + 4, big, HudFade(col, a));
        const char* dmg = TextFormat("DANO %d", comboDamage_);
        ShadowText(dmg, xr - WidthOf(dmg, 18), 246, 18, HudFade({255, 235, 190, 255}, a));
        if (scale < 0.999f) {
            const char* sc = TextFormat("ESCALADO %d%%", static_cast<int>(std::round(scale * 100.0f)));
            ShadowText(sc, xr - WidthOf(sc, 14), 270, 14, HudFade(col, a));
        }
    }
    // Nombre del especial.
    if (specialT_ > 0.0f && !special_.empty()) {
        const float k = specialT_ / kSpecialShow;   // 1 -> 0
        const float a = k > 0.25f ? 1.0f : k / 0.25f;
        const int size = 34 + (k > 0.9f ? static_cast<int>((k - 0.9f) * 120.0f) : 0);
        const int w = WidthOf(special_.c_str(), size);
        DrawRectangle(640 - w / 2 - 18, 150, w + 36, size + 12, HudFade({6, 12, 20, 255}, 0.62f * a));
        DrawRectangle(640 - w / 2 - 18, 150, 4, size + 12, HudFade({90, 210, 235, 255}, a));
        ShadowText(special_.c_str(), 640 - w / 2, 156, size, HudFade({200, 240, 255, 255}, a));
    }
    // Avisos apilados (CONTRA!, JUGGLE xN, PERFECTO...).
    int y = 214;
    for (const NoticeItem& nt : notices_) {
        const float k = nt.t / nt.life;
        const float a = k < 0.7f ? 1.0f : 1.0f - (k - 0.7f) / 0.3f;
        const int size = nt.size + (k < 0.1f ? static_cast<int>((1.0f - k / 0.1f) * 10.0f) : 0);
        const int w = WidthOf(nt.text.c_str(), size);
        ShadowText(nt.text.c_str(), 640 - w / 2, y, size, HudFade(nt.color, a));
        y += nt.size + 8;
    }
}

}  // namespace ui
}  // namespace district_fury
