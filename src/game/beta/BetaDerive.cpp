// Interruptor y despacho de las derivaciones BETA (ver BetaDerive.h).
#include "game/beta/BetaDerive.h"

namespace district_fury {
namespace {
bool gBetaDeriveEnabled = true;
}  // namespace

void SetBetaDeriveEnabled(bool enabled) { gBetaDeriveEnabled = enabled; }
bool BetaDeriveEnabled() { return gBetaDeriveEnabled; }

void DeriveBetaClipsForDistrict(KfReference& ref, const char* betaId) {
    if (betaId == nullptr) return;
    DeriveWarriors(ref, betaId);
    DeriveBeasts(ref, betaId);
}

}  // namespace district_fury
