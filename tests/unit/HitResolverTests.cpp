// F0.2: resolvedor unico de golpes y bugs del motor (B01, B03, B04, B05, B08,
// B09, B10, B11, B12, B15). Sin ventana: solo datos.
#include "game/combat/HitResolver.h"
#include "game/events/GameEvents.h"
#include "audio/AudioDirector.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;

StreetEnemy MakeEnemy(float x, float y, StreetEnemyType t = StreetEnemyType::Punk) {
    StreetEnemy e;
    e.Init({x, y, 0}, t);
    e.active = true;
    e.facing = Facing::Left;
    return e;
}

// Pone al jugador en el activo del ataque `id` (mitad del tramo activo).
void ForceAttack(Player& p, AttackId id, float frac = 0.5f) {
    const AttackDef& def = GetAttack(id);
    p.state = PlayerState::Attack;
    p.currentAttack = id;
    p.attackType = id == AttackId::RageAttack ? AttackType::Rage : AttackType::Punch;
    p.attackPhase = id == AttackId::Punch1 ? AttackPhase::Punch1 : AttackPhase::Special;
    p.attackElapsed = def.startup + def.active * frac;
    p.attackPrevElapsed = p.attackElapsed - kDt;
    p.stateTimer = 1.0f;
    p.hitTargets.clear();
}

// B01: una sola onda por lanzamiento (comando y habilidad 1).
void TestOneProjectilePerWave() {
    Player p;
    p.QueueCommand(InputCommand::SpecialWave);
    int spawns = 0;
    for (int i = 0; i < 90; ++i) {
        p.Update(kDt);
        if (p.ConsumeProjectileEvent()) ++spawns;
    }
    assert(spawns == 1);

    Player q;
    assert(q.TryStartSkill(0));   // habilidad 1 = ONDA
    spawns = 0;
    for (int i = 0; i < 90; ++i) {
        q.Update(kDt);
        if (q.ConsumeProjectileEvent()) ++spawns;
    }
    assert(spawns == 1);
}

// B08: la FURIA golpea a los 3 enemigos dentro de su arco, una vez cada uno.
void TestRageHitsAllTargetsOnce() {
    Player p;
    p.position = {1000, 565, 0};
    p.facing = Facing::Right;
    std::vector<StreetEnemy> es{MakeEnemy(1060, 565), MakeEnemy(1110, 575), MakeEnemy(940, 560)};
    ForceAttack(p, AttackId::RageAttack, 0.6f);
    CombatFeedback fb;
    HitstopClock clock;
    fb.hitstop = &clock;
    const StageHitStyle st;
    const int hp0 = es[0].hp;
    const HitReport r = ResolvePlayerVsEnemies(p, es, st, fb);
    assert(r.hits == 3);
    for (const StreetEnemy& e : es) assert(e.hp < hp0 || e.IsDefeated());
    const int a = es[0].hp, b = es[1].hp, c = es[2].hp;
    p.attackPrevElapsed = p.attackElapsed;
    p.attackElapsed += kDt;
    assert(ResolvePlayerVsEnemies(p, es, st, fb).hits == 0);   // misma activacion: no repite
    assert(es[0].hp == a && es[1].hp == b && es[2].hp == c);
    assert(clock.Active());
}

// B05: el JAB respeta el carril (abs(dy) <= 42 + 18) y no pega por la espalda.
void TestJabLaneAndSide() {
    const StageHitStyle st;
    CombatFeedback fb;
    auto hitsAt = [&](float dx, float dy) {
        Player p;
        p.position = {1000, 565, 0};
        p.facing = Facing::Right;
        ForceAttack(p, AttackId::Punch1);
        std::vector<StreetEnemy> es{MakeEnemy(1000 + dx, 565 + dy)};
        return ResolvePlayerVsEnemies(p, es, st, fb).hits == 1;
    };
    assert(hitsAt(70, 20));
    assert(hitsAt(70, -20));
    assert(!hitsAt(70, 70));
    assert(!hitsAt(70, -70));
    assert(!hitsAt(-70, 0));   // por la espalda no
    // Con behind=true el mismo arco golpea atras.
    Player p;
    p.position = {1000, 565, 0};
    p.facing = Facing::Right;
    ForceAttack(p, AttackId::Punch1);
    AttackShape s = PlayerAttackShape(p);
    StreetEnemy back = MakeEnemy(930, 565);
    assert(!ShapeHits(s, EnemyHurtRect(back), back.position.y, kLanePad, nullptr));
    s.limb.behind = true;
    assert(ShapeHits(s, EnemyHurtRect(back), back.position.y, kLanePad, nullptr));
}

