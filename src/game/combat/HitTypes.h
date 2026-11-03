// Tipos del resolvedor unico de golpes (contrato F0.1; los usa F0.2 en
// HitResolver y en Player/StreetEnemy::ApplyHit). Todavia nadie los usa: los
// valores por defecto no alteran nada. Los campos solo crecen al final.
#pragma once
#include "raylib.h"
#include "game/Types.h"
#include "game/events/GameEvents.h"   // HitKind

namespace district_fury {

// Un golpe que llega a una victima (jugador, enemigo o jefe).
struct HitInfo {
    int damage = 0;
    float dirX = 1;              // direccion del empuje: +1 derecha, -1 izquierda
    float knockback = 0, launch = 0, hitstun = 0.3f, blockstun = 0.2f, hitstop = 0.06f, shake = 0.06f;
    int juggleCost = 1;          // coste del presupuesto de juggle (V8)
    bool otg = false, breaksGuard = false, heavy = false, counter = false, wallBounce = false, groundBounce = false,
         unblockable = false, low = false, overhead = false, knockdown = false, pull = false;
    int sourceAttack = 0;        // static_cast<int>(AttackId) del golpe
    int hitIndex = 0;            // golpe n de un movimiento multi-golpe
    int attackerSkin = -1;
    Vector3D contact{};          // punto de contacto (x, carril, altura)
    // ---- F0.2 ----
    int priority = 1;            // prioridad del ataque (trade, B16)
    float pushOnBlock = 55.0f;   // empuje al bloquear (px/s en dirX)
};

enum class HitResult { Miss, Hit, Blocked, Armored, Killed, Ignored };

// Ajustes por stage del resolvedor unico (rabia extra, color de chispa, puntos).
struct StageHitStyle {
    int rageBonus = 0;
    Color spark{255, 235, 150, 255};
    Color sparkHeavy{255, 150, 45, 255};
    int scorePerDamage = 10;
    int scorePerCombo = 7;
    // ---- F0.2 ----
    int bossRageBonus = 8;                // dano extra en rabia contra el jefe
    Color sparkBlock{110, 180, 220, 255};  // chispa de golpe bloqueado
    Color sparkCounter{255, 95, 210, 255}; // chispa de contraataque (CONTRA)
    Color sparkBoss{255, 175, 55, 255};    // chispa al golpear al jefe
    Color sparkHurt{255, 80, 70, 255};     // chispa cuando golpean al jugador
};

}  // namespace district_fury
