// Resolvedor UNICO de golpes (F0.2 del plan maestro de movimientos).
//
// Stage1..Stage5 y el Modo VS resuelven aqui todos los impactos cuerpo a cuerpo
// (jugador -> enemigos / jefe / rival, enemigos -> jugador, cajas de jefe ->
// jugador) y los de sus proyectiles. Antes cada modo tenia su copia y ninguno
// llamaba a Player::OnAttackConnected (combo siempre 0, Finisher inalcanzable,
// rageGain muerto). GowApk/BetaMode conserva su propio resolvedor.
//
// Reglas (docs/PLAN_MAESTRO_MOVIMIENTOS.md, BRIEF F0.2):
// - Colision 3D: forma del golpe en el plano (X, altura) + filtro de carril
//   abs(dLane) <= def.depth + 18. Nuestros personajes usan el arco del miembro
//   (LimbArc de AttackDef) con barrido anti-tunel; KF/BETA, la caja del cuadro.
// - Multi-objetivo: cada activacion lleva su lista de objetivos golpeados
//   (Player::hitTargets); re-golpe solo cada AttackDef::multiHitInterval.
// - Prioridad y trade (B16), counter-hit si el objetivo esta en su arranque
//   (dano x1.25, hitstun +0.12 s, hitstop x1.5, evento CounterHit).
// - SIEMPRE llama a Player::OnAttackConnected (combo, rageGain, ventana).
// - Sensacion V7 (hitstop, temblor de victima, camara, camara lenta).
// - Eventos de juego: HitLanded/HitBlocked/HitArmored/CounterHit, BossHit,
//   ProjectileHit (el dano al jugador lo emite Player::ApplyHit).
#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/CombatMath.h"
#include "game/combat/HitTypes.h"
#include "game/combat/HitstopClock.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace district_fury {

// Resumen de una resolucion para que cada stage conserve su puntuacion y
// contadores (combo, score, derrotados).
struct HitReport {
    static constexpr int kMaxHits = 16;
    int hits = 0;
    int kills = 0;
    int damageDealt = 0;
    bool hitBoss = false;
    bool heavy = false;
    bool blocked = false;
    int damages[kMaxHits] = {};   // dano de cada golpe (para la puntuacion por golpe)
    void Add(int dmg) {
        if (hits < kMaxHits) damages[hits] = dmg;
        ++hits;
        damageDealt += dmg;
    }
};

// Camara: empujon direccional de V7 (1.5 + 0.22 dano px, decae exp(-t/0.12)).
struct CameraKick {
    float amplitude = 0.0f, dirX = 1.0f, t = 1.0f;
    void Trigger(int damage, float dir) {
        const float a = CameraKickAmplitude(damage);
        if (a >= Offset()) { amplitude = a; dirX = dir >= 0.0f ? 1.0f : -1.0f; t = 0.0f; }
    }
    void Update(float dt) { t += dt; }
    float Offset() const { return t > 0.6f ? 0.0f : CameraKickOffset(amplitude, t); }
    float OffsetX() const { return dirX * Offset(); }
    void Reset() { amplitude = 0.0f; t = 1.0f; }
};

// Feedback del stage: todo opcional (nullptr = no se usa).
struct CombatFeedback {
    HitstopClock* hitstop = nullptr;
    float* shake = nullptr;        // temblor aleatorio legado (segundos)
    CameraKick* camera = nullptr;  // empujon direccional V7
    float* slowMo = nullptr;       // segundos restantes de camara lenta (timeScale 0.35)
    // Chispa en el punto de contacto: (x, carril, altura), color, pesada.
    std::function<void(Vector3D, Color, bool)> spark;
};

// Jefe de un stage adaptado al resolvedor (cada stage tiene su struct propio).
struct BossProxy {
    Vector3D pos{};
    float width = 150.0f, height = 145.0f;
    float halfDepth = 30.0f;          // media profundidad del jefe en el carril
    int* hp = nullptr;
    bool blocking = false;
    float* invuln = nullptr;
    float invulnOnHit = 0.10f;        // invulnerabilidad tras cada golpe
    int blockDivisor = 5;             // dano / n si esta en guardia
    float pushOnBlock = 0.0f;         // empuje al jugador si el jefe bloquea (px/s)
    std::function<void()> onDefeat;   // se llama una vez si hp llega a 0
};

// Proyectil generico para resolver contra enemigos, jefe o jugador.
struct ProjectileHitInput {
    Vector3D pos{};
    float prevX = 0.0f;       // x del frame anterior (barrido anti-tunel)
    float radius = 18.0f;
    float height = 74.0f;     // altura del centro sobre el suelo
    int damage = 0;
    float dirX = 1.0f;
    float knockback = 420.0f;
    float depth = 40.0f;      // tolerancia de carril (+ kLanePad)
};

