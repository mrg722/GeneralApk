// Moveset de CLON ANTIGUO (skin 1) y GRINDER (skin 3).
// Stub creado en F0.1; lo rellena SOLO el bloque F1.2 (datos: MoveSet,
// MoveOverride, MoveVisual de src/game/moves/MoveTypes.h). Debe devolver nullptr
// para los skins que no son suyos: entonces GetMoveSet sigue preguntando y, si
// nadie responde, usa el MoveSet generico.
#include "game/moves/MoveRegistry.h"

namespace district_fury {

const MoveSet* MovesetFor_CloneGrinder(int skin) {
    (void)skin;
    return nullptr;
}

}  // namespace district_fury
