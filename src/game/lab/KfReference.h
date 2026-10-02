#pragma once
// HERRAMIENTA DE ESTUDIO (Modo VS // Laboratorio). Lee en tiempo de ejecucion el
// respaldo de la APK (apk_reference/king_fighter_iii/bin/animation.bin), arma
// cada frame con sus piezas (modulo + posicion + transformacion, formato
// documentado en tools/apk/kf_decode.py) y lo recolorea en memoria. No se copia
// ni se guarda arte de la APK en assets/: si el respaldo no esta, no hay bot.
#include "rendering/Animator.h"
#include <map>
#include <string>
#include <vector>

namespace district_fury {

// Personajes extraidos (solo Modo VS // Laboratorio, temporales).
struct KfRosterEntry {
    int sprite;          // indice en animation.bin
    const char* name;
    bool hero;           // plantilla de acciones de heroe (61) o de enemigo (~30)
    int maxHp;
    float scale;         // escala de dibujo (los sprites de la APK miden ~65 px)
    int tint = 0;        // 0 = colores originales; 1 = copia con los colores de Rayder
    const char* betaId = nullptr;   // BETA: data/beta/characters/<betaId>.txt (no animation.bin)
};
int KfRosterCount();
// Personajes del roster que no son BETA (los que ve el Modo VS de siempre).
int KfStableRosterCount();
const KfRosterEntry& KfRoster(int index);

struct KfReference {
    bool loaded = false;
    std::string error;
    Animator templ;                    // frames + clips por nombre (sin textura propia)
    std::vector<std::string> clipNames;
    // TODAS las habilidades del personaje (clip "abN" -> nombre en pantalla), en
    // el orden en que se reparten por paginas en los botones 1-5.
    std::vector<std::string> abilityClips;
    std::vector<std::string> abilityNames;
    float scale = 1.8f;                // el luchador mide ~65 px en la APK
    // BETA (data/beta/characters): nombre, vida y sonidos por evento ("hurt", "die"...).
    bool beta = false;
    bool betaHero = false;
    std::string displayName;
    int maxHp = 0;
    std::map<std::string, std::string> sounds;
};

// Carga perezosa (necesita ventana abierta) del personaje `rosterIndex`.
// Sus clips tienen nombres de enemigo (idle, walk, atk1..4, special, hit, air,
// knockdown, getup, defeat) y de jugador (punch1..3, kick, energy, block,
// hit_high, hit_low, airborne, recovery, dash, dash_attack, rage_attack, finisher).
const KfReference& GetKfCharacter(int rosterIndex);
// Compatibilidad: luchador principal.
inline const KfReference& GetKfReference() { return GetKfCharacter(0); }

}  // namespace district_fury
