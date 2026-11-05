#include "game/Stage3Game.h"
#include "game/CharacterVisual.h"
#include "ui/TouchControls.h"
#include "core/InputMap.h"
#include "ui/GameHUD.h"
#include "rendering/AssetManager.h"
#include "rendering/BossSprite.h"
#include "rendering/Backdrop.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {
constexpr float kPi = 3.14159265359f;
constexpr float kLaneMin = 505.0f, kLaneMax = 625.0f;
struct Spawn {
    float x, y;
    StreetEnemyType type;
};
const Spawn kScenarios[3][5] = {{{620, 570, StreetEnemyType::UrbanNinja},
                                 {820, 535, StreetEnemyType::Enforcer},
                                 {1020, 610, StreetEnemyType::Punk},
                                 {1190, 555, StreetEnemyType::UrbanNinja},
                                 {0, 0, StreetEnemyType::Punk}},
                                {{2050, 575, StreetEnemyType::ChemicalSoldier},
                                 {2220, 525, StreetEnemyType::Mutant},
                                 {2390, 610, StreetEnemyType::ArmoredGuard},
                                 {2560, 550, StreetEnemyType::ChemicalSoldier},
                                 {0, 0, StreetEnemyType::Punk}},
                                {{3490, 570, StreetEnemyType::ArmoredGuard},
                                 {3660, 525, StreetEnemyType::UrbanNinja},
                                 {3830, 610, StreetEnemyType::Enforcer},
                                 {4000, 550, StreetEnemyType::Mutant},
                                 {4170, 590, StreetEnemyType::ArmoredGuard}}};
