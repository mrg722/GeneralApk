#pragma once
#include <algorithm>

namespace district_fury {

// Congelamiento de impacto. Mientras quede tiempo, el stage no avanza la
// simulacion (jugador, enemigos, proyectiles y animadores quedan quietos);
// solo se sigue leyendo la entrada (Player::PumpInput).
struct HitstopClock {
    static constexpr float kDefault = 0.05f;   // golpe ligero conectado

    float remaining = 0.0f;

    void Trigger(float seconds = kDefault) { remaining = std::max(remaining, seconds); }
    void Reset() { remaining = 0.0f; }
    bool Active() const { return remaining > 0.0f; }
    // true = este frame esta congelado (y descuenta dt).
    bool Consume(float dt) {
        if (remaining <= 0.0f) return false;
        remaining -= dt;
        return true;
    }
};

}  // namespace district_fury
