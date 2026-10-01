#pragma once
#include "rendering/Animator.h"
#include "raylib.h"
#include <string>

namespace district_fury {

// Dibujo del jefe Rayder clon con su diseno final (rojo, sin mangas, boceto):
// el personaje KF "RAYDER CLON" (formula del heroe KF + despiece propio). Lo
// usan el jefe del Modo VS y el del Stage 5; la logica de ataques de cada uno
// no cambia, solo el dibujo. Si no carga (sin apk_reference), Ready() es false
// y cada jefe sigue con sus poses sueltas de assets/bosses/rayder_clone.
class RayderCloneVisual {
public:
    bool Ready();
    bool IsReady() const { return ok; }   // sin cargar (para dibujar)
    // attack: nombre del ataque en curso (MirrorCombo, DarkWave, Teleport, Dash,
    // Finisher) o nullptr; elapsed: segundos dentro de ese ataque.
    void Update(float dt, const char* attack, float elapsed, bool moving, bool hurt, bool defeated);
    void Draw(Vector2 feet, bool facingRight, Color tint) const;

private:
    Animator anim;
    bool tried{false};
    bool ok{false};
};

}  // namespace district_fury
