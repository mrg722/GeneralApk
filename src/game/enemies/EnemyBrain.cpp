#include "game/enemies/EnemyBrain.h"
#include "game/StreetEnemy.h"
#include <algorithm>
#include <cmath>

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

// ---------------------------------------------------------------------------
// F1.9: ataques de datos por tipo.
// ---------------------------------------------------------------------------
namespace {

// Golpe frontal "historico": reproduce la caja de antes (de 0.18 R a 0.86 R al
// frente, alto 44-88 px) como arco del miembro, en px nominales (/ visualScale).
LimbArc StrikeArc(const EnemyProfile& p, float reachFrac = 0.86f, float ph = -66.0f, float r = 22.0f) {
    LimbArc a;
    a.px = 8.0f;
    a.ph = ph;
    const float reach = p.attackRange * reachFrac / p.visualScale;
    a.L1 = std::max(30.0f, reach - a.px - r);   // la capsula llega a reach (punta + radio)
    a.L0 = a.L1 * 0.64f;
    a.theta0 = 0.0f; a.theta1 = 0.0f;
    a.r = r;
    a.ease = Ease::OutCubic;
    return a;
}

// Ataque A: el historico de cada tipo (antes activo del 34 % al 74 % de
// attackDuration, cortado por el fin de la animacion de 4 cuadros).
EnemyAttackDef Base(const EnemyProfile& p, const char* name, float s, float a, float r) {
    EnemyAttackDef d;
    d.name = name;
    d.startup = s; d.active = a; d.recovery = r;
    d.arc = StrikeArc(p);
    d.heavy = p.hasArmor;
    d.knockback = d.heavy ? 220.0f : 120.0f;
    d.hitstun = d.heavy ? 0.20f : 0.13f;
    return d;
}

EnemyMoveSet Build(StreetEnemyType type) {
    const EnemyProfile& p = GetEnemyProfile(type);
    EnemyMoveSet m;
    m.juggleBudget = (type == StreetEnemyType::Brute || type == StreetEnemyType::Mutant ||
                      type == StreetEnemyType::ArmoredGuard) ? 4 : 6;
    switch (type) {
        case StreetEnemyType::Punk: {
            EnemyAttackDef a = Base(p, "JAB", 0.18f, 0.14f, 0.12f);
            a.weight = 2.0f;
            EnemyAttackDef b = Base(p, "PATADA BAJA", 0.24f, 0.12f, 0.18f);
            b.low = true; b.sy = 0.85f; b.damageScale = 1.1f; b.knockback = 140.0f;
            b.arc = StrikeArc(p, 0.92f, -22.0f, 18.0f); b.arc.theta0 = -12.0f; b.arc.theta1 = -4.0f;
            b.vsGuard = true;
            m.attacks = {a, b};
            break;
        }
        case StreetEnemyType::Charger: {
            EnemyAttackDef a = Base(p, "GOLPE", 0.18f, 0.18f, 0.10f);
            a.weight = 1.5f;
            EnemyAttackDef b = Base(p, "HOMBRO", 0.22f, 0.12f, 0.16f);
            b.lunge = 60.0f; b.damageScale = 1.2f; b.knockback = 260.0f; b.hitstun = 0.18f;
            b.arc = StrikeArc(p, 0.62f, -72.0f, 26.0f);
            b.minDist = 70.0f;
            m.attacks = {a, b};
            break;
        }
        case StreetEnemyType::Brute: {
            EnemyAttackDef a = Base(p, "GOLPE PESADO", 0.24f, 0.16f, 0.14f);
            a.knockdown = true; a.weight = 2.0f;
            EnemyAttackDef b = Base(p, "PISOTON", 0.34f, 0.10f, 0.22f);
            b.low = true; b.areaRadius = 80.0f; b.sy = 0.9f; b.knockback = 160.0f; b.hitstun = 0.22f;
            b.maxDist = 95.0f; b.vsGuard = true;
            EnemyAttackDef c = Base(p, "CARGA CORTA", 0.30f, 0.16f, 0.20f);
            c.lunge = 80.0f; c.armor = true; c.damageScale = 1.15f; c.knockback = 300.0f;
            c.arc = StrikeArc(p, 0.70f, -70.0f, 26.0f); c.minDist = 80.0f;
            m.attacks = {a, b, c};
            break;
        }
        case StreetEnemyType::Enforcer: {
            EnemyAttackDef a = Base(p, "PORRA", 0.26f, 0.14f, 0.14f);
            a.weight = 2.0f;
            EnemyAttackDef b = Base(p, "GOLPE ALTO", 0.32f, 0.12f, 0.18f);
            b.overhead = true; b.damageScale = 1.2f; b.knockback = 180.0f; b.hitstun = 0.18f;
            b.arc = StrikeArc(p, 0.80f, -104.0f, 22.0f); b.arc.theta0 = 70.0f; b.arc.theta1 = -35.0f;
            b.arc.L0 = b.arc.L1 * 0.8f; b.vsGuard = true;
            EnemyAttackDef c = Base(p, "CONTRA", 0.12f, 0.14f, 0.18f);
            c.arc = StrikeArc(p, 1.0f); c.damageScale = 1.1f; c.knockback = 260.0f; c.afterBlock = true;
            m.attacks = {a, b, c};
            break;
        }
        case StreetEnemyType::ChemicalSoldier: {
            EnemyAttackDef a = Base(p, "ROCIADOR", 0.26f, 0.10f, 0.12f);
            a.weight = 2.0f;
            EnemyAttackDef b = Base(p, "CULATAZO", 0.16f, 0.10f, 0.14f);
            b.damageScale = 0.8f; b.knockback = 220.0f; b.arc = StrikeArc(p, 0.55f, -76.0f, 22.0f);
            b.maxDist = 110.0f; b.weight = 1.5f;
            m.attacks = {a, b};
            break;
        }
        case StreetEnemyType::UrbanNinja: {
            EnemyAttackDef a = Base(p, "TAJO", 0.15f, 0.07f, 0.08f);
            a.weight = 2.0f;
            EnemyAttackDef b = Base(p, "PATADA VOLADORA", 0.22f, 0.16f, 0.18f);
            b.lunge = 120.0f; b.launch = 300.0f; b.knockdown = true; b.knockback = 260.0f; b.damageScale = 1.1f;
            b.arc = StrikeArc(p, 0.60f, -60.0f, 22.0f); b.arc.theta0 = 10.0f; b.arc.theta1 = -8.0f;
            b.minDist = 90.0f;
            EnemyAttackDef c = Base(p, "DOBLE TAJO", 0.12f, 0.16f, 0.12f);
            c.hits = 2; c.damageScale = 0.6f; c.hitstun = 0.16f;
            m.attacks = {a, b, c};
            break;
        }
        case StreetEnemyType::Mutant: {
            EnemyAttackDef a = Base(p, "GOLPE PESADO", 0.28f, 0.12f, 0.14f);
            a.knockdown = true; a.heavy = true; a.weight = 2.0f;
            EnemyAttackDef b = Base(p, "BARRIDO", 0.30f, 0.14f, 0.20f);
            b.low = true; b.knockdown = true; b.sy = 0.85f; b.knockback = 200.0f;
            b.arc = StrikeArc(p, 0.95f, -20.0f, 18.0f); b.arc.theta0 = -8.0f; b.arc.theta1 = -2.0f;
            b.vsGuard = true;
            EnemyAttackDef c = Base(p, "RUGIDO", 0.35f, 0.10f, 0.25f);
            c.kind = EnemyMoveKind::Roar; c.areaRadius = 120.0f; c.damageScale = 0.0f; c.hitstun = 0.40f;
            c.knockback = 0.0f; c.unblockable = true; c.heavy = false; c.maxDist = 120.0f; c.weight = 0.6f;
            m.attacks = {a, b, c};
            break;
        }
        case StreetEnemyType::ArmoredGuard:
        default: {
            if (type != StreetEnemyType::ArmoredGuard) break;
            EnemyAttackDef a = Base(p, "ESCUDO", 0.27f, 0.13f, 0.14f);
            a.weight = 2.0f;
            EnemyAttackDef b = Base(p, "EMPUJON", 0.26f, 0.12f, 0.18f);
            b.knockback = 520.0f; b.damageScale = 0.8f; b.arc = StrikeArc(p, 0.70f, -70.0f, 30.0f);
            EnemyAttackDef c = Base(p, "CONTRA", 0.27f, 0.13f, 0.14f);
            c.arc = StrikeArc(p, 1.075f); c.afterBlock = true; c.counterSpecial = true;
            m.attacks = {a, b, c};
            break;
        }
    }
    return m;
}

}  // namespace

const EnemyMoveSet& GetEnemyMoveSet(StreetEnemyType type) {
    static const EnemyMoveSet kSets[8] = {
        Build(StreetEnemyType::Punk), Build(StreetEnemyType::Brute), Build(StreetEnemyType::Charger),
        Build(StreetEnemyType::Enforcer), Build(StreetEnemyType::ChemicalSoldier),
        Build(StreetEnemyType::UrbanNinja), Build(StreetEnemyType::Mutant), Build(StreetEnemyType::ArmoredGuard)};
    const int i = static_cast<int>(type);
    return kSets[(i >= 0 && i < 8) ? i : 0];
}

float EnemyAttackScale(StreetEnemyType type) { return GetEnemyProfile(type).visualScale; }

float EnemyAttackReach(const EnemyAttackDef& a, float scale) {
    if (a.areaRadius > 0.0f) return a.areaRadius * scale;
    const float th = a.arc.theta1 * 3.14159265f / 180.0f;
    return scale * (a.arc.px + a.arc.L1 * std::cos(th) + a.arc.r);
}

}  // namespace district_fury
