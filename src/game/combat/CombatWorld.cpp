#include "game/combat/CombatWorld.h"
#include "game/events/GameEvents.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {
constexpr float kPi = 3.14159265359f;

Color WithAlpha(Color color, float alpha) {
    color.a = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
    return color;
}

Color HazardColor(HazardKind kind) {
    switch (kind) {
        case HazardKind::Fire:      return {255, 120, 45, 255};
        case HazardKind::Steam:     return {205, 225, 235, 255};
        case HazardKind::Acid:      return {120, 235, 90, 255};
        case HazardKind::Electric:  return {120, 200, 255, 255};
        case HazardKind::Laser:     return {255, 70, 120, 255};
        default:                    return {255, 170, 60, 255};
    }
}

Color PickupColor(PickupType type) {
    switch (type) {
        case PickupType::Health:     return {70, 220, 120, 255};
        case PickupType::Energy:     return {70, 205, 255, 255};
        case PickupType::Rage:       return {255, 110, 70, 255};
        case PickupType::Coins:      return {255, 205, 75, 255};
        case PickupType::DamageBuff: return {255, 150, 60, 255};
        default:                     return {170, 220, 255, 255};
    }
}
}  // namespace

void CombatWorld::Reset() {
    projectiles.clear();
    particles.clear();
    pickups.clear();
    destructibles.clear();
    hazards.clear();
    hitstop = 0.0f;
    shake = 0.0f;
    combo = 0;
    maxCombo = 0;
    score = 0;
    defeated = 0;
    damageTaken = 0;
    coinsCollected = 0;
    comboTimer = 0.0f;
}

void CombatWorld::DoHitstop(float duration) { hitstop = std::max(hitstop, duration); }
void CombatWorld::DoShake(float amount) { shake = std::max(shake, amount); }

void CombatWorld::SpawnImpact(Vector3D position, Color color, bool heavy) {
    const int count = heavy ? 28 : 14;
    particles.reserve(particles.size() + static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const float angle = static_cast<float>(i) / static_cast<float>(count) * 2.0f * kPi;
        const float speed = static_cast<float>(GetRandomValue(70, heavy ? 340 : 220));
        Particle particle;
        particle.position = position;
        particle.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        particle.life = heavy ? 0.46f : 0.26f;
        particle.maxLife = particle.life;
        particle.size = static_cast<float>(GetRandomValue(3, heavy ? 9 : 6));
        particle.color = color;
        particles.push_back(particle);
    }
}

void CombatWorld::SpawnDebris(Vector3D position, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        Particle particle;
        particle.position = position;
        particle.velocity = {static_cast<float>(GetRandomValue(-220, 220)),
                             static_cast<float>(GetRandomValue(-320, -60))};
        particle.life = 0.72f;
        particle.maxLife = particle.life;
        particle.size = static_cast<float>(GetRandomValue(2, 7));
        particle.gravity = 780.0f;
        particle.color = color;
        particles.push_back(particle);
    }
}

void CombatWorld::SpawnProjectile(Vector3D position, float velocityX, int damage,
                                  bool fromEnemy, Color color, float radius, float life) {
    Projectile projectile;
    projectile.position = position;
    projectile.velocityX = velocityX;
    projectile.damage = damage;
    projectile.fromEnemy = fromEnemy;
    projectile.color = color;
    projectile.radius = radius;
    projectile.life = life;
    projectile.active = true;
    projectiles.push_back(projectile);
    GameEvent e;
    e.type = GameEventType::ProjectileSpawn;
    e.x = position.x; e.y = position.y; e.fromEnemy = fromEnemy; e.kind = HitKind::Energy;
    EmitGameEvent(e);
}

void CombatWorld::SpawnPlayerEnergyWave(const Player& player) {
    const AttackDef& def = GetAttack(AttackId::EnergyWave);
    const float direction = player.facing == Facing::Right ? 1.0f : -1.0f;
    const int damage = player.isRageMode
        ? static_cast<int>(std::round(def.damage * 1.6f))
        : def.damage;
    SpawnProjectile({player.position.x + direction * 78.0f, player.position.y - 74.0f, 0.0f},
                    direction * 760.0f, damage, false,
                    player.isRageMode ? Color{255, 190, 90, 255} : Color{60, 220, 255, 255},
                    player.isRageMode ? 26.0f : 20.0f);
    SpawnImpact({player.position.x + direction * 45.0f, player.position.y - 74.0f, 0.0f},
                {50, 215, 255, 255}, true);
}

