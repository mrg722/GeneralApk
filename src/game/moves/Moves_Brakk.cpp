// Moveset de BRAKK (skin 2).
// Stub creado en F0.1; lo rellena SOLO el bloque F1.5 (datos: MoveSet,
// MoveOverride, MoveVisual de src/game/moves/MoveTypes.h). Debe devolver nullptr
// para los skins que no son suyos: entonces GetMoveSet sigue preguntando y, si
// nadie responde, usa el MoveSet generico.
#include "game/moves/MoveRegistry.h"

namespace district_fury {

// ---- Ejemplo (F0.4) de como se llena este archivo; NO esta activo ----
// Contrato completo: src/game/moves/MoveTypes.h (MoveSet, MoveOverride,
// CancelRule) y tests/unit/MovesetTests.cpp (rutas que deben seguir valiendo).
// Ver un movimiento: ./build/moves_sheet /tmp/m <skin> --partes
//
// namespace {
// const MoveOverride kOverrides[] = {
//     // Cambiar un generico: mas dano, mas lento, su propio clip y PoseFx.
//     [] { MoveOverride o; o.id = static_cast<int>(AttackId::FwdPunch);
//          o.damageScale = 1.2f; o.timeScale = 1.1f;
//          o.visual.clips[0] = {"punch3"}; o.visual.clipCount = 1;
//          o.visual.fx.presets[o.visual.fx.presetCount++] = posefx::ANTICIPA(1.0f);   // PoseFxPresets.h
//          o.displayName = "PUNO DE AVANCE"; return o; }(),
//     // Apagar uno que no encaja con el personaje (cae al movimiento base).
//     [] { MoveOverride o; o.id = static_cast<int>(AttackId::SpinKick); o.enabled = false; return o; }(),
//     // Signature1 (QCB + J): SOLO existe con replaceDef y una fila completa.
//     [] { MoveOverride o; o.id = static_cast<int>(AttackId::Signature1); o.replaceDef = true;
//          o.def = GetAttack(AttackId::Hurricane); o.def.name = "MI FIRMA"; o.def.spCost = 20;
//          o.projectile.enabled = true; o.projectile.count = 3; o.projectile.interval = 0.08f;
//          o.visual.clips[0] = {"special"}; o.visual.clipCount = 1; return o; }(),
// };
// MoveSet Build() {
//     MoveSet m;
//     m.name = "mi personaje";
//     m.caps = CapAll & ~CapBackDash;            // lo que no tiene, cae al base
//     m.chainJ[0] = static_cast<int>(AttackId::Punch1);   // cadena J propia (hasta 6;
//     m.chainJ[1] = static_cast<int>(AttackId::Punch2);   //  Chain5/Chain6 con override)
//     m.chainJ[2] = static_cast<int>(AttackId::Punch3);
//     m.chainJCount = 3;                          // chainJ[0] == 0 = cadena por defecto
//     m.skillSlots[1] = static_cast<int>(AttackId::RisingUppercut);   // boton 2; 0 = por defecto
//     m.jumpVz = 560;                             // pesado: salto de 105 px
//     m.overrides = kOverrides; m.overrideCount = 3;
//     return m;
// }
// }  // namespace
// const MoveSet* MovesetFor_<Bloque>(int skin) {
//     static const MoveSet kSet = Build();
//     return skin == <mi skin> ? &kSet : nullptr;
// }
// ---- fin del ejemplo ----

const MoveSet* MovesetFor_Brakk(int skin) {
    (void)skin;
    return nullptr;
}

}  // namespace district_fury
