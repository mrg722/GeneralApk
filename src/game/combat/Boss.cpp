#include "game/combat/Boss.h"
#include "rendering/AssetManager.h"
#include "rendering/BossSprite.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace district_fury {
namespace {

// Altura objetivo en pantalla para los bosses que ya tienen sprite real
// (mismos valores usados en Stage3/4/5Game::DrawBoss — ver esos archivos y
// BossSprite.h para la justificacion de la escala segun el canon del
// usuario: Titan-X > Rayden, forma final ~2-3x, el clon del tamaño de un
// humano).
float SpriteTargetHeight(BossId id) {
    switch (id) {
        case BossId::TitanX: return 170.0f;
        case BossId::TitanXMejorado: return 235.0f;
        case BossId::RayderClone: return 130.0f;
        case BossId::Brakk: return 175.0f;
        case BossId::Grinder: return 150.0f;
        default: return 0.0f;
    }
}

// Brakk y Grinder no vienen recortados pose a pose: Brakk comparte lienzo
// 256x256 (suelo en y=247) y Grinder se apoya por abajo. Por eso se dibujan
// con escala uniforme (DrawSpriteUniform) y no normalizando cada pose.
bool UsesUniformCanvas(BossId id) { return id == BossId::Brakk || id == BossId::Grinder; }
// Pixeles de pantalla por pixel de sprite. Brakk: 175px de alto util sobre un
// idle de 188px. Grinder: 3x exacto (pixel art pequeno, factor entero).
float UniformScale(BossId id) { return id == BossId::Brakk ? 175.0f / 188.0f : 3.0f; }
float CanvasFootInset(BossId id) { return id == BossId::Brakk ? 9.0f : 0.0f; }

const char* SpriteFolder(BossId id) {
    switch (id) {
        case BossId::TitanX: return "titanx";
        case BossId::TitanXMejorado: return "titanx_mejorado";
        case BossId::RayderClone: return "rayder_clone";
        case BossId::Brakk: return "brakk";
        case BossId::Grinder: return "grinder";
        default: return nullptr;
    }
}

// Mapea el nombre real de cada BossAttackDef (ver BossDefinition.cpp) a una
// pose existente en assets/bosses/<boss>/. Devuelve nullptr si ese boss no
// tiene sprite o el ataque no tiene pose dedicada (se usa idle).
const char* PoseForAttack(BossId id, const char* attackName) {
    auto eq = [&](const char* s) { return attackName && std::strcmp(attackName, s) == 0; };
    if (id == BossId::Brakk) {
        if (eq("ChainSwing")) return "chain";
        if (eq("GroundSmash")) return "smash";
        if (eq("Charge")) return "charge";
        if (eq("PowerWave")) return "chain_throw";
        if (eq("Frenzy")) return "fury";
        return "idle";
    }
    if (id == BossId::Grinder) {
        if (eq("Saw")) return "saw";
        if (eq("Slam")) return "slam";
        if (eq("Ram")) return "ram";
        if (eq("Overdrive")) return "overdrive";
        return "idle";
    }
    if (id == BossId::TitanX) {
        if (eq("Dash")) return "lean";
        if (eq("GuardCounter")) return "punch2";
        if (eq("ProjectileBurst")) return "shoot_orb";
    } else if (id == BossId::TitanXMejorado) {
        if (eq("Dash")) return "dash";
        if (eq("Shockwave")) return "slam";
        if (eq("ProjectileBurst")) return "shoot_orb";
        if (eq("Overdrive")) return "rage_aura";
    } else if (id == BossId::RayderClone) {
        if (eq("MirrorCombo")) return "punch";
        if (eq("DarkWave")) return "release_orb";
        if (eq("Dash")) return "dash";
        if (eq("Finisher")) return "kick";
    }
    return nullptr;
}

}  // namespace

void Boss::Reset(BossId bossId, Vector3D startPos) {
    id = bossId;
    def = &GetBossDefinition(bossId);
    pos = startPos;
    hp = def->maxHp;
    maxHp = def->maxHp;
    phase = 1;
    attackTimer = 1.0f;
    elapsed = 0.0f;
    invuln = 0.0f;
    currentAttack = -1;
    attackResolved = false;
    defeated = false;
}

const BossPhaseDef& Boss::ActivePhase() const {
    // phases[] esta ordenado de mayor a menor hpRatioThreshold, con un
    // ultimo elemento "base" de threshold 0. El indice de fase activa es
    // 1 + cuantos umbrales (salvo el base) ya se cruzaron.
    const float ratio = maxHp > 0 ? static_cast<float>(hp) / static_cast<float>(maxHp) : 1.0f;
    std::size_t activeIndex = def->phases.size() - 1;  // por defecto, la fase base
    for (std::size_t i = 0; i + 1 < def->phases.size(); ++i) {
        if (ratio <= def->phases[i].hpRatioThreshold) { activeIndex = i; break; }
    }
    return def->phases[activeIndex];
}

void Boss::PickAttack() {
    const float ratio = maxHp > 0 ? static_cast<float>(hp) / static_cast<float>(maxHp) : 1.0f;
    std::vector<int> eligible;
    for (std::size_t i = 0; i < def->attacks.size(); ++i) {
        const BossAttackDef& a = def->attacks[i];
        if (a.minPhaseHpRatio <= 0.0f || ratio <= a.minPhaseHpRatio) eligible.push_back(static_cast<int>(i));
    }
    if (eligible.empty()) { currentAttack = -1; attackTimer = 1.0f; return; }
    currentAttack = eligible[static_cast<std::size_t>(GetRandomValue(0, static_cast<int>(eligible.size()) - 1))];
    elapsed = 0.0f;
    attackResolved = false;
}

