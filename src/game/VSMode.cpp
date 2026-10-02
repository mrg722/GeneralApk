#include "game/VSMode.h"
#include "ui/TouchControls.h"
#include "core/InputMap.h"
#include "game/lab/KfReference.h"
#include "rendering/AssetManager.h"
#include "ui/GameHUD.h"
#include "game/CharacterVisual.h"
#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace district_fury {
namespace {
constexpr float kMinX = 180.0f, kMaxX = 1120.0f, kMinY = 505.0f, kMaxY = 625.0f;
constexpr int kFieldCount = 11;
// DF-013.2: opcion BOSS del selector VS. NONE deja el flujo de enemigos de
// calle intacto; los demas usan la clase Boss compartida (BossDefinition).

const char* kBossNames[] = {"NINGUNO (ENEMIGOS)", "BRAKK",       "GRINDER", "TITAN-X",
                            "TITAN-X MEJORADO",   "RAYDER CLONE"};
constexpr int kBossOptionCount = 6; // NINGUNO + 5 bosses
const char* kStageNames[] = {"STAGE 1 // SLUM DISTRICT", "STAGE 2 // OLD STEEL YARD",
                             "STAGE 3 // ASTRA TOWER", "STAGE 4 // KESSLER TOWER",
                             "STAGE 5 // CAMARA DEL CLON"};
// Los 20 escenarios del juego (4 por stage, assets/backgrounds/stageN_scenarioMM.png).
const int kScenarioCounts[] = {4, 4, 4, 4, 4};
constexpr int kStageCount = 5;
const char* kStage1Scenarios[] = {"BARRIO BAJO // BLOQUE 17", "MERCADO ANTIGUO // LINEA DEL CANAL",
                                  "PUERTA DE ACERO // RUTA DE CARGA",
                                  "ASTILLERO DE CADENAS // TERRITORIO DE BRAKK"};
const char* kStage2Scenarios[] = {"DEEP LINE // TUBERIAS", "OLD STEEL YARD // FUNDICION",
                                  "ZONA QUIMICA // PROCESAMIENTO", "CAMARA DE GRINDER"};
const char* kStage3Scenarios[] = {"PUBLIC ATRIUM", "LABORATORIO DE CAPSULAS", "RESEARCH FLOOR", "EXECUTIVE CORE"};
const char* kStage4Scenarios[] = {"KESSLER TOWER // SEGURIDAD", "GALERIA DE PROTOTIPOS", "SALA DE SERVIDORES",
                                  "NUCLEO EJECUTIVO // TITAN-X MEJORADO"};
const char* kStage5Scenarios[] = {"CAMARA DEL CLON // PASILLO", "SALA DE CONTROL", "CAPSULAS DEL CLON",
                                  "CAMARA DEL CLON // ARENA FINAL"};
const char* kEnemyNames[] = {"PUNK",        "BRUTE",  "CHARGER",      "ENFORCER", "CHEMICAL SOLDIER",
                             "URBAN NINJA", "MUTANT", "ARMORED GUARD"};
// Personajes del VS: los de siempre + el guerrero BETA (Espadas del Caos; las
// otras 3 armas se eligen dentro de la pelea).
int VsCount() { return StableCharacterCount() + 1; }
int VsChar(int slot) { return slot < StableCharacterCount() ? slot : FirstBetaCharacter(); }
int VsSlot(int ch) { return ch < StableCharacterCount() ? ch : StableCharacterCount(); }
bool IsWarrior(int ch) { const int w = ch - FirstBetaCharacter(); return w >= 0 && w < 4; }
// Cambia el arma del guerrero conservando vida, energia, furia, posicion y direccion.
void SwitchWarriorWeapon(Player& p) {
    if (!IsWarrior(p.skin)) return;
    const int next = FirstBetaCharacter() + (p.skin - FirstBetaCharacter() + 1) % 4;
    const float hpRatio = (float)p.hp / std::max(1, p.maxHp);
    const int sp = p.sp, rage = p.rage;
    const Vector3D pos = p.position;
    const Facing facing = p.facing;
    const PlayerInput* scripted = p.scriptedInput;
    p.Reset();
    p.ApplyCharacter(next);
    p.position = pos;
    p.facing = facing;
    p.hp = std::max(1, (int)std::lround(hpRatio * p.maxHp));
    p.sp = sp;
    p.rage = rage;
    p.scriptedInput = scripted;
}
StreetEnemyType NextEnemyType(StreetEnemyType t, int dir) {
    int i = (static_cast<int>(t) + dir + 8) % 8;
    return static_cast<StreetEnemyType>(i);
}
} // namespace
VSMode::VSMode() = default;
void VSMode::Init() {
    flow = VSFlow::Select;
    stage = 0;
    scenario = 0;
    enemyCount = 1;
    selectedBoss = -1;
    selectedCharacter = 0;
    cursor = 0;
    exitRequested = false;
    playerDefeated = false;
    enemyTypes = {StreetEnemyType::Punk, StreetEnemyType::Brute, StreetEnemyType::Charger,
                  StreetEnemyType::Enforcer};
    enemies.clear();
    bossProjectiles.clear();
    player.Reset();
    player.position = {360, 585, 0};
    hitstop = 0;
    shake = 0;
}
bool VSMode::ShouldExit() const {
    return exitRequested;
}
void VSMode::ClearExit() {
    exitRequested = false;
}
int VSMode::ScenarioCount() const {
    return kScenarioCounts[std::clamp(stage, 0, kStageCount - 1)];
}
const char* VSMode::StageName() const {
    return kStageNames[std::clamp(stage, 0, kStageCount - 1)];
}
const char* VSMode::ScenarioText() const {
    const int sc = std::clamp(scenario, 0, 3);
    if (stage == 0) return kStage1Scenarios[sc];
    if (stage == 1) return kStage2Scenarios[sc];
    if (stage == 2) return kStage3Scenarios[sc];
    if (stage == 3) return kStage4Scenarios[sc];
    return kStage5Scenarios[sc];
}
const char* VSMode::BackgroundKey() const {
    return TextFormat("bg_s%d_%d", std::clamp(stage, 0, kStageCount - 1) + 1, std::clamp(scenario, 0, 3) + 1);
}
const char* VSMode::EnemyTypeName(StreetEnemyType t) {
    return kEnemyNames[static_cast<int>(t)];
}
void VSMode::ResetFight() {
    player.Reset();
    player.ApplyCharacter(selectedCharacter);
    player.position = {360, 585, 0};
    playerDefeated = false;
    hitstop = 0;
    shake = 0;
    enemies.clear();
    bossProjectiles.clear();
    combatWorld.Reset();
    if (selectedBoss >= 0) {
        boss.Reset(static_cast<BossId>(selectedBoss), {900, 585, 0});
        return;
    }
    if (RivalActive()) {
        rival = Player{};
        rival.Reset();
        rival.ApplyCharacter(rivalCharacter_);
        rival.position = {920, 585, 0};
        rival.facing = Facing::Left;
        rivalInput = PlayerInput{};
        rival.scriptedInput = &rivalInput;
        rivalAI.Reset();
        rivalWeaponTimer = 6.0f;
        return;
    }
    const std::array<float, 4> xs{700, 835, 970, 1105};
    const std::array<float, 4> ys{575, 535, 610, 555};
    for (int i = 0; i < enemyCount; ++i) {
        StreetEnemy e;
        e.Init({xs[(size_t)i], ys[(size_t)i], 0}, enemyTypes[(size_t)i]);
        e.active = true;
        enemies.push_back(e);
    }
    // Laboratorio: bot de referencia KF (arte de la APK leido del respaldo y recoloreado en memoria).
    kfDemoClip = -1;
    if (kfRival >= 0 && !enemies.empty()) {
        const KfReference& ref = GetKfCharacter(kfRival);
        if (ref.loaded) enemies[0].UseReferenceSkin(ref.templ, ref.scale, KfRoster(kfRival).name);
    }
}
void VSMode::StartFight() {
    flow = VSFlow::Fight;
    ResetFight();
}
void VSMode::Update(float dt) {
    dt = std::min(dt, .033f);
    if (flow == VSFlow::Select) {
        if (input::Pressed(KEY_ESCAPE)) {
            exitRequested = true;
            return;
        }
        if (input::Pressed(KEY_UP)) cursor = (cursor + kFieldCount - 1) % kFieldCount;
        if (input::Pressed(KEY_DOWN)) cursor = (cursor + 1) % kFieldCount;
        if (input::Pressed(KEY_LEFT) || input::Pressed(KEY_RIGHT)) {
            int dir = input::Pressed(KEY_RIGHT) ? 1 : -1;
            if (cursor == 0) {
                stage = (stage + dir + kStageCount) % kStageCount;
                scenario = std::clamp(scenario, 0, ScenarioCount() - 1);
            } else if (cursor == 1)
                scenario = std::clamp(scenario + dir, 0, ScenarioCount() - 1);
            else if (cursor == 2)
                enemyCount = std::clamp(enemyCount + dir, 1, 4);
            else if (cursor == 3)
                selectedCharacter = VsChar((VsSlot(selectedCharacter) + dir + VsCount()) % VsCount());
            else if (cursor == 4)
                selectedBoss = ((selectedBoss + 1 + dir + kBossOptionCount) % kBossOptionCount) - 1;
            else if (cursor == 9)
                kfRival = ((kfRival + 1 + dir + KfStableRosterCount() + 1) % (KfStableRosterCount() + 1)) - 1;
            else if (cursor == 10)
            {
                int r = rivalCharacter_ < 0 ? -1 : VsSlot(rivalCharacter_);
                r = ((r + 1 + dir + VsCount() + 1) % (VsCount() + 1)) - 1;
                rivalCharacter_ = r < 0 ? -1 : VsChar(r);
            }
            else {
                int slot = cursor - 5;
                enemyTypes[(size_t)slot] = NextEnemyType(enemyTypes[(size_t)slot], dir);
            }
        }
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J)) StartFight();
        return;
    }
    if (input::Pressed(KEY_ESCAPE)) {
        flow = VSFlow::Select;
        enemies.clear();
        return;
    }
    if (input::Pressed(KEY_R)) {
        ResetFight();
        return;
    }
    // N: reproduce una por una las acciones del bot de referencia para inspeccionarlas.
    if (input::Pressed(KEY_N) && !enemies.empty() && enemies[0].referenceSkin && kfRival >= 0) {
        const KfReference& ref = GetKfCharacter(kfRival);
        if (!ref.clipNames.empty()) {
            kfDemoClip = (kfDemoClip + 1) % (int)ref.clipNames.size();
            enemies[0].skinAnimator.PlayNamed(ref.clipNames[(size_t)kfDemoClip]);
            enemies[0].skinClip = ref.clipNames[(size_t)kfDemoClip];
        }
    }
    if (playerDefeated || (selectedBoss >= 0 && boss.IsDefeated()) || RivalDefeated()) {
        if (input::Pressed(KEY_ENTER) || input::Pressed(KEY_J)) ResetFight();
        return;
    }
    if (input::Pressed(KEY_Q) && IsWarrior(player.skin) && player.state != PlayerState::Defeat) {
        SwitchWarriorWeapon(player);
        selectedCharacter = player.skin;
    }
    player.PumpInput(dt);
    if (hitstop > 0) {
        hitstop -= dt;
        return;
    }
    player.Update(dt);
    player.position.x = std::clamp(player.position.x, kMinX, kMaxX);
    player.position.y = std::clamp(player.position.y, kMinY, kMaxY);
    if (selectedBoss >= 0) {
        // DF-013.2: pelea de boss en VS via la clase Boss compartida (primer
        // consumidor real de BossDefinition — ver src/game/combat/Boss.h).
        boss.Update(dt, player, &combatWorld, bossProjectiles, &hitstop, &shake);
        combatWorld.Update(dt);
        if (player.AttackIsActive() && !player.hasHit && player.attackType != AttackType::Energy &&
            boss.CanBeHit()) {
            const CombatBox hit = player.GetAttackHitbox();
            if (hit.Intersects(boss.GetHurtbox())) {
                boss.ApplyDamage(player.GetAttackDamage() + (player.isRageMode ? 8 : 0));
                player.hasHit = true;
                shake = .12f;
            }
        }
        for (auto& p : bossProjectiles) {
            if (!p.active) continue;
            p.pos.x += p.vx * dt;
            p.life -= dt;
            if (p.life <= 0) {
                p.active = false;
                continue;
            }
            const CombatBox b{p.pos.x - 18, p.pos.y - 18, 36, 36};
            if (p.fromBoss && player.state != PlayerState::Defeat && player.dashInvulnerability <= 0.0f &&
                b.Intersects(player.GetHurtbox())) {
                player.TakeDamage(p.damage);
                p.active = false;
                shake = .14f;
            }
        }
        bossProjectiles.erase(std::remove_if(bossProjectiles.begin(), bossProjectiles.end(),
                                             [](const BossProjectile& p) { return !p.active; }),
                              bossProjectiles.end());
    } else if (RivalActive()) {
        UpdateRival(dt);
        combatWorld.Update(dt);
    } else {
        for (auto& e : enemies)
            if (e.active && !e.IsDefeated()) {
                e.Update(dt, player, &combatWorld);
                e.position.x = std::clamp(e.position.x, kMinX, kMaxX);
                e.position.y = std::clamp(e.position.y, kMinY, kMaxY);
            }
        combatWorld.Update(dt);
        combatWorld.ResolveHazards(player, enemies);
        if (player.AttackIsActive() && !player.hasHit && player.attackType != AttackType::Energy) {
            const CombatBox hit = player.GetAttackHitbox();
            for (auto& e : enemies)
                if (e.active && !e.IsDefeated() && hit.Intersects(e.GetHurtbox())) {
                    const float d = player.facing == Facing::Right ? 1.f : -1.f;
                    e.TakeDamage(player.GetAttackDamage() + (player.isRageMode ? 5 : 0),
                                 {d * player.GetAttackKnockback(), 0, 0});
                    player.hasHit = true;
                    shake = .08f;
                    break;
                }
        }
        if (player.state != PlayerState::Defeat)
            for (auto& e : enemies)
                if (e.active && !e.IsDefeated() && !e.hasHit && e.AttackIsActive() &&
                    e.GetAttackHitbox().Intersects(player.GetHurtbox())) {
                    player.TakeDamage((int)e.attackDamage);
                    e.hasHit = true;
                    shake = .10f;
                    break;
                }
    }
    playerDefeated = player.state == PlayerState::Defeat;
}
bool VSMode::PlayerIsWarrior() const { return IsWarrior(player.skin); }

