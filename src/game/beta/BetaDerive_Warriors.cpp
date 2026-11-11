// Derivaciones en memoria de RONIN (ronin), GUERRERO x4 (kratos_*) y
// RONIN-KRATOS x4 (ronin_kratos_*). F1.7.
//
// Los .txt de data/kratos (y sus PNG) NO se tocan: aqui solo se edita la COPIA en
// memoria que KfReference::Load entrega a District Fury (el modo BETA apaga esta
// derivacion con SetBetaDeriveEnabled(false), asi GowApk no cambia). Solo se
// AGREGAN clips o se reemplazan nombres de estado en la copia en memoria.
//
// Problemas de origen (ver auditoria):
//  - GUERRERO / RONIN-KRATOS (35 clips): recovery == hit (gesto de dolor tras CADA
//    golpe propio); air == airborne == hit (vuela con pose de golpe de pie);
//    walk == run (patina al caminar a velocidad de carrera); un unico ataque largo
//    repartido en atk3/punch3/dash_attack (22 cuadros) y habilidades de 46-76
//    cuadros que, sin partir, duran 3-5 s.
//  - RONIN (31 clips, sin cajas de ataque en los datos): caminar = correr =
//    6 cuadros de guardia arrastrando los pies; air/airborne = 1 cuadro erguido
//    (nunca cae); jump y victory sin usar.
//
// Derivacion (clipDriven: la caja de ataque sale del cuadro DIBUJADO cuando el
// dato no trae hitbox, asi que basta con elegir bien los cuadros):
//  - recovery = cuadro 0 de idle (NUNCA el clip de dolor).
//  - air/airborne = primeros cuadros del derribo (Kratos/RK) o el cuadro 3 del
//    salto (RONIN): el feel ligero (clip de <= 2 cuadros) les da la caida rotada.
//  - walk = run con duraciones x2.2 (Kratos/RK) o idle alternado con la mitad
//    delantera de dash (RONIN): el ciclo deja de patinar.
//  - clips de ataque partidos en subrangos (gk_dash_a, gk_habA_short, gk_habB,
//    gk_habC, gk_combo1_short, gk_combo2, gk_sweep; rk_fly, rk_launch, rk_sweep)
//    con el nombre que el MoveSet (Moves_Warriors.cpp) nombra en cada movimiento.
#include "game/beta/BetaDerive.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace district_fury {
namespace {

constexpr float kMinDur = 1.0f / 60.0f;

struct Builder {
    KfReference& r;

    const AnimationClip* Clip(const char* name) const {
        auto it = r.templ.namedClips.find(name);
        return it == r.templ.namedClips.end() ? nullptr : &it->second;
    }
    bool Has(const char* name) const { return Clip(name) != nullptr; }

    void Store(const char* name, AnimationClip c) {
        if (c.frames.empty()) return;
        c.startFrame = c.frames.front();
        c.endFrame = c.frames.back();
        const bool isNew = r.templ.namedClips.find(name) == r.templ.namedClips.end();
        r.templ.namedClips[name] = std::move(c);
        if (isNew) r.clipNames.push_back(name);
    }

    // Clip nuevo con el subrango [from,to] (posiciones DENTRO de src), opcional
    // invertido y a otra velocidad. Reusa los cuadros originales (sin copiar).
    void Sub(const char* name, const char* src, int from, int to, float speed = 1.0f, bool rev = false,
             bool loop = false) {
        const AnimationClip* c = Clip(src);
        if (c == nullptr || c->frames.empty()) return;
        const int last = static_cast<int>(c->frames.size()) - 1;
        from = std::clamp(from, 0, last);
        to = std::clamp(to, from, last);
        std::vector<int> idx;
        for (int k = from; k <= to; ++k) idx.push_back(k);
        if (rev) std::reverse(idx.begin(), idx.end());
        AnimationClip out{0, 0, 1.0f / 30.0f, loop};
        for (int k : idx) {
            out.frames.push_back(c->frames[static_cast<std::size_t>(k)]);
            const float d = c->durations.empty()
                                ? c->frameDuration
                                : c->durations[std::min<std::size_t>(static_cast<std::size_t>(k),
                                                                     c->durations.size() - 1)];
            out.durations.push_back(std::max(kMinDur, d / std::max(0.05f, speed)));
        }
        Store(name, out);
    }

