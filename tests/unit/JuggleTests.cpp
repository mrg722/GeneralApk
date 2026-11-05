// F1.9: juggle real (V8), en el suelo / OTG / levantarse, rebotes y proyeccion.
// Sin ventana: solo datos (StreetEnemy::ApplyHit + Update).
#include "game/combat/HitResolver.h"
#include "game/events/GameEvents.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;

StreetEnemy MakeEnemy(float x, StreetEnemyType t = StreetEnemyType::Punk) {
    StreetEnemy e;
    e.Init({x, 565, 0}, t);
    e.active = true;
    e.facing = Facing::Left;
    e.hp = e.maxHp = 100000;
    return e;
}

Player FarPlayer() {
    Player p;
    p.position = {3000, 565, 0};
    return p;
}

HitInfo Launcher() {
    HitInfo h;
    h.damage = 10;
    h.dirX = 1.0f;
    h.knockback = 80.0f;
    h.launch = 720.0f;
    h.hitstun = 0.55f;
    h.juggleCost = 2;
    h.unblockable = true;
    return h;
}

HitInfo Light() {
    HitInfo h;
    h.damage = 9;
    h.dirX = 1.0f;
    h.knockback = 400.0f;   // en el aire se limita a 180
    h.launch = 220.0f;
    h.hitstun = 0.30f;
    h.unblockable = true;
    return h;
}

void Step(StreetEnemy& e, int frames, float dt = kDt) {
    const Player far = FarPlayer();
    for (int i = 0; i < frames; ++i) e.Update(dt, far);
}

// Tras un lanzador, 7 golpes ligeros: el presupuesto (6) se agota y el enemigo
// cae al suelo antes del 7o golpe.
void TestBudgetDropsBeforeSeventh() {
    StreetEnemy e = MakeEnemy(1000);
    assert(e.ApplyHit(Launcher()) == HitResult::Hit);
    assert(e.state == StreetEnemyState::Launched);
    bool downBeforeSeventh = false;
    int failedAt = -1;
    for (int i = 1; i <= 7; ++i) {
        Step(e, 8);
        if (e.state == StreetEnemyState::Down || e.state == StreetEnemyState::WakeUp) {
            downBeforeSeventh = i <= 7;
            break;
        }
        const int hp0 = e.hp;
        e.ApplyHit(Light());
        if (e.juggleExhausted && failedAt < 0) {
            failedAt = i;
            assert(hp0 - e.hp == 2);   // 9 x 0.84 (6o golpe de la racha) x 0.30
        }
    }
    assert(failedAt > 0 && failedAt <= 5);   // 2 (lanzador) + 4 x 1 = 6
    // Agotado: ya no sube, cae.
    for (int f = 0; f < 120 && !downBeforeSeventh; ++f) {
        Step(e, 1);
        if (e.state == StreetEnemyState::Down) downBeforeSeventh = true;
    }
    assert(downBeforeSeventh);
    assert(e.position.z == 0.0f);
}

// Brute: presupuesto 4.
void TestHeavyBudget() {
    StreetEnemy e = MakeEnemy(1000, StreetEnemyType::Brute);
    e.ApplyHit(Launcher());
    int ok = 0;
    for (int i = 0; i < 4; ++i) {
        Step(e, 6);
        e.ApplyHit(Light());
        if (!e.juggleExhausted) ++ok;
    }
    assert(ok == 2);   // 2 + 1 + 1 = 4; el 3o ya no levanta
}

HitInfo Sweep() {
    HitInfo h;
    h.damage = 12;
    h.dirX = 1.0f;
    h.knockback = 120.0f;
    h.hitstun = 0.5f;
    h.low = true;
    h.knockdown = true;
    h.unblockable = true;
    return h;
}

HitInfo Otg() {
    HitInfo h;
    h.damage = 10;
    h.dirX = 1.0f;
    h.hitstun = 0.3f;
    h.otg = true;
    h.unblockable = true;
    return h;
}

