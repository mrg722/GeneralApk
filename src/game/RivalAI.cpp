#include "game/RivalAI.h"
#include "raylib.h"
#include <cmath>

namespace district_fury {

PlayerInput RivalAI::Think(const Player& self, const Player& target, float dt) {
    PlayerInput in;
    if (self.state == PlayerState::Defeat || target.state == PlayerState::Defeat) return in;
    const float dx = target.position.x - self.position.x;
    const float dy = target.position.y - self.position.y;
    const float dist = std::fabs(dx);
    const float dir = dx >= 0.0f ? 1.0f : -1.0f;
    decide -= dt; comboGap -= dt; blockTimer -= dt; retreatTimer -= dt;

    // Bloquea a veces cuando el jugador ataca de cerca.
    if (blockTimer > 0.0f) { in.block = true; return in; }
    if (target.state == PlayerState::Attack && dist < 150.0f && decide <= 0.0f && GetRandomValue(0, 99) < 30) {
        blockTimer = 0.35f; decide = 0.4f; in.block = true; return in;
    }
    // Se aleja un momento despues de un combo (no queda pegado).
    if (retreatTimer > 0.0f) { in.moveX = -dir; in.moveY = dy > 0 ? -0.3f : 0.3f; return in; }

    // Alinea el carril (profundidad) con el jugador.
    if (std::fabs(dy) > 14.0f) in.moveY = dy > 0 ? 1.0f : -1.0f;

    // Combo en curso: J J J K como el jugador.
    if (comboLeft > 0) {
        if (comboGap <= 0.0f) {
            if (comboLeft == 1) in.kick = true; else in.punch = true;
            --comboLeft; comboGap = 0.16f;
            if (comboLeft == 0) { retreatTimer = 0.35f; decide = 0.5f; }
        }
        return in;
    }
    if (self.state == PlayerState::Attack) return in;   // deja terminar la animacion

    if (dist > 130.0f) {
        in.moveX = dir;   // se acerca (a veces con dash)
        if (dist > 380.0f && decide <= 0.0f && GetRandomValue(0, 99) < 25) { in.dash = true; decide = 0.8f; }
        // A distancia: poder de energia o habilidad.
        if (decide <= 0.0f && dist < 520.0f && GetRandomValue(0, 99) < 22) {
            const int slot = GetRandomValue(0, 4);
            if (self.SkillReady(slot)) in.skill = slot; else in.energy = true;
            decide = 1.1f;
        }
        return in;
    }
    if (decide > 0.0f) return in;
    // Cerca: elige golpe, combo, habilidad, transformacion o cambio de pagina.
    const int roll = GetRandomValue(0, 99);
    if (roll < 8 && self.SkillReady(5) && !self.IsTransformed()) { in.skill = 5; decide = 1.2f; }
    else if (roll < 36) {
        int slot = GetRandomValue(0, 4);
        for (int k = 0; k < 5 && !self.SkillReady(slot); ++k) slot = (slot + 1) % 5;
        if (self.SkillReady(slot)) { in.skill = slot; decide = 1.0f; }
        else { comboLeft = 4; comboGap = 0.0f; }
    } else if (roll < 44 && self.SkillPageCount() > 1) { in.skillPage = true; ++pageFlips; decide = 0.15f; }
    else if (roll < 80) { comboLeft = GetRandomValue(2, 4); comboGap = 0.0f; }
    else { in.kick = true; decide = 0.5f; }
    return in;
}

}  // namespace district_fury
