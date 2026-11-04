// Resolvedor unico de golpes (ver HitResolver.h).
#include "game/combat/HitResolver.h"
#include "game/events/GameEvents.h"
#include "game/CharacterVisual.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {

std::uintptr_t IdOf(const void* p) { return reinterpret_cast<std::uintptr_t>(p); }

float FacingSign(Facing f) { return f == Facing::Right ? 1.0f : -1.0f; }

// Familia del golpe del jugador (elige sonido y chispa en el AudioDirector).
HitKind PlayerHitKind(const Player& p) {
    if (p.activeSkill >= 0) return HitKind::Energy;
    if (p.attackPhase == AttackPhase::Kick || p.attackType == AttackType::Kick) return HitKind::Kick;
    const AttackDef& def = p.CurrentAttackDef();
    if (def.heavy) return HitKind::Heavy;
    if (p.currentAttack == AttackId::Punch2) return HitKind::Medium;
    return HitKind::Light;
}

bool PlayerHitHeavy(const Player& p) { return p.CurrentAttackDef().heavy || p.activeSkill >= 0; }

// Ultimo golpe de una ruta (REMATE / PATADA): hitstop x1.35 (V7).
bool LastOfRoute(AttackId id) {
    return id == AttackId::Punch3 || id == AttackId::Kick || id == AttackId::Finisher || id == AttackId::Kick3;
}

void Emit(GameEventType type, float x, float y, int skin, HitKind kind, bool heavy, int value, int attack,
          bool fromEnemy) {
    GameEvent e;
    e.type = type;
    e.x = x;
    e.y = y;
    e.skin = skin;
    e.kind = kind;
    e.heavy = heavy;
    e.weight = heavy ? 3 : 1;
    e.value = value;
    e.attack = attack;
    e.fromEnemy = fromEnemy;
    EmitGameEvent(e);
}

void Feel(CombatFeedback& fb, float hitstop, int damage, float dirX) {
    if (fb.hitstop) fb.hitstop->Trigger(hitstop);
    if (fb.camera) fb.camera->Trigger(damage, dirX);
}

void Spark(CombatFeedback& fb, Vector3D at, Color c, bool heavy) {
    if (fb.spark) fb.spark(at, c, heavy);
}

bool NoEnemiesLeft(const std::vector<StreetEnemy>& enemies) {
    for (const StreetEnemy& e : enemies)
        if (!e.IsDefeated()) return false;   // vivos o todavia por aparecer
    return true;
}

void KillSlowMo(CombatFeedback& fb) {
    if (fb.slowMo) *fb.slowMo = std::max(*fb.slowMo, kKillSlowMoSeconds);
}

// El ataque activo del enemigo toca al jugador (carril + caja en el plano).
bool EnemyTouchesPlayer(const StreetEnemy& e, const Player& p) {
    if (!e.active || e.IsDefeated() || !e.AttackIsActive()) return false;
    if (!LaneOverlap(e.position.y, p.position.y, e.attackDepth)) return false;
    const CombatBox b = e.GetAttackHitbox();
    if (!b.IsValid()) return false;
    return RectOverlap(ScreenBoxToPlane(b.x, b.y, b.width, b.height, e.position.y), PlayerHurtRect(p));
}

}  // namespace

// ---------------------------------------------------------------------------
// Formas
// ---------------------------------------------------------------------------

