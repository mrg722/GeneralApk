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

struct KfReference {
    bool loaded = false;
    std::string error;
    Animator templ;                    // frames + clips por nombre (sin textura propia)
    std::vector<std::string> clipNames;
    float scale = 1.8f;                // el luchador mide ~65 px en la APK
};

// Carga perezosa (necesita ventana abierta). spriteId 0 = luchador principal.
const KfReference& GetKfReference();

}  // namespace district_fury
