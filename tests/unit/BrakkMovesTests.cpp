// F1.5: BRAKK (skin 2) - bruto de cadena. Sin ventana.
// - Todos los clips de brakk_v2 (base + include) usan indices dentro de la
//   rejilla 12x9 (0..107) y ningun clip de ataque usa el cuadro 88 (solo el
//   arco de cadena, el cuerpo desaparece) ni los recortes vacios conocidos.
// - rage_attack queda arreglado: 3 cuadros, sin el 88.
// - hit_high/hit_low ya no son de 1 cuadro.
// - El MoveSet: cadena J de 5 (remate Chain5 con override completo), cadena K
//   de 3, sin CapJump/CapAir, habilidades 1-5 distintas entre si y con
//   movimientos fuera de las cadenas, y Hurricane/DrillKick apagados.
#include "game/Player.h"
#include "game/moves/MoveRegistry.h"
#include "rendering/SpriteManifest.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <set>
#include <string>

using namespace district_fury;

namespace {

constexpr int kCells = 12 * 9;

void TestManifest() {
    SpriteManifest m;
    assert(m.LoadFromFile("data/sprite_manifest.json"));
    for (const std::string& w : m.Warnings()) std::fprintf(stderr, "aviso: %s\n", w.c_str());
    assert(m.Warnings().empty());
    const ClipMap* clips = m.FindClips("brakk_v2");
    assert(clips != nullptr);
    // Cuadros rotos/vacios que ningun clip debe usar (solo la cadena o un
    // recorte suelto, sin el cuerpo).
    const std::set<int> kBroken = {7, 12, 15, 20, 21, 22, 28, 32, 40, 45, 47,
                                   50, 52, 54, 69, 81, 83, 84, 87, 88, 102};
    for (const auto& kv : *clips) {
        assert(!kv.second.frames.empty());
        assert(kv.second.frames.size() == kv.second.durations.size());
        for (int f : kv.second.frames) {
            assert(f >= 0 && f < kCells);
            // Los clips base del jefe no son nuestros (explosive usa el 88,
            // defeat el 102): el check de celdas rotas aplica a los de F1.5.
            if (!kv.second.fromInclude) continue;
            assert(kBroken.count(f) == 0);
        }
    }
    // rage_attack arreglado: sin el cuadro 88 y con cuerpo completo.
    const ClipDef& rage = clips->at("rage_attack");
    assert(std::find(rage.frames.begin(), rage.frames.end(), 88) == rage.frames.end());
    assert(rage.frames.size() >= 3);
    // hit_high / hit_low: ya no de 1 cuadro; hit_low = hit invertido.
    const ClipDef& hi = clips->at("hit_high");
    const ClipDef& lo = clips->at("hit_low");
    assert(hi.frames.size() >= 2 && lo.frames.size() >= 2);
    assert(hi.frames.front() == lo.frames.back() && hi.frames.back() == lo.frames.front());
}

void TestMoveSet() {
    const MoveSet* ms = MovesetFor_Brakk(2);
    assert(ms != nullptr);
    assert(MovesetFor_Brakk(0) == nullptr && MovesetFor_Brakk(6) == nullptr);
    // Bruto sin salto.
    assert((ms->caps & CapJump) == 0u && (ms->caps & CapAir) == 0u);
    // Cadena J de 5 con remate Chain5; cadena K de 3.
    assert(ms->chainJCount == 5);
    assert(ms->chainJ[0] == static_cast<int>(AttackId::Punch1));
    assert(ms->chainJ[4] == static_cast<int>(AttackId::Chain5));
    assert(ms->chainKCount == 3);
    assert(ms->chainK[0] == static_cast<int>(AttackId::Kick));
    const MoveOverride* c5 = FindOverride(*ms, static_cast<int>(AttackId::Chain5));
    assert(c5 && c5->enabled && c5->replaceDef);   // Chain5 solo existe completo

    Player p;
    p.ApplyCharacter(2);
    // Sin salto ni aereos; Hurricane y DrillKick apagados (los reemplazan las
    // Signatures); las 3 Signatures y el resto del kit encendidos.
    assert(!p.MoveEnabled(AttackId::AirPunch) && !p.MoveEnabled(AttackId::DiveKick));
    assert(!p.MoveEnabled(AttackId::Hurricane) && !p.MoveEnabled(AttackId::DrillKick));
    assert(p.MoveEnabled(AttackId::Signature1) && p.MoveEnabled(AttackId::Signature2) &&
           p.MoveEnabled(AttackId::Signature3));
    assert(p.MoveEnabled(AttackId::Chain5));
    assert(!p.MoveEnabled(AttackId::Grab));   // Fase 2 sigue apagada
    // >= 24 movimientos habilitados (sin Skill).
    int enabled = 0;
    for (int i = 1; i < static_cast<int>(AttackId::Count); ++i) {
        const AttackId a = static_cast<AttackId>(i);
        if (a == AttackId::Skill) continue;
        if (p.MoveEnabled(a)) ++enabled;
    }
    assert(enabled >= 24);
    // Habilidades 1-5: definidas, distintas entre si y ninguna repite un
    // eslabon de las cadenas.
    std::set<int> slots;
    for (int i = 0; i < 5; ++i) {
        const int s = ms->skillSlots[i];
        assert(s != 0);
        assert(slots.insert(s).second);
        for (int j = 0; j < ms->chainJCount; ++j) assert(s != ms->chainJ[j]);
        for (int j = 0; j < ms->chainKCount; ++j) assert(s != ms->chainK[j]);
        assert(p.MoveEnabled(static_cast<AttackId>(s)));
    }
}

}  // namespace

int main() {
    TestManifest();
    TestMoveSet();
    std::puts("brakk_moves_tests OK");
    return 0;
}
