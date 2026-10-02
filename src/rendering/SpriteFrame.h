#pragma once

#include "raylib.h"
#include <optional>
#include <vector>

namespace district_fury {

// BETA: pieza de un cuadro armado al dibujar (formula de piezas del juego
// original). `tex` indexa Animator::pieceTextures; (x, y) es la esquina de la
// pieza relativa a los pies (y negativa hacia arriba), sin escalar.
struct FramePiece {
    int tex = 0;
    Rectangle src = {};
    float x = 0.0f, y = 0.0f;
    bool flipX = false, flipY = false;
};

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
    // Vacio en todos los personajes de siempre; con piezas, Draw arma el cuadro.
    // En los cuadros con piezas, hitbox/hurtbox van relativos a los pies.
    std::vector<FramePiece> pieces;
};

}