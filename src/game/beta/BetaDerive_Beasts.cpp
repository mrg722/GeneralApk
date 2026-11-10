// Derivaciones en memoria de las bestias BETA jugables (skins 27-37: esqueleto,
// excavadora, momia, elefante, ave, centauros, brutos, medusas). F1.8.
//
// Los .txt de GowApk/data/characters (y sus PNG) NO se tocan: aqui solo se
// edita la COPIA en memoria que KfReference::Load entrega a District Fury (el
// modo BETA apaga esta derivacion).
//
// Problema de origen: cada bestia trae UN ataque largo (17-36 cuadros, 1.1-2.4 s)
// repetido en punch1 = punch2 = punch3 = kick = dash_attack = ... y con 1-2 cuadros
// con caja de ataque; recovery = hit (dolor tras cada golpe propio), air = 1
// cuadro, bloqueo = idle.
//
// Derivacion:
//  - El ataque se parte en subrangos (A, B, C, D) y se recombina (reversa,
//    velocidad, avance visual) en clips con el NOMBRE CANONICO que el motor busca
//    por movimiento (punch1, punch2, punch_hook, punch3, kick, kick2, kick3,
//    dash_attack, dash_kick, fwd_punch, fwd_kick, spin_kick, sweep, signature1-3).
//    Cada subrango lleva su propia caja de ataque: la original si la trae; si no,
//    un cuadro copia con la caja del cuadro con caja mas cercano del ataque.
//  - "Avance" (dx): los ataques por clip no mueven al cuerpo (root motion apagado
//    en el motor para KF/BETA), asi que el avance se hace en los cuadros: las
//    piezas y las cajas del cuadro se desplazan con una campana sin(pi t) y vuelven.
//  - recovery = ultimo cuadro del ataque (0.08 s), air/airborne = primeros cuadros
//    del derribo, block = cuadro de guardia del ataque (la momia: el escudo
//    delante), centauros sin el circulo magico (piezas de la rejilla tex 0).
//  - Habilidades 1-5 (abilityClips/abilityNames): L, firma 1, firma 2, >J y furia.
#include "game/beta/BetaDerive.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace district_fury {
namespace {

struct Mod { float dx = 0; bool both = false; bool low = false; float widen = 1; };
struct Part { const char* src; int from, to; bool rev = false; float speed = 1; Mod m{}; };

struct Fam {
    const char* id;
    Part A, B, C, D;
    int blockStep;          // paso del ataque cuyo cuadro es la guardia
    int special;            // 0 = conserva "special"; 1 = A+B; 2 = copia de "energy"
    int superSrc;           // 0 = ataque largo original; 2 = "energy"
    bool circle;            // quitar el circulo magico (centauros)
    const char* names[5];   // habilidades 1-5
};

const Fam kFams[] = {
    {"esqueleto", {"punch1", 0, 6}, {"punch1", 6, 11}, {"punch1", 11, 16}, {"punch1", 3, 10, true}, 0, 0, 0, false,
     {"TAJO LARGO", "TRES TAJOS", "ABANICO DE HUESOS", "ESTOCADA", "FURIA DE HUESOS"}},
    {"excavador", {"punch1", 0, 11}, {"punch1", 11, 19}, {"punch1", 19, 23}, {"punch1", 6, 17, true}, 1, 0, 0, false,
     {"GUADANA", "TORBELLINO", "DOBLE EMERSION", "ZARPAZO", "FURIA DE TIERRA"}},
    {"momia_escudo", {"punch1", 0, 6}, {"punch1", 6, 11}, {"punch1", 11, 16}, {"punch1", 3, 10, true}, 1, 0, 0, false,
     {"ESCUDAZO", "MURO DE ESCUDO", "VENDAS", "EMBESTIDA", "FURIA DE ARENA"}},
    {"bestia_elefante", {"punch1", 0, 16}, {"punch1", 16, 28}, {"punch1", 28, 35}, {"punch1", 6, 16}, 1, 1, 0, false,
     {"ESTAMPIDA", "PISOTON", "DOBLE PISOTON", "EMBESTIDA", "FURIA SALVAJE"}},
    {"ave_sanadora", {"punch1", 0, 8}, {"punch1", 9, 16, false, 1.0f, Mod{14}}, {"punch1", 17, 24, true}, {"punch1", 25, 32, false, 1.15f, Mod{26}}, 0, 1, 0, false,
     {"CICLON", "PICADA", "TORNADO DE PLUMAS", "PICOTAZO", "FURIA SANADORA"}},
    {"centauro", {"punch1", 0, 9}, {"punch1", 10, 17}, {"punch1", 18, 24}, {"punch1", 3, 9}, 0, 0, 0, true,
     {"LANZAZO", "LLUVIA DE LANZAS", "PISOTON MAGICO", "CARGA DE LANZA", "FURIA DE GALOPE"}},
    {"centauro_rojo", {"punch1", 0, 9}, {"punch1", 10, 17}, {"punch1", 18, 24}, {"punch1", 3, 9}, 0, 0, 0, true,
     {"LANZAZO", "LLUVIA DE LANZAS", "PISOTON MAGICO", "CARGA DE LANZA", "FURIA DE GALOPE"}},
    {"bruto_cadena", {"punch1", 4, 17}, {"punch1", 10, 18}, {"punch1", 17, 23}, {"punch1", 8, 17, false, 1.2f}, 0, 2, 2, false,
     {"BOLAZO", "REMOLINO DE BOLA", "LANZAMIENTO", "ARRASTRE", "FURIA DE CADENA"}},
    {"bruto_cadena_rojo", {"punch1", 4, 17}, {"punch1", 10, 18}, {"punch1", 17, 23}, {"punch1", 8, 17, false, 1.2f}, 0, 2, 2, false,
     {"BOLAZO", "REMOLINO DE BOLA", "LANZAMIENTO", "ARRASTRE", "FURIA DE CADENA"}},
    {"medusa", {"punch1", 0, 9}, {"punch1", 0, 13, false, 1.3f}, {"kick", 0, 6}, {"kick", 7, 14}, 1, 0, 0, false,
     {"MIRADA", "COLETAZO", "COLETAZO GIRATORIO", "LATIGO", "FURIA DE SERPIENTE"}},
    {"medusa_roja", {"punch1", 0, 9}, {"punch1", 0, 13, false, 1.3f}, {"kick", 0, 6}, {"kick", 7, 14}, 1, 0, 0, false,
     {"MIRADA", "COLETAZO", "COLETAZO GIRATORIO", "LATIGO", "FURIA DE SERPIENTE"}},
};

const Fam* FindFam(const char* id) {
    for (const Fam& f : kFams) if (std::strcmp(f.id, id) == 0) return &f;
    return nullptr;
}

constexpr float kMinDur = 1.0f / 60.0f;

struct Builder {
    KfReference& r;
    std::vector<SpriteFrame>& frames() { return r.templ.frames; }