// Barrida -> Down directo; solo un golpe otg por derribo; WakeUp invulnerable.
void TestOtgOnceAndWakeUp() {
    StreetEnemy e = MakeEnemy(1000);
    assert(e.ApplyHit(Sweep()) == HitResult::Hit);
    assert(e.state == StreetEnemyState::Down);
    assert(e.ApplyHit(Light()) == HitResult::Ignored);   // no otg: no entra
    const int hp0 = e.hp;
    assert(e.ApplyHit(Otg()) == HitResult::Hit);
    assert(e.hp < hp0);
    for (int f = 0; f < 60 && e.state != StreetEnemyState::Down; ++f) Step(e, 1);
    assert(e.state == StreetEnemyState::Down);
    assert(e.ApplyHit(Otg()) == HitResult::Ignored);      // 2o otg del mismo derribo
    // Down 0.55 s -> WakeUp 0.35 s invulnerable -> Idle.
    int wake = 0;
    for (int f = 0; f < 120 && e.state != StreetEnemyState::Idle; ++f) {
        Step(e, 1);
        if (e.state == StreetEnemyState::WakeUp) {
            ++wake;
            assert(e.ApplyHit(Light()) == HitResult::Ignored);
            assert(e.ApplyHit(Otg()) == HitResult::Ignored);
        }
    }
    assert(e.state == StreetEnemyState::Idle);
    assert(wake >= 19 && wake <= 22);   // ~0.35 s a 60 fps
    assert(e.ApplyHit(Light()) != HitResult::Ignored);
}

// groundBounce: rebota (vz 300) una sola vez.
void TestGroundBounceOnce() {
    StreetEnemy e = MakeEnemy(1000);
    e.ApplyHit(Launcher());
    Step(e, 10);
    HitInfo spike = Light();
    spike.launch = -700.0f;
    spike.groundBounce = true;
    e.ApplyHit(spike);
    int landings = 0;
    bool bounced = false;
    for (int f = 0; f < 200 && e.state != StreetEnemyState::Down; ++f) {
        const float vz = e.velocity.z;
        Step(e, 1);
        if (vz < 0.0f && e.velocity.z > 0.0f && e.state == StreetEnemyState::Launched) {
            bounced = true;
            ++landings;
            // Otra picada con rebote: ya no rebota.
            e.ApplyHit(spike);
        }
    }
    assert(bounced && landings == 1);
    assert(e.state == StreetEnemyState::Down);
}

// wallBounce: al tocar el borde de la arena, vx = -0.5 vx, vz = 260, estado WallBounce.
void TestWallBounce() {
    StreetEnemy e = MakeEnemy(1000);
    e.arenaMaxX = 1120.0f;
    HitInfo h = Light();
    h.launch = 300.0f;
    h.knockback = 600.0f;
    h.wallBounce = true;
    h.knockdown = true;
    e.ApplyHit(h);
    bool sawWall = false;
    for (int f = 0; f < 120; ++f) {
        Step(e, 1);
        if (e.state == StreetEnemyState::WallBounce) {
            sawWall = true;
            assert(e.velocity.x < 0.0f);
            break;
        }
    }
    assert(sawWall);
    assert(e.position.x <= 1120.0f);
}

// En el aire, 5 golpes no lo alejan mas de 200 px.
void TestAirKnockbackLimited() {
    StreetEnemy e = MakeEnemy(1000);
    e.ApplyHit(Launcher());
    const float x0 = e.position.x;
    for (int i = 0; i < 5; ++i) {
        Step(e, 7);
        e.ApplyHit(Light());
    }
    for (int f = 0; f < 120 && e.state != StreetEnemyState::Down; ++f) Step(e, 1);
    std::printf("juggle: 5 golpes en el aire -> %.1f px\n", e.position.x - x0);
    assert(e.position.x - x0 <= 200.0f);
}

