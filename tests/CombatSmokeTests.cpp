#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/Boss.h"
#include <cassert>

using namespace district_fury;

int main() {
    CombatBox a{0, 0, 10, 10};
    CombatBox b{5, 5, 10, 10};
    CombatBox c{20, 20, 5, 5};
    assert(a.Intersects(b));
    assert(!a.Intersects(c));

    Player player;
    assert(player.hp == player.maxHp);
    player.TakeDamage(20);
    assert(player.hp == 80);
    assert(player.rage >= 15);

    player.Reset();
    player.SetState(PlayerState::Block);
    player.TakeDamage(20);
    assert(player.hp == 100);
    assert(player.shield < player.maxShield);
    assert(player.IsBlocking());

    player.shield = 20;
    player.dashInvulnerability = 0;
    player.TakeDamage(100);
    assert(player.shield == 0);
    assert(player.IsGuardBroken());

    player.Reset();
    player.shield = 40;
    player.Update(2.0f);
    assert(player.shield > 40);

    player.TakeDamage(10);
    const int recoveredHp = player.hp;
    player.TakeDamage(10);
    assert(player.hp == recoveredHp);

    player.state = PlayerState::Attack;
    player.currentAttack = AttackId::Punch1;   // sin ataque asignado la hitbox nunca esta activa
    player.attackType = AttackType::Punch;
    player.attackElapsed = 0.15f;
    assert(player.AttackIsActive());

    // Rayden Cruz (sin perfil de animacion) avanza al golpear, como Rayder/Brakk.
    player.Reset();
    player.skin = 0;
    player.facing = Facing::Right;
    const float startX = player.position.x;
    player.SetState(PlayerState::Attack);
    player.currentAttack = AttackId::Punch3;
    player.attackType = AttackType::Punch;
    player.attackElapsed = 0.0f;
    player.stateTimer = 1.0f;
    for (int i = 0; i < 5; ++i) player.Update(0.02f);
    assert(player.position.x > startX + 1.0f);

    // Jefe del Modo VS: vida llena = fase 1; baja a 2 y 3 al cruzar los umbrales.
    {
        Boss b;
        std::vector<BossProjectile> projs;
        Player target;
        b.Reset(BossId::Brakk, {900, 585, 0});
        b.Update(0.01f, target, nullptr, projs, nullptr, nullptr);
        assert(b.GetPhase() == 1);
        b.ApplyDamage(static_cast<int>(b.GetMaxHp() * 0.40f));
        b.Update(0.2f, target, nullptr, projs, nullptr, nullptr);
        assert(b.GetPhase() == 2);
        b.ApplyDamage(static_cast<int>(b.GetMaxHp() * 0.40f));
        b.Update(0.2f, target, nullptr, projs, nullptr, nullptr);
        assert(b.GetPhase() == 3);
    }

    StreetEnemy enemy;
    enemy.Init({100, 575, 0}, StreetEnemyType::Brute);
    enemy.active = true;
    const int originalHp = enemy.hp;
    enemy.TakeDamage(20, {0, 0, 0});
    assert(enemy.hp == originalHp - 20);
    enemy.TakeDamage(10000, {0, 0, 0});
    assert(enemy.IsDefeated());
    return 0;
}