AttackShape PlayerAttackShape(const Player& p) {
    AttackShape s;
    if (!p.AttackIsActive()) return s;
    const AttackDef& def = p.CurrentAttackDef();
    s.lane = p.position.y;
    s.depth = def.depth;
    s.facing = FacingSign(p.facing);
    if (const MoveOverride* ov = p.OverrideFor(p.currentAttack); ov && ov->areaRadius > 0.0f && !p.clipDriven) {
        // F0.4: golpe de area (MoveOverride.areaRadius): todo lo que este en el
        // radio, a los dos lados, desde los pies hasta 1.4 cuerpos de alto.
        const float r = ov->areaRadius * p.BodyScale();
        s.rect = {p.position.x - r, p.position.z - 10.0f, p.position.x + r, p.position.z + 150.0f * p.BodyScale()};
        s.depth = std::max(def.depth, ov->areaRadius * 0.5f);
        s.valid = true;
        return s;
    }
    if (p.clipDriven || !def.hasArc) {
        // KF / BETA: la caja del cuadro (back limitado a 30 px); sin arco: la caja
        // historica de AttackDef. Se pasa al plano (X, altura) desde el carril.
        const CombatBox b = p.GetAttackHitbox();
        if (!b.IsValid()) return s;
        s.rect = ScreenBoxToPlane(b.x, b.y, b.width, b.height, p.position.y);
        s.valid = true;
        return s;
    }
    s.arc = true;
    s.limb = def.arc;
    s.x = p.position.x;
    s.z = p.position.z;
    s.scale = p.BodyScale();
    const float active = std::max(1e-4f, def.active);
    s.u = std::clamp((p.attackElapsed - def.startup) / active, 0.0f, 1.0f);
    s.uPrev = std::clamp((p.attackPrevElapsed - def.startup) / active, 0.0f, 1.0f);
    s.valid = true;
    return s;
}

bool ShapeHits(const AttackShape& s, const Rect2& hurt, float lane, float halfDepth, Vector2* contact) {
    if (!s.valid) return false;
    if (!LaneOverlap(s.lane, lane, s.depth, halfDepth)) return false;
    if (s.arc) {
        const ArcHit h = LimbArcVsRect(s.limb, s.x, s.z, s.facing, s.scale, s.uPrev, s.u, hurt);
        if (h.hit && contact) *contact = h.contact;
        return h.hit;
    }
    if (!RectOverlap(s.rect, hurt)) return false;
    if (contact) *contact = RectContact(s.rect, hurt);
    return true;
}

Rect2 PlayerHurtRect(const Player& p) {
    const CombatBox b = p.GetHurtbox();
    return ScreenBoxToPlane(b.x, b.y, b.width, b.height, p.position.y);
}

Rect2 EnemyHurtRect(const StreetEnemy& e) {
    const EnemyProfile& prof = e.Profile();
    return BodyRect(e.position.x, prof.bodyWidth, e.position.z, prof.bodyHeight);
}

HitInfo PlayerHitInfo(const Player& p, int rageBonus) {
    const AttackDef& def = p.CurrentAttackDef();
    HitInfo info;
    info.damage = p.GetAttackDamage() + (p.isRageMode ? rageBonus : 0);
    info.dirX = FacingSign(p.facing);
    info.knockback = def.knockback;
    info.launch = def.launch;
    info.hitstun = def.hitstun;
    info.breaksGuard = def.breaksGuard;
    info.heavy = PlayerHitHeavy(p);
    info.priority = def.priority;
    info.sourceAttack = static_cast<int>(p.currentAttack);
    info.attackerSkin = p.skin;
    // F0.4: propiedades del ataque (AttackDef). Derribo: propiedad de la fila
    // (en multi-golpe, solo el ultimo) o el umbral historico (dano >= 28).
    const bool lastHit = def.hitCount <= 1 || p.attackHitsDone + 1 >= def.hitCount;
    info.knockdown = (def.knockdown && lastHit) || info.damage >= 28;
    info.otg = def.otg;
    info.wallBounce = def.wallBounce;
    info.groundBounce = def.groundBounce;
    info.low = def.low;
    info.overhead = def.overhead;
    info.juggleCost = def.juggleCost;
    info.blockstun = def.blockstun;
    info.hitIndex = p.attackHitsDone;
    if (def.pushback > 0.0f) info.pushOnBlock = def.pushback;
    if (const MoveOverride* ov = p.OverrideFor(p.currentAttack); ov && ov->pullPx > 0.0f) info.pull = true;
    return info;
}

