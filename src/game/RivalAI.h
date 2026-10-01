#pragma once
#include "game/Player.h"

namespace district_fury {

// Rival controlado por la maquina en el Modo VS: un Player completo (mismos
// movimientos, habilidades por paginas, transformacion y bloqueo que el
// jugador) manejado con entradas simuladas, como un bot de pelea.
class RivalAI {
public:
    void Reset() { *this = RivalAI{}; }
    PlayerInput Think(const Player& self, const Player& target, float dt);

private:
    float decide{0.6f};     // tiempo hasta la proxima decision
    float blockTimer{0.0f};
    float retreatTimer{0.0f};
    int comboLeft{0};
    float comboGap{0.0f};
    int pageFlips{0};
};

}  // namespace district_fury
