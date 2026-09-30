#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include "game/combat/Boss.h"
#include <array>
#include <vector>

namespace district_fury {

enum class VSFlow { Select, Fight };

class VSMode {
public:
    VSMode();
    void Init();
    void Update(float dt);
    void Draw() const;
    bool ShouldExit() const;
    void ClearExit();

private:
    VSFlow flow{VSFlow::Select};
    int stage{0};
    int scenario{0};
    int enemyCount{1};
    // DF-013.2: -1 = sin boss (pelea normal contra enemigos de calle);
    // 0..4 = BossId (Brakk/Grinder/TitanX/TitanXMejorado/RayderClone).
    // Primer consumidor real de la clase Boss/BossDefinition compartida
    // (ver src/game/combat/Boss.h y ARCHITECTURE.md).
    int selectedBoss{-1};
    // DF-013.2 (19-09): personaje del jugador en VS. 0 = Rayden ORIGINAL
    // (por defecto, intacto), 1 = Rayden clon. Ver Player::skin.
    int selectedCharacter{0};
    int cursor{0};
    bool exitRequested{false};
    bool playerDefeated{false};
    Player player;
    std::array<StreetEnemyType, 4> enemyTypes{
        StreetEnemyType::Punk,
        StreetEnemyType::Brute,
        StreetEnemyType::Charger,
        StreetEnemyType::Enforcer
    };
    std::vector<StreetEnemy> enemies;
    // DF-013.2: mismo cableado aditivo de CombatWorld que en los 3 stages
    // (ver docs/AUTONOMOUS_PROGRESS.md) — VS pasa a compartir el mismo
    // sistema de hazards/impactos, no una implementacion aparte.
    CombatWorld combatWorld;
    Boss boss;
    std::vector<BossProjectile> bossProjectiles;
    float hitstop{0.0f};
    float shake{0.0f};

    void ResetFight();
    void StartFight();
    void UpdateFight(float dt);
    void DrawSelection() const;
    void DrawFight() const;
    void DrawBackground() const;
    void DrawHud() const;
    int ScenarioCount() const;
    const char* StageName() const;
    const char* ScenarioText() const;
    const char* BackgroundKey() const;
    static const char* EnemyTypeName(StreetEnemyType type);
};

}