// B10: golpeado por la espalda, el jugador se desplaza en la direccion del golpe.
void TestHitFromBehindPushesForward() {
    Player p;
    p.position = {1000, 565, 0};
    p.facing = Facing::Right;
    std::vector<StreetEnemy> es{MakeEnemy(940, 565)};
    StreetEnemy& e = es[0];
    e.facing = Facing::Right;
    e.state = StreetEnemyState::Attack;
    e.attackElapsed = e.attackDuration * 0.5f;
    e.telegraphing = false;
    e.hasHit = false;
    CombatFeedback fb;
    const float x0 = p.position.x;
    ResolveEnemiesVsPlayer(es, p, fb);
    assert(p.state == PlayerState::Hit);
    assert(p.velocity.x > 0.0f);
    for (int i = 0; i < 6; ++i) p.Update(kDt);
    assert(p.position.x > x0 + 1.0f);

    // Derribo (Mutant): tambien hacia adelante, no hacia el atacante.
    Player q;
    q.position = {1000, 565, 0};
    q.facing = Facing::Right;
    HitInfo heavy;
    heavy.damage = 10;
    heavy.dirX = 1.0f;
    heavy.knockdown = true;
    assert(q.ApplyHit(heavy) == HitResult::Hit && q.state == PlayerState::Airborne && q.velocity.x > 0.0f);

    // Envoltorio historico: sin direccion, hacia atras de su facing.
    Player w;
    w.facing = Facing::Right;
    w.TakeDamage(30);
    assert(w.state == PlayerState::Airborne && w.velocity.x < 0.0f);
}

// B09: los empujes de 5 bloqueos se integran y se apagan (no quedan acumulados).
void TestBlockPushDecays() {
    Player p;
    PlayerInput in;
    in.block = true;
    p.scriptedInput = &in;
    p.Update(kDt);
    assert(p.IsBlocking());
    const float x0 = p.position.x;
    for (int i = 0; i < 5; ++i) {
        p.dashInvulnerability = 0.0f;
        p.TakeDamage(10);
        p.Update(kDt);
    }
    for (int i = 0; i < 30; ++i) p.Update(kDt);
    assert(std::fabs(p.velocity.x) < 5.0f);
    assert(p.position.x < x0 - 5.0f);   // el empuje si se vio
}

// B11: deslizamiento del enemigo tras una PATADA casi igual a 30 y a 60 fps.
float KickSlide(float dt) {
    StreetEnemy e = MakeEnemy(1000, 565, StreetEnemyType::Brute);
    Player far;
    far.position = {3000, 565, 0};
    const AttackDef& kick = GetAttack(AttackId::Kick);
    HitInfo info;
    info.damage = 1;
    info.dirX = 1.0f;
    info.knockback = kick.knockback;
    info.hitstun = kick.hitstun;
    info.unblockable = true;
    assert(e.ApplyHit(info) == HitResult::Hit);
    const float x0 = e.position.x;
    for (float t = 0; t < 1.0f && e.state == StreetEnemyState::Hit; t += dt) e.Update(dt, far);
    return e.position.x - x0;
}
void TestSlideFrameRate() {
    const float a = KickSlide(1.0f / 60.0f), b = KickSlide(1.0f / 30.0f);
    assert(a > 10.0f);
    assert(std::fabs(a - b) / a < 0.10f);
}

// B03/B04/B12/B13: J J J con un enemigo delante abre la ventana de combo; tras
// el 4o golpe, en furia y con 35 SP, la K sale como FINISHER.
void TestComboAndFinisher() {
    Player p;
    p.position = {1000, 565, 0};
    p.facing = Facing::Right;
    PlayerInput in;
    p.scriptedInput = &in;
    std::vector<StreetEnemy> es{MakeEnemy(1070, 565, StreetEnemyType::Enforcer)};
    es[0].hp = es[0].maxHp = 1000;
    CombatFeedback fb;
    const StageHitStyle st;
    bool sawFinisher = false;
    int maxCombo = 0;
    for (int f = 0; f < 140; ++f) {
        in = PlayerInput{};
        if (f == 0 || f == 10 || f == 20 || f == 30) in.punch = true;
        if (f == 70) {
            p.rageState = RageState::Active;
            p.rageStateTimer = 5.0f;
            p.isRageMode = true;
            p.rage = p.maxRage;
            p.sp = 40;
        }
        if (f == 70) in.kick = true;
        p.Update(kDt);
        ResolvePlayerVsEnemies(p, es, st, fb);
        es[0].state = StreetEnemyState::Idle;   // diana quieta (sin IA)
        maxCombo = std::max(maxCombo, p.comboCount);
        if (f == 50) assert(p.comboCount >= 3);
        if (p.currentAttack == AttackId::Finisher) sawFinisher = true;
    }
    assert(maxCombo >= 4);
    assert(sawFinisher);
}