Color A(Color c, float a) {
    c.a = (unsigned char)(std::clamp(a, 0.0f, 1.0f) * 255.0f);
    return c;
}
} // namespace
Stage3Game::Stage3Game() = default;
void Stage3Game::Init() {
    ResetRun();
    flow = Flow::Intro;
}
void Stage3Game::ResetRun() {
    player.Reset();
    player.position = {180, 585, 0};
    enemies.clear();
    projectiles.clear();
    particles.clear();
    combatWorld.Reset();
    scenario = 1;
    combo = maxCombo = defeated = damageTaken = score = 0;
    stageTime = comboTimer = shake = bannerTimer = transitionTimer = 0;
    hitstop.Reset(); camKick.Reset(); slowMo = 0;
    cameraX = 640;
    arenaLocked = false;
    scenarioGatekeeperSpawned = false;
    bossSpawned = false;
    stageComplete = false;
    boss = TitanX{};
    storyMessage = "Rayden enters Astra Tower. The chain network ends in the corporate command floor.";
    BuildScenario(1);
}
int Stage3Game::ScenarioStartX() const {
    return scenario == 1 ? 180 : scenario == 2 ? 1850 : 3290;
}
int Stage3Game::ScenarioEndX() const {
    return scenario == 1 ? 1700 : scenario == 2 ? 3140 : 5000;
}
const char* Stage3Game::ScenarioName() const {
    switch (scenario) {
    case 1:
        return "ASTRA TOWER // PUBLIC ATRIUM";
    case 2:
        return "ASTRA TOWER // RESEARCH FLOOR";
    default:
        return "ASTRA TOWER // EXECUTIVE CORE";
    }
}
const char* Stage3Game::ScenarioObjective() const {
    switch (scenario) {
    case 1:
        return "Break the security cordon.";
    case 2:
        return "Cross the restricted research floor.";
    default:
        return "Reach the executive core and expose Titan-X.";
    }
}
const char* Stage3Game::DifficultyText() const {
    return difficulty == Difficulty::Easy ? "EASY" : difficulty == Difficulty::Hard ? "HARD" : "NORMAL";
}
void Stage3Game::ApplyDifficulty() {
    const float hp = difficulty == Difficulty::Easy ? .84f : difficulty == Difficulty::Hard ? 1.22f : 1.f;
    const float dmg = difficulty == Difficulty::Easy ? .82f : difficulty == Difficulty::Hard ? 1.20f : 1.f;
    for (auto& e : enemies) {
        e.hp = std::max(1, (int)std::round(e.hp * hp));
        e.maxHp = e.hp;
        e.attackDamage = std::max(1.0f, e.attackDamage * dmg);
    }
    boss.maxHp = difficulty == Difficulty::Easy ? 1250 : difficulty == Difficulty::Hard ? 1800 : 1500;
    boss.hp = boss.maxHp;
}
void Stage3Game::BuildScenario(int id) {
    enemies.clear();
    scenarioGatekeeperSpawned = false;
    const int i = std::clamp(id - 1, 0, 2);
    for (const auto& s : kScenarios[i])
        if (s.x > 0) {
            StreetEnemy e;
            e.Init({s.x, s.y, 0}, s.type);
            e.active = false;
            enemies.push_back(e);
        }
    ApplyDifficulty();
}
bool Stage3Game::AllEnemiesDefeated() const {
    return !enemies.empty() &&
           std::all_of(enemies.begin(), enemies.end(), [](const StreetEnemy& e) { return e.IsDefeated(); });
}
void Stage3Game::SpawnScenarioGatekeeper() {
    if (scenario >= 3) {
        AdvanceScenario();
        return;
    }
    scenarioGatekeeperSpawned = true;
    arenaLocked = true;
    EmitSimpleEvent(GameEventType::ArenaLock, -1);   // F0.2: musica (sin Sfx)
    flow = Flow::Gatekeeper;
    bannerTimer = 1.7f;
    enemies.clear();
    StreetEnemy e;
    const auto type = scenario == 1 ? StreetEnemyType::ArmoredGuard : StreetEnemyType::Mutant;
    e.Init({(float)ScenarioEndX() - 180, 570, 0}, type);
    e.active = true;
    e.hp += scenario == 1 ? 320 : 450;
    e.maxHp = e.hp;
    e.attackDamage += scenario == 1 ? 5.f : 8.f;
    enemies.push_back(e);
    storyMessage = scenario == 1 ? "ASTRA SECURITY CAPTAIN: unauthorized access detected."
                                 : "RESEARCH WARDEN: the tower will not surrender its prototype floor.";
}
void Stage3Game::AdvanceScenario() {
    if (scenario >= 3) {
        EnterBoss();
        return;
    }
    ++scenario;
    combo = 0;
    comboTimer = 0;
    transitionTimer = 2.2f;
    bannerTimer = 2.2f;
    flow = Flow::Intro;
    BuildScenario(scenario);
    player.position.x = (float)ScenarioStartX();
    storyMessage = scenario == 2
                       ? "The public atrium was only a filter. Research access is deeper inside the tower."
                       : "The prototype floor is breached. One door remains: the executive core.";
}
void Stage3Game::EnterBoss() {
    bossSpawned = true;
    boss = TitanX{};
    boss.pos = {5350, 575, 0};
    ApplyDifficulty();
    boss.phase = 1;
    boss.attackTimer = 1.3f;
    boss.powerTimer = 2;
    boss.shieldTimer = .7f;
    boss.blocking = false;
    arenaLocked = true;
    EmitSimpleEvent(GameEventType::ArenaLock, -1);   // F0.2: musica (sin Sfx)
    player.position.x = 4880;
    flow = Flow::BossIntro;
    bannerTimer = 2.8f;
    storyMessage = "TITAN-X // ASTRA CORE: military-grade combat AI activated.";
}
void Stage3Game::DefeatBoss() {
    boss.hp = 0;
    boss.defeated = true;
    boss.blocking = false;
    arenaLocked = false;
    stageComplete = true;
    flow = Flow::Clear;
    player.EnterVictory();   // F0.4: pose de victoria al vencer al jefe
    score += 5000 + maxCombo * 50;
    SpawnImpact(boss.pos, {80, 220, 255, 255}, true);
}
void Stage3Game::SpawnImpact(Vector3D p, Color c, bool heavy) {
    const int n = heavy ? 30 : 14;
    for (int i = 0; i < n; ++i) {
        float a = (float)i / n * 2 * kPi;
        float s = (float)GetRandomValue(70, heavy ? 360 : 220);
        particles.push_back({p,
                             {std::cos(a) * s, std::sin(a) * s},
                             heavy ? .5f : .28f,
                             heavy ? .5f : .28f,
                             (float)GetRandomValue(3, heavy ? 9 : 6),
                             c});
    }
}
void Stage3Game::SpawnEnergyProjectile() {
    float d = player.facing == Facing::Right ? 1.f : -1.f;
    projectiles.push_back({{player.position.x + d * 70, player.position.y - 70, 0},
                           d * 760,
                           .9f,
                           22,
                           player.isRageMode ? 34 : 24,
                           false,
                           true});
    SpawnImpact({player.position.x + d * 45, player.position.y - 70, 0}, {55, 215, 255, 255}, true);
}
void Stage3Game::SpawnBossProjectile(int damage, float speed) {
    float d = player.position.x >= boss.pos.x ? -1.f : 1.f;
    projectiles.push_back(
        {{boss.pos.x + d * 90, boss.pos.y - 70, 0}, d * speed, 1.45f, 25, damage, true, true});
    SpawnImpact({boss.pos.x + d * 55, boss.pos.y - 70, 0}, {255, 75, 190, 255}, true);
}
void Stage3Game::UpdateProjectiles(float dt) {
    CombatFeedback fb = Feedback();
    StageHitStyle st = HitStyle();
    for (auto& p : projectiles) {
        if (!p.active) continue;
        const float prevX = p.pos.x;
        p.pos.x += p.vx * dt;
        p.life -= dt;
        if (p.life <= 0) {
            p.active = false;
            continue;
        }
        // Las ondas se guardan en coordenadas de pantalla (carril - 70).
        ProjectileHitInput pr;
        pr.pos = {p.pos.x, p.pos.y + 70.f, 70.f};
        pr.prevX = prevX;
        pr.radius = p.radius;
        pr.height = 70.f;
        pr.damage = p.damage;
        pr.dirX = p.vx > 0 ? 1.f : -1.f;
        if (!p.fromBoss) {
            st.spark = {50, 215, 255, 255};
            HitReport rep;
            if (ResolveProjectileVsEnemies(pr, enemies, st, fb, &rep, player.skin) >= 0) {
                p.active = false;
                ++combo;
                comboTimer = 1;
                maxCombo = std::max(maxCombo, combo);
                score += 180 + combo * 10;
            }
            if (flow == Flow::Boss && !boss.defeated && p.active) {
                BossProxy bp = BossTarget(160.f, 150.f);
                bp.pushOnBlock = 0.0f;   // la onda no empuja al jugador
                st.sparkBoss = {255, 185, 55, 255};
                st.sparkBlock = {110, 180, 230, 255};
                st.spark = {55, 215, 255, 255};
                if (ResolveProjectileVsBoss(pr, bp, st, fb)) {
                    p.active = false;
                    ++combo;
                    comboTimer = 1;
                    maxCombo = std::max(maxCombo, combo);
                    score += bp.blocking ? 100 : 300;
                    if (boss.hp <= 0) DefeatBoss();
                }
            }
        } else if (p.active) {
            st.sparkHurt = {255, 65, 150, 255};
            if (ResolveProjectileVsPlayer(pr, player, fb, false, &st, &damageTaken)) {
                combo = 0;
                comboTimer = 0;
                p.active = false;
            }
        }
    }
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }),
        projectiles.end());
}
void Stage3Game::HandlePlayerHits() {
    CombatFeedback fb = Feedback();
    const StageHitStyle st = HitStyle();
    const HitReport r = ResolvePlayerVsEnemies(player, enemies, st, fb);
    ApplyComboScore(r, st, combo, maxCombo, comboTimer, &score);
}
void Stage3Game::HandleEnemyHits() {
    CombatFeedback fb = Feedback();
    const StageHitStyle st = HitStyle();
    const int before = player.hp;
    ResolveEnemiesVsPlayer(enemies, player, fb, 2, &st, &damageTaken);
    if (before != player.hp) {
        combo = 0;
        comboTimer = 0;
    }
}
void Stage3Game::HandleBossHits() {
    if (flow != Flow::Boss || boss.defeated) return;
    CombatFeedback fb = Feedback();
    StageHitStyle st = HitStyle();
    BossProxy bp = BossTarget(164.f, 150.f);
    if (!ResolvePlayerVsBoss(player, bp, st, fb)) return;
    ++combo;
    comboTimer = 1;
    maxCombo = std::max(maxCombo, combo);
    score += bp.blocking ? 120 : 280 + combo * 12;
    if (boss.hp <= 0) DefeatBoss();
}
void Stage3Game::UpdateCombat(float dt) {
    if (comboTimer > 0)
        comboTimer -= dt;
    else
        combo = 0;
    player.Update(dt);
    if (player.ConsumeProjectileEvent()) SpawnEnergyProjectile();   // B01
    for (auto& e : enemies)
        if (e.active) e.Update(dt, player, &combatWorld);
    HandlePlayerHits();
    UpdateProjectiles(dt);
    HandleEnemyHits();
    combatWorld.Update(dt);
    combatWorld.ResolveHazards(player, enemies);
    float left = (float)ScenarioStartX(),
          right = arenaLocked ? (float)ScenarioEndX() - 150.f : (float)ScenarioEndX();
    player.position.x = std::clamp(player.position.x, left, right);
    player.position.y = std::clamp(player.position.y, kLaneMin, kLaneMax);
    if (AllEnemiesDefeated() && !scenarioGatekeeperSpawned) {
        ++defeated;
        SpawnScenarioGatekeeper();
        return;
    }
    if (scenarioGatekeeperSpawned && AllEnemiesDefeated()) {
        ++defeated;
        AdvanceScenario();
        return;
    }
    if (player.state == PlayerState::Defeat) flow = Flow::GameOver;
}
void Stage3Game::UpdateBoss(float dt) {
    if (boss.invuln > 0) boss.invuln -= dt;
    if (boss.shieldTimer > 0) boss.shieldTimer -= dt;
    if (boss.attackTimer > 0) boss.attackTimer -= dt;
    if (flow == Flow::BossIntro) {
        bannerTimer -= dt;
        player.position.x = std::min(player.position.x, boss.pos.x - 270.f);
        if (bannerTimer <= 0) flow = Flow::Boss;
        return;
    }
    if (boss.defeated) return;
    float ratio = (float)boss.hp / boss.maxHp;
    int phase = ratio <= .33f ? 3 : ratio <= .66f ? 2 : 1;
    if (phase != boss.phase) {
        boss.phase = phase;
        EmitSimpleEvent(GameEventType::BossPhase, phase);
        boss.shieldTimer = 1;
        boss.blocking = true;
        SpawnImpact(boss.pos, {255, 70, 190, 255}, true);
        shake = .25f;
    }
    bool threat = player.AttackIsActive() || player.attackType == AttackType::Energy;
    if (boss.shieldTimer <= 0) {
        float chance = boss.phase == 1 ? .34f : boss.phase == 2 ? .46f : .58f;
        if (threat && boss.attackTimer < .25f && GetRandomValue(0, 1000) < (int)(chance * 1000)) {
            boss.blocking = true;
            boss.shieldTimer = boss.phase == 3 ? .75f : .55f;
        } else if (boss.blocking && boss.attackTimer <= 0)
            boss.blocking = false;
    }
    if (boss.attackTimer <= 0 && !boss.blocking) {
        boss.attackTimer = boss.phase == 3 ? .72f : boss.phase == 2 ? .95f : 1.18f;
        int attack = GetRandomValue(0, boss.phase == 3 ? 3 : 2);
        float dir = player.position.x > boss.pos.x ? 1.f : -1.f;
        if (attack == 0) {
            boss.pos.x += dir * (boss.phase == 3 ? 220.f : 170.f);
            SpawnImpact(boss.pos, {255, 80, 190, 255}, false);
        } else if (attack == 1)
            SpawnBossProjectile(boss.phase == 3   ? 34
                                : boss.phase == 2 ? 29
                                                  : 24,
                                boss.phase == 3 ? 850.f : 680.f);
        else {
            float distance = std::abs(player.position.x - boss.pos.x);
            if (distance < 270) {
                int before = player.hp;
                player.ApplyHit(BossHitInfo(boss.phase == 3 ? 42 : boss.phase == 2 ? 34 : 28, boss.pos.x, player));
                damageTaken += before - player.hp;
                combo = 0;
                comboTimer = 0;
                SpawnImpact(player.position, {255, 65, 150, 255}, true);
                hitstop.Trigger(.11f);
                shake = .2f;
            } else
                SpawnBossProjectile(boss.phase == 3 ? 30 : 24, 760.f);
        }
    }
    boss.pos.x = std::clamp(boss.pos.x, 5050.f, 5680.f);
    boss.pos.y = std::clamp(boss.pos.y, kLaneMin, kLaneMax);
    HandleBossHits();
    if (player.state == PlayerState::Defeat) flow = Flow::GameOver;
}
void Stage3Game::UpdateParticles(float dt) {
    for (auto& p : particles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y += 180 * dt;
        p.life -= dt;
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0; }),
        particles.end());
}
void Stage3Game::Update(float dt) {
    if (input::Pressed(KEY_P) || input::Pressed(KEY_ESCAPE))
        if (flow != Flow::Clear && flow != Flow::GameOver && flow != Flow::BossIntro) {
            flow = flow == Flow::Pause ? Flow::Combat : Flow::Pause;
        }
    if (flow == Flow::Pause) {
        if (input::Pressed(KEY_ENTER)) flow = Flow::Combat;
        return;
    }
    if (flow == Flow::Intro) {
        if (scenario == 1 && transitionTimer <= 0) {
            if (input::Pressed(KEY_ONE)) difficulty = Difficulty::Easy;
            if (input::Pressed(KEY_TWO)) difficulty = Difficulty::Normal;
            if (input::Pressed(KEY_THREE)) difficulty = Difficulty::Hard;
            // Cruceta tactil: izquierda/derecha cambian la dificultad.
            if (input::Pressed(KEY_RIGHT))
                difficulty = difficulty == Difficulty::Easy ? Difficulty::Normal : Difficulty::Hard;
            if (input::Pressed(KEY_LEFT))
                difficulty = difficulty == Difficulty::Hard ? Difficulty::Normal : Difficulty::Easy;
            if (input::Pressed(KEY_ENTER)) {
                flow = Flow::Combat;
                bannerTimer = 1.8f;
                ApplyDifficulty();
            }
        } else if (transitionTimer > 0) {
            transitionTimer -= dt;
            if (transitionTimer <= 0) flow = Flow::Combat;
        }
    } else if (flow == Flow::Combat || flow == Flow::Gatekeeper) {
        stageTime += dt;
        if (bannerTimer > 0) bannerTimer -= dt;
        PumpPlayerInput(player, hitstop, dt);   // B15
        if (!hitstop.Consume(dt)) {
            camKick.Update(dt);
            if (slowMo > 0) { slowMo -= dt; dt *= kKillSlowMoScale; }   // V7
            UpdateCombat(dt);
        }
    } else if (flow == Flow::BossIntro || flow == Flow::Boss) {
        stageTime += dt;
        PumpPlayerInput(player, hitstop, dt);   // B15
        if (!hitstop.Consume(dt)) {
            camKick.Update(dt);
            if (slowMo > 0) { slowMo -= dt; dt *= kKillSlowMoScale; }   // V7
            player.Update(dt);
            if (player.ConsumeProjectileEvent()) SpawnEnergyProjectile();   // B01
            UpdateBoss(dt);
            UpdateProjectiles(dt);
        }
    } else if (flow == Flow::Clear) {
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J)) advanceRequested = true;
    } else if (flow == Flow::GameOver) {
        if (input::Pressed(KEY_ENTER)) {
            ResetRun();
            flow = Flow::Combat;
        }
    }
    if (comboTimer > 0)
        comboTimer -= dt;
    else
        combo = 0;
    UpdateParticles(dt);
    cameraX = std::clamp(player.position.x, 640.f, 5360.f);
}
void Stage3Game::DrawWorld() const {
    // Camera2D es {offset, target, ...}: antes iban al reves y la camara se movia al reves
    // (al avanzar, jugador y fondo salian de la pantalla).
    Camera2D cam{{640, 360}, {cameraX + camKick.OffsetX(), 360}, 0, 1};   // V7
    BeginMode2D(cam);
    // DF-013.2: fondo real por escenario; el 4o es la sala del boss. La
    // escenografia de bloques solo se dibuja si falta el fondo (antes lo tapaba).
    const Texture2D scenarioBg =
        AssetManager::Get().GetTexture(TextFormat("bg_s3_%d", bossSpawned ? 4 : std::clamp(scenario, 1, 3)));
    if (scenarioBg.id) {
        DrawScenarioBackdrop(scenarioBg, cameraX, 0.35f);
    } else {
        DrawRectangle(-200, 0, 6400, 720, {6, 8, 18, 255});
        DrawRectangle(-200, 160, 6400, 300, {11, 18, 38, 255});
        for (int x = 0; x < 6200; x += 320) {
            DrawRectangle(x, 170, 270, 180, {15, 25, 48, 255});
            DrawRectangle(x + 18, 195, 96, 72, {25, 70, 105, 255});
            DrawRectangle(x + 138, 195, 96, 72, {105, 42, 104, 255});
            DrawLine(x, 350, x + 270, 350, {70, 95, 130, 180});
        }
        for (int x = 0; x < 6200; x += 160) {
            DrawLineEx({(float)x, 365}, {(float)x + 80, 430}, 4, {35, 55, 85, 220});
            DrawLineEx({(float)x + 80, 430}, {(float)x + 160, 365}, 4, {35, 55, 85, 220});
        }
        DrawRectangle(-200, 430, 6400, 220, {9, 12, 22, 255});
        DrawRectangle(-200, 645, 6400, 75, {3, 5, 10, 255});
        for (int x = 0; x < 6200; x += 240) {
            DrawRectangle(x, 455, 12, 170, {45, 55, 80, 255});
            DrawRectangle(x + 55, 470, 170, 9, {100, 40, 115, 220});
            DrawCircle(x + 30, 445, 5, {70, 220, 255, 220});
        }
    }
    combatWorld.DrawGround();
    for (const auto& p : particles)
        DrawCircleV({p.pos.x, p.pos.y - p.pos.z}, p.size, A(p.color, p.life / p.maxLife));
    for (const auto& p : projectiles) {
        if (!p.fromBoss && player.DrawEnergyProjectile({p.pos.x, p.pos.y - 70}, p.vx < 0, (float)GetTime()))
            continue;
        Color c = p.fromBoss ? Color{255, 65, 190, 255} : Color{60, 220, 255, 255};
        DrawCircleV({p.pos.x, p.pos.y - 70}, p.radius, A(c, .9f));
        DrawCircleV({p.pos.x, p.pos.y - 70}, p.radius * .42f, {245, 245, 255, 255});
    }
    combatWorld.DrawEffects();
    for (const auto& e : enemies)
        if (e.active) e.Draw();
    if (!bossSpawned || !boss.defeated) player.Draw();
    if (bossSpawned && (flow == Flow::BossIntro || flow == Flow::Boss)) DrawBoss();
    EndMode2D();
}
// F1.1: dibujo compartido de los jefes Titan-X (src/game/visual/Poses_TitanX.cpp).
void DrawTitanXBossBody(int skin, PlayerState state, int attackId, float elapsed, float startup, float active,
                        float recovery, float stateTime, float walkPhase, Vector2 feet, float targetHeight,
                        bool facingRight, Color tint);
