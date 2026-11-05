// F1.2: poses del CLON ANTIGUO (skin 1) y de GRINDER (skin 3). Sin ventana.
// - toda pose referenciada (tabla de recortes, segmentos del MoveSet) existe como
//   PNG en assets/bosses/<carpeta>/;
// - ningun `source rect` sale de su textura (tamano leido de la cabecera PNG);
// - las 3 primeras secuencias de la cadena J del clon dan poses o
//   transformaciones distintas entre si;
// - cada movimiento habilitado tiene visual propio (segmentos en S, A y R) sin
//   `idle` en el activo, y el conteo llega a la meta (>= 28 / >= 22);
// - habilidades 1-5 distintas entre si y de las cadenas; Grinder no salta.
#include "game/Player.h"
#include "game/moves/MoveRegistry.h"
#include "game/player/PoseFxEval.h"
#include "game/visual/PoseTables.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace district_fury {
int CloneGrinderPieces(const char* folder, const char* pose, bool mirrored, PosePiece out[2]);
bool CloneGrinderCutInfo(int index, const char*& folder, const char*& pose, Rectangle& a, Rectangle& b);
}  // namespace district_fury

using namespace district_fury;

namespace {

// Ancho/alto de un PNG (cabecera IHDR); false si no existe o no es PNG.
bool PngSize(const std::string& path, int& w, int& h) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    unsigned char b[24];
    const bool ok = std::fread(b, 1, 24, f) == 24 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G';
    std::fclose(f);
    if (!ok) return false;
    w = (b[16] << 24) | (b[17] << 16) | (b[18] << 8) | b[19];
    h = (b[20] << 24) | (b[21] << 16) | (b[22] << 8) | b[23];
    return true;
}

void CheckPiece(const char* folder, const PosePiece& p) {
    assert(p.pose != nullptr);
    int w = 0, h = 0;
    const std::string path = std::string("assets/bosses/") + folder + "/" + p.pose + ".png";
    if (!PngSize(path, w, h)) { std::fprintf(stderr, "falta %s\n", path.c_str()); assert(false); }
    if (p.src.width > 0) {
        assert(p.src.x >= 0 && p.src.y >= 0 && p.src.height > 0);
        assert(p.src.x + p.src.width <= w && p.src.y + p.src.height <= h);
    }
}

void TestCutTable() {
    const char* folder; const char* pose; Rectangle a, b;
    int n = 0;
    for (int i = 0; CloneGrinderCutInfo(i, folder, pose, a, b); ++i, ++n) {
        int w = 0, h = 0;
        const std::string path = std::string("assets/bosses/") + folder + "/" + pose + ".png";
        assert(PngSize(path, w, h));
        for (const Rectangle& r : {a, b}) {
            if (r.width <= 0) continue;
            assert(r.x >= 0 && r.y >= 0 && r.x + r.width <= w && r.y + r.height <= h);
        }
        // Las piezas devueltas comparten el ancla: el punto de los pies cae en el
        // mismo pixel de textura en las dos.
        PosePiece pc[2];
        const int k = CloneGrinderPieces(folder, pose, false, pc);
        assert(k == (b.width > 0 ? 2 : 1));
        if (k == 2) {
            const float ax0 = pc[0].src.x + pc[0].src.width * 0.5f - pc[0].offset.x;
            const float ax1 = pc[1].src.x + pc[1].src.width * 0.5f - pc[1].offset.x;
            const float ay0 = pc[0].src.y + pc[0].src.height - pc[0].offset.y;
            const float ay1 = pc[1].src.y + pc[1].src.height - pc[1].offset.y;
            assert(std::fabs(ax0 - ax1) < 1e-3f && std::fabs(ay0 - ay1) < 1e-3f);
        }
    }
    assert(n == 11 + 7);   // todas las poses de las dos carpetas
    // Restos de recorte fuera: la pierna suelta de kick.png (x < 88 abajo) y el
    // trozo de bota del borde derecho de hurt.png / ready.png.
    PosePiece pc[2];
    assert(CloneGrinderPieces("rayder_clone", "kick", false, pc) == 2 && pc[0].src.x >= 86 && pc[1].src.x >= 88);
    assert(CloneGrinderPieces("rayder_clone", "hurt", false, pc) == 2 && pc[1].src.x + pc[1].src.width <= 155);
    assert(CloneGrinderPieces("rayder_clone", "ready", false, pc) == 2 && pc[1].src.x + pc[1].src.width <= 163);
}

// Pose y transformacion de un movimiento en (fase, v).
struct Look { std::string pose; FxXform x; };
Look LookOf(const MoveSet& set, AttackId id, char ph, float v) {
    const MoveOverride* ov = FindOverride(set, static_cast<int>(id));
    assert(ov != nullptr && ov->visual.segCount > 0);
    Look l;
    for (int i = 0; i < ov->visual.segCount; ++i) {
        const PoseSegment& s = ov->visual.segs[i];
        if (s.phase == ph && v >= s.v0 && v <= s.v1) { l.pose = s.pieces[0].pose; break; }
    }
    FxClock c; c.phase = ph; c.v = v; c.t = v * 0.1f; c.u = (ph == 'S' ? 0 : ph == 'A' ? 1 : 2) / 3.0f + v / 3.0f;
    c.total = c.u * 0.4f; c.dir = 1;
    l.x = EvalPoseFx(ov->visual.fx, c);
    return l;
}
bool Different(const Look& a, const Look& b) {
    if (a.pose != b.pose) return true;
    return std::fabs(a.x.sx - b.x.sx) > 0.01f || std::fabs(a.x.sy - b.x.sy) > 0.01f ||
           std::fabs(a.x.angle - b.x.angle) > 1.0f || std::fabs(a.x.dx - b.x.dx) > 1.0f ||
           std::fabs(a.x.dy - b.x.dy) > 1.0f || a.x.flip != b.x.flip;
}

