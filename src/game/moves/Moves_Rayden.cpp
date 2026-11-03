// Moveset de RAYDEN (ORIGINAL), skin 0: SOLO se le AGREGAN movimientos; su arte y sus 19 clips no se tocan.
// Stub creado en F0.1; lo rellena SOLO el bloque F1.3 (datos: MoveSet,
// MoveOverride, MoveVisual de src/game/moves/MoveTypes.h). Debe devolver nullptr
// para los skins que no son suyos: entonces GetMoveSet sigue preguntando y, si
// nadie responde, usa el MoveSet generico.
#include "game/moves/MoveRegistry.h"

namespace district_fury {

const MoveSet* MovesetFor_Rayden(int skin) {
    (void)skin;
    return nullptr;
}

}  // namespace district_fury
