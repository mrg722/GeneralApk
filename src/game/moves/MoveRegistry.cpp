// Registro de movesets por personaje (ver MoveRegistry.h).
#include "game/moves/MoveRegistry.h"

namespace district_fury {

const MoveSet& GenericMoveSet() {
    static const MoveSet kGeneric{};
    return kGeneric;
}

const MoveSet& GetMoveSet(int skin) {
    using Provider = const MoveSet* (*)(int);
    static const Provider kProviders[] = {
        MovesetFor_TitanX, MovesetFor_CloneGrinder, MovesetFor_Rayden, MovesetFor_Max,
        MovesetFor_Brakk,  MovesetFor_Kf,           MovesetFor_Warriors, MovesetFor_Beasts,
    };
    for (Provider p : kProviders)
        if (const MoveSet* set = p(skin)) return *set;
    return GenericMoveSet();
}

const MoveOverride* FindOverride(const MoveSet& set, int id) {
    if (set.overrides == nullptr) return nullptr;
    for (int i = 0; i < set.overrideCount; ++i)
        if (set.overrides[i].id == id) return &set.overrides[i];
    return nullptr;
}

}  // namespace district_fury
