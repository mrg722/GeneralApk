// Registro de movesets por personaje (ver MoveRegistry.h).
#include "game/moves/MoveRegistry.h"

namespace district_fury {

const MoveSet& GenericMoveSet() {
    // F0.4: cadenas y botones de habilidad por defecto (V2).
    static const MoveSet kGeneric = [] {
        MoveSet m;
        FillDefaultChains(m);
        return m;
    }();
    return kGeneric;
}

void FillDefaultChains(MoveSet& m) {
    const int j[4] = {static_cast<int>(AttackId::Punch1), static_cast<int>(AttackId::Punch2),
                      static_cast<int>(AttackId::PunchHook), static_cast<int>(AttackId::Punch3)};
    const int k[3] = {static_cast<int>(AttackId::Kick), static_cast<int>(AttackId::Kick2),
                      static_cast<int>(AttackId::Kick3)};
    if (m.chainJ[0] == 0) {
        for (int i = 0; i < 6; ++i) m.chainJ[i] = i < 4 ? j[i] : 0;
        m.chainJCount = 4;
    }
    if (m.chainK[0] == 0) {
        for (int i = 0; i < 4; ++i) m.chainK[i] = i < 3 ? k[i] : 0;
        m.chainKCount = 3;
    }
    for (int i = 0; i < 5; ++i)
        if (m.skillSlots[i] == 0) m.skillSlots[i] = DefaultSkillSlot(i);
}

int DefaultSkillSlot(int i) {
    static const AttackId kSlots[5] = {AttackId::EnergyWave, AttackId::Uppercut, AttackId::RageAttack,
                                       AttackId::DashAttack, AttackId::Finisher};
    return i >= 0 && i < 5 ? static_cast<int>(kSlots[i]) : 0;
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