HitInfo EnemyHitInfo(const StreetEnemy& e, const Player& target) {
    HitInfo info;
    info.damage = static_cast<int>(e.attackDamage);
    info.dirX = target.position.x >= e.position.x ? 1.0f : -1.0f;
    // Derribo por perfil (B17), no por dano: el Mutant siempre derriba y el Brute
    // con su golpe pesado (HeavySlam). El resto empuja y aturde un poco.
    const bool slam = e.state == StreetEnemyState::Special && e.pendingSpecial == EnemySpecial::HeavySlam;
    const bool heavy = e.type == StreetEnemyType::Mutant || slam;
    info.knockdown = heavy;
    info.heavy = heavy || e.Profile().hasArmor;
    info.knockback = info.heavy ? 220.0f : 120.0f;
    info.hitstun = info.heavy ? 0.20f : 0.13f;
    info.priority = e.AttackPriority();
    return info;
}

// ---------------------------------------------------------------------------
// Jugador -> enemigos
// ---------------------------------------------------------------------------

HitReport ResolvePlayerVsEnemies(Player& player, std::vector<StreetEnemy>& enemies, const StageHitStyle& style,
                                 CombatFeedback& fb) {
    HitReport r;
    if (!player.AttackIsActive() || player.state == PlayerState::Defeat) return r;
    const AttackDef& def = player.CurrentAttackDef();
    if (def.spawnsProjectile || player.attackType == AttackType::Energy) return r;   // la onda la resuelve su proyectil
    const AttackShape shape = PlayerAttackShape(player);
    if (!shape.valid) return r;

    const HitInfo base = PlayerHitInfo(player, style.rageBonus);
    const HitKind kind = PlayerHitKind(player);
    bool connected = false, anyKill = false;
    float stop = 0.0f;
    int maxDamage = 0;
    for (StreetEnemy& e : enemies) {
        if (!e.active || e.IsDefeated()) continue;
        const std::uintptr_t id = IdOf(&e);
        if (player.AlreadyHit(id)) continue;
        Vector2 c{};
        if (!ShapeHits(shape, EnemyHurtRect(e), e.position.y, kLanePad, &c)) continue;

        // B16: si el enemigo tambien conecta este mismo frame, gana la prioridad
        // mayor; empate = trade (los dos golpes entran, hitstop compartido).
        if (!e.hasHit && EnemyTouchesPlayer(e, player)) {
            const int ep = e.AttackPriority();
            if (ep > def.priority) continue;   // su golpe sale primero (ResolveEnemiesVsPlayer)
            if (ep == def.priority) {
                HitInfo back = EnemyHitInfo(e, player);
                const HitResult br = player.ApplyHit(back);
                e.hasHit = true;
                if (br != HitResult::Ignored) {
                    Spark(fb, {player.position.x, player.position.y, 70.0f}, style.sparkHurt, false);
                    stop = std::max(stop, HitstopSeconds(back.damage, false, false, br == HitResult::Blocked));
                }
            }
        }

        HitInfo info = base;
        // Picadas (launch < 0) solo empujan hacia abajo a quien esta en el aire;
        // en el suelo no levantan ni hunden (el rebote lo hace F1.9).
        if (info.launch < 0.0f && e.position.z <= 0.0f) info.launch = 0.0f;
        info.counter = e.InAttackStartup();
        if (info.counter) {   // counter-hit: x1.25 dano, +0.12 s de hitstun
            info.damage = static_cast<int>(std::round(info.damage * 1.25f));
            info.hitstun += 0.12f;
        }
        info.contact = {c.x, e.position.y, c.y};
        const HitResult res = e.ApplyHit(info);
        if (res == HitResult::Ignored) continue;
        player.MarkHit(id);
        player.lastHitTargetX = e.position.x;   // persecucion tras un lanzador (F0.4)
        if (info.pull) {   // MoveOverride.pullPx: atrae a la victima hacia el atacante
            const MoveOverride* ov = player.OverrideFor(player.currentAttack);
            const float pull = ov ? ov->pullPx : 0.0f;
            const float dx = player.position.x - e.position.x;
            e.position.x += std::clamp(dx, -pull, pull) * 0.5f;
        }
        connected = true;
        r.Add(info.damage);
        const bool blocked = res == HitResult::Blocked;
        r.blocked = r.blocked || blocked;
        r.heavy = r.heavy || info.heavy;
        if (res == HitResult::Killed) { ++r.kills; anyKill = true; }
        maxDamage = std::max(maxDamage, info.damage);

        const GameEventType type = blocked ? GameEventType::HitBlocked
                                 : res == HitResult::Armored ? GameEventType::HitArmored
                                 : info.counter ? GameEventType::CounterHit : GameEventType::HitLanded;
        Emit(type, c.x, e.position.y, player.skin, blocked ? HitKind::Block : kind, info.heavy, info.damage,
             info.sourceAttack, false);
        if (info.counter && type == GameEventType::CounterHit)   // el impacto tambien suena
            Emit(GameEventType::HitLanded, c.x, e.position.y, player.skin, kind, info.heavy, info.damage,
                 info.sourceAttack, false);
        const Color col = blocked ? style.sparkBlock : info.counter ? style.sparkCounter
                        : info.heavy ? style.sparkHeavy : style.spark;
        Spark(fb, info.contact, col, info.heavy || info.counter);
        stop = std::max(stop, HitstopSeconds(info.damage, info.counter, LastOfRoute(player.currentAttack), blocked,
                                             def.hitstopScale));
    }
    if (connected) {
        player.OnAttackConnected(def);   // B03: combo, rabia y ventana SIEMPRE
        Feel(fb, stop, maxDamage, base.dirX);
        if (anyKill && NoEnemiesLeft(enemies)) KillSlowMo(fb);
    }
    return r;
}

