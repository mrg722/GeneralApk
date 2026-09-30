// Stage 1: combate contra enemigos (golpes, proyectiles, impactos, particulas).
#include "game/stage1/Stage1Common.h"

namespace district_fury {

void Stage1StoryGame::SpawnImpact(Vector3D p, Color c, bool heavy) {
    int n = heavy ? 28 : 14;
    for (int i = 0; i < n; ++i) {
        float a = (float)i / n * 2 * kPi, s = (float)GetRandomValue(70, heavy ? 340 : 220);
        particles.push_back({p,
                             {std::cos(a) * s, std::sin(a) * s},
                             heavy ? .46f : .26f,
                             heavy ? .46f : .26f,
                             (float)GetRandomValue(3, heavy ? 9 : 6),
                             c});
    }
}

void Stage1StoryGame::SpawnEnergyProjectile() {
    float d = player.facing == Facing::Right ? 1.f : -1.f;
    projectiles.push_back({{player.position.x + d * 78, player.position.y - 74, 0},
                           d * 760,
                           .9f,
                           20,
                           player.isRageMode ? 32 : 23,
                           true,
                           false});
    SpawnImpact({player.position.x + d * 45, player.position.y - 74, 0}, {50, 215, 255, 255}, true);
}

void Stage1StoryGame::SpawnBossPower() {
    float d = player.position.x >= boss.position.x ? -1.f : 1.f;
    int dmg = boss.phase == 3 ? 32 : boss.phase == 2 ? 27 : 22;
    projectiles.push_back(
        {{boss.position.x + d * 85, boss.position.y - 72, 0}, d * 650, 1.5f, 24, dmg, true, true});
    SpawnImpact({boss.position.x + d * 55, boss.position.y - 72, 0}, {255, 80, 55, 255}, true);
}

void Stage1StoryGame::UpdateProjectiles(float dt) {
    for (auto& p : projectiles) {
        if (!p.active) continue;
        p.position.x += p.velocity * dt;
        p.life -= dt;
        if (p.life <= 0) {
            p.active = false;
            continue;
        }
        CombatBox b{p.position.x - p.radius, p.position.y - p.radius, p.radius * 2, p.radius * 2};
        if (p.fromBoss) {
            if (b.Intersects(player.GetHurtbox()) && !player.IsBlocking() &&
                player.state != PlayerState::Defeat && player.dashInvulnerability <= 0) {
                int before = player.hp;
                player.TakeDamage(p.damage);
                damageTaken += before - player.hp;
                p.active = false;
                combo = 0;
                comboTimer = 0;
                SpawnImpact(player.position, {255, 70, 50, 255}, true);
                hitstop.Trigger(.10f);
                shake = .18f;
            }
        } else {
            for (auto& e : enemies)
                if (e.active && !e.IsDefeated() && b.Intersects(e.GetHurtbox())) {
                    int damage = p.damage;
                    bool dead = e.hp <= damage;
                    e.TakeDamage(damage, {p.velocity > 0 ? 420.f : -420.f, 0, 0});
                    if (dead) ++defeated;
                    p.active = false;
                    ++combo;
                    comboTimer = 1;
                    maxCombo = std::max(maxCombo, combo);
                    score += 150 + combo * 8;
                    SpawnImpact(p.position, {40, 215, 255, 255}, true);
                    hitstop.Trigger(.10f);
                    shake = .13f;
                    break;
                }
            if (flow == StoryFlow::Boss && !boss.defeated && p.active) {
                CombatBox bb{boss.position.x - 70, boss.position.y - 140, 140, 140};
                if (b.Intersects(bb) && boss.invulnerability <= 0) {
                    int damage = p.damage;
                    if (boss.blocking) damage = std::max(1, damage / 5);
                    boss.hp = std::max(0, boss.hp - damage);
                    boss.invulnerability = .12f;
                    p.active = false;
                    ++combo;
                    comboTimer = 1;
                    maxCombo = std::max(maxCombo, combo);
                    score += 260 + combo * 12;
                    SpawnImpact(p.position,
                                boss.blocking ? Color{120, 180, 220, 255} : Color{255, 175, 55, 255}, true);
                    hitstop.Trigger(.12f);
                    shake = .16f;
                }
            }
        }
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                     [](const StoryProjectile& p) { return !p.active; }),
                      projectiles.end());
}