void Boss::Update(float dt, Player& player, CombatWorld* world, std::vector<BossProjectile>& projectiles, float* hitstop, float* shake) {
    if (!def || defeated) return;
    if (invuln > 0.0f) invuln -= dt;

    const int newPhase = static_cast<int>(std::distance(
        def->phases.begin(),
        std::find_if(def->phases.begin(), def->phases.end(), [&](const BossPhaseDef& p) { return &p == &ActivePhase(); }))) + 1;
    if (newPhase != phase) {
        phase = newPhase;
        if (world) world->DoShake(0.2f);
    }
    const BossPhaseDef& ph = ActivePhase();

    const float dx = player.position.x - pos.x;
    const bool facingRight = dx > 0.0f;

    if (currentAttack < 0) {
        attackTimer -= dt;
        constexpr float kPreferredDistance = 190.0f;
        if (std::abs(dx) > kPreferredDistance) pos.x += (facingRight ? 1.0f : -1.0f) * 95.0f * ph.speedMultiplier * dt;
        if (attackTimer <= 0.0f) PickAttack();
    } else {
        const BossAttackDef& atk = def->attacks[static_cast<std::size_t>(currentAttack)];
        elapsed += dt;
        const bool active = elapsed >= atk.startup && elapsed < atk.startup + atk.active;
        if (active && !attackResolved) {
            if (atk.spawnsProjectile) {
                projectiles.push_back({{pos.x + (facingRight ? 60.0f : -60.0f), pos.y - 80.0f, 0.0f},
                                        (facingRight ? 1.0f : -1.0f) * atk.projectileSpeed, 1.3f,
                                        static_cast<int>(atk.damage * ph.damageMultiplier), true, true});
                attackResolved = true;
            } else {
                const CombatBox hit{facingRight ? pos.x : pos.x - atk.boxForward - atk.boxWidth,
                                     pos.y - atk.boxHeight, atk.boxWidth, atk.boxHeight};
                if (hit.Intersects(player.GetHurtbox()) && player.state != PlayerState::Hit) {
                    const int dmg = static_cast<int>(atk.damage * ph.damageMultiplier);
                    player.TakeDamage(dmg);
                    if (hitstop) *hitstop = std::max(*hitstop, atk.hitstop);
                    if (shake) *shake = std::max(*shake, atk.shake);
                    attackResolved = true;
                }
            }
        }
        if (elapsed >= atk.startup + atk.active + atk.recovery) {
            currentAttack = -1;
            elapsed = 0.0f;
            attackTimer = atk.cooldown * ph.attackCooldownMultiplier;
        }
    }

    pos.x = std::clamp(pos.x, 150.0f, 1130.0f);
    if (hp <= 0) defeated = true;
}

void Boss::ApplyDamage(int dmg) {
    if (!CanBeHit()) return;
    hp = std::max(0, hp - dmg);
    invuln = 0.1f;
    if (hp <= 0) defeated = true;
}

CombatBox Boss::GetHurtbox() const {
    if (!def) return CombatBox{};
    return CombatBox{pos.x - def->bodyWidth * 0.5f, pos.y - def->bodyHeight, def->bodyWidth, def->bodyHeight};
}

void Boss::Draw(float playerX) const {
    if (!def || defeated) return;
    const bool facingRight = playerX > pos.x;
    const char* folder = SpriteFolder(id);
    if (folder) {
        const char* pose = UsesUniformCanvas(id) ? "idle" : "idle1";
        if (currentAttack >= 0) {
            const char* mapped = PoseForAttack(id, def->attacks[static_cast<std::size_t>(currentAttack)].name);
            if (mapped) pose = mapped;
        }
        char key[96];
        std::snprintf(key, sizeof(key), "%s_%s", folder, pose);
        const Texture2D tex = AssetManager::Get().GetTexture(key);
        const Color tint = invuln > 0.0f ? Color{255, 190, 190, 255} : WHITE;
        // El sprite mira a la izquierda por defecto para titanx/titanx_mejorado
        // y a la derecha para rayder_clone (ver comentarios de Stage3/4/5Game).
        const bool flip = (id == BossId::RayderClone || id == BossId::Brakk || id == BossId::Grinder) ? !facingRight : facingRight;
        if (UsesUniformCanvas(id)) {
            DrawSpriteUniform(tex, {pos.x, pos.y}, UniformScale(id), flip, tint, CanvasFootInset(id));
        } else {
            DrawBossPose(tex, {pos.x, pos.y}, SpriteTargetHeight(id), flip, tint);
        }
        return;
    }
    // Brakk / Grinder: sin sprite real todavia -> silueta generica a partir
    // de BossDefinition (mas simple que el dibujo a mano de Stage1/Stage2,
    // que sigue siendo el que se usa en Historia).
    const float w = def->bodyWidth, h = def->bodyHeight;
    DrawEllipse(static_cast<int>(pos.x), static_cast<int>(pos.y), w * 0.4f, h * 0.08f, {0, 0, 0, 150});
    DrawRectangle(static_cast<int>(pos.x - w * 0.5f), static_cast<int>(pos.y - h), static_cast<int>(w), static_cast<int>(h), def->baseColor);
    DrawRectangleLines(static_cast<int>(pos.x - w * 0.5f), static_cast<int>(pos.y - h), static_cast<int>(w), static_cast<int>(h), {0, 0, 0, 180});
    DrawCircle(static_cast<int>(pos.x), static_cast<int>(pos.y - h - h * 0.12f), w * 0.16f, def->baseColor);
}

}  // namespace district_fury