// ---------------------------------------------------------------------------
// Enemigos -> jugador
// ---------------------------------------------------------------------------

void ResolveEnemiesVsPlayer(std::vector<StreetEnemy>& enemies, Player& player, CombatFeedback& fb,
                            int maxAttackersPerFrame, const StageHitStyle* style, int* damageTaken) {
    if (player.state == PlayerState::Defeat) return;
    const StageHitStyle def;
    const StageHitStyle& st = style ? *style : def;
    int attackers = 0;
    for (StreetEnemy& e : enemies) {
        if (!e.active || e.IsDefeated() || !e.AttackIsActive()) continue;
        if (++attackers > maxAttackersPerFrame) continue;
        if (e.hasHit || !EnemyTouchesPlayer(e, player)) continue;
        HitInfo info = EnemyHitInfo(e, player);
        info.counter = player.InAttackStartup();
        if (info.counter) {
            info.damage = static_cast<int>(std::round(info.damage * 1.25f));
            info.hitstun += 0.12f;
        }
        const int before = player.hp;
        const bool wasBlocking = player.IsBlocking();
        const HitResult res = player.ApplyHit(info);
        e.hasHit = true;
        if (res != HitResult::Ignored) {
            if (damageTaken) *damageTaken += before - player.hp;
            const bool blocked = res == HitResult::Blocked || wasBlocking;
            if (info.counter && !blocked)
                Emit(GameEventType::CounterHit, player.position.x, player.position.y, -1, HitKind::Heavy, true,
                     info.damage, 0, true);
            Spark(fb, {player.position.x - info.dirX * 10.0f, player.position.y, 70.0f},
                  blocked ? Color{80, 190, 255, 255} : st.sparkHurt, false);
            Feel(fb, HitstopSeconds(info.damage, info.counter, false, blocked), info.damage, info.dirX);
        }
        break;   // como siempre: el primero que toca este frame
    }
}

// ---------------------------------------------------------------------------
// Jugador -> jefe
// ---------------------------------------------------------------------------