void Stage1StoryGame::HandlePlayerHits() {
    if (!player.AttackIsActive() || player.hasHit || player.attackType == AttackType::Energy) return;
    CombatBox hit = player.GetAttackHitbox();
    for (auto& e : enemies)
        if (e.active && !e.IsDefeated() && hit.Intersects(e.GetHurtbox())) {
            int dmg = player.GetAttackDamage() + (player.isRageMode ? 5 : 0);
            bool dead = e.hp <= dmg;
            float d = player.facing == Facing::Right ? 1.f : -1.f;
            e.TakeDamage(dmg, {d * player.GetAttackKnockback(), 0, player.CurrentAttackDef().launch});
            if (dead) ++defeated;
            player.hasHit = true;
            ++combo;
            comboTimer = 1;
            maxCombo = std::max(maxCombo, combo);
            score += dmg * 10 + combo * 7;
            bool heavy = player.attackType == AttackType::Kick || player.comboStep >= 2;
            SpawnImpact({e.position.x, e.position.y - 68},
                        heavy ? Color{255, 150, 45, 255} : Color{255, 235, 150, 255}, heavy);
            hitstop.Trigger(heavy ? std::max(.08f, player.CurrentAttackDef().hitstop)
                                  : HitstopClock::kDefault);
            shake = heavy ? .14f : .07f;
            break;
        }
}

void Stage1StoryGame::HandleEnemyHits() {
    if (player.state == PlayerState::Defeat) return;
    for (auto& e : enemies)
        if (e.active && !e.IsDefeated() && e.AttackIsActive() && !e.hasHit &&
            e.GetAttackHitbox().Intersects(player.GetHurtbox())) {
            int before = player.hp;
            player.TakeDamage(e.attackDamage);
            if (before != player.hp) {
                damageTaken += before - player.hp;
                combo = 0;
                comboTimer = 0;
                SpawnImpact(player.position,
                            player.IsBlocking() ? Color{80, 190, 255, 255} : Color{255, 80, 70, 255}, false);
                hitstop.Trigger(.06f);
                shake = .08f;
            }
            e.hasHit = true;
            break;
        }
}

void Stage1StoryGame::HandleBossHits() {
    if (flow != StoryFlow::Boss || boss.defeated || boss.invulnerability > 0 || !player.AttackIsActive() ||
        player.hasHit || player.attackType == AttackType::Energy)
        return;
    CombatBox hit = player.GetAttackHitbox(), target{boss.position.x - 78, boss.position.y - 145, 156, 145};
    if (!hit.Intersects(target)) return;
    int dmg = player.GetAttackDamage() + (player.isRageMode ? 8 : 0);
    if (boss.blocking) {
        dmg = std::max(1, dmg / 5);
        SpawnImpact({boss.position.x, boss.position.y - 95}, {110, 180, 220, 255}, true);
    } else
        SpawnImpact({boss.position.x, boss.position.y - 95}, {255, 175, 55, 255}, true);
    boss.hp = std::max(0, boss.hp - dmg);
    boss.invulnerability = .10f;
    player.hasHit = true;
    ++combo;
    comboTimer = 1;
    maxCombo = std::max(maxCombo, combo);
    score += 240 + combo * 12;
    hitstop.Trigger(.11f);
    shake = .18f;
    if (boss.hp <= 0) DefeatFinalBoss();
}

void Stage1StoryGame::UpdateCombat(float dt) {
    if (comboTimer > 0)
        comboTimer -= dt;
    else
        combo = 0;
    player.Update(dt);
    if (player.state == PlayerState::Attack && player.attackType == AttackType::Energy &&
        player.energyReleased) {
        SpawnEnergyProjectile();
        player.energyReleased = false;
    }
    UpdateArena(dt);
    for (auto& e : enemies)
        if (e.active) e.Update(dt, player, &combatWorld);
    for (size_t i = 0; i < enemies.size(); ++i)
        if (enemies[i].active && !enemies[i].IsDefeated())
            for (size_t j = i + 1; j < enemies.size(); ++j)
                if (enemies[j].active && !enemies[j].IsDefeated()) {
                    float dx = enemies[j].position.x - enemies[i].position.x,
                          dy = enemies[j].position.y - enemies[i].position.y,
                          dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < 76.f) {
                        float nx = dist > .01f ? dx / dist : 1.f, ny = dist > .01f ? dy / dist : 0.f,
                              push = (76.f - dist) * .5f;
                        enemies[i].position.x -= nx * push;
                        enemies[j].position.x += nx * push;
                        enemies[i].position.y -= ny * push;
                        enemies[j].position.y += ny * push;
                    }
                }
    HandlePlayerHits();
    UpdateProjectiles(dt);
    HandleEnemyHits();
    combatWorld.Update(dt);
    combatWorld.ResolveHazards(player, enemies);
    ClampToArena();
    if (scenarioBossSpawned && AllCurrentEnemiesDefeated()) {
        AdvanceScenario();
        return;
    }
    if (player.state == PlayerState::Defeat) flow = StoryFlow::GameOver;
}

void Stage1StoryGame::UpdateParticles(float dt) {
    for (auto& p : particles) {
        p.life -= dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.velocity.x *= .92f;
        p.velocity.y *= .92f;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const StoryParticle& p) { return p.life <= 0; }),
                    particles.end());
}

} // namespace district_fury