void TestCloneChainDistinct() {
    const MoveSet& set = GetMoveSet(1);
    const AttackId ids[3] = {static_cast<AttackId>(set.chainJ[0]), static_cast<AttackId>(set.chainJ[1]),
                             static_cast<AttackId>(set.chainJ[2])};
    assert(ids[0] == AttackId::Punch1 && ids[1] == AttackId::Punch2 && ids[2] == AttackId::PunchHook);
    for (int i = 0; i < 3; ++i)
        for (int j = i + 1; j < 3; ++j)
            assert(Different(LookOf(set, ids[i], 'A', 0.5f), LookOf(set, ids[j], 'A', 0.5f)));
    // El 4o (puno volador) usa otra pose (dash) y avanza de verdad.
    assert(LookOf(set, AttackId::Punch3, 'A', 0.5f).pose == "dash");
    assert(FindOverride(set, static_cast<int>(AttackId::Punch3))->root.dx >= 80);
}

int CountAndCheck(int skin, const char* folder) {
    const MoveSet& set = GetMoveSet(skin);
    assert(set.overrideCount > 0);
    for (int i = 0; i < set.overrideCount; ++i) {
        const MoveVisual& mv = set.overrides[i].visual;
        for (int s = 0; s < mv.segCount; ++s)
            for (int k = 0; k < mv.segs[s].pieceCount; ++k) CheckPiece(folder, mv.segs[s].pieces[k]);
    }
    Player p;
    p.ApplyCharacter(skin);
    int enabled = 0;
    for (int m = 1; m < kAttackIdCount; ++m) {
        const AttackId id = static_cast<AttackId>(m);
        if (id == AttackId::Skill || !p.MoveEnabled(id)) continue;
        ++enabled;
        const MoveOverride* ov = FindOverride(set, m);
        if (ov == nullptr || ov->visual.segCount == 0) {
            std::fprintf(stderr, "skin %d: %s sin visual propio\n", skin, AttackIdName(id));
            assert(false);
        }
        bool s = false, a = false, r = false;
        for (int k = 0; k < ov->visual.segCount; ++k) {
            const PoseSegment& g = ov->visual.segs[k];
            s = s || g.phase == 'S'; a = a || g.phase == 'A'; r = r || g.phase == 'R';
            if (g.phase == 'A') assert(std::strncmp(g.pieces[0].pose, "idle", 4) != 0);
        }
        assert(s && a && r);
        assert(ov->displayName != nullptr);
    }
    // Habilidades 1-5: distintas entre si, habilitadas y fuera de las cadenas.
    for (int i = 0; i < 5; ++i) {
        assert(p.MoveEnabled(static_cast<AttackId>(set.skillSlots[i])));
        for (int j = i + 1; j < 5; ++j) assert(set.skillSlots[i] != set.skillSlots[j]);
        for (int c = 0; c < set.chainJCount; ++c) assert(set.skillSlots[i] != set.chainJ[c]);
        for (int c = 0; c < set.chainKCount; ++c) assert(set.skillSlots[i] != set.chainK[c]);
    }
    return enabled;
}

void TestStatesTable() {
    // Caminar alterna poses al ritmo de walkPhase (clon); el derribo pasa de
    // `hurt` en el aire a `death` en el suelo; levantarse no es `death`.
    PoseContext c; c.state = PlayerState::Walk;
    PoseFrame f;
    std::string seen[4];
    for (int i = 0; i < 4; ++i) {
        c.walkPhase = (i + 0.5f) * 2.0f * 3.14159265f / 4.0f;
        assert(ResolvePoseFrame(1, c, f));
        seen[i] = f.pieces[0].pose;
    }
    assert(seen[0] == "idle1" && seen[1] == "idle3" && seen[2] == "idle1" && seen[3] == "idle4");
    for (int skin : {1, 3}) {
        PoseContext k; k.state = PlayerState::Airborne; k.z = 60;
        assert(ResolvePoseFrame(skin, k, f) && std::strcmp(f.pieces[0].pose, "hurt") == 0 && f.fx.presetCount == 1);
        k.z = 0; k.state = PlayerState::Knockdown;
        assert(ResolvePoseFrame(skin, k, f) && std::strcmp(f.pieces[0].pose, "death") == 0);
        k.state = PlayerState::Recovery; k.stateTime = 0.05f;
        assert(ResolvePoseFrame(skin, k, f) && std::strcmp(f.pieces[0].pose, "death") != 0);
    }
}

}  // namespace

int main() {
    TestCutTable();
    TestCloneChainDistinct();
    const int clone = CountAndCheck(1, "rayder_clone");
    const int grinder = CountAndCheck(3, "grinder");
    std::printf("clon: %d movimientos, grinder: %d movimientos\n", clone, grinder);
    assert(clone >= 28 && grinder >= 22);
    Player g;
    g.ApplyCharacter(3);
    assert(!g.MoveEnabled(AttackId::AirPunch) && !g.MoveEnabled(AttackId::DiveKick));
    assert(!(GetMoveSet(3).caps & CapJump));
    TestStatesTable();
    std::puts("PoseCloneGrinderTests OK");
    return 0;
}
