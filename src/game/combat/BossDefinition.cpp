#include "game/combat/BossDefinition.h"

// Datos extraidos del codigo REAL de cada stage (no inventados): hp/maxHp,
// umbrales de fase y nombres de ataque se copiaron de Stage1StoryGame.cpp,
// Stage2Game.cpp, Stage3Game.cpp, Stage4Game.cpp y Stage5Game.cpp tal como
// existen hoy. Los timings/danos por-ataque de Brakk, Titan-X (ambas
// formas) y Rayder Clone quedan con valores de partida razonables (no se
// migraron numero por numero todavia) porque este archivo AUN NO se
// consume desde ningun Stage*Game — es el paso 1 de la fase 2 descrita en
// ARCHITECTURE.md. Grinder (Stage2) si tiene sus 4 ataques con los danos
// reales (20 base, x1.25 en fase 3 = 25, y overdrive de fase con
// screenshake mayor) porque Stage2 fue el candidato piloto de migracion
// elegido en el plan.
//
// IMPORTANTE: mientras este archivo no se incluya desde ningun
// Stage*Game.cpp, NINGUN comportamiento del juego cambia. No se ha
// "terminado" la unificacion de bosses; esto es solo la tabla de datos.

namespace district_fury {
namespace {

// Orden de campos en BossAttackDef (ver BossDefinition.h):
// name, damage,
// startup, active, recovery, range, depth,
// boxWidth, boxHeight, boxForward,
// knockback, hitstun, cooldown, priority,
// heavy, spawnsProjectile, projectileSpeed,
// hitstop, shake, minPhaseHpRatio

const BossDefinition kBrakk{
    "Brakk", "BRAKK // CADENA Y ACERO",
    1200, 140.0f,
    150.0f, 145.0f, 1.0f, {200, 90, 60, 255},
    {
        // hpRatioThreshold, speedMult, cooldownMult, damageMult, tint
        {0.68f, 1.15f, 0.85f, 1.15f, {255, 90, 60, 60}},
        {0.34f, 1.30f, 0.70f, 1.30f, {255, 60, 40, 90}},
        {0.0f,  1.0f,  1.0f,  1.0f,  {0, 0, 0, 0}},
    },
    {
        {"ChainSwing", 18,
         0.30f, 0.22f, 0.35f, 260.0f, 120.0f,
         300.0f, 105.0f, 40.0f,
         360.0f, 0.30f, 1.6f, 2,
         false, false, 0.0f,
         0.10f, 0.15f, 0.0f},

        {"GroundSmash", 24,
         0.40f, 0.20f, 0.45f, 380.0f, 145.0f,
         380.0f, 100.0f, 60.0f,
         520.0f, 0.38f, 2.2f, 3,
         true, false, 0.0f,
         0.14f, 0.22f, 0.0f},

        {"Charge", 20,
         0.30f, 0.40f, 0.30f, 560.0f, 115.0f,
         190.0f, 115.0f, 55.0f,
         480.0f, 0.32f, 1.9f, 2,
         false, false, 0.0f,
         0.12f, 0.18f, 0.0f},

        {"PowerWave", 16,
         0.25f, 0.30f, 0.30f, 380.0f, 145.0f,
         380.0f, 135.0f, 30.0f,
         260.0f, 0.24f, 1.5f, 1,
         false, true, 480.0f,
         0.10f, 0.16f, 0.34f},

        {"Frenzy", 30,
         0.15f, 0.55f, 0.35f, 190.0f, 115.0f,
         190.0f, 115.0f, 65.0f,
         540.0f, 0.42f, 2.4f, 4,
         true, false, 0.0f,
         0.16f, 0.28f, 0.68f},
    },
    false,
};

const BossDefinition kGrinder{
    "Grinder", "GRINDER // EL SEGADOR",
    1100, 100.0f,
    140.0f, 145.0f, 1.0f, {70, 205, 115, 255},
    {
        // Umbrales y multiplicadores de cooldown copiados de
        // Stage2Game::UpdateBoss (ratio<=.35 => fase 3, ratio<=.70 => fase 2;
        // attackTimer 1.8s/1.45s/1.15s por fase; dano base 20, 25 y 30).
        {0.70f, 1.0f, 0.81f, 1.25f, {255, 175, 55, 40}},
        {0.35f, 1.0f, 0.64f, 1.50f, {255, 70, 45, 90}},
        {0.0f,  1.0f, 1.0f,  1.0f,  {0, 0, 0, 0}},
    },
    {
        // Hitboxes y danos base copiados literalmente de
        // Stage2Game::UpdateBoss.
        {"Saw", 20,
         0.32f, 0.25f, 0.28f, 300.0f, 110.0f,
         300.0f, 110.0f, 0.0f,
         300.0f, 0.20f, 1.8f, 2,
         false, false, 0.0f,
         0.12f, 0.18f, 0.0f},

        {"Slam", 20,
         0.55f, 0.20f, 0.30f, 560.0f, 100.0f,
         560.0f, 100.0f, 0.0f,
         380.0f, 0.26f, 1.8f, 3,
         true, false, 0.0f,
         0.14f, 0.20f, 0.0f},

        {"Ram", 20,
         0.42f, 0.42f, 0.53f, 190.0f, 115.0f,
         190.0f, 115.0f, 0.0f,
         420.0f, 0.24f, 1.8f, 2,
         false, false, 0.0f,
         0.12f, 0.18f, 0.0f},

        {"Overdrive", 20,
         0.32f, 0.32f, 0.53f, 380.0f, 145.0f,
         380.0f, 145.0f, 0.0f,
         260.0f, 0.20f, 1.8f, 4,
         true, false, 0.0f,
         0.16f, 0.25f, 0.0f},
    },
    false,
};

const BossDefinition kTitanX{
    "Titan-X (prototipo)", "TITAN-X // NUCLEO ASTRA",
    1500, 120.0f,
    160.0f, 155.0f, 1.0f, {70, 200, 150, 255},
    {
        // Organico-mecanico verde, forma humanoide/menos musculosa (canon
        // GAME_DESIGN.md). Stage3 no usa un enum BossAttack propio: alterna
        // Dash/Shield/PowerWave por temporizadores independientes
        // (dashTimer/shieldTimer/powerTimer, ver Stage3Game::UpdateBoss).
        // Se listan aqui como equivalentes; la migracion real debe revisar
        // esa funcion linea a linea para no perder el matiz de guardia
        // (shieldTimer/blocking).
        {0.66f, 1.15f, 0.85f, 1.10f, {90, 230, 170, 50}},
        {0.33f, 1.30f, 0.70f, 1.25f, {60, 255, 150, 90}},
        {0.0f,  1.0f,  1.0f,  1.0f,  {0, 0, 0, 0}},
    },
    {
        {"Dash", 22,
         0.28f, 0.35f, 0.35f, 520.0f, 120.0f,
         200.0f, 110.0f, 0.0f,
         500.0f, 0.30f, 1.7f, 2,
         false, false, 0.0f,
         0.12f, 0.18f, 0.0f},

        {"GuardCounter", 0,
         0.10f, 0.60f, 0.20f, 150.0f, 120.0f,
         150.0f, 105.0f, 0.0f,
         200.0f, 0.10f, 1.4f, 3,
         false, false, 0.0f,
         0.06f, 0.08f, 0.0f},

        {"ProjectileBurst", 18,
         0.30f, 0.25f, 0.35f, 460.0f, 140.0f,
         460.0f, 130.0f, 0.0f,
         250.0f, 0.18f, 2.0f, 1,
         false, true, 520.0f,
         0.10f, 0.14f, 0.35f},
    },
    true,
};

const BossDefinition kTitanXMejorado{
    "Titan-X Mejorado", "TITAN-X MEJORADO // NUCLEO KESSLER",
    1900, 130.0f,
    216.0f, 209.0f, 1.35f, {255, 150, 60, 255},
    {
        // Mismo organico-mecanico verde de Titan-X; escala mayor (ver
        // Stage4Game::DrawBoss, s=1.35f+(phase-1)*0.08f) y 4 fases en vez
        // de 3 (umbrales copiados de Stage4Game::UpdateBoss: .75/.5/.25).
        {0.75f, 1.08f, 0.92f, 1.05f, {255, 190, 90, 40}},
        {0.50f, 1.16f, 0.85f, 1.15f, {255, 160, 70, 70}},
        {0.25f, 1.24f, 0.75f, 1.30f, {255, 120, 50, 100}},
        {0.0f,  1.0f,  1.0f,  1.0f,  {0, 0, 0, 0}},
    },
    {
        // Nombres y forma tomados de Stage4Game.h BossAttack; danos base
        // pendientes de traspaso numerico exacto de Stage4Game::UpdateBoss.
        {"Dash", 24,
         0.30f, 0.35f, 0.35f, 640.0f, 125.0f,
         220.0f, 115.0f, 0.0f,
         540.0f, 0.32f, 1.6f, 2,
         false, false, 0.0f,
         0.13f, 0.19f, 0.0f},

        {"Shockwave", 26,
         0.45f, 0.22f, 0.40f, 560.0f, 100.0f,
         560.0f, 100.0f, 0.0f,
         420.0f, 0.30f, 2.0f, 3,
         true, false, 0.0f,
         0.18f, 0.24f, 0.0f},

        {"ProjectileBurst", 20,
         0.30f, 0.25f, 0.35f, 480.0f, 140.0f,
         480.0f, 130.0f, 0.0f,
         260.0f, 0.18f, 1.9f, 1,
         false, true, 560.0f,
         0.10f, 0.14f, 0.0f},

        {"Overdrive", 32,
         0.30f, 0.35f, 0.55f, 400.0f, 150.0f,
         400.0f, 140.0f, 0.0f,
         280.0f, 0.22f, 2.4f, 4,
         true, false, 0.0f,
         0.18f, 0.28f, 0.70f},
    },
    true,
};

const BossDefinition kRayderClone{
    "Rayder Clone", "RAYDER CLONE // ESPEJO OSCURO",
    2200, 210.0f,
    112.0f, 175.0f, 1.0f, {200, 40, 60, 255},
    {
        // El clon imita el kit del jugador; umbrales tomados de
        // Stage5Game::UpdateBoss (.65/.30).
        {0.65f, 1.10f, 0.85f, 1.10f, {255, 60, 90, 40}},
        {0.30f, 1.22f, 0.70f, 1.25f, {255, 30, 60, 80}},
        {0.0f,  1.0f,  1.0f,  1.0f,  {0, 0, 0, 0}},
    },
    {
        {"MirrorCombo", 16,
         0.18f, 0.30f, 0.30f, 130.0f, 110.0f,
         130.0f, 105.0f, 0.0f,
         260.0f, 0.20f, 1.3f, 2,
         false, false, 0.0f,
         0.10f, 0.14f, 0.0f},

        {"DarkWave", 22,
         0.30f, 0.25f, 0.35f, 460.0f, 140.0f,
         460.0f, 130.0f, 0.0f,
         220.0f, 0.16f, 1.8f, 1,
         false, true, 640.0f,
         0.10f, 0.15f, 0.0f},

        {"Dash", 20,
         0.22f, 0.30f, 0.30f, 300.0f, 115.0f,
         220.0f, 110.0f, 0.0f,
         400.0f, 0.22f, 1.5f, 2,
         false, false, 0.0f,
         0.12f, 0.16f, 0.0f},

        {"Finisher", 38,
         0.35f, 0.40f, 0.55f, 300.0f, 150.0f,
         300.0f, 140.0f, 0.0f,
         540.0f, 0.36f, 2.8f, 4,
         true, false, 0.0f,
         0.20f, 0.30f, 0.70f},
    },
    false,
};

}  // namespace

const BossDefinition& GetBossDefinition(BossId id) {
    switch (id) {
        case BossId::Brakk: return kBrakk;
        case BossId::Grinder: return kGrinder;
        case BossId::TitanX: return kTitanX;
        case BossId::TitanXMejorado: return kTitanXMejorado;
        case BossId::RayderClone: return kRayderClone;
        default: return kBrakk;
    }
}

const char* BossDisplayName(BossId id) {
    return GetBossDefinition(id).displayName;
}

}  // namespace district_fury
