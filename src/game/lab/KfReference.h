#pragma once
// HERRAMIENTA DE ESTUDIO (Modo VS // Laboratorio). Lee en tiempo de ejecucion el
// respaldo de la APK (apk_reference/king_fighter_iii/bin/animation.bin), arma
// cada frame con sus piezas (modulo + posicion + transformacion, formato
// documentado en tools/apk/kf_decode.py) y lo recolorea en memoria. No se copia
// ni se guarda arte de la APK en assets/: si el respaldo no esta, no hay bot.
#include "rendering/Animator.h"
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
};
int KfRosterCount();
const KfRosterEntry& KfRoster(int index);

struct KfReference {
    bool loaded = false;
    std::string error;
    Animator templ;                    // frames + clips por nombre (sin textura propia)
    std::vector<std::string> clipNames;
    float scale = 1.8f;                // el luchador mide ~65 px en la APK
};

// Carga perezosa (necesita ventana abierta) del personaje `rosterIndex`.
// Sus clips tienen nombres de enemigo (idle, walk, atk1..4, special, hit, air,
// knockdown, getup, defeat) y de jugador (punch1..3, kick, energy, block,
// hit_high, hit_low, airborne, recovery, dash, dash_attack, rage_attack, finisher).
const KfReference& GetKfCharacter(int rosterIndex);
// Compatibilidad: luchador principal.
inline const KfReference& GetKfReference() { return GetKfCharacter(0); }

}  // namespace district_fury
