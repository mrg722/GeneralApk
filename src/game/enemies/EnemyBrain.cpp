#include "game/enemies/EnemyBrain.h"
#include "game/StreetEnemy.h"

namespace district_fury {
namespace {

// Cada perfil responde a la pregunta del brief: "cada enemigo debe requerir una
// respuesta diferente del jugador", no solo numeros distintos.
const EnemyProfile kPunk = {
    "PUNK", 58, 172.0f, 12.0f, 110.0f, 38.0f, 0.62f,
    1.00f, {225, 65, 80, 255}, 54.0f, 108.0f,
    0.85f, 0.05f, 0.0f, 70.0f, 150.0f, 0.0f, 0.12f, 0.55f, 0.10f,
    false, 0,
    EnemySpecial::None, 0.0f, 0.0f, 0,
    "Presion frontal. Castigalo con el combo basico."
};

const EnemyProfile kBrute = {
    "BRUTE", 130, 95.0f, 20.0f, 132.0f, 46.0f, 0.72f,
    1.10f, {220, 140, 70, 255}, 64.0f, 112.0f,
    0.70f, 0.10f, 40.0f, 90.0f, 70.0f, 0.0f, 0.34f, 0.95f, 0.05f,
    true, 14,
    EnemySpecial::HeavySlam, 4.2f, 150.0f, 28,
    "Lento pero con armor. Golpea y sal del alcance."
};

const EnemyProfile kCharger = {
    "CHARGER", 72, 220.0f, 14.0f, 120.0f, 42.0f, 0.54f,
    1.00f, {90, 155, 220, 255}, 54.0f, 108.0f,
    0.95f, 0.02f, 0.0f, 60.0f, 200.0f, 0.0f, 0.10f, 0.45f, 0.05f,
    false, 0,
    EnemySpecial::Charge, 3.0f, 520.0f, 18,
    "Carga en linea recta. Esquivalo con dash lateral."
};

const EnemyProfile kEnforcer = {
    "ENFORCER", 190, 122.0f, 24.0f, 146.0f, 50.0f, 0.76f,
    1.12f, {235, 185, 70, 255}, 66.0f, 116.0f,
    0.55f, 0.28f, 55.0f, 140.0f, 110.0f, 0.25f, 0.22f, 0.85f, 0.20f,
    false, 0,
    EnemySpecial::None, 0.0f, 0.0f, 0,
    "Controla la distancia. Entra despues de su ataque."
};

const EnemyProfile kChemicalSoldier = {
    "CHEMICAL", 155, 118.0f, 22.0f, 154.0f, 58.0f, 0.82f,
    1.08f, {65, 205, 105, 255}, 64.0f, 116.0f,
    0.35f, 0.15f, 35.0f, 230.0f, 130.0f, 0.35f, 0.26f, 1.10f, 0.35f,
    false, 0,
    EnemySpecial::ChemicalCloud, 5.0f, 320.0f, 8,
    "Mantiene distancia y deja zonas quimicas. Cierra rapido."
};

const EnemyProfile kUrbanNinja = {
    "URBAN NINJA", 82, 245.0f, 18.0f, 138.0f, 48.0f, 0.48f,
    1.00f, {80, 205, 220, 255}, 52.0f, 108.0f,
    0.80f, 0.12f, 20.0f, 90.0f, 250.0f, 0.0f, 0.08f, 0.60f, 0.85f,
    false, 0,
    EnemySpecial::Flank, 2.6f, 420.0f, 20,
    "Flanquea por profundidad. Vigila tu espalda."
};

const EnemyProfile kMutant = {
    "MUTANT", 220, 82.0f, 29.0f, 142.0f, 54.0f, 0.86f,
    1.16f, {105, 205, 75, 255}, 70.0f, 122.0f,
    1.00f, 0.05f, 30.0f, 60.0f, 80.0f, 0.0f, 0.30f, 0.80f, 0.05f,
    true, 18,
    EnemySpecial::Frenzy, 5.5f, 160.0f, 22,
    "Presion constante y resistencia alta. Usa la Onda."
};

const EnemyProfile kArmoredGuard = {
    "ARMORED", 250, 105.0f, 27.0f, 150.0f, 54.0f, 0.80f,
    1.14f, {105, 145, 180, 255}, 68.0f, 118.0f,
    0.45f, 0.62f, 90.0f, 120.0f, 95.0f, 0.20f, 0.20f, 1.00f, 0.15f,
    true, 20,
    EnemySpecial::GuardCounter, 3.4f, 150.0f, 26,
    "Bloquea y contraataca. Rompe la guardia con el remate."
};

}  // namespace

const EnemyProfile& GetEnemyProfile(StreetEnemyType type) {
    switch (type) {
        case StreetEnemyType::Brute:           return kBrute;
        case StreetEnemyType::Charger:         return kCharger;
        case StreetEnemyType::Enforcer:        return kEnforcer;
        case StreetEnemyType::ChemicalSoldier: return kChemicalSoldier;
        case StreetEnemyType::UrbanNinja:      return kUrbanNinja;
        case StreetEnemyType::Mutant:          return kMutant;
        case StreetEnemyType::ArmoredGuard:    return kArmoredGuard;
        default:                                return kPunk;
    }
}

const char* EnemySpecialName(EnemySpecial special) {
    switch (special) {
        case EnemySpecial::Charge:        return "CARGA";
        case EnemySpecial::ChemicalCloud: return "QUIMICO";
        case EnemySpecial::Flank:         return "FLANQUEO";
        case EnemySpecial::GuardCounter:  return "CONTRA";
        case EnemySpecial::HeavySlam:     return "IMPACTO";
        case EnemySpecial::Frenzy:        return "FRENESI";
        default:                          return "-";
    }
}

}  // namespace district_fury
