#pragma once
#include "raylib.h"
#include <cmath>

// DF-013.2 — dibujo de bosses/NPC con sprite real.
//
// Dos APIs:
//
// 1) DrawBossPose(): normaliza CADA pose a una altura objetivo. Sirve para los
//    sets recortados uno a uno (Titan-X, Titan-X Mejorado, Rayder Clone), donde
//    cada PNG trae solo su silueta.
//
// 2) DrawSpriteUniform(): escala TODAS las poses con el mismo factor y ancla
//    los pies. Es lo correcto cuando las poses comparten lienzo o linea de
//    suelo (Brakk viene en lienzo 256x256 con el suelo en y=247, Grinder y
//    Kessler vienen recortados y se apoyan por abajo). Asi el personaje no
//    "crece" ni "encoge" al cambiar de pose, que es el error tipico al
//    normalizar por altura cada frame.
//
// Ambas usan filtro POINT (se fija al cargar la textura) y, cuando el sprite
// original es pequeño (pixel art), redondean la escala a un entero para no
// romper la rejilla de pixeles.
namespace district_fury {

inline float SnapPixelScale(float scale, int sourceHeight) {
    if (sourceHeight >= 140 || scale < 1.0f) return scale;   // arte grande: escala libre
    const float snapped = std::round(scale);
    return snapped < 1.0f ? 1.0f : snapped;
}

inline void DrawBossPose(Texture2D tex, Vector2 feetPosition, float targetHeight, bool flipX, Color tint = WHITE) {
    if (tex.id == 0 || tex.height <= 0 || tex.width <= 0) return;
    const float scale = targetHeight / static_cast<float>(tex.height);
    const float w = static_cast<float>(tex.width) * scale;
    const float h = static_cast<float>(tex.height) * scale;
    const Rectangle source{0.0f, 0.0f, flipX ? -static_cast<float>(tex.width) : static_cast<float>(tex.width), static_cast<float>(tex.height)};
    const Rectangle dest{feetPosition.x - w * 0.5f, feetPosition.y - h, w, h};
    DrawTexturePro(tex, source, dest, {0.0f, 0.0f}, 0.0f, tint);
}

// scale: pixeles de pantalla por pixel de sprite (igual para todas las poses).
// footInset: pixeles de sprite que quedan por DEBAJO de los pies dentro del
// lienzo (Brakk: 256 - 247 = 9). Mantiene los pies sobre la linea del suelo.
inline void DrawSpriteUniform(Texture2D tex, Vector2 feetPosition, float scale, bool flipX, Color tint = WHITE, float footInset = 0.0f) {
    if (tex.id == 0 || tex.height <= 0 || tex.width <= 0) return;
    const float w = static_cast<float>(tex.width) * scale;
    const float h = static_cast<float>(tex.height) * scale;
    const Rectangle source{0.0f, 0.0f, flipX ? -static_cast<float>(tex.width) : static_cast<float>(tex.width), static_cast<float>(tex.height)};
    const Rectangle dest{feetPosition.x - w * 0.5f, feetPosition.y - h + footInset * scale, w, h};
    DrawTexturePro(tex, source, dest, {0.0f, 0.0f}, 0.0f, tint);
}

}  // namespace district_fury
