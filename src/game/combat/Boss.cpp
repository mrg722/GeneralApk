#include "game/combat/Boss.h"
#include "rendering/AssetManager.h"
#include "rendering/BossSprite.h"
#include "rendering/SpriteManifest.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace district_fury {

// F1.1: dibujo compartido de los jefes Titan-X (definida en
// src/game/visual/Poses_TitanX.cpp): orientacion por pose, escala uniforme, PoseFx.
void DrawTitanXBossBody(int skin, PlayerState state, int attackId, float elapsed, float startup, float active,
                        float recovery, float stateTime, float walkPhase, Vector2 feet, float targetHeight,
                        bool facingRight, Color tint);

namespace {

// F1.1: ataque del jefe Titan-X -> movimiento del MoveSet jugable con su visual.
AttackId TitanXBossMove(BossId id, const char* name) {
    auto eq = [&](const char* s) { return name && std::strcmp(name, s) == 0; };
    if (id == BossId::TitanX) {
        if (eq("Dash")) return AttackId::DashAttack;
        if (eq("GuardCounter")) return AttackId::Punch1;   // golpe de carga completo
        if (eq("ProjectileBurst")) return AttackId::EnergyWave;
        return AttackId::Punch1;
    }
    if (eq("Dash")) return AttackId::FwdPunch;
    if (eq("Shockwave")) return AttackId::Kick;
    if (eq("ProjectileBurst")) return AttackId::EnergyWave;
    if (eq("Overdrive")) return AttackId::Signature3;
    return AttackId::Punch1;
}

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
    animTried = false;
    animV2 = false;
    cloneVisual = RayderCloneVisual{};
    attackTimer = 1.0f;
    elapsed = 0.0f;
    invuln = 0.0f;
    currentAttack = -1;
    attackResolved = false;
    defeated = false;
}

int Boss::PhaseNumber() const {
    // phases[] va de mayor a menor hpRatioThreshold y termina en la fase base
    // (threshold 0). Fase = 1 + umbrales ya cruzados (vida llena = fase 1).
    const float ratio = maxHp > 0 ? static_cast<float>(hp) / static_cast<float>(maxHp) : 1.0f;
    int crossed = 0;
    for (std::size_t i = 0; i + 1 < def->phases.size(); ++i)
        if (ratio <= def->phases[i].hpRatioThreshold) ++crossed;
    return 1 + crossed;
}

const BossPhaseDef& Boss::ActivePhase() const {
    // Sin umbral cruzado: la fase base (la ultima). Si no, el umbral mas bajo cruzado.
    const int crossed = PhaseNumber() - 1;
    const std::size_t idx = crossed == 0 ? def->phases.size() - 1 : static_cast<std::size_t>(crossed - 1);
    return def->phases[idx];
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
    // F2.2 (tarea 3): telegrafia. El jefe avisa su ataque al empezar el arranque,
    // como hacen los enemigos de calle (F1.9). Reusa GameEventType::Telegraph (el
    // audio/dibujo ya lo conoce); los pesados (Golpe sismico/Shockwave, Overdrive,
    // el Ariete/Dash con empuje) avisan "fuerte" (heavy + mas peso).
    const BossAttackDef& atk = def->attacks[static_cast<std::size_t>(currentAttack)];
    GameEvent ev;
    ev.type = GameEventType::Telegraph;
    ev.x = pos.x; ev.y = pos.y;
    ev.fromEnemy = true;
    ev.heavy = atk.heavy;
    ev.weight = atk.heavy ? 3 : 1;
    EmitGameEvent(ev);
}

