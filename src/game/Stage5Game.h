#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include "game/npc/KesslerCameo.h"
#include <vector>

// DF-013.2 — Stage 5: Camara del Clon (final de campana).
//
// 19-09: el stage pasa de ser una sola arena a tener 4 zonas, como el resto
// de la campana: acceso al laboratorio, camara de Kessler y sala de clonacion
// (con guardias del proyecto, reutilizando los enemigos que ya existen) y
// finalmente el duelo contra el clon. Cada zona tiene su propio fondo.
//
// El clon sigue siendo el "espejo narrativo" que describe el diseno
// (docs/AUTONOMOUS_PROGRESS.md / GAME_DESIGN.md) — Rayden contra su propio
// clon corrupto. El clon reutiliza el mismo AttackData/CombatWorld que el
// resto del juego (nada de sistemas paralelos) y su kit imita al del
// jugador: dash, teleport, Energy Wave oscura, Rage, Finisher, 3 fases.
// Boss dibujado con primitivas raylib (mismo enfoque que los otros 4
// bosses) hasta tener sprite real.
namespace district_fury {
class Stage5Game {
public:
    Stage5Game();
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
    enum class BossAttack { None, MirrorCombo, DarkWave, Teleport, Dash, Finisher };
    struct RayderClone {
        Vector3D pos{0,575,0}; int hp{2200}; int maxHp{2200}; int phase{1};
        float attackTimer{1}; float elapsed{0}; float invuln{0}; float teleportCooldown{3.0f};
        bool defeated{false}; bool facingRight{false}; BossAttack attack{BossAttack::None};
    };
    Player player;
    std::vector<StreetEnemy> enemies;
    CombatWorld combatWorld;
    KesslerCameo kessler;  // NPC narrativo: no se golpea ni golpea
    std::vector<Projectile> projectiles;
    std::vector<Particle> particles;
    Flow flow{Flow::Intro};
    RayderClone boss;
    float cameraX{640}; float stageTime{0}; float hitstop{0}; float shake{0}; float comboTimer{0};
    int combo{0}; int maxCombo{0}; int damageTaken{0};
    float bannerTimer{0};
    bool advanceRequested{false};
    // Zona actual: 1-3 son las salas previas con guardias, 4 es el duelo.
    int zone{1};
    bool zoneSpawned{false};
    void UpdateCombat(float dt); void UpdateBoss(float dt); void UpdateProjectiles(float dt); void UpdateParticles(float dt);
    void SpawnZone(int id); void HandleEnemyHits(); void HandlePlayerHits(); void HandleBossHits(); void SpawnEnergy(); void SpawnImpact(Vector3D pos, Color color, bool heavy); void DefeatBoss();
    void DrawWorld() const; void DrawHUD() const; void DrawBoss() const; void DrawClear() const;
};
}