// DF-013.2: Titan-X (prototipo, Stage 3) ahora se dibuja con el sprite real
// aportado por el usuario en vez de las primitivas raylib originales. La
// pose se elige a partir del estado real del boss (blocking/invuln/
// attackTimer); este boss no tiene una ventana de "ataque activo" separada
// (resuelve el ataque instantaneo en UpdateBoss), asi que no hay una pose
// de ataque sostenida que mostrar con precision — se aproxima con "lean"
// justo antes de que se cumpla el temporizador. Ver docs/AUTONOMOUS_PROGRESS.md.
void Stage3Game::DrawBoss() const {
    if (!bossSpawned || boss.defeated) return;
    const float x = boss.pos.x, y = boss.pos.y;
    // F1.1: el mismo cuerpo que el Titan-X jugable (orientacion por pose, escala
    // uniforme, PoseFx): mira al jugador. Este jefe resuelve sus ataques en un
    // instante (UpdateBoss); se ensena la CARGA del golpe en los 0.18 s previos
    // al temporizador y el golpe completo + recuperacion en los 0.3 s siguientes.
    const bool facingRight = player.position.x > x;
    if (boss.blocking) DrawCircleV({x, y - 95}, 105, A({90, 170, 255, 255}, .22f));
    const Color tint = boss.invuln > 0 ? Color{255, 180, 180, 255} : WHITE;
    constexpr float kS = 9 / 60.0f, kA = 5 / 60.0f, kR = 12 / 60.0f;   // MARTILLO de Moves_TitanX.cpp
    const float reset = boss.phase == 3 ? .72f : boss.phase == 2 ? .95f : 1.18f;
    const float since = reset - boss.attackTimer;
    const int punch = static_cast<int>(AttackId::Punch1);
    if (boss.invuln > .08f)
        DrawTitanXBossBody(4, PlayerState::Hit, 0, 0, 0, 0, 0, .12f - boss.invuln, 0, {x, y}, 170.f, facingRight, tint);
    else if (boss.blocking)
        DrawTitanXBossBody(4, PlayerState::Block, 0, 0, 0, 0, 0, boss.elapsed, 0, {x, y}, 170.f, facingRight, tint);
    else if (boss.attackTimer > 0 && boss.attackTimer <= .18f)
        DrawTitanXBossBody(4, PlayerState::Attack, punch, (.18f - boss.attackTimer) / .18f * kS, kS, kA, kR, 0, 0, {x, y},
                           170.f, facingRight, tint);
    else if (since >= 0 && since < .3f)
        DrawTitanXBossBody(4, PlayerState::Attack, punch, kS + since / .3f * (kA + kR), kS, kA, kR, 0, 0, {x, y}, 170.f,
                           facingRight, tint);
    else
        DrawTitanXBossBody(4, PlayerState::Idle, 0, 0, 0, 0, 0, boss.elapsed, 0, {x, y}, 170.f, facingRight, tint);
}
void Stage3Game::DrawHUD() const {
    DrawRectangle(18, 16, 430, 88, {5, 8, 16, 225});
    DrawText("DISTRICT FURY // ASTRA TOWER", 32, 27, 18, {180, 220, 235, 255});
    DrawText(ScenarioName(), 32, 51, 15, {255, 115, 205, 255});
    DrawText(ScenarioObjective(), 32, 73, 13, {195, 205, 220, 255});
    // DF-013.2: antes esta zona solo dibujaba HP y Rage sueltos, sin panel ni
    // Shield/SP (inconsistente con Stage1/Stage2/VS). Se reemplaza por el
    // panel compartido, que agrega Shield/SP/retrato faltantes. El combo
    // propio de este stage (mas abajo, "%d HIT") se conserva y se apaga el
    // combo interno del panel compartido para no duplicarlo.
    {
        ui::PlayerVitals vitals{};
    vitals.player = &player;
        vitals.hp = player.hp;
        vitals.maxHp = player.maxHp;
        vitals.shield = player.shield;
        vitals.maxShield = player.maxShield;
        vitals.sp = player.sp;
        vitals.maxSp = player.maxSp;
        vitals.rage = player.rage;
        vitals.maxRage = player.maxRage;
        vitals.isRageMode = player.isRageMode;
        vitals.combo = 0;
        vitals.title = TextFormat("%s // ASTRA TOWER", ShortCharacterName(player.skin).c_str());
        vitals.x = 18;
        vitals.y = 110;
        vitals.width = 430;
        vitals.panelHeight = 96;
        ui::DrawPlayerVitals(vitals);
    }
    if (combo > 1) {
        DrawText(TextFormat("%d HIT", combo), 1050, 112, 30, {255, 205, 75, 255});
        DrawText(TextFormat("MAX %d", maxCombo), 1060, 143, 13, {210, 220, 235, 255});
    }
    DrawText(TextFormat("SCORE %06d", score), 1040, 24, 16, {240, 240, 245, 255});
    DrawText(TextFormat("TIME %05.1f", stageTime), 1040, 46, 13, {170, 190, 210, 255});
    DrawText(TextFormat("DIFF %s", DifficultyText()), 1040, 66, 12, {170, 190, 210, 255});
    if (bossSpawned && (flow == Flow::BossIntro || flow == Flow::Boss)) {
        DrawRectangle(380, 18, 520, 54, {7, 8, 18, 230});
        DrawText("TITAN-X // ASTRA CORE", 515, 25, 20, {255, 90, 190, 255});
        DrawRectangle(430, 51, 420, 12, {28, 30, 42, 255});
        DrawRectangle(430, 51, (int)(420.f * std::max(0.f, (float)boss.hp / boss.maxHp)), 12,
                      {95, 190, 255, 255});
        DrawText(TextFormat("PHASE %d", boss.phase), 875, 49, 12, {190, 205, 220, 255});
    }
}
void Stage3Game::DrawClear() const {
    DrawRectangle(0, 0, 1280, 720, {4, 7, 14, 235});
    DrawText("ASTRA TOWER // CLEARED", 355, 135, 34, {80, 225, 255, 255});
    DrawText("TITAN-X has been shut down.", 475, 190, 18, {225, 230, 240, 255});
    float r = score + maxCombo * 120.f - damageTaken * 3.f - stageTime * 2.f;
    const char* rank = r > 12000  ? "SSS"
                       : r > 9000 ? "SS"
                       : r > 7000 ? "S"
                       : r > 5000 ? "A"
                       : r > 3500 ? "B"
                       : r > 2000 ? "C"
                                  : "D";
    DrawText(TextFormat("RANK %s", rank), 550, 255, 42, {255, 205, 75, 255});
    DrawText(TextFormat("SCORE %06d", score), 525, 330, 18, RAYWHITE);
    DrawText(TextFormat("MAX COMBO %d", maxCombo), 520, 360, 16, {205, 215, 230, 255});
    DrawText(TextFormat("DAMAGE TAKEN %d", damageTaken), 515, 390, 16, {205, 215, 230, 255});
    DrawText("ENTER — STAGE 4: KESSLER TOWER", 455, 485, 16, {185, 205, 220, 255});
}
void Stage3Game::DrawOverlay() const {
    if (flow == Flow::Intro) {
        DrawRectangle(0, 0, 1280, 720, {4, 6, 14, 225});
        DrawText("DISTRICT FURY", 465, 110, 42, {230, 235, 245, 255});
        DrawText("STAGE 3 // ASTRA TOWER", 440, 168, 24, {255, 90, 190, 255});
        DrawText(storyMessage, 230, 230, 18, {205, 215, 230, 255});
        if (scenario == 1 && transitionTimer <= 0) {
            DrawText("1 EASY    2 NORMAL    3 HARD    (< > CAMBIAR)", 420, 305, 17, {170, 195, 215, 255});
            DrawText(TextFormat("SELECTED: %s", DifficultyText()), 545, 340, 18, {80, 220, 255, 255});
            DrawText("ENTER  START", 535, 440, 20, RAYWHITE);
        } else
            DrawText("TRANSITIONING...", 525, 350, 18, {80, 220, 255, 255});
        if (!touch::Enabled())
            DrawText("WASD MOVE   J PUNCH   K KICK   L ENERGY   SHIFT DASH   SPACE RAGE", 265, 555, 13,
                     {165, 180, 200, 255});
    } else if (flow == Flow::Gatekeeper) {
        DrawRectangle(280, 270, 720, 120, {5, 8, 16, 225});
        DrawText("GATEKEEPER", 530, 292, 25, {255, 120, 190, 255});
        DrawText(storyMessage, 350, 335, 15, {210, 220, 235, 255});
    } else if (flow == Flow::BossIntro) {
        DrawRectangle(170, 270, 940, 130, {4, 6, 15, 235});
        DrawText("TITAN-X // ASTRA CORE", 430, 292, 28, {255, 80, 190, 255});
        DrawText(storyMessage, 305, 340, 16, {210, 220, 235, 255});
        DrawText("READ THE TELEGRAPH. BREAK THE SHIELD WINDOW.", 400, 370, 13, {90, 220, 255, 255});
    } else if (flow == Flow::Pause) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 175});
        DrawText("PAUSED", 550, 280, 38, RAYWHITE);
        DrawText("ENTER / P / ESC  RESUME", 485, 340, 16, {190, 205, 220, 255});
    } else if (flow == Flow::GameOver) {
        DrawRectangle(0, 0, 1280, 720, {15, 3, 10, 220});
        DrawText("ASTRA TOWER // FAILURE", 380, 210, 32, {255, 75, 125, 255});
        DrawText("ENTER  RESTART", 515, 350, 18, RAYWHITE);
    } else if (flow == Flow::Clear)
        DrawClear();
}
void Stage3Game::Draw() const {
    ClearBackground({5, 7, 14, 255});
    DrawWorld();
    DrawHUD();
    DrawOverlay();
}
// ---- F0.2: resolvedor unico de golpes ----
CombatFeedback Stage3Game::Feedback() {
    CombatFeedback fb;
    fb.hitstop = &hitstop;
    fb.camera = &camKick;
    fb.slowMo = &slowMo;
    fb.spark = [this](Vector3D at, Color c, bool heavy) { SpawnImpact(at, c, heavy); };
    return fb;
}
StageHitStyle Stage3Game::HitStyle() const {
    StageHitStyle st;
    st.rageBonus = 6;
    st.bossRageBonus = 9;
    st.scorePerDamage = 10;
    st.scorePerCombo = 8;
    st.spark = {255, 235, 150, 255};
    st.sparkHeavy = {255, 150, 45, 255};
    st.sparkHurt = {255, 65, 120, 255};
    st.sparkBoss = {255, 175, 55, 255};
    st.sparkBlock = {105, 180, 230, 255};
    return st;
}
BossProxy Stage3Game::BossTarget(float w, float h) {
    BossProxy bp;
    bp.pos = boss.pos;
    bp.width = w;
    bp.height = h;
    bp.hp = &boss.hp;
    bp.blocking = boss.blocking;
    bp.invuln = &boss.invuln;
    bp.invulnOnHit = .11f;
    bp.blockDivisor = 6;
    bp.pushOnBlock = 150.0f;
    return bp;
}

} // namespace district_fury
