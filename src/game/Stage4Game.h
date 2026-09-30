#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include "game/npc/KesslerCameo.h"
#include <vector>

// DF-013.2 — Stage 4: Kessler Tower.
// Penultimo stage de la campana (ver docs/AUTONOMOUS_PROGRESS.md y el canon
// de Titan-X en GAME_DESIGN.md). El boss es TITAN-X MEJORADO: la misma
// identidad organico-mecanica/verde de Stage 3, no un personaje nuevo, pero
// mas grande, con mas fases y ataques nuevos (dash + shockwave), tal como
// pide el diseno para su segunda aparicion. Sigue el mismo patron que
// Stage2Game/Stage3Game: sin sprite real todavia (bloqueado por falta de
// PNG con canal alfa), dibujado con primitivas raylib igual que Brakk,
// Grinder y el Titan-X de Stage 3 — no es un placeholder nuevo, es el mismo
// enfoque ya usado en los 3 bosses existentes.
namespace district_fury {
class Stage4Game {
public:
    Stage4Game();
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
    struct Projectile { Vector3D pos; float vx; float life; int damage; bool active; bool fromBoss; };
    struct Particle { Vector3D pos; Vector2 vel; float life; float maxLife; float size; Color color; };
    enum class Flow { Intro, Combat, BossIntro, Boss, Clear, GameOver, Pause };
    enum class BossAttack { None, Dash, Shockwave, ProjectileBurst, Overdrive };
    // DF-013.2: Titan-X Mejorado — mismo personaje, escala/HP/fases mayores
    // (ver GAME_DESIGN.md "Titan-X — aclaracion de diseno").
    struct TitanXImproved { Vector3D pos{0,575,0}; int hp{1900}; int maxHp{1900}; int phase{1}; float attackTimer{1}; float elapsed{0}; float invuln{0}; bool defeated{false}; BossAttack attack{BossAttack::None}; };
    Player player;
    std::vector<StreetEnemy> enemies;
    CombatWorld combatWorld;
    KesslerCameo kessler;  // NPC narrativo: no se golpea ni golpea
    std::vector<Projectile> projectiles;
    std::vector<Particle> particles;
    Flow flow{Flow::Intro};
    TitanXImproved boss;
    float cameraX{640}; float stageTime{0}; float hitstop{0}; float shake{0}; float comboTimer{0};
    int combo{0}; int maxCombo{0}; int damageTaken{0}; int defeated{0}; int wave{0};
    bool arenaLocked{false};
    bool advanceRequested{false}; float bannerTimer{0};
    void BuildStage(); void SpawnWave(int id); void UpdateCombat(float dt); void UpdateBoss(float dt); void UpdateProjectiles(float dt); void UpdateParticles(float dt);
    void HandlePlayerHits(); void HandleEnemyHits(); void HandleBossHits(); void SpawnEnergy(); void SpawnImpact(Vector3D pos, Color color, bool heavy); void EnterBoss(); void DefeatBoss();
    void DrawWorld() const; void DrawHUD() const; void DrawBoss() const; void DrawClear() const; void DrawArenaLock() const;
};
}