void VSMode::UpdateRival(float dt) {
    // El guerrero rival tambien cambia de arma de vez en cuando (sus "transformaciones").
    if (IsWarrior(rival.skin) && rival.state != PlayerState::Attack && rival.state != PlayerState::Defeat &&
        (rivalWeaponTimer -= dt) <= 0.0f) {
        SwitchWarriorWeapon(rival);
        rivalWeaponTimer = 10.0f + GetRandomValue(0, 60) / 10.0f;
    }
    rivalInput = rivalAI.Think(rival, player, dt);
    if (rival.state != PlayerState::Attack)
        rival.facing = player.position.x > rival.position.x ? Facing::Right : Facing::Left;
    rival.PumpInput(dt);
    rival.Update(dt);
    rival.position.x = std::clamp(rival.position.x, kMinX, kMaxX);
    rival.position.y = std::clamp(rival.position.y, kMinY, kMaxY);
    // Golpes de ida y vuelta: las mismas cajas y danos que contra enemigos.
    if (player.AttackIsActive() && !player.hasHit && player.attackType != AttackType::Energy &&
        rival.state != PlayerState::Defeat && player.GetAttackHitbox().Intersects(rival.GetHurtbox())) {
        rival.TakeDamage(player.GetAttackDamage() + (player.isRageMode ? 5 : 0));
        player.hasHit = true;
        shake = .08f;
    }
    if (rival.AttackIsActive() && !rival.hasHit && player.state != PlayerState::Defeat &&
        rival.GetAttackHitbox().Intersects(player.GetHurtbox())) {
        player.TakeDamage(rival.GetAttackDamage() + (rival.isRageMode ? 5 : 0));
        rival.hasHit = true;
        shake = .08f;
    }
}
void VSMode::DrawBackground() const {
    DrawRectangle(0, 0, 1280, 720, {5, 8, 12, 255});
    // El escenario elegido (STAGE + ESCENARIO), recortado al centro en 16:9 para
    // no aplastarlo. La calle "BETA" solo se usa si ese escenario no existe.
    Texture2D bg = AssetManager::Get().GetTexture(BackgroundKey());
    if (!bg.id) bg = AssetManager::Get().GetTextureByPath("assets/backgrounds/hd/vs_beta.png");
    if (bg.id) {
        const float srcW = std::min((float)bg.width, bg.height * 16.0f / 9.0f);
        const float sx = (bg.width - srcW) * 0.5f;
        DrawTexturePro(bg, {sx, 0, srcW, (float)bg.height}, {0, 0, 1280, 720}, {0, 0}, 0, WHITE);
        return;
    }
    DrawRectangle(0, 625, 1280, 95, {7, 10, 13, 220});
    for (int x = 0; x < 1280; x += 160) {
        DrawRectangle(x, 617, 108, 8, {55, 60, 61, 235});
        DrawRectangle(x + 25, 647, 68, 5, {94, 78, 48, 190});
    }
}
void VSMode::DrawHud() const {
    // DF-013.2: VS ya tenia el retrato (fue la referencia para el resto de
    // modos); ahora usa el mismo componente compartido en vez de su propia
    // copia, para que un cambio futuro al panel se haga en un solo lugar.
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
        vitals.title = TextFormat("%s // MODO VS", GetCharacterVisual(selectedCharacter).name);
        vitals.x = 16;
        vitals.y = 16;
        vitals.width = 530;
        vitals.panelHeight = 122;
        ui::DrawPlayerVitals(vitals);
    }
    DrawText(TextFormat("STAGE %d  //  %s", stage + 1, ScenarioText()), 294, 100, 11, {175, 200, 210, 230});
    DrawRectangle(818, 16, 446, 122, {3, 7, 11, 220});
    if (selectedBoss >= 0) {
        DrawText(boss.Def().displayName, 840, 25, 18, {255, 150, 150, 255});
        DrawText(TextFormat("FASE %d", boss.GetPhase()), 840, 52, 13, WHITE);
        DrawRectangle(840, 72, 400, 12, {28, 18, 20, 255});
        DrawRectangle(840, 72, (int)(400.f * std::max(0, boss.GetHp()) / std::max(1, boss.GetMaxHp())), 12,
                      {225, 60, 90, 255});
        DrawText(TextFormat("%d / %d HP", boss.GetHp(), boss.GetMaxHp()), 840, 92, 12, {200, 210, 215, 230});
    } else if (RivalActive()) {
        DrawText(TextFormat("RIVAL: %s", GetCharacterVisual(rivalCharacter_).name), 840, 25, 16, {255, 200, 90, 255});
        DrawText(rival.IsTransformed() ? "TRANSFORMADO" : "IA", 840, 52, 13, WHITE);
        DrawRectangle(840, 72, 400, 12, {28, 18, 20, 255});
        DrawRectangle(840, 72, (int)(400.f * std::max(0, rival.hp) / std::max(1, rival.maxHp)), 12, {225, 60, 90, 255});
        DrawText(TextFormat("%d / %d VIDA", rival.hp, rival.maxHp), 840, 92, 12, {200, 210, 215, 230});
    } else {
        DrawText(StageName(), 840, 25, 18, {255, 205, 75, 255});
        DrawText(TextFormat("ENEMIGOS %d/4", enemyCount), 840, 52, 13, WHITE);
        for (int i = 0; i < enemyCount; ++i) {
            const auto& e = enemies[(size_t)i];
            DrawText(TextFormat("%d  %s", i + 1, EnemyTypeName(e.type)), 840, 72 + i * 15, 11,
                     e.IsDefeated() ? Color{110, 120, 125, 180} : WHITE);
        }
    }
}
void VSMode::DrawSelection() const {
    DrawBackground();
    DrawRectangle(205, 48, 870, 610, {3, 7, 11, 242});
    DrawRectangleLines(205, 48, 870, 610, {55, 90, 105, 170});
    DrawRectangle(205, 48, 6, 610, {60, 205, 240, 230});
    DrawRectangle(1069, 48, 6, 610, {255, 205, 75, 210});
    DrawText("MODO VS // LABORATORIO", 405, 76, 36, {225, 235, 240, 255});
    DrawText("PRUEBA DIRECTA DE SPRITES, ESCENARIOS Y COMBATE", 335, 121, 13, {120, 185, 205, 240});
    const int y[] = {140, 176, 212, 248, 284, 320, 356, 392, 428, 464, 500};
    const Color active = {255, 215, 80, 255};
    const char* labels[] = {"STAGE",     "ESCENARIO", "CANTIDAD",  "PERSONAJE", "BOSS",
                            "ENEMIGO 1", "ENEMIGO 2", "ENEMIGO 3", "ENEMIGO 4", "RIVAL KF (LAB)",
                            "RIVAL (IA)"};
    for (int i = 0; i < kFieldCount; ++i) {
        bool selected = cursor == i;
        DrawRectangle(335, y[i] - 8, 610, 34, selected ? Color{20, 28, 34, 230} : Color{8, 15, 21, 190});
        DrawRectangleLines(335, y[i] - 8, 610, 34,
                           selected ? Color{255, 205, 75, 210} : Color{70, 95, 105, 90});
        DrawText(labels[i], 360, y[i], 14, selected ? active : WHITE);
    }
    DrawText(StageName(), 600, y[0], 14, {190, 220, 230, 255});
    DrawText(ScenarioText(), 600, y[1], 14, {190, 220, 230, 255});
    DrawText(TextFormat("%d ENEMIGO%s", enemyCount, enemyCount == 1 ? "" : "S"), 600, y[2], 14,
             selectedBoss >= 0 ? Color{85, 95, 100, 130} : Color{190, 220, 230, 255});
    // Numero de personaje: deja claro que hay mas (KF incluidos) con < y >.
    DrawText(TextFormat("%s   %d/%d", IsWarrior(selectedCharacter) ? "GUERRERO (4 ARMAS, Q CAMBIA)" : GetCharacterVisual(selectedCharacter).name, VsSlot(selectedCharacter) + 1, VsCount()),
             600, y[3], 14, selectedCharacter == 1 ? Color{255, 160, 170, 255} : Color{190, 220, 230, 255});
    DrawText(kBossNames[selectedBoss + 1], 600, y[4], 14,
             selectedBoss >= 0 ? Color{255, 150, 150, 255} : Color{190, 220, 230, 255});
    for (int i = 0; i < 4; ++i) {
        bool enabled = i < enemyCount && selectedBoss < 0;
        DrawText(EnemyTypeName(enemyTypes[(size_t)i]), 600, y[5 + i], 14,
                 enabled ? Color{190, 220, 230, 255} : Color{85, 95, 100, 130});
    }
    if (kfRival < 0)
        DrawText("NO", 600, y[9], 14, {190, 220, 230, 255});
    else {
        const KfReference& ref = GetKfCharacter(kfRival);
        DrawText(ref.loaded ? TextFormat("%s (ENEMIGO 1)", KfRoster(kfRival).name)
                            : "NO DISPONIBLE: falta apk_reference/",
                 600, y[9], 14, ref.loaded ? Color{255, 200, 90, 255} : Color{255, 110, 100, 255});
    }
    // Rival (IA): cualquier personaje con todos sus movimientos y habilidades.
    if (rivalCharacter_ < 0)
        DrawText("NO (PELEA CONTRA ENEMIGOS)", 600, y[10], 14, {190, 220, 230, 255});
    else
        DrawText(TextFormat("%s   %d/%d", IsWarrior(rivalCharacter_) ? "GUERRERO (CAMBIA DE ARMA)" : GetCharacterVisual(rivalCharacter_).name, VsSlot(rivalCharacter_) + 1, VsCount()),
                 600, y[10], 14, {255, 200, 90, 255});
    if (selectedBoss >= 0)
        DrawText("BOSS ACTIVO: los campos de enemigos y el rival se ignoran (1 vs 1).", 335, 529, 12,
                 {255, 180, 120, 220});
    else if (rivalCharacter_ >= 0)
        DrawText("RIVAL ACTIVO: 1 vs 1 contra la maquina (los enemigos se ignoran).", 335, 529, 12,
                 {255, 200, 120, 220});
    DrawText("↑/↓ CAMPO    ←/→ CAMBIAR    ENTER/J INICIAR", 391, 556, 14, {170, 195, 205, 245});
    DrawText("ESC VOLVER AL MENU", 485, 584, 13, {130, 155, 165, 220});
}
void VSMode::DrawFight() const {
    DrawBackground();
    combatWorld.DrawGround();
    if (selectedBoss >= 0) {
        if (player.position.y < boss.GetPos().y) {
            player.Draw();
            boss.Draw(player.position.x);
        } else {
            boss.Draw(player.position.x);
            player.Draw();
        }
        for (const auto& p : bossProjectiles) {
            if (!p.active) continue;
            Vector2 s = p.pos.ToScreen();
            DrawCircle((int)s.x, (int)s.y - 70, 20, {255, 80, 120, 90});
            DrawCircle((int)s.x, (int)s.y - 70, 12, {255, 80, 120, 255});
        }
    } else if (RivalActive()) {
        if (player.position.y < rival.position.y) { player.Draw(); rival.Draw(); }
        else { rival.Draw(); player.Draw(); }
    } else {
        std::array<std::pair<float, int>, 5> order{};
        int n = 0;
        order[n++] = {player.position.y, -1};
        for (int i = 0; i < (int)enemies.size(); ++i)
            if (enemies[(size_t)i].active) order[n++] = {enemies[(size_t)i].position.y, i};
        std::sort(order.begin(), order.begin() + n, [](auto& a, auto& b) { return a.first < b.first; });
        for (int i = 0; i < n; ++i)
            if (order[i].second < 0)
                player.Draw();
            else
                enemies[(size_t)order[i].second].Draw();
        for (int i = 0; i < (int)enemies.size(); ++i) {
            const auto& e = enemies[(size_t)i];
            Vector2 p = e.position.ToScreen();
            // La barra de vida la dibuja el propio enemigo sobre su sprite (antes
            // habia una segunda barra aqui). Solo se agrega el numero de slot.
            (void)p;
            (void)e;
        }
    }
    combatWorld.DrawEffects();
    DrawHud();
    if (!touch::Enabled()) {
        DrawRectangle(260, 657, 760, 45, {3, 7, 11, 225});
        DrawText(PlayerIsWarrior() ? "J GOLPE   K PATADA   1-5 HABILIDADES   B BLOQUEO   SHIFT ESQUIVA   SPACE FURIA   Q ARMA"
                                   : "J GOLPE   K PATADA   L ENERGIA   B BLOQUEO   SHIFT DASH   SPACE FURIA", 296, 669, 12,
                 {170, 200, 210, 240});
        DrawText(!enemies.empty() && enemies[0].referenceSkin
                     ? TextFormat("R REINICIAR   ESC CONFIGURACION   N ACCION DEL BOT KF: %s",
                                  enemies[0].skinClip.c_str())
                     : "R REINICIAR   ESC CONFIGURACION",
                 489, 687, 11, {135, 155, 165, 220});
    }
    if (playerDefeated) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 160});
        const char* lost = TextFormat("%s DERROTADO", GetCharacterVisual(selectedCharacter).name);
        DrawText(lost, 640 - MeasureText(lost, 46) / 2, 288, 46, {240, 80, 80, 255});
        DrawText(touch::Enabled() ? "OK REINICIAR   ATRAS CONFIGURACION" : "ENTER/J REINICIAR   ESC CONFIGURACION", 430, 357, 18, WHITE);
    } else if (selectedBoss >= 0 && boss.IsDefeated()) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 150});
        DrawText("BOSS DERROTADO", 470, 288, 42, {120, 240, 160, 255});
        DrawText(touch::Enabled() ? "OK REINICIAR   ATRAS CONFIGURACION" : "ENTER/J REINICIAR   ESC CONFIGURACION", 430, 350, 18, WHITE);
    } else if (RivalDefeated()) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 150});
        const char* won = TextFormat("%s DERROTADO", GetCharacterVisual(rivalCharacter_).name);
        DrawText(won, 640 - MeasureText(won, 38) / 2, 288, 38, {120, 240, 160, 255});
        DrawText(touch::Enabled() ? "OK REINICIAR   ATRAS CONFIGURACION" : "ENTER/J REINICIAR   ESC CONFIGURACION", 430, 350, 18, WHITE);
    }
}
void VSMode::Draw() const {
    if (flow == VSFlow::Select)
        DrawSelection();
    else
        DrawFight();
}
} // namespace district_fury