bool ResolvePlayerVsBoss(Player& player, BossProxy& boss, const StageHitStyle& style, CombatFeedback& fb) {
    if (boss.hp == nullptr || *boss.hp <= 0) return false;
    if (boss.invuln != nullptr && *boss.invuln > 0.0f) return false;
    if (!player.AttackIsActive() || player.state == PlayerState::Defeat) return false;
    const AttackDef& def = player.CurrentAttackDef();
    if (def.spawnsProjectile || player.attackType == AttackType::Energy) return false;
    const std::uintptr_t id = IdOf(boss.hp);
    if (player.AlreadyHit(id)) return false;
    const AttackShape shape = PlayerAttackShape(player);
    Vector2 c{};
    if (!ShapeHits(shape, BodyRect(boss.pos.x, boss.width, boss.pos.z, boss.height), boss.pos.y, boss.halfDepth, &c))
        return false;

    int dmg = player.GetAttackDamage() + (player.isRageMode ? style.bossRageBonus : 0);
    if (boss.blocking) {
        dmg = std::max(1, dmg / std::max(1, boss.blockDivisor));
        if (boss.pushOnBlock != 0.0f) player.velocity.x -= FacingSign(player.facing) * boss.pushOnBlock;
    }
    *boss.hp = std::max(0, *boss.hp - dmg);
    if (boss.invuln != nullptr) *boss.invuln = boss.invulnOnHit;
    player.MarkHit(id);
    player.lastHitTargetX = boss.pos.x;
    player.OnAttackConnected(def);

    const bool heavy = PlayerHitHeavy(player);
    Emit(GameEventType::BossHit, c.x, boss.pos.y, player.skin, boss.blocking ? HitKind::Block : PlayerHitKind(player),
         heavy, dmg, static_cast<int>(player.currentAttack), false);
    if (boss.blocking)
        Emit(GameEventType::HitBlocked, c.x, boss.pos.y, player.skin, HitKind::Block, heavy, dmg,
             static_cast<int>(player.currentAttack), false);
    Spark(fb, {c.x, boss.pos.y, c.y}, boss.blocking ? style.sparkBlock : style.sparkBoss, true);
    Feel(fb, HitstopSeconds(dmg, false, LastOfRoute(player.currentAttack), boss.blocking, def.hitstopScale), dmg,
         FacingSign(player.facing));
    if (*boss.hp <= 0) {
        KillSlowMo(fb);
        if (boss.onDefeat) boss.onDefeat();
    }
    return true;
}

// ---------------------------------------------------------------------------
// Jugador <-> jugador (rival de VS)
// ---------------------------------------------------------------------------

namespace {
bool PlayerConnects(const Player& a, const Player& b) {
    if (!a.AttackIsActive() || b.state == PlayerState::Defeat || a.state == PlayerState::Defeat) return false;
    const AttackDef& def = a.CurrentAttackDef();
    if (def.spawnsProjectile || a.attackType == AttackType::Energy) return false;
    if (a.AlreadyHit(IdOf(&b))) return false;
    return ShapeHits(PlayerAttackShape(a), PlayerHurtRect(b), b.position.y, kLanePad, nullptr);
}

int ApplyPlayerHit(Player& a, Player& b, CombatFeedback& fb, const StageHitStyle& style, float& stop) {
    const AttackDef& def = a.CurrentAttackDef();
    HitInfo info = PlayerHitInfo(a, style.rageBonus);
    info.counter = b.InAttackStartup();
    // Atacante KF/BETA (golpes por clip): sus filas de AttackDef son genericas
    // (todavia sin datos por clip, F1.6/F1.7), asi que contra otro jugador
    // conservan el empuje y el hitstun historicos (0 px/s, 0.13 s) y no hacen
    // counter-hit (sin arranque/activo propio por clip todavia).
    if (a.clipDriven) { info.knockback = 0.0f; info.hitstun = 0.13f; info.counter = false; }
    if (info.counter) {
        info.damage = static_cast<int>(std::round(info.damage * 1.25f));
        info.hitstun += 0.12f;
    }
    Vector2 c{};
    ShapeHits(PlayerAttackShape(a), PlayerHurtRect(b), b.position.y, kLanePad, &c);
    info.contact = {c.x, b.position.y, c.y};
    const int before = b.hp;
    if (info.launch < 0.0f && b.position.z <= 0.0f) info.launch = 0.0f;
    const HitResult res = b.ApplyHit(info);
    a.MarkHit(IdOf(&b));
    if (res == HitResult::Ignored) return 0;
    a.lastHitTargetX = b.position.x;
    a.OnAttackConnected(def);
    const bool blocked = res == HitResult::Blocked;
    const GameEventType type = blocked ? GameEventType::HitBlocked
                             : res == HitResult::Armored ? GameEventType::HitArmored
                             : info.counter ? GameEventType::CounterHit : GameEventType::HitLanded;
    Emit(type, c.x, b.position.y, a.skin, blocked ? HitKind::Block : PlayerHitKind(a), info.heavy, info.damage,
         info.sourceAttack, false);
    if (type == GameEventType::CounterHit)
        Emit(GameEventType::HitLanded, c.x, b.position.y, a.skin, PlayerHitKind(a), info.heavy, info.damage,
             info.sourceAttack, false);
    Spark(fb, info.contact, blocked ? style.sparkBlock : info.counter ? style.sparkCounter
                            : info.heavy ? style.sparkHeavy : style.spark, info.heavy);
    stop = std::max(stop, HitstopSeconds(info.damage, info.counter, LastOfRoute(a.currentAttack), blocked,
                                         def.hitstopScale));
    if (fb.camera) fb.camera->Trigger(info.damage, info.dirX);
    return std::max(0, before - b.hp);
}
}  // namespace