    // Copia de src con las duraciones multiplicadas (para caminar lento, etc.).
    void Scaled(const char* name, const char* src, float durMul, bool loop) {
        const AnimationClip* c = Clip(src);
        if (c == nullptr || c->frames.empty()) return;
        AnimationClip out = *c;
        out.loop = loop;
        if (out.durations.empty()) out.frameDuration = std::max(kMinDur, out.frameDuration * durMul);
        for (float& d : out.durations) d = std::max(kMinDur, d * durMul);
        Store(name, out);
    }

    // Clip hecho de (posicion en src, duracion). -1 en src = frame global directo.
    void FromPositions(const char* name, const char* src, const std::vector<std::pair<int, float>>& st,
                       bool loop = false) {
        const AnimationClip* c = Clip(src);
        if (c == nullptr || c->frames.empty()) return;
        const int last = static_cast<int>(c->frames.size()) - 1;
        AnimationClip out{0, 0, 1.0f / 30.0f, loop};
        for (const auto& s : st) {
            out.frames.push_back(c->frames[static_cast<std::size_t>(std::clamp(s.first, 0, last))]);
            out.durations.push_back(std::max(kMinDur, s.second));
        }
        Store(name, out);
    }

    // Frame global en la posicion pos del clip src (-1 si no existe).
    int FrameAt(const char* src, int pos) const {
        const AnimationClip* c = Clip(src);
        if (c == nullptr || c->frames.empty()) return -1;
        return c->frames[static_cast<std::size_t>(std::clamp(pos, 0, static_cast<int>(c->frames.size()) - 1))];
    }