// ---- helpers del jugador (publicos para tests) ----
// Forma del golpe del jugador en el plano (X, altura) + carril.
struct AttackShape {
    bool valid = false;
    bool arc = false;          // true: LimbArc de AttackDef; false: rectangulo
    LimbArc limb{};
    float x = 0, z = 0, facing = 1, scale = 1, uPrev = 0, u = 0;
    Rect2 rect{};
    float lane = 0;            // carril del atacante
    float depth = 0;           // def.depth
};
AttackShape PlayerAttackShape(const Player& p);
// Prueba la forma contra una hurtbox (plano) en el carril `lane` +- halfDepth.
bool ShapeHits(const AttackShape& s, const Rect2& hurt, float lane, float halfDepth, Vector2* contact);
// Hurtbox del jugador en el plano (deriva de GetHurtbox, incluye la de BETA).
Rect2 PlayerHurtRect(const Player& p);
// Hurtbox del enemigo en el plano: X [ex-w/2, ex+w/2], altura [z, z+alto].
Rect2 EnemyHurtRect(const StreetEnemy& e);
// HitInfo del ataque en curso del jugador (dano con multiplicadores + bonus).
HitInfo PlayerHitInfo(const Player& p, int rageBonus);
// HitInfo del ataque en curso de un enemigo contra el jugador.
HitInfo EnemyHitInfo(const StreetEnemy& e, const Player& target);

// B15: lee la entrada sin envejecer el buffer mientras el hitstop congela.
inline void PumpPlayerInput(Player& p, const HitstopClock& clock, float dt) {
    p.PumpInput(clock.Active() ? 0.0f : dt);
}

// ---- resolucion ----
HitReport ResolvePlayerVsEnemies(Player& player, std::vector<StreetEnemy>& enemies, const StageHitStyle& style,
                                 CombatFeedback& fb);
void ResolveEnemiesVsPlayer(std::vector<StreetEnemy>& enemies, Player& player, CombatFeedback& fb,
                            int maxAttackersPerFrame = 2, const StageHitStyle* style = nullptr,
                            int* damageTaken = nullptr);
bool ResolvePlayerVsBoss(Player& player, BossProxy& boss, const StageHitStyle& style, CombatFeedback& fb);
// Rival de VS (otro Player): ambos golpes se prueban en el mismo frame
// (prioridad / trade). Devuelve el dano hecho por a (>0) y por b en *damageB.
int ResolvePlayerVsPlayer(Player& a, Player& b, CombatFeedback& fb, const StageHitStyle& style,
                          int* damageB = nullptr);
// Caja de ataque de un jefe (pantalla) contra el jugador. dirX = sentido del
// golpe. Devuelve el resultado de Player::ApplyHit (Miss si no toca).
HitResult ResolveBoxVsPlayer(const CombatBox& box, float laneY, float depth, const HitInfo& info, Player& player,
                             CombatFeedback& fb, const StageHitStyle* style = nullptr);
// Solo la prueba geometrica de una caja de jefe contra el jugador (carril +
// plano), para stages que aplican su propio feedback.
bool BoxTouchesPlayer(const CombatBox& box, float laneY, float depth, const Player& player);
// Golpe de jefe de Historia: dano, empuje hacia el lado del jugador y derribo con
// el umbral historico (dano >= 28).
HitInfo BossHitInfo(int damage, float bossX, const Player& player);
// Proyectil del jugador contra enemigos: indice del enemigo golpeado o -1.
int ResolveProjectileVsEnemies(const ProjectileHitInput& pr, std::vector<StreetEnemy>& enemies,
                               const StageHitStyle& style, CombatFeedback& fb, HitReport* report = nullptr,
                               int attackerSkin = -1);
// Proyectil del jugador contra el jefe.
bool ResolveProjectileVsBoss(const ProjectileHitInput& pr, BossProxy& boss, const StageHitStyle& style,
                             CombatFeedback& fb);
// Proyectil enemigo/jefe contra el jugador (no lo para estar en guardia si
// unblockable). Devuelve true si toco (aunque lo bloqueara).
bool ResolveProjectileVsPlayer(const ProjectileHitInput& pr, Player& player, CombatFeedback& fb,
                               bool unblockable = false, const StageHitStyle* style = nullptr,
                               int* damageTaken = nullptr);

// Suma combo/puntuacion de un HitReport a los contadores de un stage.
void ApplyComboScore(const HitReport& r, const StageHitStyle& style, int& combo, int& maxCombo, float& comboTimer,
                     int* score);

}  // namespace district_fury