namespace {
bool IsBetaSkin(const Player& p) { return p.skin >= 0 && p.skin < CharacterCount() && GetCharacterVisual(p.skin).beta; }

// Personajes BETA por piezas (GUERRERO, RONIN-KRATOS, bestias): conservan la
// resolucion historica entre jugadores (como en GowApk/BetaMode) hasta que
// F1.7/F1.8 les den datos propios: caja del cuadro sin carril, dano directo,
// sin hitstop ni empuje; el que golpea primero interrumpe al otro.
int LegacyBetaDuel(Player& a, Player& b, const StageHitStyle& style, int* damageB) {
    int dealtA = 0, dealtB = 0;
    if (a.AttackIsActive() && !a.hasHit && a.attackType != AttackType::Energy && b.state != PlayerState::Defeat &&
        a.GetAttackHitbox().Intersects(b.GetHurtbox())) {
        const int before = b.hp;
        b.TakeDamage(a.GetAttackDamage() + (a.isRageMode ? style.rageBonus : 0));
        a.hasHit = true;
        dealtA = std::max(0, before - b.hp);
        Emit(GameEventType::HitLanded, b.position.x, b.position.y, a.skin, PlayerHitKind(a), PlayerHitHeavy(a),
             dealtA, static_cast<int>(a.currentAttack), false);
    }
    if (b.AttackIsActive() && !b.hasHit && a.state != PlayerState::Defeat &&
        b.GetAttackHitbox().Intersects(a.GetHurtbox())) {
        const int before = a.hp;
        a.TakeDamage(b.GetAttackDamage() + (b.isRageMode ? style.rageBonus : 0));
        b.hasHit = true;
        dealtB = std::max(0, before - a.hp);
        Emit(GameEventType::HitLanded, a.position.x, a.position.y, b.skin, PlayerHitKind(b), PlayerHitHeavy(b),
             dealtB, static_cast<int>(b.currentAttack), false);
    }
    if (damageB) *damageB = dealtB;
    return dealtA;
}
}  // namespace

int ResolvePlayerVsPlayer(Player& a, Player& b, CombatFeedback& fb, const StageHitStyle& style, int* damageB) {
    if (IsBetaSkin(a) || IsBetaSkin(b)) return LegacyBetaDuel(a, b, style, damageB);
    bool aHits = PlayerConnects(a, b);
    bool bHits = PlayerConnects(b, a);
    // B16: prioridad; empate = trade. Los ataques por clip (KF/BETA) no traen una
    // prioridad real (su fila es la generica de HABILIDAD): con ellos, trade.
    if (aHits && bHits && !a.clipDriven && !b.clipDriven) {
        const int pa = a.CurrentAttackDef().priority, pb = b.CurrentAttackDef().priority;
        if (pa > pb) bHits = false;
        else if (pb > pa) aHits = false;
    }
    float stop = 0.0f;
    int dealtA = 0, dealtB = 0;
    if (aHits) dealtA = ApplyPlayerHit(a, b, fb, style, stop);
    if (bHits) dealtB = ApplyPlayerHit(b, a, fb, style, stop);
    if (stop > 0.0f && fb.hitstop) fb.hitstop->Trigger(stop);
    if (damageB) *damageB = dealtB;
    return dealtA;
}

