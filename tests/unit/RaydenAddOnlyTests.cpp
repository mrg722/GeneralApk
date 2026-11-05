// F1.3: RAYDEN (ORIGINAL), skin 0 - SOLO se le agrega. Sin ventana.
// - Los 19 clips historicos del juego `rayden` tienen los mismos cuadros y
//   duraciones que en la base (tabla literal de data/sprite_manifest.json en
//   base-fase1), tambien con los includes cargados.
// - Ningun clip de data/manifest/rayden_moves.json repite un nombre existente ni
//   un nombre que el motor busca por su cuenta (victory/transform/turn/run/jump);
//   todos se cargan (sin avisos) y usan celdas existentes del atlas (0-21).
// - GetMoveSet(0) no tiene override para los 9 ids historicos ni visual propio
//   para los estados historicos; >= 30 movimientos habilitados, cada nuevo con su
//   clip nuevo; habilidades 1-5 distintas de las cadenas y entre si.
#include "game/Player.h"
#include "game/moves/MoveRegistry.h"
#include "game/player/PoseFxEval.h"
#include "rendering/SpriteManifest.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace district_fury;

namespace {

struct Historic { const char* name; std::vector<int> frames; std::vector<float> durations; bool loop; };

// Copia literal de los 19 clips de rayden en base-fase1 ("duration" fija = una por cuadro).
const std::vector<Historic>& Base() {
    static const std::vector<Historic> k = {
        {"idle", {0, 1, 2, 3}, {.12f, .12f, .12f, .12f}, true},
        {"walk", {4, 5, 6, 7}, {.105f, .105f, .105f, .105f}, true},
        {"punch1", {16, 8}, {.06f, .24f}, false},
        {"punch2", {9}, {.3f}, false},
        {"punch3", {16, 11}, {.07f, .35f}, false},
        {"kick", {16, 10}, {.05f, .37f}, false},
        {"energy", {12}, {.55f}, false},
        {"dash_attack", {8, 9}, {.12f, .26f}, false},
        {"rage_attack", {10, 11}, {.14f, .41f}, false},
        {"finisher", {12, 11}, {.2f, .46f}, false},
        {"dash", {14}, {.08f}, false},
        {"block", {21}, {.1f}, true},
        {"hit_high", {13}, {.13f}, false},
        {"hit_low", {17}, {.13f}, false},
        {"airborne", {18, 19}, {.12f, .3f}, false},
        {"knockdown", {15}, {.1f}, false},
        {"getup", {20, 1}, {.2f, .12f}, false},
        {"defeat", {19, 15}, {.12f, .1f}, false},
        {"recovery", {2}, {.1f}, false},
    };
    return k;
}

bool Near(float a, float b) { return a - b < 1e-4f && b - a < 1e-4f; }

void CheckHistoricClips(const ClipMap& clips, bool allowExtra) {
    int historic = 0;
    for (const auto& kv : clips) if (!kv.second.fromInclude) ++historic;
    assert(historic == 19);
    assert(allowExtra || clips.size() == 19);
    for (const Historic& h : Base()) {
        const auto it = clips.find(h.name);
        assert(it != clips.end());
        const ClipDef& c = it->second;
        assert(!c.fromInclude && c.frames == h.frames && c.loop == h.loop);
        assert(c.durations.size() == h.durations.size());
        for (std::size_t i = 0; i < h.durations.size(); ++i) assert(Near(c.durations[i], h.durations[i]));
        assert(c.scales.empty() && c.offsets.empty());
    }
}

std::string ReadFile(const char* path) {
    std::string text;
    FILE* f = std::fopen(path, "rb");
    assert(f);
    char buf[4096]; std::size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
    std::fclose(f);
    return text;
}

void TestManifest() {
    SpriteManifest real;
    assert(real.LoadFromFile("data/sprite_manifest.json"));
    for (const std::string& w : real.Warnings()) std::fprintf(stderr, "aviso: %s\n", w.c_str());
    assert(real.Warnings().empty());
    SpriteManifest bare;
    assert(bare.LoadFromString(ReadFile("data/sprite_manifest.json"), false));
    CheckHistoricClips(*bare.FindClips("rayden"), false);
    CheckHistoricClips(*real.FindClips("rayden"), true);

    // El include por si solo: nombres nuevos, todos cargados, celdas 0-21.
    SpriteManifest inc;
    assert(inc.LoadFromString(ReadFile("data/manifest/rayden_moves.json"), false));
    const ClipMap* mine = inc.FindClips("rayden");
    assert(mine && mine->size() >= 25);
    static const char* kEngineNames[] = {"victory", "transform", "turn", "run", "jump", "special", "super"};
    for (const auto& kv : *mine) {
        assert(bare.FindClips("rayden")->count(kv.first) == 0);
        for (const char* n : kEngineNames) assert(kv.first != n);
        const ClipDef& loaded = real.FindClips("rayden")->at(kv.first);
        assert(loaded.fromInclude && loaded.frames == kv.second.frames);
        for (int f : kv.second.frames) assert(f >= 0 && f <= 21);
    }
}

void TestMoveSet() {
    const MoveSet& ms = GetMoveSet(0);
    assert(&ms != &GenericMoveSet());
    for (int id = 1; id <= static_cast<int>(AttackId::Skill); ++id) assert(FindOverride(ms, id) == nullptr);
    // Estados historicos sin visual propio (se dibujan como siempre).
    assert(!HasVisual(ms.hit) && !HasVisual(ms.knockdown) && !HasVisual(ms.getup) && !HasVisual(ms.walk) &&
           !HasVisual(ms.block));
    assert(ms.feel == FeelLevel::Full && ms.walkSpeedScale == 1.0f && ms.bodyScale == 1.0f);
    // Estados nuevos: salto y victoria con clips nuevos.
    assert(HasVisual(ms.jumpRise) && HasVisual(ms.jumpFall) && HasVisual(ms.victory));
    // Ningun otro skin recibe este MoveSet.
    for (int skin = 1; skin <= 41; ++skin) assert(&GetMoveSet(skin) != &ms);

    SpriteManifest real;
    assert(real.LoadFromFile("data/sprite_manifest.json"));
    const ClipMap& clips = *real.FindClips("rayden");
    Player p;
    p.ApplyCharacter(0);
    int enabled = 0;
    for (int id = 1; id < kAttackIdCount; ++id) {
        const AttackId a = static_cast<AttackId>(id);
        if (!p.MoveEnabled(a)) continue;
        ++enabled;
        if (id <= static_cast<int>(AttackId::Skill)) continue;
        const MoveOverride* ov = FindOverride(ms, id);
        assert(ov && ov->displayName && ov->visual.clipCount > 0);
        for (int c = 0; c < ov->visual.clipCount; ++c) {
            const auto it = clips.find(ov->visual.clips[c].clip);
            assert(it != clips.end() && it->second.fromInclude);   // clip NUEVO
        }
    }
    std::printf("RAYDEN movimientos habilitados: %d\n", enabled);
    assert(enabled >= 30);
    assert(p.MoveEnabled(AttackId::Signature1) && p.MoveEnabled(AttackId::Signature2) &&
           p.MoveEnabled(AttackId::Signature3));
    assert(!p.MoveEnabled(AttackId::Grab));   // Fase 2 sigue apagada

    // Habilidades 1-5: distintas entre si y de las cadenas J/K.
    std::set<int> chain(ms.chainJ, ms.chainJ + ms.chainJCount);
    chain.insert(ms.chainK, ms.chainK + ms.chainKCount);
    std::set<int> skills;
    for (int i = 0; i < 5; ++i) {
        assert(ms.skillSlots[i] != 0 && chain.count(ms.skillSlots[i]) == 0);
        assert(p.MoveEnabled(static_cast<AttackId>(ms.skillSlots[i])));
        skills.insert(ms.skillSlots[i]);
    }
    assert(skills.size() == 5 && ms.skillSlots[0] == static_cast<int>(AttackId::EnergyWave));
}

}  // namespace

int main() {
    TestManifest();
    TestMoveSet();
    std::printf("RaydenAddOnlyTests OK\n");
    return 0;
}