    // Clip hecho de cuadros de varios clips: (clip, posicion, duracion). Util para
    // una pose de salto que incluye un cuadro de golpe que SI llega al frente (la
    // caja de clipDriven sale del dibujo, y el salto a secas no alcanza).
    void Mix(const char* name, const std::vector<std::tuple<const char*, int, float>>& items) {
        AnimationClip out{0, 0, 1.0f / 30.0f, false};
        for (const auto& it : items) {
            const int fi = FrameAt(std::get<0>(it), std::get<1>(it));
            if (fi < 0) continue;
            out.frames.push_back(fi);
            out.durations.push_back(std::max(kMinDur, std::get<2>(it)));
        }
        Store(name, out);
    }
};

// -------------------------------------------------------------------------
// RONIN (artista marcial de ki). Sin cajas de ataque en los datos: la caja la
// marca el cuadro dibujado. Faltan arte de caminar, aire/derribo y movimientos
// propios (patada voladora, lanzador, barrida).
// -------------------------------------------------------------------------
void DeriveRonin(KfReference& r) {
    Builder b{r};
    if (!b.Has("idle") || !b.Has("jump") || !b.Has("kick")) return;

    // Caminar = idle alternado con la mitad delantera de dash (paso con avance,
    // sin patinar); correr sigue siendo "run" (los 6 cuadros de guardia).
    if (b.Has("dash")) {
        const int i0 = b.FrameAt("idle", 0), i1 = b.FrameAt("idle", 1);
        const int d0 = b.FrameAt("dash", 0), d1 = b.FrameAt("dash", 1);
        if (i0 >= 0 && d0 >= 0) {
            AnimationClip w{0, 0, 1.0f / 30.0f, true};
            for (const std::pair<int, float>& s :
                 {std::pair<int, float>{i0, 0.13f}, {d0, 0.11f}, {i1, 0.13f}, {d1, 0.11f}}) {
                w.frames.push_back(s.first);
                w.durations.push_back(s.second);
            }
            b.Store("walk", std::move(w));
        }
    }

    // Aire/derribo en el aire = cuadro 3 del salto (el feel ligero lo rota al caer).
    const int jumpFall = b.FrameAt("jump", 3);
    if (jumpFall >= 0) {
        AnimationClip a{0, 0, 1.0f / 30.0f, false};
        a.frames.push_back(jumpFall);
        a.durations.push_back(0.12f);
        b.Store("air", a);
        b.Store("airborne", a);
    }

    // Recuperacion = cuadro de guardia (idle 0), nunca el clip de dolor.
    b.FromPositions("recovery", "idle", {{0, 0.10f}});

    // Patada voladora = pose de salto (2-4) con el cuadro de golpe de patada en
    // medio para que la caja (del dibujo) llegue al frente; lanzador = salto 0-2
    // con el remate (punch3) hacia arriba; barrida = tramo de golpe de la patada.
    b.Mix("rk_fly", {{"jump", 2, 0.07f}, {"kick", 2, 0.12f}, {"jump", 3, 0.07f}, {"jump", 4, 0.07f}});
    b.Mix("rk_launch", {{"jump", 0, 0.06f}, {"punch3", 1, 0.10f}, {"jump", 1, 0.07f}, {"jump", 2, 0.07f}});
    b.Sub("rk_sweep", "kick", 1, 3, 1.0f);
}

// -------------------------------------------------------------------------
// GUERRERO / RONIN-KRATOS (misma estructura, 35 clips). recovery == hit,
// air == airborne == hit, walk == run; ataques largos que hay que partir.
// -------------------------------------------------------------------------
void DeriveKratos(KfReference& r) {
    Builder b{r};
    if (!b.Has("idle") || !b.Has("run") || !b.Has("knockdown")) return;

    // Sin dolor fantasma tras un golpe propio: recuperacion = cuadro 0 de idle.
    b.FromPositions("recovery", "idle", {{0, 0.09f}});

    // Aire/derribo = primeros cuadros del derribo real (no la pose de dolor de pie).
    b.Sub("air", "knockdown", 0, 2, 1.0f);
    b.Sub("airborne", "knockdown", 0, 2, 1.0f);

    // Caminar = correr con duraciones x2.2 (deja de patinar); correr = "run" normal.
    b.Scaled("walk", "run", 2.2f, true);

    // Cadena J: el 4o golpe = primera mitad del ataque largo (dash_attack 0-10).
    if (b.Has("dash_attack")) b.Sub("gk_dash_a", "dash_attack", 0, 10, 1.0f);
    // Cadena K: 2o = HABILIDAD A corta (primeros cuadros de ab0).
    if (b.Has("ab0")) b.Sub("gk_habA_short", "ab0", 0, 8, 1.1f);
    // Comandos: HABILIDAD B (DP+K) y HABILIDAD C (QCB+K) en subrangos manejables.
    if (b.Has("ab1")) b.Sub("gk_habB", "ab1", 0, 24, 1.0f);
    if (b.Has("ab2")) b.Sub("gk_habC", "ab2", 0, 30, 1.0f);
    // Furia = COMBO 1 corta; firma 2 = COMBO 2 (recortada para no durar 4.5 s).
    if (b.Has("ab3")) b.Sub("gk_combo1_short", "ab3", 0, 5, 1.0f);
    if (b.Has("ab4")) b.Sub("gk_combo2", "ab4", 0, 40, 1.0f);
    // Barrida (vK): tramo de golpe de la patada (los cuadros que llegan al frente).
    if (b.Has("kick")) b.Sub("gk_sweep", "kick", 1, 7, 1.0f);
}

}  // namespace

void DeriveWarriors(KfReference& ref, const char* betaId) {
    if (betaId == nullptr) return;
    if (std::strcmp(betaId, "ronin") == 0) {
        DeriveRonin(ref);
    } else if (std::strncmp(betaId, "kratos_", 7) == 0 || std::strncmp(betaId, "ronin_kratos_", 13) == 0) {
        DeriveKratos(ref);
    }
}

}  // namespace district_fury
