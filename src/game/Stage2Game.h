#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include <vector>

namespace district_fury {
class Stage2Game {
public:
    Stage2Game();
    void Init();
    void Update(float dt);
    void Draw() const;
    // DF-013.2: acceso al jugador para aplicar las mejoras de campana antes
    // de Init(). No cambia nada del comportamiento existente.
    Player& PlayerRef() { return player; }
    // DF-013.2 (19-09) — campana encadenada. El stage avisa que el jugador
    // termino y pidio avanzar (ENTER en la pantalla de victoria); main.cpp
    // carga el stage siguiente. F1-F5 siguen siendo atajos de debug.
    bool StageCleared() const { return flow == Flow::Clear; }
    bool ConsumeAdvance() { const bool a = advanceRequested; advanceRequested = false; return a; }
private:
    struct Projectile { Vector3D pos; float vx; float life; int damage; bool active; };
    struct Particle { Vector3D pos; Vector2 vel; float life; float maxLife; float size; Color color; };
    enum class Flow { Intro, Combat, BossIntro, Boss, Clear, GameOver, Pause };
    enum class BossAttack { None, Saw, Slam, Ram, Overdrive };
    struct Grinder { Vector3D pos{0,575,0}; int hp{1100}; int maxHp{1100}; int phase{1}; float attackTimer{1}; float elapsed{0}; float invuln{0}; bool defeated{false}; BossAttack attack{BossAttack::None}; };
    Player player;
    std::vector<StreetEnemy> enemies;
    // DF-013.2: mismo cableado aditivo aplicado en Stage1StoryGame (ver
    // docs/AUTONOMOUS_PROGRESS.md) — activa hazards/impactos de specials.
    CombatWorld combatWorld;
    std::vector<Projectile> projectiles;
    std::vector<Particle> particles;
    Flow flow{Flow::Intro};
    Grinder boss;
    float cameraX{640}; float stageTime{0}; float hitstop{0}; float shake{0}; float comboTimer{0};
    int combo{0}; int maxCombo{0}; int damageTaken{0}; int defeated{0}; int wave{0};
    bool arenaLocked{false};
    bool advanceRequested{false}; float bannerTimer{0};
    void BuildStage(); void SpawnWave(int id); void UpdateCombat(float dt); void UpdateBoss(float dt); void UpdateProjectiles(float dt); void UpdateParticles(float dt);
    void HandlePlayerHits(); void HandleEnemyHits(); void HandleBossHits(); void SpawnEnergy(); void SpawnImpact(Vector3D pos, Color color, bool heavy); void EnterBoss(); void DefeatBoss();
    void DrawWorld() const; void DrawHUD() const; void DrawBoss() const; void DrawClear() const; void DrawArenaLock() const;
};
}
