#pragma once
#include "game/lab/KfReference.h"
#include <string>

namespace district_fury {

// Carga un personaje armado por piezas desde data/kratos/<id>.txt o
// GowApk/data/characters/<id>.txt (generado por
// tools/beta/build_beta_characters.py desde nuevosSprites). Los cuadros se arman
// con las piezas originales (assets/kratos/actor/N.png) sin escalar ni deformar;
// conservan las cajas de golpe y de cuerpo, los tiempos y los sonidos por cuadro.
// Devuelve un KfReference para reutilizar todo lo que ya usa Player
// (clips por nombre, habilidades por paginas, rival por IA).
KfReference LoadBetaCharacter(const std::string& id);

}  // namespace district_fury
