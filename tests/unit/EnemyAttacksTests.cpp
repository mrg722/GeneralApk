// F1.9: ataques de datos por tipo de enemigo (EnemyAttackDef): >= 2 por tipo,
// tiempos reales (no el 34-74 % fijo), caja escalada con visualScale (B06e),
// telegrafia, counter-hit fiable y que cada ataque conecte con el jugador.
#include "game/combat/HitResolver.h"
#include "game/events/GameEvents.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace district_fury;

namespace {
constexpr float kDt = 1.0f / 60.0f;
const StreetEnemyType kTypes[] = {StreetEnemyType::Punk,     StreetEnemyType::Brute,
                                  StreetEnemyType::Charger,  StreetEnemyType::Enforcer,
                                  StreetEnemyType::ChemicalSoldier, StreetEnemyType::UrbanNinja,
                                  StreetEnemyType::Mutant,   StreetEnemyType::ArmoredGuard};

StreetEnemy MakeEnemy(float x, StreetEnemyType t) {
    StreetEnemy e;
    e.Init({x, 565, 0}, t);
    e.active = true;
    e.facing = Facing::Right;
    return e;
}

void TestEachTypeHasAttacks() {
    for (StreetEnemyType t : kTypes) {
        const EnemyMoveSet& m = GetEnemyMoveSet(t);
        int valid = 0;
        for (const EnemyAttackDef& a : m.attacks) {
            assert(a.name && a.name[0]);
            assert(a.startup > 0.0f && a.active > 0.0f && a.recovery >= 0.0f);
            assert(a.areaRadius > 0.0f || (a.arc.L1 > 0.0f && a.arc.r > 0.0f));
            if (!a.afterBlock) ++valid;
        }
        std::printf("enemigo %-12s ataques %d (elegibles %d) presupuesto %d\n", GetEnemyProfile(t).displayName,
                    static_cast<int>(m.attacks.size()), valid, m.juggleBudget);
        assert(m.attacks.size() >= 2);
        assert(valid >= 2 || (valid >= 1 && m.attacks.size() >= 3));
        const bool heavy = t == StreetEnemyType::Brute || t == StreetEnemyType::Mutant ||
                           t == StreetEnemyType::ArmoredGuard;
        assert(m.juggleBudget == (heavy ? 4 : 6));
    }
}

// La caja (arco) escala con visualScale: misma definicion nominal, altura x escala.
void TestBoxScalesWithVisualScale() {
    StreetEnemy punk = MakeEnemy(1000, StreetEnemyType::Punk);
    StreetEnemy mutant = MakeEnemy(1000, StreetEnemyType::Mutant);
    for (StreetEnemy* e : {&punk, &mutant}) {
        e->StartAttack(0);
        e->attackElapsed = e->CurrentAttack().startup + e->CurrentAttack().active * 0.5f;
        e->attackPrevElapsed = e->attackElapsed;
        assert(e->AttackIsActive());
    }
    const CombatBox a = punk.GetAttackHitbox(), b = mutant.GetAttackHitbox();
    assert(a.IsValid() && b.IsValid());
    const float ha = 565.0f - (a.y + a.height * 0.5f), hb = 565.0f - (b.y + b.height * 0.5f);
    const float ratio = hb / ha, want = GetEnemyProfile(StreetEnemyType::Mutant).visualScale /
                                        GetEnemyProfile(StreetEnemyType::Punk).visualScale;
    std::printf("caja: centro punk %.1f px, mutant %.1f px (x%.3f, visualScale x%.3f)\n", ha, hb, ratio, want);
    assert(std::fabs(ratio - want) < 0.02f);
    assert(std::fabs(b.height / a.height - want) < 0.02f);
}

// Cada ataque: telegrafia (evento) en el arranque, InStartup -> activo -> fin, y
// conecta con un jugador colocado dentro de su alcance.
void TestEveryAttackConnects() {
    for (StreetEnemyType t : kTypes) {
        const EnemyMoveSet& m = GetEnemyMoveSet(t);
        for (int i = 0; i < static_cast<int>(m.attacks.size()); ++i) {
            const EnemyAttackDef& def = m.attacks[static_cast<std::size_t>(i)];
            std::vector<StreetEnemy> es{MakeEnemy(1000, t)};
            StreetEnemy& e = es[0];
            Player p;
            const float reach = EnemyAttackReach(def, e.AttackScale());
            // Con lunge, el jugador espera donde llega el atacante (casi todo el
            // lunge ocurre en el arranque).
            p.position = {1000.0f + def.lunge * 0.9f + std::max(20.0f, reach * 0.55f), 565, 0};
            p.facing = Facing::Left;
            ClearGameEvents();
            e.StartAttack(i);
            bool telegraph = false;
            GameEvent ev;
            while (PopGameEvent(ev))
                if (ev.type == GameEventType::Telegraph && ev.fromEnemy) telegraph = true;
            assert(telegraph);
            assert(e.InStartup() && !e.AttackIsActive());
            CombatFeedback fb;
            const int hp0 = p.hp;
            bool sawActive = false;
            for (int f = 0; f < 90 && e.state == StreetEnemyState::Attack; ++f) {
                e.Update(kDt, p);
                if (e.AttackIsActive()) {
                    sawActive = true;
                    assert(!e.InStartup());
                }
                ResolveEnemiesVsPlayer(es, p, fb, 2);
                if (p.state == PlayerState::Hit) {   // multi-golpe: deja entrar el siguiente
                    p.stateTimer = 0.0f;
                    p.dashInvulnerability = 0.0f;
                }
            }
            assert(sawActive);
            assert(e.state != StreetEnemyState::Attack);   // termina en S + A + R
            std::printf("  %-12s %-16s S%.2f A%.2f R%.2f alcance %.0f -> dano %d\n", GetEnemyProfile(t).displayName,
                        def.name, def.startup, def.active, def.recovery, reach, hp0 - p.hp);
            assert(p.hp < hp0);
            if (def.hits > 1) {
                const int one = std::max(1, static_cast<int>(std::lround(e.attackDamage * def.damageScale)));
                assert(hp0 - p.hp >= def.hits * one - 1);
            }
        }
    }
    ClearGameEvents();
}

// Propiedades que llegan al jugador: patada baja (low), golpe alto (overhead),
// patada voladora (launch + derribo), empujon (kb 520).
void TestHitProperties() {
    Player p;
    p.position = {1060, 565, 0};
    StreetEnemy punk = MakeEnemy(1000, StreetEnemyType::Punk);
    punk.StartAttack(1);
    assert(EnemyHitInfo(punk, p).low);
    StreetEnemy enf = MakeEnemy(1000, StreetEnemyType::Enforcer);
    enf.StartAttack(1);
    assert(EnemyHitInfo(enf, p).overhead);
    StreetEnemy ninja = MakeEnemy(1000, StreetEnemyType::UrbanNinja);
    ninja.StartAttack(1);
    const HitInfo k = EnemyHitInfo(ninja, p);
    assert(k.launch == 300.0f && k.knockdown);
    StreetEnemy guard = MakeEnemy(1000, StreetEnemyType::ArmoredGuard);
    guard.StartAttack(1);
    assert(EnemyHitInfo(guard, p).knockback == 520.0f);
}

// Root motion: la patada voladora avanza ~120 px; la carga corta del Brute tiene
// superarmadura en arranque + activo.
void TestLungeAndArmor() {
    StreetEnemy ninja = MakeEnemy(1000, StreetEnemyType::UrbanNinja);
    Player far;
    far.position = {3000, 565, 0};
    ninja.StartAttack(1);
    for (int f = 0; f < 90 && ninja.state == StreetEnemyState::Attack; ++f) ninja.Update(kDt, far);
    std::printf("lunge patada voladora: %.1f px\n", ninja.position.x - 1000.0f);
    assert(std::fabs(ninja.position.x - 1120.0f) < 3.0f);

    StreetEnemy brute = MakeEnemy(1000, StreetEnemyType::Brute);
    brute.StartAttack(2);
    brute.Update(kDt, far);
    HitInfo h;
    h.damage = 30;   // por encima del umbral de armor del perfil
    h.dirX = -1.0f;
    h.unblockable = true;
    assert(brute.ApplyHit(h) == HitResult::Armored);
    assert(brute.state == StreetEnemyState::Attack);
}
}  // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    TestEachTypeHasAttacks();
    TestBoxScalesWithVisualScale();
    TestEveryAttackConnects();
    TestHitProperties();
    TestLungeAndArmor();
    std::printf("EnemyAttacksTests OK\n");
    return 0;
}