// ---------------------------------------------------------------------------
// Caja de jefe -> jugador
// ---------------------------------------------------------------------------

HitResult ResolveBoxVsPlayer(const CombatBox& box, float laneY, float depth, const HitInfo& info, Player& player,
                             CombatFeedback& fb, const StageHitStyle* style) {
    if (!box.IsValid() || player.state == PlayerState::Defeat) return HitResult::Miss;
    if (!LaneOverlap(laneY, player.position.y, depth)) return HitResult::Miss;
    const Rect2 hit = ScreenBoxToPlane(box.x, box.y, box.width, box.height, laneY);
    if (!RectOverlap(hit, PlayerHurtRect(player))) return HitResult::Miss;
    const HitResult res = player.ApplyHit(info);
    if (res != HitResult::Ignored) {
        const bool blocked = res == HitResult::Blocked;
        const Color hurt = style ? style->sparkHurt : Color{255, 80, 70, 255};
        Spark(fb, {player.position.x, player.position.y, 70.0f}, blocked ? Color{80, 190, 255, 255} : hurt, true);
        Feel(fb, HitstopSeconds(info.damage, false, false, blocked), info.damage, info.dirX);
    }
    return res;
}

bool BoxTouchesPlayer(const CombatBox& box, float laneY, float depth, const Player& player) {
    if (!box.IsValid() || player.state == PlayerState::Defeat) return false;
    if (!LaneOverlap(laneY, player.position.y, depth)) return false;
    return RectOverlap(ScreenBoxToPlane(box.x, box.y, box.width, box.height, laneY), PlayerHurtRect(player));
}

HitInfo BossHitInfo(int damage, float bossX, const Player& player) {
    HitInfo info;
    info.damage = damage;
    info.dirX = player.position.x >= bossX ? 1.0f : -1.0f;
    info.knockback = 0.0f;
    info.hitstun = 0.13f;
    info.knockdown = damage >= 28;
    info.heavy = true;
    return info;
}

// ---------------------------------------------------------------------------
// Proyectiles
// ---------------------------------------------------------------------------

namespace {
Capsule2 ProjectileCapsule(const ProjectileHitInput& pr) {
    return {{pr.prevX, pr.height}, {pr.pos.x, pr.height}, pr.radius};
}
}  // namespace

int ResolveProjectileVsEnemies(const ProjectileHitInput& pr, std::vector<StreetEnemy>& enemies,
                               const StageHitStyle& style, CombatFeedback& fb, HitReport* report, int attackerSkin) {
    const Capsule2 cap = ProjectileCapsule(pr);
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        StreetEnemy& e = enemies[i];
        if (!e.active || e.IsDefeated()) continue;
        if (!LaneOverlap(pr.pos.y, e.position.y, pr.depth)) continue;
        const Rect2 hurt = EnemyHurtRect(e);
        if (!CapsuleRect(cap, hurt)) continue;
        const AttackDef& wave = GetAttack(AttackId::EnergyWave);
        HitInfo info;
        info.damage = pr.damage;
        info.dirX = pr.dirX;
        info.knockback = pr.knockback;
        info.hitstun = wave.hitstun;
        info.breaksGuard = wave.breaksGuard;
        info.heavy = true;
        info.sourceAttack = static_cast<int>(AttackId::EnergyWave);
        info.attackerSkin = attackerSkin;
        const Vector2 c = ContactPoint(cap, hurt);
        info.contact = {c.x, e.position.y, c.y};
        const HitResult res = e.ApplyHit(info);
        if (res == HitResult::Ignored) continue;
        if (report) {
            report->Add(info.damage);
            if (res == HitResult::Killed) ++report->kills;
        }
        Emit(GameEventType::ProjectileHit, c.x, e.position.y, attackerSkin, HitKind::Energy, true, info.damage,
             info.sourceAttack, false);
        Spark(fb, info.contact, style.spark, true);
        Feel(fb, HitstopSeconds(info.damage, false, false, res == HitResult::Blocked), info.damage, pr.dirX);
        if (res == HitResult::Killed && NoEnemiesLeft(enemies)) KillSlowMo(fb);
        return static_cast<int>(i);
    }
    return -1;
}

