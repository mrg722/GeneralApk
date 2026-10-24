#include "game/RivalAIProfile.h"
#include "game/CharacterVisual.h"
#include <cstring>

namespace district_fury {
namespace {

bool Starts(const char* name, const char* prefix) {
    return std::strncmp(name, prefix, std::strlen(prefix)) == 0;
}

AiProfile Make(AiArchetype a, float aggression, float blockChance, float dashChance,
               float skillChance, float retreatBias, float preferredRange) {
    return AiProfile{a, aggression, blockChance, dashChance, skillChance, retreatBias, preferredRange};
}

AiProfile Preset(AiArchetype a) {
    switch (a) {
        case AiArchetype::Aggressive: return Make(a, 1.4f, 0.4f, 1.2f, 0.8f, 0.4f, 110.0f);
        case AiArchetype::Zoner:      return Make(a, 0.8f, 1.0f, 0.6f, 1.8f, 1.3f, 260.0f);
        case AiArchetype::Heavy:      return Make(a, 0.9f, 0.5f, 0.0f, 0.9f, 0.3f, 140.0f);
        case AiArchetype::Defensive:  return Make(a, 0.7f, 1.8f, 0.7f, 1.0f, 1.6f, 160.0f);
        case AiArchetype::Fast:       return Make(a, 1.2f, 0.7f, 1.6f, 1.1f, 1.4f, 130.0f);
        case AiArchetype::Support:    return Make(a, 0.5f, 1.2f, 1.3f, 1.3f, 1.8f, 300.0f);
        case AiArchetype::Balanced:
        default:                     return Make(a, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 150.0f);
    }
}

// skin 6 es el personaje "RAYDER"/"MAX" (protagonista alterno, energia a
// rango); se identifica por indice, no por nombre, porque ese nombre puede
// cambiar (ver tarea de renombrado) sin que esto deba tocarse.
constexpr int kMaxSkinIndex = 6;

AiArchetype ArchetypeForName(const char* name) {
    if (Starts(name, "RAYDEN")) return AiArchetype::Balanced;
    if (Starts(name, "CLON ANTIGUO")) return AiArchetype::Aggressive;
    if (Starts(name, "BRAKK")) return AiArchetype::Heavy;
    if (Starts(name, "GRINDER")) return AiArchetype::Aggressive;
    if (Starts(name, "TITAN-X")) return AiArchetype::Heavy;
    if (Starts(name, "KF HEROE")) return AiArchetype::Zoner;          // incl. TRANSFORMADO
    if (Starts(name, "KF HEROINA")) return AiArchetype::Fast;         // incl. TRANSFORMADA
    if (Starts(name, "KF MATON")) return AiArchetype::Balanced;
    if (Starts(name, "KF NAVAJERA")) return AiArchetype::Fast;
    if (Starts(name, "KF SOLDADO")) return AiArchetype::Balanced;
    if (Starts(name, "KF RUBIA")) return AiArchetype::Fast;
    if (Starts(name, "KF GORRA")) return AiArchetype::Balanced;
    if (Starts(name, "KF PELEADOR")) return AiArchetype::Aggressive;
    if (Starts(name, "KF CUCHILLERO")) return AiArchetype::Fast;
    if (Starts(name, "KF JEFE GARRA")) return AiArchetype::Defensive;
    if (Starts(name, "KF BUFONA")) return AiArchetype::Aggressive;
    if (Starts(name, "RAYDER CLON BETA")) return AiArchetype::Zoner;
    if (Starts(name, "RAYDER CLON")) return AiArchetype::Fast;
    if (std::strcmp(name, "RONIN") == 0) return AiArchetype::Fast;    // no confundir con RONIN-KRATOS
    // Los dos luchadores de 4 armas (Kratos y RONIN-KRATOS) son el
    // enfrentamiento mas jugado en VS: se quedan en Balanced (bloquean lo
    // normal) para no volverse fragiles contra un rival que pega seguido.
    if (Starts(name, "RONIN-KRATOS")) return AiArchetype::Balanced;
    if (Starts(name, "GUERRERO")) return AiArchetype::Balanced;       // Kratos, 4 armas
    if (Starts(name, "SOLDADO ESQUELETO")) return AiArchetype::Balanced;
    if (Starts(name, "BESTIA EXCAVADORA")) return AiArchetype::Fast;
    if (Starts(name, "MOMIA CON ESCUDO")) return AiArchetype::Defensive;
    if (Starts(name, "BESTIA ELEFANTE")) return AiArchetype::Heavy;
    if (Starts(name, "AVE SANADORA")) return AiArchetype::Support;
    if (Starts(name, "CENTAURO")) return AiArchetype::Heavy;          // incl. ROJO
    if (Starts(name, "BRUTO DE LA BOLA")) return AiArchetype::Heavy;  // incl. ROJO
    if (Starts(name, "MEDUSA")) return AiArchetype::Defensive;        // incl. ROJA
    return AiArchetype::Balanced;
}

}  // namespace

AiProfile ProfileFor(int skin) {
    if (skin == kMaxSkinIndex) return Preset(AiArchetype::Zoner);
    const CharacterVisual& cv = GetCharacterVisual(skin);
    return Preset(ArchetypeForName(cv.name));
}

}  // namespace district_fury
