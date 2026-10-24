#include "game/RivalAI.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

namespace {
int Scaled(int basePct, float mult) {
    return std::clamp(static_cast<int>(basePct * mult), 0, 99);
}
}  // namespace

// IA 2.0: la forma de decidir es la misma para todos (bloquea, se acerca,
// combina golpes, usa habilidades, se retira), pero el PERFIL de cada
// personaje (RivalAIProfile.h, derivado de su archetype) cambia cuanto
// bloquea, si hace dash, a que distancia le gusta pelear, cuanto abusa de
// habilidades y cuanto se retira: el mismo codigo produce un Brakk lento y
// pesado que casi no bloquea y nunca hace dash, un KF Heroe que se queda a
// distancia abusando de energia, o un RONIN que entra y sale con dash.
PlayerInput RivalAI::Think(const Player& self, const Player& target, float dt) {
    PlayerInput in;
    if (self.state == PlayerState::Defeat || target.state == PlayerState::Defeat) return in;
    const AiProfile profile = ProfileFor(self.skin);
    const float dx = target.position.x - self.position.x;
    const float dy = target.position.y - self.position.y;
    const float dist = std::fabs(dx);
    const float dir = dx >= 0.0f ? 1.0f : -1.0f;
    decide -= dt * profile.aggression; comboGap -= dt; blockTimer -= dt; retreatTimer -= dt;

    // Bloquea a veces cuando el jugador ataca de cerca (mas los defensivos,
    // casi nunca los pesados/agresivos).
    if (blockTimer > 0.0f) { in.block = true; return in; }
    if (target.state == PlayerState::Attack && dist < 150.0f && decide <= 0.0f &&
        GetRandomValue(0, 99) < Scaled(30, profile.blockChance)) {
        blockTimer = 0.35f; decide = 0.4f; in.block = true; return in;
    }
    // Se aleja un momento despues de un combo (no queda pegado); los
    // defensivos/soporte se alejan mas tiempo, los agresivos casi nada.
    if (retreatTimer > 0.0f) { in.moveX = -dir; in.moveY = dy > 0 ? -0.3f : 0.3f; return in; }

    // Alinea el carril (profundidad) con el jugador.
    if (std::fabs(dy) > 14.0f) in.moveY = dy > 0 ? 1.0f : -1.0f;

    // Combo en curso: J J J K como el jugador.
    if (comboLeft > 0) {
        if (comboGap <= 0.0f) {
            if (comboLeft == 1) in.kick = true; else in.punch = true;
            --comboLeft; comboGap = 0.16f;
            if (comboLeft == 0) { retreatTimer = 0.35f * profile.retreatBias; decide = 0.5f; }
        }
        return in;
    }
    if (self.state == PlayerState::Attack) return in;   // deja terminar la animacion

    if (dist > profile.preferredRange) {
        in.moveX = dir;   // se acerca (a veces con dash)
        if (profile.dashChance > 0.0f && dist > 380.0f && decide <= 0.0f &&
            GetRandomValue(0, 99) < Scaled(25, profile.dashChance)) { in.dash = true; decide = 0.8f; }
        // A distancia: poder de energia o habilidad (los zoners/soporte abusan de esto).
        if (decide <= 0.0f && dist < 520.0f && GetRandomValue(0, 99) < Scaled(22, profile.skillChance)) {
            const int slot = GetRandomValue(0, 4);
            if (self.SkillReady(slot)) in.skill = slot; else in.energy = true;
            decide = 1.1f;
        }
        return in;
    }
    if (decide > 0.0f) return in;
    // Cerca: elige golpe, combo, habilidad, transformacion o cambio de pagina.
    const int roll = GetRandomValue(0, 99);
    if (roll < Scaled(8, profile.skillChance) && self.SkillReady(5) && !self.IsTransformed()) {
        in.skill = 5; decide = 1.2f;
    } else if (roll < Scaled(36, profile.skillChance)) {
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
