// Estado SOLO de dibujo del jugador (contrato F0.1). Vive como miembro
// `mutable PlayerVisualState visual;` de Player para que Draw() (const) pueda
// actualizarlo: estelas con textura, fase visual del movimiento, etc.
// Nunca afecta la jugabilidad. Vacio por ahora: lo llena F0.3.
#pragma once

namespace district_fury {

struct PlayerVisualState {
};

}  // namespace district_fury
