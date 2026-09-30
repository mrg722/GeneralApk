#include "core/ApplicationState.h"
#include "game/Types.h"
#include "rendering/Animator.h"
#include "rendering/SpriteAtlas.h"

#include <cstdlib>
#include <iostream>

int main() {
    using district_fury::CombatBox;
    using district_fury::AnimationClip;
    using district_fury::Animator;
    using district_fury::SpriteFrame;
    using district_fury::SpriteAtlas;
    using district_fury::core::ApplicationState;
    using district_fury::core::shouldContinue;

    if (!shouldContinue(ApplicationState::Running)) {
        std::cerr << "A running application must continue.\n";
        return EXIT_FAILURE;
    }

    if (shouldContinue(ApplicationState::ExitRequested)) {
        std::cerr << "An exit request must stop the application.\n";
        return EXIT_FAILURE;
    }

    const CombatBox player = {100.0f, 100.0f, 50.0f, 100.0f};
    const CombatBox attack = {130.0f, 140.0f, 80.0f, 35.0f};
    const CombatBox miss = {220.0f, 260.0f, 40.0f, 40.0f};

    if (!player.Intersects(attack)) {
        std::cerr << "Overlapping combat boxes must intersect.\n";
        return EXIT_FAILURE;
    }

    if (player.Intersects(miss)) {
        std::cerr << "Separated combat boxes must not intersect.\n";
        return EXIT_FAILURE;
    }

    Animator animator;
    animator.SetFrames({
        SpriteFrame{{0.0f, 0.0f, 32.0f, 64.0f}, 32.0f, 64.0f, 16.0f, 64.0f, 0.05f},
        SpriteFrame{{32.0f, 0.0f, 40.0f, 64.0f}, 40.0f, 64.0f, 20.0f, 64.0f, 0.20f}
    });
    animator.Play(AnimationClip{0, 1, 0.1f, false});
    animator.Update(0.06f);
    if (animator.currentFrame != 1) {
        std::cerr << "Frame metadata must advance the animator.\n";
        return EXIT_FAILURE;
    }
    animator.Update(0.21f);
    if (!animator.isFinished || animator.currentFrame != 1) {
        std::cerr << "Non-looping animation must freeze on its final frame.\n";
        return EXIT_FAILURE;
    }

    animator.Play(AnimationClip{0, 0, 0.1f, false, {1, 0}});
    if (animator.currentClip.startFrame != 1 || animator.currentClip.endFrame != 1 ||
        animator.currentFrame != 1) {
        std::cerr << "Explicit animation frame lists must select their authored range.\n";
        return EXIT_FAILURE;
    }

    SpriteAtlas atlas;
    atlas.id = "rayden";
    atlas.texturePath = "assets/characters/rayden.png";
    atlas.columns = 4;
    atlas.rows = 4;
    atlas.frames = animator.frames;
    if (!atlas.IsValid()) {
        std::cerr << "Valid sprite metadata must pass atlas validation.\n";
        return EXIT_FAILURE;
    }
    atlas.frames[0].pivotY = 65.0f;
    if (atlas.IsValid()) {
        std::cerr << "Invalid pivot metadata must fail atlas validation.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