// Misma altura maxima a 30 y a 60 fps (+-5 %).
float Apex(float dt) {
    StreetEnemy e = MakeEnemy(1000);
    e.ApplyHit(Launcher());
    float apex = 0.0f;
    for (float t = 0; t < 2.0f && e.state == StreetEnemyState::Launched; t += dt) {
        Step(e, 1, dt);
        apex = std::max(apex, e.position.z);
    }
    return apex;
}
void TestFrameRateApex() {
    const float a = Apex(1.0f / 60.0f), b = Apex(1.0f / 30.0f);
    std::printf("juggle: apex 60 fps %.1f, 30 fps %.1f\n", a, b);
    assert(a > 150.0f);
    assert(std::fabs(a - b) / a <= 0.05f);
}

// Escalado de dano por racha (V8): a partir del 4o golpe seguido baja 8 % por golpe.
void TestComboScaling() {
    StreetEnemy e = MakeEnemy(1000);
    HitInfo h;
    h.damage = 20;
    h.dirX = 1.0f;
    h.hitstun = 0.5f;
    h.unblockable = true;
    int last = 0;
    for (int i = 0; i < 6; ++i) {
        const int hp0 = e.hp;
        e.ApplyHit(h);
        last = hp0 - e.hp;
        if (i < 4) assert(last == 20);
    }
    assert(last == 17);   // 6o golpe: 1 - 0.08 * 2 = 0.84
}

// Proyeccion (API de agarre): vuela, golpea a otro enemigo y al caer hace 8 + Down.
void TestThrowBody() {
    std::vector<StreetEnemy> es{MakeEnemy(1000), MakeEnemy(1150)};
    StreetEnemy& a = es[0];
    assert(a.CanBeGrabbed());
    a.SetGrabbed(true, {1030, 565, 40});
    assert(a.state == StreetEnemyState::Grabbed && a.position.z == 40.0f);
    HitInfo t;
    t.damage = 0;
    t.attackerSkin = 0;
    a.Throw(t, {600.0f, 0.0f, 300.0f});
    assert(a.state == StreetEnemyState::Thrown);
    Player p = FarPlayer();
    CombatFeedback fb;
    const StageHitStyle st;
    const int hpB = es[1].hp, hpA = a.hp;
    int hits = 0;
    for (int f = 0; f < 90 && a.state == StreetEnemyState::Thrown; ++f) {
        a.Update(kDt, p);
        hits += ResolvePlayerVsEnemies(p, es, st, fb).hits;
    }
    assert(hits == 1);
    assert(es[1].hp == hpB - 8);
    assert(a.state == StreetEnemyState::Down && hpA - a.hp == 8);
    // Los pesados no se agarran; los que tienen armadura se zafan a 0.6 s.
    StreetEnemy brute = MakeEnemy(1000, StreetEnemyType::Brute);
    assert(!brute.CanBeGrabbed());
    brute.SetGrabbed(true, {1000, 565, 0});
    Step(brute, 40);
    assert(brute.state != StreetEnemyState::Grabbed);
}

// Stomp: el resolvedor marca player.stompTargetAhead con un enemigo en Down delante.
void TestStompTarget() {
    Player p;
    p.position = {1000, 565, 0};
    p.facing = Facing::Right;
    std::vector<StreetEnemy> es{MakeEnemy(1050)};
    CombatFeedback fb;
    const StageHitStyle st;
    ResolvePlayerVsEnemies(p, es, st, fb);
    assert(!p.stompTargetAhead);
    es[0].ApplyHit(Sweep());
    ResolvePlayerVsEnemies(p, es, st, fb);
    assert(p.stompTargetAhead);
    p.facing = Facing::Left;   // detras: no
    ResolvePlayerVsEnemies(p, es, st, fb);
    assert(!p.stompTargetAhead);
}
}  // namespace

int main() {
    TestBudgetDropsBeforeSeventh();
    TestHeavyBudget();
    TestOtgOnceAndWakeUp();
    TestGroundBounceOnce();
    TestWallBounce();
    TestAirKnockbackLimited();
    TestFrameRateApex();
    TestComboScaling();
    TestThrowBody();
    TestStompTarget();
    ClearGameEvents();
    std::printf("JuggleTests OK\n");
    return 0;
}
