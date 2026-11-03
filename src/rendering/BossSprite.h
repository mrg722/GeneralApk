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

// F0.3 — poses sueltas con escala UNIFORME por personaje (la misma para todas
// sus poses: se acaba el "cada pose a la misma altura"), recorte, rotacion y
// estiramiento. Dibuja `src` de `tex` (src.width <= 0 = toda la textura) a
// `scale` px de pantalla por px de textura (por sx/sy), apoyando el centro
// inferior del recorte, desplazado `offset` px de textura (x hacia la derecha
// del dibujo, y hacia abajo), en `feet`. Espejo si flipX (el offset tambien se
// refleja). Gira angleDeg grados ALREDEDOR DE LOS PIES. Las de arriba no cambian
// (las usan los jefes).
inline void DrawBossPoseEx(Texture2D tex, Rectangle src, Vector2 feet, float scale, bool flipX, float angleDeg,
                           float sx, float sy, Color tint = WHITE, Vector2 offset = {0.0f, 0.0f}) {
    if (tex.id == 0 || tex.height <= 0 || tex.width <= 0) return;
    if (src.width <= 0.0f || src.height <= 0.0f) src = {0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)};
    const float w = src.width * scale * sx;
    const float h = src.height * scale * sy;
    if (w == 0.0f || h <= 0.0f) return;
    const Rectangle source{src.x, src.y, flipX ? -src.width : src.width, src.height};
    const float ox = offset.x * scale * sx * (flipX ? -1.0f : 1.0f);
    const float oy = offset.y * scale * sy;
    DrawTexturePro(tex, source, {feet.x, feet.y, w, h}, {w * 0.5f - ox, h - oy}, angleDeg, tint);
}

// Escala uniforme de un set de poses sueltas: `targetHeight` px de pantalla para
// la altura del reposo (idle1). 0 si falta la textura.
inline float PoseSetScale(Texture2D idle, float targetHeight) {
    return (idle.id == 0 || idle.height <= 0) ? 0.0f : targetHeight / static_cast<float>(idle.height);
}

}  // namespace district_fury
