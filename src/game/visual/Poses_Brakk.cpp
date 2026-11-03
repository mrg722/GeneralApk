// Poses sueltas heredadas de BRAKK (skin 2, assets/bosses/brakk). Hoy Brakk
// jugable dibuja con su atlas "brakk_v2", asi que esta tabla solo responde si la
// ficha pierde el atlas. Movido tal cual desde CharacterPose (F0.1). Lo edita F1.5.
#include "game/visual/PoseTables.h"
#include <cstring>

namespace district_fury {

const char* PoseBrakkLegacy(PlayerState state, AttackType attack, bool rage, double time) {
    (void)time;
    if (state == PlayerState::Defeat) return "death";
    if (state == PlayerState::Hit || state == PlayerState::Knockdown || state == PlayerState::GuardBreak) return "hurt";
    if (state == PlayerState::Dash) return "charge";
    if (state == PlayerState::Attack) {
        if (attack == AttackType::Energy) return "chain_throw";
        if (rage) return "fury";
        return attack == AttackType::Kick ? "heavy" : "basic";
    }
    if (state == PlayerState::Walk) return "walk";
    return "idle";
}

bool PoseFrameBrakkLegacy(const PoseContext& ctx, PoseFrame& out) {
    return SinglePose(out, "brakk", PoseBrakkLegacy(ctx.state, ctx.attackType, ctx.rage, ctx.time));
}

int PoseFacesRightBrakk(const char* folder, const char* pose) {
    (void)pose;
    if (folder == nullptr) return -1;
    if (std::strcmp(folder, "brakk") == 0) return 1;   // ficha: facesRightByDefault = true
    return -1;
}

}  // namespace district_fury