bool ResolveProjectileVsBoss(const ProjectileHitInput& pr, BossProxy& boss, const StageHitStyle& style,
                             CombatFeedback& fb) {
    if (boss.hp == nullptr || *boss.hp <= 0) return false;
    if (boss.invuln != nullptr && *boss.invuln > 0.0f) return false;
    if (!LaneOverlap(pr.pos.y, boss.pos.y, pr.depth, boss.halfDepth)) return false;
    const Capsule2 cap = ProjectileCapsule(pr);
    const Rect2 hurt = BodyRect(boss.pos.x, boss.width, boss.pos.z, boss.height);
    if (!CapsuleRect(cap, hurt)) return false;
    int dmg = pr.damage;
    if (boss.blocking) dmg = std::max(1, dmg / std::max(1, boss.blockDivisor));
    *boss.hp = std::max(0, *boss.hp - dmg);
    if (boss.invuln != nullptr) *boss.invuln = 0.12f;
    const Vector2 c = ContactPoint(cap, hurt);
    Emit(GameEventType::ProjectileHit, c.x, boss.pos.y, -1, HitKind::Energy, true, dmg,
         static_cast<int>(AttackId::EnergyWave), false);
    Emit(GameEventType::BossHit, c.x, boss.pos.y, -1, HitKind::Energy, true, dmg,
         static_cast<int>(AttackId::EnergyWave), false);
    Spark(fb, {c.x, boss.pos.y, c.y}, boss.blocking ? style.sparkBlock : style.sparkBoss, true);
    Feel(fb, HitstopSeconds(dmg, false, false, boss.blocking), dmg, pr.dirX);
    if (*boss.hp <= 0) {
        KillSlowMo(fb);
        if (boss.onDefeat) boss.onDefeat();
    }
    return true;
}

bool ResolveProjectileVsPlayer(const ProjectileHitInput& pr, Player& player, CombatFeedback& fb, bool unblockable,
                               const StageHitStyle* style, int* damageTaken) {
    if (player.state == PlayerState::Defeat || player.dashInvulnerability > 0.0f) return false;
    if (!LaneOverlap(pr.pos.y, player.position.y, pr.depth)) return false;
    const Capsule2 cap = ProjectileCapsule(pr);
    if (!CapsuleRect(cap, PlayerHurtRect(player))) return false;
    HitInfo info;
    info.damage = pr.damage;
    info.dirX = pr.dirX;
    info.knockback = 0.0f;
    info.hitstun = 0.13f;
    info.knockdown = pr.damage >= 28;   // como TakeDamage(int)
    info.heavy = true;
    info.unblockable = unblockable;
    const int before = player.hp;
    const HitResult res = player.ApplyHit(info);
    if (damageTaken) *damageTaken += before - player.hp;
    Emit(GameEventType::ProjectileHit, player.position.x, player.position.y, -1, HitKind::Energy, true, pr.damage, 0,
         true);
    const Color hurt = style ? style->sparkHurt : Color{255, 70, 50, 255};
    Spark(fb, {player.position.x, player.position.y, 70.0f}, res == HitResult::Blocked ? Color{80, 190, 255, 255} : hurt,
          true);
    Feel(fb, HitstopSeconds(pr.damage, false, false, res == HitResult::Blocked), pr.damage, pr.dirX);
    return true;
}

void ApplyComboScore(const HitReport& r, const StageHitStyle& style, int& combo, int& maxCombo, float& comboTimer,
                     int* score) {
    for (int i = 0; i < r.hits; ++i) {
        ++combo;
        comboTimer = 1.0f;
        maxCombo = std::max(maxCombo, combo);
        const int dmg = i < HitReport::kMaxHits ? r.damages[i] : r.damages[HitReport::kMaxHits - 1];
        if (score) *score += dmg * style.scorePerDamage + combo * style.scorePerCombo;
    }
}

}  // namespace district_fury
