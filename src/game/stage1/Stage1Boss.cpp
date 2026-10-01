// Stage 1: jefe final Brakk (IA, pelea, derrota).
#include "game/stage1/Stage1Common.h"
#include "rendering/SpriteManifest.h"

namespace district_fury {

// DF-014 balance: vida de Brakk 1000/1200/1450 -> 650/820/1050. La pelea nunca
// habia sido jugable (el jugador no se actualizaba) y con los valores
// originales duraba ~100 s de golpes continuos en Normal. Medido con
// tests/Stage1PlaythroughTest.cpp.
void Stage1StoryGame::EnterFinalBoss() {
    finalBossSpawned = true;
    arenaLocked = true;
    flow = StoryFlow::BossIntro;
    bannerTimer = 2.8f;
    boss = StoryBoss{};
    bossAnimMode = -1;
    boss.position = {5480, 575, 0};
    boss.maxHp = difficulty == StoryDifficulty::Easy ? 650 : difficulty == StoryDifficulty::Hard ? 1050 : 820;
    boss.hp = boss.maxHp;
    boss.phase = 1;
    boss.attackTimer = 1.1f;
    boss.powerTimer = 2.0f;
    boss.blockTimer = .8f;
    storyMessage = "BRAKK // LA CADENA: el ejecutor de la banda entra al astillero.";
}

void Stage1StoryGame::UpdateBoss(float dt) {
    if (boss.invulnerability > 0) boss.invulnerability -= dt;
    if (boss.blockTimer > 0) boss.blockTimer -= dt;
    if (boss.powerTimer > 0) boss.powerTimer -= dt;
    if (flow == StoryFlow::BossIntro) {
        bannerTimer -= dt;
        player.position.x = std::min(player.position.x, boss.position.x - 270);
        if (bannerTimer <= 0) flow = StoryFlow::Boss;
        return;
    }
    if (boss.defeated) return;
    float ratio = (float)boss.hp / boss.maxHp;
    int phase = ratio <= .34f ? 3 : ratio <= .68f ? 2 : 1;
    if (phase != boss.phase) {
        boss.phase = phase;
        boss.attack = StoryBossAttack::Frenzy;
        boss.attackElapsed = 0;
        boss.blockTimer = .65f;
        boss.blocking = true;
        boss.powerTimer = .4f;
        SpawnImpact(boss.position, {255, 70, 45, 255}, true);
        shake = .25f;
    }
    bool threat = player.AttackIsActive() || player.attackType == AttackType::Energy;
    if (boss.blockTimer <= 0 && boss.attack == StoryBossAttack::None) {
        int gc = boss.phase == 1 ? 24 : boss.phase == 2 ? 34 : 46;
        if (threat && GetRandomValue(0, 99) < gc) {
            boss.blockTimer = boss.phase == 3 ? .72f : .58f;
            boss.blocking = true;
        } else
            boss.blocking = false;
    }
    if (boss.blocking) {
        boss.attackElapsed = 0;
        boss.position.x += std::clamp(player.position.x - boss.position.x, -45.f, 45.f) * dt;
        if (boss.blockTimer <= 0) {
            boss.blocking = false;
            boss.attackTimer = .15f;
        }
        HandleBossHits();
        return;
    }
    boss.attackTimer -= dt;
    boss.attackElapsed += dt;
    float dx = player.position.x - boss.position.x, dy = player.position.y - boss.position.y,
          ax = std::abs(dx);
    if (boss.attack == StoryBossAttack::None && boss.attackTimer <= 0) {
        int pick = GetRandomValue(0, boss.phase == 1 ? 3 : 5);
        boss.attack = pick == 0   ? StoryBossAttack::ChainSwing
                      : pick == 1 ? StoryBossAttack::GroundSmash
                      : pick == 2 ? StoryBossAttack::Charge
                      : pick == 3 ? StoryBossAttack::PowerWave
                                  : StoryBossAttack::Frenzy;
        boss.attackElapsed = 0;
        boss.attackTimer = boss.phase == 3 ? 1.05f : boss.phase == 2 ? 1.35f : 1.7f;
    }
    if (boss.attack == StoryBossAttack::None && ax > 190)
        boss.position.x += (dx > 0 ? 1 : -1) *
                           (boss.phase == 3   ? 130.f
                            : boss.phase == 2 ? 100.f
                                              : 78.f) *
                           dt;
    if (boss.attack == StoryBossAttack::None && std::abs(dy) > 30)
        boss.position.y += (dy > 0 ? 1 : -1) * 65.f * dt;
    float tele = boss.attack == StoryBossAttack::Charge        ? .42f
                 : boss.attack == StoryBossAttack::GroundSmash ? .55f
                 : boss.attack == StoryBossAttack::PowerWave   ? .48f
                                                               : .30f;
    bool active = false;
    CombatBox hit{};
    if (boss.attack != StoryBossAttack::None && boss.attackElapsed >= tele) {
        if (boss.attack == StoryBossAttack::ChainSwing) {
            hit = {boss.position.x - 155, boss.position.y - 115, 310, 110};
            active = boss.attackElapsed <= tele + .22f;
        } else if (boss.attack == StoryBossAttack::GroundSmash) {
            hit = {boss.position.x - 290, boss.position.y - 88, 580, 110};
            active = boss.attackElapsed <= tele + .20f;
        } else if (boss.attack == StoryBossAttack::Charge) {
            boss.position.x += (dx > 0 ? 1 : -1) * 560.f * dt;
            hit = {boss.position.x - 100, boss.position.y - 110, 200, 120};
            active = boss.attackElapsed <= tele + .40f;
        } else if (boss.attack == StoryBossAttack::PowerWave) {
            if (boss.attackElapsed < tele + .05f) SpawnBossPower();
        } else {
            hit = {boss.position.x - 190, boss.position.y - 135, 380, 145};
            active = boss.attackElapsed <= tele + .30f;
        }
        if (active && hit.Intersects(player.GetHurtbox()) && player.state != PlayerState::Hit) {
            int dmg = boss.phase == 3 ? 34 : boss.phase == 2 ? 27 : 22;
            int before = player.hp;
            player.TakeDamage(dmg);
            damageTaken += before - player.hp;
            combo = 0;
            comboTimer = 0;
            SpawnImpact(player.position, {255, 70, 50, 255}, true);
            hitstop.Trigger(.10f);
            shake = .20f;
        }
    }
    if (boss.attack != StoryBossAttack::None &&
        boss.attackElapsed > (boss.attack == StoryBossAttack::Charge      ? .92f
                              : boss.attack == StoryBossAttack::PowerWave ? 1.0f
                                                                          : .86f)) {
        boss.attack = StoryBossAttack::None;
        boss.attackElapsed = 0;
    }
    boss.position.x = std::clamp(boss.position.x, 5050.f, 5850.f);
    boss.position.y = std::clamp(boss.position.y, kLaneMin, kLaneMax);
    HandleBossHits();
    if (boss.hp <= 0) DefeatFinalBoss();
}

// DF-014: antes el jugador no se actualizaba durante la pelea con Brakk
// (solo UpdateBoss), por lo que no podia moverse ni atacar y el nivel no
// se podia terminar.
void Stage1StoryGame::UpdateBossFight(float dt) {
    // Animacion del cuerpo de Brakk. Hoja mejorada (atlas "brakk_v2"): un clip
    // por accion. Si no esta, se usa el atlas del Brute como antes.
    if (!brakkV2 && bossAnim.texture.id == 0) {
        const AtlasProfile* prof = SpriteManifest::Get().FindAtlas("brakk_v2");
        brakkV2 = prof && bossAnim.InitFromManifest("brakk_v2", AssetManager::Get().GetTextureByPath(prof->path));
    }
    if (brakkV2) {
        const float dx = std::abs(player.position.x - boss.position.x);
        const char* clip = boss.defeated                                   ? "defeat"
                           : boss.invulnerability > 0.08f                  ? "hit"
                           : boss.blocking                                 ? "block"
                           : boss.attack == StoryBossAttack::ChainSwing    ? "chain"
                           : boss.attack == StoryBossAttack::GroundSmash   ? "smash"
                           : boss.attack == StoryBossAttack::Charge        ? "charge"
                           : boss.attack == StoryBossAttack::PowerWave     ? "chain_throw"
                           : boss.attack == StoryBossAttack::Frenzy        ? (boss.phase >= 3 ? "explosive" : "fury")
                           : dx > 190.f                                    ? (boss.phase >= 3 ? "run" : "walk")
                                                                           : "idle";
        if (bossAnim.currentClipName != clip) bossAnim.PlayNamed(clip);
        bossAnim.Update(dt);
    } else if (StreetEnemy::PrepareAtlasAnimator(bossAnim, StreetEnemyType::Brute)) {
        // 0 reposo, 1 caminar, 2 ataque, 3 golpe, 4 derrota.
        const int mode = boss.defeated                                           ? 4
                         : boss.invulnerability > 0.08f                          ? 3
                         : boss.attack != StoryBossAttack::None                  ? 2
                         : std::abs(player.position.x - boss.position.x) > 190.f ? 1
                                                                                 : 0;
        if (mode != bossAnimMode) {
            bossAnimMode = mode;
            if (mode == 0)
                bossAnim.Play({0, 3, .13f, true});
            else if (mode == 1)
                bossAnim.Play({0, 3, .09f, true});
            else if (mode == 2)
                bossAnim.Play({4, 7, .10f, false});
            else if (mode == 3)
                bossAnim.Play({8, 9, .09f, false});
            else
                bossAnim.Play({10, 11, .16f, false});
        }
        bossAnim.Update(dt);
    }
    if (comboTimer > 0)
        comboTimer -= dt;
    else
        combo = 0;
    if (flow == StoryFlow::Boss) {
        player.Update(dt);
        if (player.state == PlayerState::Attack && player.attackType == AttackType::Energy &&
            player.energyReleased) {
            SpawnEnergyProjectile();
            player.energyReleased = false;
        }
        UpdateProjectiles(dt);
    }
    UpdateBoss(dt);
    player.position.x = std::clamp(player.position.x, 4960.f, 5900.f);
    player.position.y = std::clamp(player.position.y, kLaneMin, kLaneMax);
}

void Stage1StoryGame::DefeatFinalBoss() {
    if (boss.defeated) return;
    boss.defeated = true;
    boss.blocking = false;
    boss.attack = StoryBossAttack::None;
    score += 6000;
    xp += 650;
    coins += 1200;
    gems += 8;
    level = 1 + xp / 1000;
    stageComplete = true;
    stageTime = std::max(stageTime, .1f);
    bestScore = std::max(bestScore, CalculateScore());
    bestRank = std::max(bestRank, CalculateRank());
    SaveProgress();
    SpawnImpact(boss.position, {255, 155, 45, 255}, true);
    shake = .5f;
    flow = StoryFlow::StageClear;
    arenaLocked = false;
}

} // namespace district_fury