void Boss::Update(float dt, Player& player, CombatWorld* world, std::vector<BossProjectile>& projectiles, HitstopClock* hitstop, float* shake) {
    if (!def || defeated) return;
    if (invuln > 0.0f) invuln -= dt;

    const int newPhase = PhaseNumber();
    if (newPhase != phase) {
        phase = newPhase;
        EmitSimpleEvent(GameEventType::BossPhase, phase);
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
                // B06f: caja simetrica (antes a la derecha ignoraba boxForward y a la
                // izquierda lo sumaba): sale del jefe hacia donde mira.
                const CombatBox hit{facingRight ? pos.x : pos.x - atk.boxWidth,
                                     pos.y - atk.boxHeight, atk.boxWidth, atk.boxHeight};
                if (player.state != PlayerState::Hit && BoxTouchesPlayer(hit, pos.y, atk.depth, player)) {
                    const int dmg = static_cast<int>(atk.damage * ph.damageMultiplier);
                    player.ApplyHit(BossHitInfo(dmg, pos.x, player));   // empuje hacia el lado del jugador (B10)
                    GameEvent ev;
                    ev.type = GameEventType::BossAttack;
                    ev.x = pos.x; ev.y = pos.y; ev.value = dmg; ev.heavy = true; ev.fromEnemy = true;
                    EmitGameEvent(ev);
                    if (hitstop) hitstop->Trigger(atk.hitstop);
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
    UpdateAnimation(dt, dx);
}

// Mismo criterio que Stage1StoryGame::UpdateBossFight (pelea de Brakk del Nivel 1).
void Boss::UpdateAnimation(float dt, float dx) {
    if (!IsWindowReady()) return;
    if (id == BossId::RayderClone) {
        const char* atk = currentAttack >= 0 ? def->attacks[static_cast<std::size_t>(currentAttack)].name : nullptr;
        cloneVisual.Update(dt, atk, elapsed, std::abs(dx) > 190.0f, invuln > 0.05f, defeated);
        return;
    }
    if (id != BossId::Brakk) return;
    if (!animTried) {
        animTried = true;
        const AtlasProfile* prof = SpriteManifest::Get().FindAtlas("brakk_v2");
        animV2 = prof && anim.InitFromManifest("brakk_v2", AssetManager::Get().GetTextureByPath(prof->path));
    }
    if (!animV2) return;
    const char* atk = currentAttack >= 0 ? def->attacks[static_cast<std::size_t>(currentAttack)].name : nullptr;
    auto eq = [&](const char* s) { return atk && std::strcmp(atk, s) == 0; };
    const char* clip = defeated                ? "defeat"
                       : invuln > 0.05f        ? "hit"
                       : eq("ChainSwing")      ? "chain"
                       : eq("GroundSmash")     ? "smash"
                       : eq("Charge")          ? "charge"
                       : eq("PowerWave")       ? "chain_throw"
                       : eq("Frenzy")          ? (phase >= 3 ? "explosive" : "fury")
                       : atk                   ? "chain"
                       : std::abs(dx) > 190.0f ? (phase >= 3 ? "run" : "walk")
                                               : "idle";
    if (anim.currentClipName != clip) anim.PlayNamed(clip);
    anim.Update(dt);
}

void Boss::ApplyDamage(int dmg) {
    if (!CanBeHit()) return;
    hp = std::max(0, hp - dmg);
    invuln = 0.1f;
    if (hp <= 0) defeated = true;
}

BossProxy Boss::Proxy() {
    BossProxy bp;
    bp.pos = pos;
    if (def) { bp.width = def->bodyWidth; bp.height = def->bodyHeight; }
    bp.hp = &hp;
    bp.invuln = &invuln;
    bp.invulnOnHit = 0.1f;   // como ApplyDamage
    bp.onDefeat = [this]() { defeated = true; };
    return bp;
}

CombatBox Boss::GetHurtbox() const {
    if (!def) return CombatBox{};
    return CombatBox{pos.x - def->bodyWidth * 0.5f, pos.y - def->bodyHeight, def->bodyWidth, def->bodyHeight};
}

void Boss::Draw(float playerX) const {
    if (!def || defeated) return;
    const bool facingRight = playerX > pos.x;
    if (id == BossId::RayderClone && cloneVisual.IsReady()) {
        DrawEllipse(static_cast<int>(pos.x), static_cast<int>(pos.y), 34, 9, {0, 0, 0, 150});
        cloneVisual.Draw({pos.x, pos.y}, facingRight, invuln > 0.0f ? Color{255, 190, 190, 255} : WHITE);
        return;
    }
    if (animV2 && anim.texture.id != 0 && !anim.frames.empty()) {
        // Hoja mejorada: mira a la derecha; ~175 px de alto como en el Nivel 1.
        DrawEllipse(static_cast<int>(pos.x), static_cast<int>(pos.y), 70, 14, {0, 0, 0, 150});
        const Color tv = invuln > 0.0f ? Color{255, 180, 180, 255} : WHITE;
        anim.Draw({pos.x, pos.y}, 1.45f, !facingRight, tv);
        return;
    }
    if (id == BossId::TitanX || id == BossId::TitanXMejorado) {
        // F1.1: mismas poses, orientacion y movimientos que el Titan-X jugable; mira al jugador.
        const int skin = id == BossId::TitanX ? 4 : 5;
        const float h = SpriteTargetHeight(id);
        DrawEllipse(static_cast<int>(pos.x), static_cast<int>(pos.y), h * 0.30f, h * 0.06f, {0, 0, 0, 150});
        const Color tint = invuln > 0.0f ? Color{255, 190, 190, 255} : WHITE;
        const float t = static_cast<float>(GetTime());
        if (invuln > 0.02f) {
            DrawTitanXBossBody(skin, PlayerState::Hit, 0, 0, 0, 0, 0, 0.1f - invuln, 0, {pos.x, pos.y}, h, facingRight, tint);
        } else if (currentAttack >= 0) {
            const BossAttackDef& a = def->attacks[static_cast<std::size_t>(currentAttack)];
            DrawTitanXBossBody(skin, PlayerState::Attack, static_cast<int>(TitanXBossMove(id, a.name)), elapsed, a.startup,
                               a.active, a.recovery, elapsed, 0, {pos.x, pos.y}, h, facingRight, tint);
        } else if (std::abs(playerX - pos.x) > 190.0f) {   // se acerca (Update avanza hacia el jugador)
            DrawTitanXBossBody(skin, PlayerState::Walk, 0, 0, 0, 0, 0, t, std::fabs(pos.x) * 0.03f, {pos.x, pos.y}, h,
                               facingRight, tint);
        } else {
            DrawTitanXBossBody(skin, PlayerState::Idle, 0, 0, 0, 0, 0, t, 0, {pos.x, pos.y}, h, facingRight, tint);
        }
        return;
    }
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
