#pragma once
#include "raylib.h"

// Controles tactiles (celular). Todo en coordenadas virtuales 1280x720: main
// informa la transformacion pantalla -> virtual (letterbox). Cada boton marca
// una tecla virtual en core/InputMap, asi el juego no distingue tactil de teclado.
namespace district_fury {
class Player;
namespace touch {

enum class Context {
    Combat,     // joystick + GOLPE/PATADA/ONDA/GANCHO/BLOQUEO/DASH/FURIA + PAUSA
    Menu,       // cruceta + OK + ATRAS
    EndScreen,  // cruceta + OK + REINTENTAR + MENU (derrota / etapa superada)
    Reward,     // 1 / 2 / 3 (eleccion de mejora)
};

void SetEnabled(bool enabled);
bool Enabled();
// scale = pixeles de pantalla por pixel virtual; offset = esquina del area de juego.
void SetScreenTransform(float scale, float offsetX, float offsetY);
// Lee los toques, fija las teclas virtuales y hace input::Commit(). Llamar una
// vez por frame antes de actualizar el juego (tambien si esta desactivado).
void Update(Context context);
// Dibuja los controles (dentro del area virtual). `player` opcional: atenua ONDA
// sin energia y hace brillar FURIA cuando esta lista.
void Draw(Context context, const Player* player);

// Para tests: evalua un toque en coordenadas virtuales como si fuera real.
void InjectTouchForTest(Vector2 virtualPoint);

}  // namespace touch
}  // namespace district_fury
