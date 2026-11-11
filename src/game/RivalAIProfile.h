#pragma once

// IA 2.0: cada personaje pelea distinto en vez de compartir una unica IA
// generica. `ProfileFor(skin)` deriva un perfil de combate a partir de los
// datos que YA tiene cada personaje en CharacterVisual (vida, multiplicador
// de dano/velocidad, si es del laboratorio KF, si es BETA/enemigo de oleada,
// etc.) mas unos pocos casos especiales por nombre para los que no hay una
// senal numerica (p.ej. un sanador se nota por el nombre, no por las stats).
// No depende del nombre "RAYDER"/"MAX" para nada estructural, asi que un
// renombre de ese personaje no le afecta.
namespace district_fury {

enum class AiArchetype {
    Balanced,    // peleador humano generico: mezcla pareja de golpe/combo/habilidad.
    Aggressive,  // presiona, poco retroceso, combos largos, casi no bloquea.
    Zoner,       // prefiere distancia, abusa de energia/habilidades a rango.
    Heavy,       // lento y pesado: no hace dash, pega fuerte, bloquea poco (no lo necesita).
    Defensive,   // bloquea y se retira mas, solo entra cuando el jugador se abre.
    Fast,        // hostiga: dash frecuente, entra y sale, golpes cortos.
    Support,     // evita el choque, mantiene distancia, ataca poco.
};

struct AiProfile {
    AiArchetype archetype = AiArchetype::Balanced;
    float aggression = 1.0f;       // multiplica la frecuencia de iniciativa (acercarse/atacar)
    float blockChance = 1.0f;      // multiplica la probabilidad de bloquear
    float dashChance = 1.0f;       // multiplica la probabilidad de dash al acercarse
    float skillChance = 1.0f;      // multiplica la probabilidad de usar habilidad/energia/especial
    float retreatBias = 1.0f;      // multiplica cuanto se aleja tras un combo
    float preferredRange = 150.0f; // distancia "comoda" para empezar a atacar de cerca
    // F2.2: gusto por cada familia del moveset nuevo (0 = nunca, 1 = normal, >1 = mucho).
    float airChance = 1.0f;        // saltos y persecucion/ataque aereo
    float launcherChance = 1.0f;   // lanzador J J K / vJ + juggle
    float grabChance = 1.0f;       // agarres (si el personaje los tiene; F2.1 los enciende)
    float commandChance = 1.0f;    // normales de comando (>J <K vJ vK...)
};

AiProfile ProfileFor(int skin);

}  // namespace district_fury
