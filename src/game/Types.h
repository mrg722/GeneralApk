#pragma once
#include "raylib.h"

namespace district_fury {

struct Vector3D {
    float x = 0.0f; // world horizontal position
    float y = 0.0f; // world depth/lane position
    float z = 0.0f; // altitude

    Vector2 ToScreen() const {
        return {x, y - z};
    }
};

enum class Facing {
    Left = -1,
    Right = 1
};

struct CombatBox {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool IsValid() const {
        return width > 0.0f && height > 0.0f;
    }

    bool Intersects(const CombatBox& other) const {
        return IsValid() && other.IsValid() &&
               x < other.x + other.width &&
               x + width > other.x &&
               y < other.y + other.height &&
               y + height > other.y;
    }
};

constexpr float kStageStartX = 90.0f;
constexpr float kStageEndX = 6000.0f;
constexpr float kLaneMinY = 505.0f;
constexpr float kLaneMaxY = 625.0f;
constexpr float kGroundY = 650.0f;
constexpr float kLaneSpan = kLaneMaxY - kLaneMinY;

// Escala de profundidad canonica. Vivia duplicada en Player.cpp y StreetEnemy.cpp
// con rangos distintos a los carriles reales (DF-013 D6).
inline float DepthScaleFor(float y) {
    const float t = (y - kLaneMinY) / kLaneSpan;
    const float clamped = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return 0.86f + 0.26f * clamped;
}
constexpr float kViewportWidth = 1280.0f;
constexpr float kViewportHeight = 720.0f;

}