    const AnimationClip* Clip(const char* name) const {
        auto it = r.templ.namedClips.find(name);
        return it == r.templ.namedClips.end() ? nullptr : &it->second;
    }

    // Caja de ataque del cuadro con caja mas cercano (por paso) del clip `c`.
    bool NearestHit(const AnimationClip& c, int step, Rectangle& out) {
        int best = -1, bestD = 1 << 30;
        for (int i = 0; i < static_cast<int>(c.frames.size()); ++i) {
            const int fi = c.frames[static_cast<std::size_t>(i)];
            if (fi < 0 || fi >= static_cast<int>(frames().size()) || !frames()[static_cast<std::size_t>(fi)].hitbox) continue;
            if (std::abs(i - step) < bestD) { bestD = std::abs(i - step); best = fi; }
        }
        if (best < 0) return false;
        out = *frames()[static_cast<std::size_t>(best)].hitbox;
        return true;
    }

    // Pasos (cuadro, duracion) de una parte, con sus cajas y avance aplicados.
    std::vector<std::pair<int, float>> Steps(const Part& p) {
        std::vector<std::pair<int, float>> out;
        const AnimationClip* c = Clip((std::string("df_src_") + p.src).c_str());
        if (!c || c->frames.empty()) return out;
        const int last = static_cast<int>(c->frames.size()) - 1;
        const int from = std::clamp(p.from, 0, last), to = std::clamp(p.to, from, last);
        std::vector<int> idx;
        for (int k = from; k <= to; ++k) idx.push_back(k);
        if (p.rev) std::reverse(idx.begin(), idx.end());
        const int n = static_cast<int>(idx.size());
        bool hasHit = false;
        for (int k : idx) {
            const int fi = c->frames[static_cast<std::size_t>(k)];
            if (frames()[static_cast<std::size_t>(fi)].hitbox) hasHit = true;
        }
        const int peak = hasHit ? -1 : n / 2;
        Rectangle synth{};
        const bool haveSynth = !hasHit && NearestHit(*c, (from + to) / 2, synth);
        const bool needMod = p.m.dx != 0 || p.m.both || p.m.low || p.m.widen != 1.0f;
        for (int i = 0; i < n; ++i) {
            const int k = idx[static_cast<std::size_t>(i)];
            int fi = c->frames[static_cast<std::size_t>(k)];
            const float d = std::max(kMinDur, (c->durations.empty() ? c->frameDuration
                                                : c->durations[std::min<std::size_t>(static_cast<std::size_t>(k), c->durations.size() - 1)]) /
                                               std::max(0.05f, p.speed));
            const bool synthHere = (i == peak) && haveSynth;
            const bool hasBox = frames()[static_cast<std::size_t>(fi)].hitbox.has_value() || synthHere;
            if (needMod || synthHere) {
                SpriteFrame f = frames()[static_cast<std::size_t>(fi)];
                const float shift = p.m.dx * std::sin(3.14159265f * (static_cast<float>(i) + 0.5f) / static_cast<float>(n));
                for (FramePiece& pc : f.pieces) pc.x += shift;
                if (f.hurtbox) f.hurtbox->x += shift;
                if (hasBox) {
                    Rectangle h = f.hitbox ? *f.hitbox : synth;
                    h.x += shift;
                    if (p.m.widen != 1.0f) h.width *= p.m.widen;
                    if (p.m.low) { h.y = -46; h.height = 46; }
                    if (p.m.both) { const float reach = std::max(std::fabs(h.x), h.x + h.width); h.x = -reach; h.width = 2 * reach; }
                    f.hitbox = h;
                }
                fi = static_cast<int>(frames().size());
                frames().push_back(f);
            }
            out.push_back({fi, d});
        }
        return out;
    }