void CombatWorld::SpawnPickup(Vector3D position, PickupType type, int amount) {
    Pickup pickup;
    pickup.position = position;
    pickup.type = type;
    pickup.amount = amount;
    pickup.life = 14.0f;
    pickup.bob = static_cast<float>(GetRandomValue(0, 100)) * 0.01f;
    pickup.active = true;
    pickups.push_back(pickup);
}

void CombatWorld::AddDestructible(Vector3D position, int hp, bool dropsReward, Color color) {
    Destructible destructible;
    destructible.position = position;
    destructible.hp = hp;
    destructible.maxHp = hp;
    destructible.dropsReward = dropsReward;
    destructible.color = color;
    destructible.active = true;
    destructibles.push_back(destructible);
}

void CombatWorld::AddHazard(Vector3D position, HazardKind kind, float width, float height,
                            int damage, float cycle, float activeWindow, float phase) {
    Hazard hazard;
    hazard.position = position;
    hazard.kind = kind;
    hazard.width = width;
    hazard.height = height;
    hazard.damage = damage;
    hazard.cycle = std::max(0.2f, cycle);
    hazard.activeWindow = std::clamp(activeWindow, 0.05f, hazard.cycle);
    hazard.timer = phase;
    hazard.active = true;
    hazards.push_back(hazard);
}

bool CombatWorld::ConsumeHitstop(float dt) {
    if (hitstop <= 0.0f) return false;
    hitstop -= dt;
    return true;
}

void CombatWorld::UpdateParticles(float dt) {
    for (auto& particle : particles) {
        particle.life -= dt;
        particle.velocity.y += particle.gravity * dt;
        particle.position.x += particle.velocity.x * dt;
        particle.position.y += particle.velocity.y * dt;
        if (particle.gravity <= 0.0f) {
            particle.velocity.x *= 0.92f;
            particle.velocity.y *= 0.92f;
        }
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p) { return p.life <= 0.0f; }),
                    particles.end());
}

void CombatWorld::UpdateProjectiles(float dt) {
    for (auto& projectile : projectiles) {
        if (!projectile.active) continue;
        projectile.position.x += projectile.velocityX * dt;
        projectile.life -= dt;
        if (projectile.life <= 0.0f) projectile.active = false;
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                     [](const Projectile& p) { return !p.active; }),
                      projectiles.end());
}

void CombatWorld::Update(float dt) {
    shake = std::max(0.0f, shake - dt);
    if (comboTimer > 0.0f) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) combo = 0;
    }
    UpdateParticles(dt);
    UpdateProjectiles(dt);

    for (auto& pickup : pickups) {
        if (!pickup.active) continue;
        pickup.life -= dt;
        pickup.bob += dt;
        if (pickup.life <= 0.0f) pickup.active = false;
    }
    pickups.erase(std::remove_if(pickups.begin(), pickups.end(),
                                 [](const Pickup& p) { return !p.active; }),
                  pickups.end());

    for (auto& destructible : destructibles) {
        destructible.shake = std::max(0.0f, destructible.shake - dt * 4.0f);
    }

    for (auto& hazard : hazards) {
        if (!hazard.active) continue;
        hazard.timer += dt;
        if (hazard.timer >= hazard.cycle) hazard.timer -= hazard.cycle;
        hazard.tickTimer = std::max(0.0f, hazard.tickTimer - dt);
    }
}

void CombatWorld::RegisterCombo(int scoreGain) {
    ++combo;
    comboTimer = comboWindow;
    maxCombo = std::max(maxCombo, combo);
    score += scoreGain + combo * 8;
}

void CombatWorld::BreakCombo() {
    combo = 0;
    comboTimer = 0.0f;
}

void CombatWorld::ResolveDestructibles(Player& player) {
    if (!player.AttackIsActive() || player.hasHit) return;
    const CombatBox hitbox = player.GetAttackHitbox();
    if (!hitbox.IsValid()) return;

    for (auto& destructible : destructibles) {
        if (!destructible.active) continue;
        const CombatBox crate{destructible.position.x - destructible.width * 0.5f,
                              destructible.position.y - destructible.height,
                              destructible.width, destructible.height};
        if (!hitbox.Intersects(crate)) continue;

        destructible.hp -= player.GetAttackDamage();
        destructible.shake = 1.0f;
        player.hasHit = true;
        SpawnDebris(destructible.position, destructible.color, 9);
        DoHitstop(0.04f);
        DoShake(0.06f);
        GameEvent ev;
        ev.type = GameEventType::PropHit;
        ev.x = destructible.position.x; ev.y = destructible.position.y; ev.kind = HitKind::Prop;
        EmitGameEvent(ev);

        if (destructible.hp <= 0) {
            destructible.active = false;
            score += 60;
            SpawnDebris(destructible.position, destructible.color, 18);
            if (destructible.dropsReward) {
                const int roll = GetRandomValue(0, 99);
                if (roll < 45) SpawnPickup(destructible.position, PickupType::Coins, 25);
                else if (roll < 70) SpawnPickup(destructible.position, PickupType::Health, 18);
                else if (roll < 90) SpawnPickup(destructible.position, PickupType::Energy, 25);
                else SpawnPickup(destructible.position, PickupType::Rage, 30);
            }
        }
        break;
    }

    destructibles.erase(std::remove_if(destructibles.begin(), destructibles.end(),
                                       [](const Destructible& d) { return !d.active; }),
                        destructibles.end());
}

