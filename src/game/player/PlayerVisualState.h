// Estado SOLO de dibujo del jugador (contrato F0.1, lo llena F0.3). Vive como
// miembro `mutable PlayerVisualState visual;` de Player para que Draw() (const)
// pueda actualizarlo. Nunca afecta la jugabilidad.
#pragma once
#include "raylib.h"

namespace district_fury {

// Una estela: copia de la textura + recorte + transformacion del cuerpo en el
// momento en que se tomo. Se dibuja con alfa 110 -> 0 durante 0.22 s.
// frame >= 0: cuadro con piezas (BETA) que se arma con el animador del jugador.
struct TrailCopy {
    Texture2D tex{};
    Rectangle src{};        // recorte en la textura (ancho positivo)
    Vector2 size{};         // tamano de dibujo sin escalar (px de textura)
    Vector2 pos{};          // pies en pantalla
    Vector2 origin{};       // punto de la textura (px del recorte) que va en los pies
    float scale = 1, angle = 0, sx = 1, sy = 1;
    bool flip = false;
    Color tint{255, 255, 255, 255};   // color (el alfa lo pone la vida)
    float life = 0;
    int frame = -1;
};

struct PlayerVisualState {
    static constexpr int kMaxTrails = 8;
    TrailCopy trails[kMaxTrails];
    float trailTimer = 0;
    // Tiempo en el estado actual (para CAIDA_ROT, visuales de estado...).
    float stateClock = 0;
    int stateSeen = -1;        // static_cast<int>(PlayerState) del ultimo Update
    float prevZ = 0;
};

}  // namespace district_fury