// B15: un J pulsado al empezar un hitstop largo no caduca mientras dura.
void TestBufferFrozenInHitstop() {
    for (int frozen : {11, 20}) {
        Player p;
        PlayerInput in;
        p.scriptedInput = &in;
        HitstopClock clock;
        clock.Trigger(frozen * kDt - 0.001f);
        for (int f = 0; f < frozen + 3; ++f) {
            in = PlayerInput{};
            if (f == 0) in.punch = true;
            PumpPlayerInput(p, clock, kDt);
            if (clock.Consume(kDt)) continue;
            p.Update(kDt);
        }
        assert(p.state == PlayerState::Attack && p.currentAttack == AttackId::Punch1);
    }
}

// B04: expirar la ventana de combo no borra el ataque en curso.
void TestComboWindowKeepsCurrentAttack() {
    Player p;
    p.QueueCommand(InputCommand::Kick);
    p.Update(kDt);
    assert(p.currentAttack == AttackId::Kick);
    p.comboWindow = 0.01f;
    for (int i = 0; i < 5 && p.state == PlayerState::Attack; ++i) {
        p.Update(kDt);
        if (p.state == PlayerState::Attack) assert(p.currentAttack == AttackId::Kick);
    }
}

// Eventos: el golpe conectado emite HitLanded y el dano al jugador PlayerHurt.
void TestEvents() {
    ClearGameEvents();
    Player p;
    p.position = {1000, 565, 0};
    p.facing = Facing::Right;
    ForceAttack(p, AttackId::Punch1);
    std::vector<StreetEnemy> es{MakeEnemy(1070, 565)};
    CombatFeedback fb;
    ResolvePlayerVsEnemies(p, es, StageHitStyle{}, fb);
    Player v;
    v.TakeDamage(5);
    bool landed = false, hurt = false;
    DrainGameEvents([&](const GameEvent& e) {
        if (e.type == GameEventType::HitLanded && e.skin == 0) landed = true;
        if (e.type == GameEventType::PlayerHurt) hurt = true;
    });
    assert(landed && hurt);
}
// AudioDirector: mapa legado 1:1 (lo que antes sonaba en PlayerDraw/Player).
void TestLegacyAudioMap() {
    Sfx out;
    GameEvent e;
    e.type = GameEventType::HitLanded;
    e.skin = 0;
    e.kind = HitKind::Light;
    assert(LegacySfxFor(e, out) && out == Sfx::Punch);
    e.kind = HitKind::Kick;
    assert(LegacySfxFor(e, out) && out == Sfx::Kick);
    e.heavy = true;
    assert(LegacySfxFor(e, out) && out == Sfx::HeavyHit);
    e.kind = HitKind::Energy;
    assert(LegacySfxFor(e, out) && out == Sfx::EnergyImpact);
    e.skin = 6;   // MAX: sus variantes por golpe de la cadena
    e.heavy = false;
    e.kind = HitKind::Light;
    e.attack = static_cast<int>(AttackId::Punch2);
    assert(LegacySfxFor(e, out) && out == Sfx::RayderGolpe2);
    e.kind = HitKind::Kick;
    e.attack = static_cast<int>(AttackId::Kick);
    assert(LegacySfxFor(e, out) && out == Sfx::RayderPatada1);
    GameEvent h;
    h.type = GameEventType::PlayerHurt;
    assert(LegacySfxFor(h, out) && out == Sfx::Hit);   // antes mudo
    h.type = GameEventType::EnemyKilled;
    assert(LegacySfxFor(h, out) && out == Sfx::EnemyDeath);
    h.type = GameEventType::KnockdownLand;
    assert(LegacySfxFor(h, out) && out == Sfx::Land);
    h.type = GameEventType::Land;
    assert(LegacySfxFor(h, out) && out == Sfx::Land);
    assert(IsMusicEvent(GameEventType::ModeEnter) && IsMusicEvent(GameEventType::BossPhase));
}
}  // namespace

int main() {
    TestOneProjectilePerWave();
    TestRageHitsAllTargetsOnce();
    TestJabLaneAndSide();
    TestHitFromBehindPushesForward();
    TestBlockPushDecays();
    TestSlideFrameRate();
    TestComboAndFinisher();
    TestBufferFrozenInHitstop();
    TestComboWindowKeepsCurrentAttack();
    TestEvents();
    TestLegacyAudioMap();
    std::printf("HitResolverTests OK\n");
    return 0;
}