void CombatWorld::ResolveHazards(Player& player, std::vector<StreetEnemy>& enemies) {
    for (auto& hazard : hazards) {
        if (!hazard.active) continue;
        const bool dangerous = hazard.timer <= hazard.activeWindow;
        if (!dangerous || hazard.tickTimer > 0.0f) continue;

        const CombatBox area{hazard.position.x - hazard.width * 0.5f,
                             hazard.position.y - hazard.height,
                             hazard.width, hazard.height};

        bool triggered = false;
        if (player.state != PlayerState::Defeat && player.dashInvulnerability <= 0.0f &&
            area.Intersects(player.GetHurtbox())) {
            const int before = player.hp;
            player.TakeDamage(hazard.damage);
            damageTaken += before - player.hp;
            BreakCombo();
            SpawnImpact(player.position, HazardColor(hazard.kind), false);
            DoShake(0.10f);
            triggered = true;
        }

        // Los hazards tambien lastiman a los enemigos: son parte del gameplay,
        // no decorado (brief 15).
        for (auto& enemy : enemies) {
            if (!enemy.active || enemy.IsDefeated()) continue;
            if (!area.Intersects(enemy.GetHurtbox())) continue;
            const bool dead = enemy.hp <= hazard.damage;
            enemy.TakeDamage(hazard.damage, {0.0f, 0.0f, 0.0f});
            if (dead) ++defeated;
            SpawnImpact({enemy.position.x, enemy.position.y - 60.0f, 0.0f},
                        HazardColor(hazard.kind), false);
            triggered = true;
        }

        if (triggered) hazard.tickTimer = 0.55f;
    }
}

void CombatWorld::ResolvePickups(Player& player) {
    if (player.state == PlayerState::Defeat) return;
    for (auto& pickup : pickups) {
        if (!pickup.active) continue;
        const float dx = pickup.position.x - player.position.x;
        const float dy = pickup.position.y - player.position.y;
        if (std::sqrt(dx * dx + dy * dy) > 58.0f) continue;

        switch (pickup.type) {
            case PickupType::Health:
                player.hp = std::min(player.maxHp, player.hp + pickup.amount);
                break;
            case PickupType::Energy:
                player.sp = std::min(player.maxSp, player.sp + pickup.amount);
                break;
            case PickupType::Rage:
                player.AddRage(pickup.amount);
                break;
            case PickupType::Coins:
                coinsCollected += pickup.amount;
                score += pickup.amount;
                break;
            case PickupType::DamageBuff:
                player.damageBuffTimer = 10.0f;
                break;
            case PickupType::SpeedBuff:
                player.speedBuffTimer = 10.0f;
                break;
        }

        pickup.active = false;
        SpawnImpact(pickup.position, PickupColor(pickup.type), false);
        GameEvent ev;
        ev.type = GameEventType::Pickup;
        ev.x = pickup.position.x; ev.y = pickup.position.y; ev.value = static_cast<int>(pickup.type);
        EmitGameEvent(ev);
    }
}