    // Un clip derivado. hitT/total: el 1er cuadro con caja golpea a hitT s y el clip
    // dura `total` s (el reloj de la hoja de movimientos y de AttackDef va por ahi).
    void Put(const char* name, const std::vector<Part>& parts, float hitT = 0.12f, float total = 0.55f) {
        AnimationClip c{0, 0, 1.0f / 30.0f, false};
        for (const Part& p : parts)
            for (const auto& s : Steps(p)) { c.frames.push_back(s.first); c.durations.push_back(s.second); }
        int hit = -1;
        float t0 = 0, sum = 0;
        for (std::size_t i = 0; i < c.frames.size(); ++i) {
            if (hit < 0 && frames()[static_cast<std::size_t>(c.frames[i])].hitbox) { hit = static_cast<int>(i); t0 = sum; }
            sum += c.durations[i];
        }
        if (hit >= 0 && t0 > 1e-4f && sum - t0 > 1e-4f && total > hitT) {
            const float k1 = std::clamp(hitT / t0, 0.3f, 3.0f), k2 = std::clamp((total - hitT) / (sum - t0), 0.3f, 3.0f);
            for (std::size_t i = 0; i < c.durations.size(); ++i)
                c.durations[i] = std::max(kMinDur, c.durations[i] * (static_cast<int>(i) < hit ? k1 : k2));
        }
        Store(name, c);
    }
    void PutFrames(const char* name, const std::vector<std::pair<int, float>>& st, bool loop = false) {
        AnimationClip c{0, 0, 1.0f / 30.0f, loop};
        for (const auto& s : st) { c.frames.push_back(s.first); c.durations.push_back(s.second); }
        Store(name, c);
    }
    void Store(const char* name, AnimationClip c) {
        if (c.frames.empty()) return;
        c.startFrame = c.frames.front();
        c.endFrame = c.frames.back();
        const bool isNew = r.templ.namedClips.find(name) == r.templ.namedClips.end();
        r.templ.namedClips[name] = c;
        if (isNew) r.clipNames.push_back(name);
    }
    void Copy(const char* dst, const AnimationClip& src) { Store(dst, src); }
};

Part With(Part p, Mod m, bool rev, float speed) { p.m = m; p.rev = rev; p.speed = speed; return p; }

void Derive(KfReference& ref, const Fam& f) {
    Builder b{ref};
    if (!b.Clip("punch1") || !b.Clip("idle")) return;
    const AnimationClip origP1 = *b.Clip("punch1");
    b.Copy("df_src_punch1", origP1);   // los subrangos salen SIEMPRE de los clips originales
    if (b.Clip("kick")) b.Copy("df_src_kick", *b.Clip("kick"));
    const AnimationClip* en = b.Clip("energy");
    const AnimationClip origEnergy = en ? *en : origP1;
    const AnimationClip* kn = b.Clip("knockdown");
    const AnimationClip origKnock = kn ? *kn : *b.Clip("hit");

    // 0) Centauros: el circulo magico son las dos mitades espejadas de la
    // rejilla tex 0 bajo los cascos (124 x 70-74 px).
    if (f.circle)
        for (SpriteFrame& fr : b.frames())
            fr.pieces.erase(std::remove_if(fr.pieces.begin(), fr.pieces.end(),
                                           [](const FramePiece& p) {
                                               return p.tex == 0 && p.y > -45 && p.y < 10 && p.src.width >= 110 &&
                                                      p.src.height <= 90;
                                           }),
                            fr.pieces.end());

    const Part &A = f.A, &B = f.B, &C = f.C, &D = f.D;
    // 1) Cadena J: A, B, C y el remate D.
    b.Put("punch1", {A});
    b.Put("punch2", {B});
    b.Put("punch_hook", {C});
    b.Put("punch3", {D});
    b.Put("df_atk_a", {A}); b.Put("df_atk_b", {B}); b.Put("df_atk_c", {C}); b.Put("df_atk_d", {D});
    // 2) Cadena K: tajo al reves, B con avance, el remate hacia delante.
    b.Put("kick", {With(A, {}, !A.rev, 1.1f)});
    b.Put("kick2", {With(B, Mod{24}, B.rev, 1.0f)});
    b.Put("kick3", {With(D, Mod{30}, !D.rev, 1.0f)});
    // 3) Dash, comando y barrida.
    b.Put("dash_attack", {With(A, Mod{70}, A.rev, 1.2f)}, 0.12f, 0.50f);
    b.Put("dash_kick", {With(B, Mod{60}, B.rev, 1.2f)}, 0.12f, 0.50f);
    b.Put("fwd_punch", {With(A, Mod{36, false, false, 1.15f}, A.rev, 1.0f)});
    b.Put("fwd_kick", {With(B, Mod{30}, B.rev, 1.0f)});
    b.Put("spin_kick", {With(C, Mod{0, true}, C.rev, 1.0f)});
    b.Put("sweep", {With(B, Mod{16, false, true}, B.rev, 1.0f)});
    // 4) Firmas.
    b.Put("signature1", {With(A, {}, A.rev, 1.15f), With(B, {}, B.rev, 1.15f), With(C, {}, C.rev, 1.15f)}, 0.12f, 1.40f);
    b.Put("signature2", {With(D, Mod{20}, D.rev, 1.0f)}, 0.25f, 0.70f);
    b.Put("signature3", {With(C, Mod{40}, C.rev, 1.0f), With(D, {}, D.rev, 1.0f)}, 0.25f, 0.90f);
    // 5) Furia / super: el ataque largo original; L ("special") segun la bestia.
    b.Copy("rage_attack", origP1);
    b.Copy("finisher", origP1);
    b.Copy("super", f.superSrc == 2 ? origEnergy : origP1);
    if (f.special == 1) b.Put("special", {With(A, {}, A.rev, 1.2f), With(B, {}, B.rev, 1.2f)}, 0.25f, 1.1f);
    else if (f.special == 2) b.Copy("special", origEnergy);

    // 6) Estados sin dolor fantasma.
    const int last = origP1.frames.back();
    b.PutFrames("recovery", {{last, 0.08f}});
    {   // air/airborne: primeros cuadros del derribo (sin el cuadro de reposo).
        std::vector<std::pair<int, float>> st;
        const int idle0 = b.Clip("idle")->frames.front();
        for (std::size_t i = 0; i < origKnock.frames.size() && st.size() < 3; ++i)
            if (origKnock.frames[i] != idle0) st.push_back({origKnock.frames[i], 0.07f});
        if (st.empty()) st.push_back({origKnock.frames.front(), 0.1f});
        b.PutFrames("airborne", st);
        b.PutFrames("air", st);
    }
    {   // bloqueo: cuadro de guardia del ataque mantenido.
        const std::size_t s = std::min<std::size_t>(static_cast<std::size_t>(f.blockStep), origP1.frames.size() - 1);
        b.PutFrames("block", {{origP1.frames[s], 0.1f}}, true);
    }

    // 7) Habilidades 1-5 (L, firma 1, firma 2, >J, furia).
    ref.abilityClips = {"special", "signature1", "signature2", "fwd_punch", "super"};
    ref.abilityNames.assign(f.names, f.names + 5);
}

}  // namespace

void DeriveBeasts(KfReference& ref, const char* betaId) {
    if (betaId == nullptr) return;
    const Fam* f = FindFam(betaId);
    if (f == nullptr) return;
    Derive(ref, *f);
}

}  // namespace district_fury
