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

CombatFeedback Stage1StoryGame::Feedback() {
    CombatFeedback fb;
    fb.hitstop = &hitstop;
    fb.camera = &camKick;
    fb.slowMo = &slowMo;
    fb.spark = [this](Vector3D at, Color c, bool heavy) { SpawnImpact(at, c, heavy); };
    return fb;
}

// Ajustes del Stage 1 para el resolvedor unico (F0.2): +5 de dano en rabia
// contra enemigos y +8 contra Brakk; puntos dano x10 + combo x7.
StageHitStyle Stage1StoryGame::HitStyle() const {
    StageHitStyle st;
    st.rageBonus = 5;
    st.bossRageBonus = 8;
    st.scorePerDamage = 10;
    st.scorePerCombo = 7;
    st.spark = {255, 235, 150, 255};
    st.sparkHeavy = {255, 150, 45, 255};
    st.sparkHurt = {255, 80, 70, 255};
    return st;
}

void Stage1StoryGame::UpdateProjectiles(float dt) {
    CombatFeedback fb = Feedback();
    StageHitStyle st = HitStyle();
    for (auto& p : projectiles) {
        if (!p.active) continue;
        const float prevX = p.position.x;
        p.position.x += p.velocity * dt;
        p.life -= dt;
        if (p.life <= 0) {
            p.active = false;
            continue;
        }
        // La onda se guarda en coordenadas de pantalla (carril - 74).
        ProjectileHitInput pr;
        pr.pos = {p.position.x, p.position.y + (p.fromBoss ? 72.f : 74.f), p.fromBoss ? 72.f : 74.f};
        pr.prevX = prevX;
        pr.radius = p.radius;
        pr.height = pr.pos.z;
        pr.damage = p.damage;
        pr.dirX = p.velocity > 0 ? 1.f : -1.f;
        if (p.fromBoss) {
            // En guardia la onda de Brakk atraviesa sin dano (como siempre).
            if (player.IsBlocking() || player.state == PlayerState::Defeat || player.dashInvulnerability > 0) continue;
            st.sparkHurt = {255, 70, 50, 255};
            if (ResolveProjectileVsPlayer(pr, player, fb, false, &st, &damageTaken)) {
                p.active = false;
                combo = 0;
                comboTimer = 0;
            }
        } else {
            st.spark = {40, 215, 255, 255};
            HitReport rep;
            if (ResolveProjectileVsEnemies(pr, enemies, st, fb, &rep, player.skin) >= 0) {
                defeated += rep.kills;
                p.active = false;
                ++combo;
                comboTimer = 1;
                maxCombo = std::max(maxCombo, combo);
                score += 150 + combo * 8;
            }
            if (flow == StoryFlow::Boss && !boss.defeated && p.active) {
                BossProxy bp;
                bp.pos = boss.position;
                bp.width = 140;
                bp.height = 140;
                bp.hp = &boss.hp;
                bp.blocking = boss.blocking;
                bp.invuln = &boss.invulnerability;
                st.sparkBoss = {255, 175, 55, 255};
                st.sparkBlock = {120, 180, 220, 255};
                if (ResolveProjectileVsBoss(pr, bp, st, fb)) {
                    p.active = false;
                    ++combo;
                    comboTimer = 1;
                    maxCombo = std::max(maxCombo, combo);
                    score += 260 + combo * 12;
                }
            }
        }
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                     [](const StoryProjectile& p) { return !p.active; }),
                      projectiles.end());
}

// F0.2: golpes del jugador por el resolvedor unico (multi-objetivo, carril,
// arco del miembro, OnAttackConnected, hitstop V7).
void Stage1StoryGame::HandlePlayerHits() {
    CombatFeedback fb = Feedback();
    const StageHitStyle st = HitStyle();
    const HitReport r = ResolvePlayerVsEnemies(player, enemies, st, fb);
    defeated += r.kills;
    ApplyComboScore(r, st, combo, maxCombo, comboTimer, &score);
}

void Stage1StoryGame::HandleEnemyHits() {
    CombatFeedback fb = Feedback();
    const StageHitStyle st = HitStyle();
    const int before = player.hp;
    ResolveEnemiesVsPlayer(enemies, player, fb, 99, &st, &damageTaken);
    if (before != player.hp) {
        combo = 0;
        comboTimer = 0;
    }
}

void Stage1StoryGame::HandleBossHits() {
    if (flow != StoryFlow::Boss || boss.defeated) return;
    CombatFeedback fb = Feedback();
    StageHitStyle st = HitStyle();
    st.sparkBoss = {255, 175, 55, 255};
    BossProxy bp;
    bp.pos = boss.position;
    bp.width = 156;
    bp.height = 145;
    bp.hp = &boss.hp;
    bp.blocking = boss.blocking;
    bp.invuln = &boss.invulnerability;
    bp.invulnOnHit = .10f;
    bp.blockDivisor = 5;
    if (!ResolvePlayerVsBoss(player, bp, st, fb)) return;
    ++combo;
    comboTimer = 1;
    maxCombo = std::max(maxCombo, combo);
    score += 240 + combo * 12;
    if (boss.hp <= 0) DefeatFinalBoss();
}

void Stage1StoryGame::UpdateCombat(float dt) {
    if (comboTimer > 0)
        comboTimer -= dt;
    else
        combo = 0;
    player.Update(dt);
    if (player.ConsumeProjectileEvent()) SpawnEnergyProjectile();   // B01: una vez por onda
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
