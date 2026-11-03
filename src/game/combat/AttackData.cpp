#include "game/combat/AttackData.h"

namespace district_fury {
namespace {

// name, dmg, startup, active, recovery, range, depth,
// boxW, boxH, boxFwd, boxCY,
// knockback, hitstun, launch,
// sp, rage, prio, cooldown,
// combo, cancel, guardBreak, heavy, projectile,
// hitstop, shake, comboWindow
// [F0.2] arc (LimbArc V5: px, ph, L0, L1, theta0, theta1, r), hasArc
const AttackDef kAttacks[static_cast<int>(AttackId::Count)] = {
    // None
    {"NINGUNO", 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f,
     0, 0, 0, 0.0f,
     false, false, false, false, false,
     0.0f, 0.0f, 0.0f},

    // Punch1 — apertura rapida, cancelable, poco compromiso.
    {"GOLPE", 10, 0.09f, 0.13f, 0.08f, 115.0f, 42.0f,
     82.0f, 48.0f, 54.0f, -76.0f,
     280.0f, 0.22f, 0.0f,
     0, 7, 1, 0.0f,
     true, true, false, false, false,
     0.060f, 0.060f, 0.48f,
     LimbArc{8, -84, 36, 74, -8, 0, 14}, true},

    // Punch2 — continuacion, algo mas de empuje.
    {"GOLPE 2", 12, 0.08f, 0.13f, 0.09f, 120.0f, 42.0f,
     88.0f, 50.0f, 58.0f, -76.0f,
     320.0f, 0.24f, 0.0f,
     0, 8, 1, 0.0f,
     true, true, false, false, false,
     0.070f, 0.075f, 0.46f,
     LimbArc{8, -86, 40, 82, -4, 2, 15}, true},

    // Punch3 — cierre del combo: derriba y rompe guardia.
    {"REMATE", 18, 0.11f, 0.15f, 0.16f, 128.0f, 44.0f,
     96.0f, 56.0f, 62.0f, -74.0f,
     470.0f, 0.34f, 180.0f,
     0, 12, 2, 0.0f,
     false, false, true, true, false,
     0.110f, 0.150f, 0.30f,
     LimbArc{6, -92, 50, 92, 20, -15, 22}, true},

    // Kick — lento, gran alcance, lanza.
    {"PATADA", 16, 0.11f, 0.17f, 0.14f, 135.0f, 44.0f,
     122.0f, 58.0f, 66.0f, -64.0f,
     440.0f, 0.32f, 140.0f,
     0, 11, 2, 0.05f,
     false, false, false, true, false,
     0.105f, 0.140f, 0.34f,
     LimbArc{0, -58, 50, 110, -20, -5, 18}, true},

    // EnergyWave — carga corta, proyectil, atraviesa guardia.
    {"ONDA DE ENERGIA", 20, 0.20f, 0.23f, 0.12f, 220.0f, 55.0f,
     220.0f, 76.0f, 122.0f, -72.0f,
     500.0f, 0.30f, 0.0f,
     20, 6, 3, 0.18f,
     false, false, true, true, true,
     0.100f, 0.130f, 0.40f},

    // DashAttack — embestida con armor durante el arranque.
    {"EMBESTIDA", 15, 0.06f, 0.16f, 0.16f, 130.0f, 40.0f,
     104.0f, 60.0f, 60.0f, -70.0f,
     520.0f, 0.30f, 0.0f,
     0, 10, 2, 0.35f,
     false, false, true, true, false,
     0.095f, 0.130f, 0.36f,
     LimbArc{6, -72, 50, 100, -5, 0, 22}, true},

    // RageAttack — solo en Rage. Barrido amplio.
    {"FURIA", 30, 0.13f, 0.22f, 0.20f, 190.0f, 62.0f,
     230.0f, 96.0f, 96.0f, -78.0f,
     620.0f, 0.42f, 220.0f,
     0, 0, 4, 0.90f,
     false, false, true, true, false,
     0.140f, 0.240f, 0.30f,
     LimbArc{0, -78, 60, 130, -40, 40, 34, Ease::OutCubic, false, true}, true},

    // Finisher — remate de combo alto.
    {"FINISHER", 44, 0.16f, 0.24f, 0.26f, 200.0f, 64.0f,
     250.0f, 110.0f, 104.0f, -80.0f,
     760.0f, 0.55f, 300.0f,
     35, 0, 5, 1.60f,
     false, false, true, true, false,
     0.180f, 0.300f, 0.30f,
     LimbArc{0, -80, 70, 140, -30, 30, 40}, true},

    // Skill — habilidad (botones 1-6). Golpea varias veces mientras dura su
    // animacion; la caja de golpe la marca el dibujo del frame (ver Player).
    {"HABILIDAD", 9, 0.0f, 0.0f, 0.0f, 160.0f, 64.0f,
     160.0f, 110.0f, 80.0f, -70.0f,
     420.0f, 0.30f, 120.0f,
     0, 6, 4, 0.0f,
     false, false, true, true, false,
     0.070f, 0.120f, 0.30f},
};

}  // namespace

float AttackTotalDuration(const AttackDef& def) {
    return def.startup + def.active + def.recovery;
}

const AttackDef& GetAttack(AttackId id) {
    int index = static_cast<int>(id);
    if (index < 0 || index >= static_cast<int>(AttackId::Count)) index = 0;
    return kAttacks[index];
}

AttackId NextPunchInChain(AttackId current) {
    switch (current) {
        case AttackId::Punch1: return AttackId::Punch2;
        case AttackId::Punch2: return AttackId::Punch3;
        default:               return AttackId::Punch1;
    }
}

bool AttackIsEnergy(AttackId id) { return id == AttackId::EnergyWave; }
bool AttackIsHeavy(AttackId id) { return GetAttack(id).heavy; }

}  // namespace district_fury
