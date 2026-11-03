// Evaluador PoseFx (ver PoseFxEval.h).
#include "game/player/PoseFxEval.h"
#include "game/Player.h"
#include <unordered_map>

namespace district_fury {
namespace {

int PhaseIndex(char ph) { return ph == 'S' ? 0 : ph == 'R' ? 2 : 1; }

std::unordered_map<int, const MoveSet*>& TestSets() {
    static std::unordered_map<int, const MoveSet*> sets;
    return sets;
}

}  // namespace

FxClock AttackClock(float elapsed, float startup, float active, float recovery, float dir) {
    FxClock c;
    c.dir = dir;
    c.total = std::max(0.0f, elapsed);
    startup = std::max(0.0f, startup); active = std::max(0.0f, active); recovery = std::max(0.0f, recovery);
    const float whole = startup + active + recovery;
    c.u = whole > 0 ? posefx::Clamp01(c.total / whole) : 1.0f;
    if (c.total < startup) {
        c.phase = 'S'; c.t = c.total; c.v = startup > 0 ? c.total / startup : 1.0f;
    } else if (c.total < startup + active) {
        c.phase = 'A'; c.t = c.total - startup; c.v = active > 0 ? c.t / active : 1.0f;
    } else {
        c.phase = 'R'; c.t = c.total - startup - active; c.v = recovery > 0 ? posefx::Clamp01(c.t / recovery) : 1.0f;
    }
    c.v = posefx::Clamp01(c.v);
    return c;
}

FxClock StateClock(float stateTime, float duration, bool loop, float dir) {
    FxClock c;
    c.dir = dir; c.phase = 'A';
    c.total = c.t = std::max(0.0f, stateTime);
    if (duration > 0) {
        const float q = c.total / duration;
        c.v = loop ? q - std::floor(q) : posefx::Clamp01(q);
    } else {
        c.v = 0;
    }
    c.u = c.v;
    return c;
}

FxXform EvalPoseFx(const PoseFx& fx, const FxClock& c, Ease keyEase) {
    FxXform acc;
    const int np = std::clamp(fx.presetCount, 0, 6);
    for (int i = 0; i < np; ++i) {
        const PresetUse& p = fx.presets[i];
        if (p.preset == FxPreset::None) continue;
        if (p.phase == '*') Accumulate(acc, posefx::Eval(p, c.u, c.total, c.dir));
        else if (p.phase == c.phase) Accumulate(acc, posefx::Eval(p, c.v, c.t, c.dir));
    }
    const int nk = std::clamp(fx.keyCount, 0, 8);
    if (nk > 0) {
        // Posicion global g = fase (0 S, 1 A, 2 R) + v; neutro implicito en 0 y 3.
        struct K { float g; FxXform x; };
        K ks[10]; int n = 0;
        ks[n++] = {0.0f, FxXform{}};
        for (int i = 0; i < nk; ++i) {
            const FxKey& k = fx.keys[i];
            FxXform x; x.sx = k.sx; x.sy = k.sy; x.angle = k.angle * c.dir; x.dx = k.dx * c.dir; x.dy = k.dy;
            K item{PhaseIndex(k.phase) + posefx::Clamp01(k.v), x};
            int j = n;   // insercion ordenada (estable)
            while (j > 0 && ks[j - 1].g > item.g) { ks[j] = ks[j - 1]; --j; }
            ks[j] = item; ++n;
        }
        ks[n++] = {3.0f, FxXform{}};
        const float g = PhaseIndex(c.phase) + c.v;
        int seg = 0;
        while (seg < n - 2 && g > ks[seg + 1].g) ++seg;
        const K& a = ks[seg]; const K& b = ks[seg + 1];
        const float span = b.g - a.g;
        const float e = span > 1e-5f ? posefx::ApplyEase(keyEase, (g - a.g) / span) : 1.0f;
        FxXform x;
        x.sx = a.x.sx + (b.x.sx - a.x.sx) * e; x.sy = a.x.sy + (b.x.sy - a.x.sy) * e;
        x.angle = a.x.angle + (b.x.angle - a.x.angle) * e;
        x.dx = a.x.dx + (b.x.dx - a.x.dx) * e; x.dy = a.x.dy + (b.x.dy - a.x.dy) * e;
        Accumulate(acc, x);
    }
    return acc;
}

FxXform EvalRootHop(const RootCurve& r, float u) {
    FxXform x;
    if (r.dz == 0 || r.t1 <= r.t0 || u < r.t0 || u > r.t1) return x;
    const float w = (u - r.t0) / (r.t1 - r.t0);
    x.dy = -r.dz * std::sin(posefx::kPi * w);
    return x;
}

int OverlayFrame(const OverlayFx& o, const FxClock& c) {
    if (o.first < 0 || o.count <= 0) return -1;
    float v;
    if (o.phase == '*') v = c.u;
    else if (o.phase == c.phase) v = c.v;
    else return -1;
    const int k = std::min(o.count - 1, static_cast<int>(v * o.count));
    return o.first + std::max(0, k);
}

int EffectiveGhostCount(const PoseFx& fx) {
    if (fx.ghostCount > 0) return fx.ghostCount;
    for (int i = 0; i < std::clamp(fx.presetCount, 0, 6); ++i)
        if (fx.presets[i].preset == FxPreset::Giro || fx.presets[i].preset == FxPreset::Barrida) return 3;
    return 0;
}

bool HasFx(const PoseFx& fx) {
    return fx.presetCount > 0 || fx.keyCount > 0 || fx.ghostCount > 0 || fx.overlayCount > 0;
}

bool HasVisual(const MoveVisual& v) { return v.clipCount > 0 || v.segCount > 0 || HasFx(v.fx); }

bool PickAtlasFrame(const MoveVisual& mv, float u, float seconds, bool useSeconds, const Animator& primary,
                    const Animator* secondary, AtlasPick& out) {
    struct Seg { const Animator* anim; const AnimationClip* clip; int from, to; float speed; bool reverse; float secs; bool sec; };
    Seg segs[3]; int n = 0; float natural = 0;
    auto find = [&](const char* name, const Animator*& who, bool& sec) -> const AnimationClip* {
        if (name == nullptr) return nullptr;
        auto it = primary.namedClips.find(name);
        if (it != primary.namedClips.end()) { who = &primary; sec = false; return &it->second; }
        return nullptr;
    };
    for (int i = 0; i < std::clamp(mv.clipCount, 0, 3); ++i) {
        const ClipSeg& cs = mv.clips[i];
        const Animator* who = nullptr; bool sec = false;
        const AnimationClip* clip = find(cs.clip, who, sec);
        if (!clip) clip = find(mv.clipFallback, who, sec);
        if (!clip && secondary) {
            auto it = secondary->namedClips.find(cs.clip ? cs.clip : "");
            if (it != secondary->namedClips.end()) { who = secondary; sec = true; clip = &it->second; }
        }
        if (!clip || clip->frames.empty()) continue;
        const int last = static_cast<int>(clip->frames.size()) - 1;
        const int from = cs.from < 0 ? 0 : std::min(cs.from, last);
        const int to = cs.to < 0 ? last : std::clamp(cs.to, from, last);
        float secs = 0;
        for (int k = from; k <= to; ++k)
            secs += std::max(0.016f, clip->durations.empty() ? clip->frameDuration
                                                              : clip->durations[std::min<std::size_t>(k, clip->durations.size() - 1)]);
        secs /= std::max(0.05f, cs.speed);
        segs[n++] = {who, clip, from, to, cs.speed, cs.reverse, secs, sec};
        natural += secs;
    }
    if (n == 0 || natural <= 0) return false;
    float tau;
    if (useSeconds) {
        const float total = mv.duration > 0 ? mv.duration : natural;
        float t = seconds;
        if (mv.loop) t = std::fmod(std::max(0.0f, t), total);
        tau = std::clamp(t / total, 0.0f, 1.0f) * natural;
    } else {
        tau = posefx::Clamp01(u) * natural;
    }
    int s = 0;
    while (s < n - 1 && tau >= segs[s].secs) { tau -= segs[s].secs; ++s; }
    const Seg& sg = segs[s];
    const float speed = std::max(0.05f, sg.speed);
    int step = sg.from;
    float acc = 0;
    const int count = sg.to - sg.from + 1;
    for (int i = 0; i < count; ++i) {
        const int k = sg.reverse ? sg.to - i : sg.from + i;
        const float d = std::max(0.016f, sg.clip->durations.empty() ? sg.clip->frameDuration
                                                                     : sg.clip->durations[std::min<std::size_t>(k, sg.clip->durations.size() - 1)]) / speed;
        step = k;
        if (tau < acc + d) break;
        acc += d;
    }
    out.anim = sg.anim;
    out.clip = sg.clip;
    out.step = step;
    out.frame = sg.clip->frames[static_cast<std::size_t>(step)];
    out.fromSecondary = sg.sec;
    return true;
}

const MoveSet& VisualMoveSet(int skin) {
    const auto& sets = TestSets();
    if (!sets.empty()) {
        const auto it = sets.find(skin);
        if (it != sets.end() && it->second) return *it->second;
    }
    return GetMoveSet(skin);
}

void SetVisualMoveSetForTests(int skin, const MoveSet* set) {
    if (set) TestSets()[skin] = set;
    else TestSets().erase(skin);
}

FeelLevel EffectiveFeel(const MoveSet& set, bool clipCharacter) {
    if (clipCharacter) return set.feel == FeelLevel::Light ? FeelLevel::Light : FeelLevel::None;
    return set.feel;
}

const MoveVisual* StateVisual(const MoveSet& set, PlayerState state, float velocityZ) {
    const MoveVisual* v = nullptr;
    switch (state) {
        case PlayerState::Hit: case PlayerState::GuardBreak: v = &set.hit; break;
        case PlayerState::Knockdown: case PlayerState::Airborne: v = &set.knockdown; break;
        case PlayerState::Victory: v = &set.victory; break;
        case PlayerState::Jump: v = velocityZ > 0 ? &set.jumpRise : &set.jumpFall; break;
        case PlayerState::Walk: v = &set.walk; break;
        case PlayerState::Block: v = &set.block; break;
        default: break;
    }
    return v && HasVisual(*v) ? v : nullptr;
}

}  // namespace district_fury