void CombatWorld::DrawGround() const {
    for (const auto& destructible : destructibles) {
        if (!destructible.active) continue;
        const float wobble = destructible.shake > 0.0f
            ? static_cast<float>(GetRandomValue(-2, 2)) * destructible.shake : 0.0f;
        const Vector2 screen = destructible.position.ToScreen();
        const float scale = DepthScaleFor(destructible.position.y);
        const float width = destructible.width * scale;
        const float height = destructible.height * scale;
        DrawEllipse(static_cast<int>(screen.x), static_cast<int>(screen.y),
                    width * 0.5f, 7.0f * scale, {0, 0, 0, 130});
        DrawRectangle(static_cast<int>(screen.x - width * 0.5f + wobble),
                      static_cast<int>(screen.y - height), static_cast<int>(width),
                      static_cast<int>(height), destructible.color);
        DrawRectangleLines(static_cast<int>(screen.x - width * 0.5f + wobble),
                           static_cast<int>(screen.y - height), static_cast<int>(width),
                           static_cast<int>(height), {12, 14, 16, 200});
        const float ratio = destructible.maxHp > 0
            ? static_cast<float>(destructible.hp) / destructible.maxHp : 1.0f;
        if (ratio < 1.0f) {
            DrawRectangle(static_cast<int>(screen.x - width * 0.5f),
                          static_cast<int>(screen.y - height - 8), static_cast<int>(width), 3,
                          {10, 12, 14, 200});
            DrawRectangle(static_cast<int>(screen.x - width * 0.5f),
                          static_cast<int>(screen.y - height - 8),
                          static_cast<int>(width * ratio), 3, {255, 180, 70, 230});
        }
    }

    for (const auto& hazard : hazards) {
        if (!hazard.active) continue;
        const bool dangerous = hazard.timer <= hazard.activeWindow;
        const float warmup = std::clamp((hazard.timer - hazard.activeWindow) /
                                        std::max(0.05f, hazard.cycle - hazard.activeWindow),
                                        0.0f, 1.0f);
        const Color color = HazardColor(hazard.kind);
        const int x = static_cast<int>(hazard.position.x - hazard.width * 0.5f);
        const int y = static_cast<int>(hazard.position.y - hazard.height);
        const int w = static_cast<int>(hazard.width);
        const int h = static_cast<int>(hazard.height);

        if (dangerous) {
            DrawRectangle(x, y, w, h, WithAlpha(color, 0.34f));
            DrawRectangleLines(x, y, w, h, WithAlpha(color, 0.95f));
            for (int i = 0; i < 5; ++i) {
                const int px = x + GetRandomValue(0, std::max(1, w));
                const int py = y + GetRandomValue(0, std::max(1, h));
                DrawCircle(px, py, static_cast<float>(GetRandomValue(2, 5)),
                           WithAlpha(color, 0.85f));
            }
        } else {
            // Telegraph: el area se marca antes de activarse.
            DrawRectangleLines(x, y, w, h, WithAlpha(color, 0.18f + 0.45f * (1.0f - warmup)));
        }
    }
}

void CombatWorld::DrawEffects() const {
    for (const auto& pickup : pickups) {
        if (!pickup.active) continue;
        const Vector2 screen = pickup.position.ToScreen();
        const float bob = std::sin(pickup.bob * 4.0f) * 6.0f;
        const Color color = PickupColor(pickup.type);
        const float fade = pickup.life < 3.0f
            ? (std::sin(pickup.life * 14.0f) * 0.5f + 0.5f) : 1.0f;
        DrawEllipse(static_cast<int>(screen.x), static_cast<int>(screen.y), 14, 5,
                    {0, 0, 0, 120});
        DrawCircle(static_cast<int>(screen.x), static_cast<int>(screen.y - 26 + bob), 14,
                   WithAlpha(color, 0.30f * fade));
        DrawCircle(static_cast<int>(screen.x), static_cast<int>(screen.y - 26 + bob), 8,
                   WithAlpha(color, fade));
        DrawCircleLines(static_cast<int>(screen.x), static_cast<int>(screen.y - 26 + bob), 13,
                        WithAlpha({255, 255, 255, 255}, 0.55f * fade));
    }

    for (const auto& projectile : projectiles) {
        if (!projectile.active) continue;
        const Vector2 screen = projectile.position.ToScreen();
        DrawCircle(static_cast<int>(screen.x), static_cast<int>(screen.y),
                   projectile.radius, WithAlpha(projectile.color, 0.28f));
        DrawCircle(static_cast<int>(screen.x), static_cast<int>(screen.y),
                   projectile.radius * 0.55f, projectile.color);
        DrawCircle(static_cast<int>(screen.x), static_cast<int>(screen.y),
                   projectile.radius * 0.25f, {245, 250, 255, 255});
    }

    for (const auto& particle : particles) {
        const Vector2 screen = particle.position.ToScreen();
        const float alpha = particle.maxLife > 0.0f ? particle.life / particle.maxLife : 0.0f;
        DrawCircle(static_cast<int>(screen.x), static_cast<int>(screen.y),
                   particle.size * alpha, WithAlpha(particle.color, alpha));
    }
}

}  // namespace district_fury
