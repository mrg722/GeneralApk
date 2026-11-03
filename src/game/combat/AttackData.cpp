#include "game/combat/AttackData.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace district_fury {
namespace {

// name, dmg, startup, active, recovery, range, depth,
// boxW, boxH, boxFwd, boxCY,
// knockback, hitstun, launch,
// sp, rage, prio, cooldown,
// combo, cancel, guardBreak, heavy, projectile,
// hitstop, shake, comboWindow
// [F0.2] arc (LimbArc V5: px, ph, L0, L1, theta0, theta1, r), hasArc
const AttackDef kHistoric[10] = {
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

constexpr float F(int frames) { return frames / 60.0f; }

// Fila generica F0.4 (BRIEF F0.4, tabla de la tarea 1). S/A/R en frames, Kb px/s,
// Hs = hitstun s, Lch = launch px/s. La caja historica (boxWidth...) se deriva
// del arco para quien no lo usa (KF por cuadro, depuracion).
AttackDef Row(const char* name, int dmg, int s, int a, int r, float kb, float hs, float lch, LimbArc arc,
              bool hasArc = true) {
    AttackDef d{};
    d.name = name;
    d.damage = dmg;
    d.startup = F(s); d.active = F(a); d.recovery = F(r);
    const float reach = std::fabs(arc.px) + std::max(arc.L0, arc.L1) + arc.r;
    d.range = reach + 20.0f;
    d.depth = 44.0f;
    d.boxWidth = std::max(arc.L0, arc.L1) + arc.r;
    d.boxHeight = std::max(40.0f, 2.0f * arc.r + 24.0f);
    d.boxForward = arc.px + 0.5f * std::max(arc.L0, arc.L1);
    d.boxCenterOffsetY = arc.ph;
    d.knockback = kb; d.hitstun = hs; d.launch = lch;
    d.spCost = 0;
    d.rageGain = std::max(4, dmg * 2 / 3);
    d.priority = dmg >= 16 ? 2 : 1;
    d.cooldown = 0.0f;
    d.canCombo = true; d.canCancel = true;
    d.breaksGuard = false; d.heavy = dmg >= 16; d.spawnsProjectile = false;
    d.hitstop = 0.06f + 0.003f * dmg; d.shake = 0.05f + 0.005f * dmg;
    d.comboWindow = 0.42f;
    d.arc = arc; d.hasArc = hasArc;
    return d;
}

// Root motion hacia el frente durante arranque + activo (fraccion del total).
RootCurve Fwd(float dx, Ease ease, const AttackDef& d) {
    RootCurve r;
    r.dx = dx; r.dz = 0; r.t0 = 0;
    const float total = std::max(1e-4f, d.startup + d.active + d.recovery);
    r.t1 = (d.startup + d.active) / total;
    r.ease = ease;
    return r;
}

AttackDef Disabled(const char* name) {
    AttackDef d{};
    d.name = name;
    d.requiresOverride = true;
    d.canCombo = true; d.canCancel = true;
    d.depth = 44.0f;
    return d;
}

std::vector<AttackDef> BuildTable() {
    std::vector<AttackDef> t(kHistoric, kHistoric + 10);
    // Ajustes F0.4 a los historicos: SOLO campos nuevos y el REMATE de la tabla
    // del brief (Kb 520, Hs .45, Lch 260, derribo, hitstop x1.3; tiempos de hoy).
    {
        AttackDef& p3 = t[static_cast<int>(AttackId::Punch3)];
        p3.knockback = 520.0f; p3.hitstun = 0.45f; p3.launch = 260.0f;
        p3.knockdown = true; p3.hitstopScale = 1.3f;
        t[static_cast<int>(AttackId::RageAttack)].knockdown = true;
        t[static_cast<int>(AttackId::Finisher)].knockdown = true;
    }
    using LA = LimbArc;
    // PunchHook: barrido horizontal.
    t.push_back(Row("GANCHO", 13, 7, 5, 11, 300, .34f, 0, LA{4, -88, 60, 70, -55, 20, 18}));
    // Kick2 / Kick3.
    t.push_back(Row("MEDIA LUNA", 16, 8, 6, 12, 400, .36f, 0, LA{0, -62, 100, 100, -70, 15, 20}));
    {
        AttackDef d = Row("TALON GIRATORIO", 22, 11, 6, 18, 600, .40f, 300, LA{0, -78, 105, 105, 60, -10, 22});
        d.knockdown = true; d.wallBounce = true;
        t.push_back(d);
    }
    // KickLauncher: lanzador (theta1 en la convencion V5, + = arriba; ver informe).
    {
        AttackDef d = Row("PATADA ALTA", 14, 8, 5, 16, 80, .55f, 720, LA{0, -70, 95, 95, -40, 110, 18});
        d.launcher = true; d.jumpCancel = true; d.juggleCost = 2;
        t.push_back(d);
    }
    {
        AttackDef d = Row("PUNO DE AVANCE", 16, 12, 5, 14, 420, .36f, 0, LA{8, -86, 60, 96, 0, 0, 18});
        d.overhead = true; d.root = Fwd(70, Ease::OutCubic, d);
        t.push_back(d);
    }
    {
        AttackDef d = Row("PATADA DE AVANCE", 15, 10, 6, 14, 430, .34f, 0, LA{0, -66, 70, 110, -15, -5, 18});
        d.root = Fwd(60, Ease::OutCubic, d);
        t.push_back(d);
    }
    {   // Codazo a la espalda SIN girar: pivote detras y angulo 180 (apunta atras).
        AttackDef d = Row("CODAZO", 12, 6, 4, 12, 380, .34f, 0, LA{-6, -84, 30, 62, 180, 180, 16});
        d.hitsBehind = true; d.boxForward = -40.0f;
        t.push_back(d);
    }
    {
        LA a{0, -64, 90, 90, -10, 10, 20}; a.bothSides = true;
        AttackDef d = Row("PATADA GIRATORIA", 16, 9, 8, 16, 420, .34f, 0, a);
        d.hitsBehind = true;
        t.push_back(d);
    }
    {
        AttackDef d = Row("BARRIDA", 12, 7, 6, 16, 120, .50f, 0, LA{0, -10, 40, 110, -2, -2, 14});
        d.low = true; d.knockdown = true;
        t.push_back(d);
    }
    {
        AttackDef d = Row("GANCHO ASCENDENTE", 14, 6, 6, 18, 60, .60f, 680, LA{6, -70, 50, 95, -30, 95, 18});
        d.launcher = true; d.jumpCancel = true; d.juggleCost = 2;
        d.armorFrom = 0.0f; d.armorTo = d.startup; d.armorHits = 1;
        const float total = d.startup + d.active + d.recovery;
        d.root.dz = 10; d.root.t0 = d.startup / total; d.root.t1 = (d.startup + d.active) / total;
        d.root.ease = Ease::Linear;
        t.push_back(d);
    }
    {   // Pisoton: caja (24,-6) 60x30 a ras de suelo.
        AttackDef d = Row("PISOTON", 10, 10, 4, 14, 0, .30f, 0, LA{24, -6, 30, 30, 0, 0, 15}, false);
        d.boxWidth = 60; d.boxHeight = 30; d.boxForward = 24 + 30; d.boxCenterOffsetY = -6;
        d.range = 90; d.otg = true;
        t.push_back(d);
    }
    {
        AttackDef d = Row("DESLIZAMIENTO", 13, 4, 10, 14, 300, .40f, 0, LA{0, -20, 60, 100, -5, -5, 16});
        d.low = true; d.knockdown = true; d.root = Fwd(120, Ease::OutQuad, d);
        d.cooldown = 0.35f;
        t.push_back(d);
    }
    {
        AttackDef d = Row("PUNO AEREO", 9, 4, 6, 8, 160, .30f, 220, LA{10, -80, 70, 70, -20, 10, 14});
        d.air = true; d.landRecovery = F(4);
        t.push_back(d);
    }
    {
        AttackDef d = Row("PUNO AEREO 2", 10, 4, 6, 8, 180, .30f, 220, LA{10, -80, 78, 78, -20, 10, 15});
        d.air = true; d.landRecovery = F(4);
        t.push_back(d);
    }
    {
        AttackDef d = Row("PATADA VOLADORA", 14, 5, 12, 6, 380, .34f, 0, LA{0, -50, 95, 95, -25, -25, 18});
        d.air = true; d.knockdown = true; d.landRecovery = F(4);
        t.push_back(d);
    }
    {   // Patada picada: activa hasta tocar el suelo (45 grados a 700 px/s); R = 10 f al caer.
        AttackDef d = Row("PATADA PICADA", 14, 6, 60, 10, 300, .34f, -500, LA{0, -40, 60, 60, -45, -45, 16});
        d.air = true; d.groundBounce = true; d.landRecovery = F(10);
        t.push_back(d);
    }
    {
        AttackDef d = Row("MARTILLO", 18, 8, 5, 14, 200, .40f, -700, LA{8, -70, 80, 80, 60, -60, 20});
        d.air = true; d.groundBounce = true; d.landRecovery = F(4);
        t.push_back(d);
    }
    {   // Shoryu: 2 golpes (cada 6 f), invulnerable f1-5, sube 90 px.
        AttackDef d = Row("SHORYU", 16, 3, 12, 22, 200, .50f, 760, LA{6, -80, 60, 100, -60, 100, 20});
        d.hitCount = 2; d.multiHitInterval = F(6); d.launcher = true; d.jumpCancel = true; d.juggleCost = 2;
        d.invulnTo = F(5); d.heavy = true; d.priority = 3;
        const float total = d.startup + d.active + d.recovery;
        d.root.dz = 90; d.root.t0 = 0; d.root.t1 = (d.startup + d.active) / total; d.root.ease = Ease::OutQuad;
        t.push_back(d);
    }
    {   // Huracan: 5 golpes cada 6 f, a los dos lados, avanza 160 px.
        LA a{0, -70, 90, 90, 0, 0, 20}; a.bothSides = true;
        AttackDef d = Row("HURACAN", 6, 8, 30, 16, 260, .30f, 0, a);
        d.hitCount = 5; d.multiHitInterval = F(6); d.hitsBehind = true; d.spCost = 15; d.priority = 3;
        d.root = Fwd(160, Ease::InOutSine, d); d.heavy = true;
        t.push_back(d);
    }
    {   // Taladro: 3 golpes, avanza 140 px; derriba (el ultimo golpe).
        AttackDef d = Row("TALADRO", 8, 10, 14, 18, 300, .30f, 0, LA{0, -60, 100, 100, -10, -10, 18});
        d.hitCount = 3; d.multiHitInterval = F(14) / 3.0f; d.spCost = 15; d.priority = 3;
        d.root = Fwd(140, Ease::InOutSine, d); d.heavy = true; d.knockdown = true;
        t.push_back(d);
    }
    t.push_back(Disabled("FIRMA 1"));
    t.push_back(Disabled("FIRMA 2"));
    t.push_back(Disabled("FIRMA 3"));
    // Fase 2 (F2.1): datos de combate.md 3.2/3.10, deshabilitados hasta entonces.
    {
        AttackDef d = Row("AGARRE", 0, 4, 4, 10, 0, .60f, 0, LA{8, -80, 20, 34, 0, 0, 16});
        d.requiresOverride = true; t.push_back(d);
    }
    {
        AttackDef d = Row("RODILLAZO", 6, 6, 4, 9, 60, .32f, 0, LA{6, -60, 20, 40, 0, 10, 16});
        d.requiresOverride = true; t.push_back(d);
    }
    {
        AttackDef d = Row("PROYECCION", 22, 8, 6, 20, 300, .60f, 260, LA{6, -80, 30, 60, 30, 60, 22});
        d.requiresOverride = true; d.knockdown = true; t.push_back(d);
    }
    {
        AttackDef d = Row("SUPLEX", 24, 10, 6, 22, 300, .60f, 300, LA{-6, -80, 30, 60, 150, 180, 22});
        d.requiresOverride = true; d.knockdown = true; d.hitsBehind = true; t.push_back(d);
    }
    {
        AttackDef d = Row("CONTRA", 12, 4, 6, 18, 500, .40f, 0, LA{8, -84, 40, 82, -4, 2, 18});
        d.requiresOverride = true; d.invulnTo = F(8); d.spCost = 25; t.push_back(d);
    }
    {
        AttackDef d = Row("BARRIDA AL LEVANTARSE", 8, 6, 6, 16, 200, .34f, 0, LA{0, -12, 40, 100, -2, -2, 14});
        d.requiresOverride = true; d.invulnTo = F(6); d.low = true; t.push_back(d);
    }
    t.push_back(Disabled("CADENA 5"));
    t.push_back(Disabled("CADENA 6"));
    return t;
}

const std::vector<AttackDef>& Table() {
    static const std::vector<AttackDef> kTable = BuildTable();
    return kTable;
}

}  // namespace

float AttackTotalDuration(const AttackDef& def) {
    return def.startup + def.active + def.recovery;
}

const AttackDef& GetAttack(AttackId id) {
    const std::vector<AttackDef>& t = Table();
    int index = static_cast<int>(id);
    if (index < 0 || index >= static_cast<int>(t.size())) index = 0;
    return t[static_cast<std::size_t>(index)];
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

bool AttackIsKickFamily(AttackId id) {
    switch (id) {
        case AttackId::Kick: case AttackId::Kick2: case AttackId::Kick3: case AttackId::KickLauncher:
        case AttackId::FwdKick: case AttackId::SpinKick: case AttackId::Sweep: case AttackId::DashKick:
        case AttackId::AirKick: case AttackId::DiveKick: case AttackId::Hurricane: case AttackId::DrillKick:
        case AttackId::Stomp:
            return true;
        default: return false;
    }
}

bool AttackIsSpecialMove(AttackId id) {
    switch (id) {
        case AttackId::EnergyWave: case AttackId::RisingUppercut: case AttackId::Hurricane:
        case AttackId::DrillKick: case AttackId::Signature1: case AttackId::Signature2: case AttackId::Signature3:
            return true;
        default: return false;
    }
}

bool AttackIsSuper(AttackId id) { return id == AttackId::RageAttack || id == AttackId::Finisher; }
bool AttackIsAir(AttackId id) { return GetAttack(id).air; }
bool AttackIsFase2(AttackId id) {
    const int i = static_cast<int>(id);
    return i >= static_cast<int>(AttackId::Grab) && i <= static_cast<int>(AttackId::WakeupAttack);
}

const char* AttackIdName(AttackId id) {
    static const char* kNames[] = {
        "None", "Punch1", "Punch2", "Punch3", "Kick", "EnergyWave", "DashAttack", "RageAttack", "Finisher", "Skill",
        "PunchHook", "Kick2", "Kick3", "KickLauncher", "FwdPunch", "FwdKick", "BackElbow", "SpinKick", "Sweep",
        "Uppercut", "Stomp", "DashKick", "AirPunch", "AirPunch2", "AirKick", "DiveKick", "AirSpike",
        "RisingUppercut", "Hurricane", "DrillKick", "Signature1", "Signature2", "Signature3", "Grab", "GrabKnee",
        "ThrowForward", "ThrowBack", "GuardCounter", "WakeupAttack", "Chain5", "Chain6"};
    static_assert(sizeof(kNames) / sizeof(kNames[0]) == kAttackIdCount, "AttackIdName desalineado");
    const int i = static_cast<int>(id);
    return i >= 0 && i < kAttackIdCount ? kNames[i] : "?";
}

}  // namespace district_fury
