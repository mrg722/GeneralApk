#pragma once

#include "raylib.h"
#include <optional>

namespace district_fury {

struct SpriteFrame {
    Rectangle source = {};
    float width = 0.0f;
    float height = 0.0f;
    float pivotX = 0.0f;
    float pivotY = 0.0f;
    float duration = 0.1f;
    Rectangle visualBounds = {};
    std::optional<Vector2> shadowPoint;
    std::optional<Vector2> attackPoint;
    std::optional<Rectangle> hurtbox;
    std::optional<Rectangle> hitbox;
};

}