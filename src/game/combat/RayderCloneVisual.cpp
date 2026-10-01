#include "game/combat/RayderCloneVisual.h"
#include "game/lab/KfReference.h"
#include <cstring>

namespace district_fury {
namespace {
constexpr int kCloneRoster = 15;   // "RAYDER CLON" en el roster KF (KfReference.cpp)
// Altura en pantalla parecida a la del Rayder clon jugable (escala 0.5 x 1.2).
constexpr float kDrawScale = 0.66f;

const char* ClipFor(const char* attack, float t, bool moving, bool hurt, bool defeated) {
    auto is = [&](const char* s) { return attack && std::strcmp(attack, s) == 0; };
    if (defeated) return "defeat";
    if (hurt) return "hit";
    // Misma secuencia por fases que la del jefe (preparacion -> golpe -> vuelta).
    if (is("MirrorCombo")) return t < 0.22f ? "punch1" : t < 0.46f ? "punch2" : "punch3";
    if (is("DarkWave")) return "energy";
    if (is("Teleport")) return "dash";
    if (is("Dash")) return "dash_attack";
    if (is("Finisher")) return "rage_attack";
    if (attack) return "punch1";
    return moving ? "walk" : "idle";
}
}  // namespace

bool RayderCloneVisual::Ready() {
    if (!tried) {
        tried = true;
        const KfReference& k = GetKfCharacter(kCloneRoster);
        if (k.loaded && k.templ.texture.id != 0) {
            anim = k.templ;
            anim.PlayNamed("idle");
            ok = true;
        }
    }
    return ok;
}

void RayderCloneVisual::Update(float dt, const char* attack, float elapsed, bool moving, bool hurt, bool defeated) {
    if (!Ready()) return;
    const char* clip = ClipFor(attack, elapsed, moving, hurt, defeated);
    if (anim.currentClipName != clip) anim.PlayNamed(clip);
    anim.Update(dt);
}

void RayderCloneVisual::Draw(Vector2 feet, bool facingRight, Color tint) const {
    if (!ok) return;
    anim.Draw(feet, kDrawScale, !facingRight, tint);
}

}  // namespace district_fury
