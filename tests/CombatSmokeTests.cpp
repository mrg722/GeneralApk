#include "game/Player.h"
#include "game/StreetEnemy.h"
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
