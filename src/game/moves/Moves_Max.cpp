// Moveset de MAX (skin 6, ex-RAYDER; otro personaje, no es Rayden).
// Stub creado en F0.1; lo rellena SOLO el bloque F1.4 (datos: MoveSet,
// MoveOverride, MoveVisual de src/game/moves/MoveTypes.h). Debe devolver nullptr
// para los skins que no son suyos: entonces GetMoveSet sigue preguntando y, si
// nadie responde, usa el MoveSet generico.
#include "game/moves/MoveRegistry.h"

namespace district_fury {

const MoveSet* MovesetFor_Max(int skin) {
    (void)skin;
    return nullptr;
}

}  // namespace district_fury
