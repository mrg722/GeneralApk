// F1.7: RONIN (22), GUERRERO x4 (23-26) y RONIN-KRATOS x4 (38-41). Sin ventana:
// carga los .txt de data/kratos en un KfReference sin texturas, deriva en memoria
// (BetaDerive_Warriors) y comprueba que:
//  - RONIN: caminar deja de ser la carrera (idle+dash), aire/derribo bajan al
//    suelo (cuadro de salto, no el erguido), y existen los clips propios
//    (rk_fly, rk_launch, rk_sweep);
//  - GUERRERO/RONIN-KRATOS: recovery ya no es el clip de dolor, aire = primeros
//    cuadros del derribo, caminar = correr ralentizado, y existen los subrangos
//    (gk_dash_a, gk_habA_short, gk_habB, gk_habC, gk_combo1_short, gk_combo2,
//    gk_sweep);
//  - la derivacion ignora los betaId que no son suyos (idempotente);
//  - MovesetFor_Warriors devuelve nullptr fuera de sus skins (el >= 18 por skin y
//    "golpeo = s" se validan con la ventana abierta via moves_sheet, F1.7).
// Los .txt de data/kratos NO se tocan: la derivacion solo cambia la copia en memoria.
#include "game/beta/BetaDerive.h"
#include "game/moves/MoveRegistry.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace district_fury;

namespace {

KfReference LoadHeadless(const std::string& id) {
    KfReference r;
    std::ifstream in("data/kratos/" + id + ".txt");
    assert(in.good() && "no se encontro data/kratos/<id>.txt (correr desde la raiz del repo)");
    std::vector<SpriteFrame> frames;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "frame") {
            int idx, n;
            float b[4], a[4];
            ls >> idx >> n >> b[0] >> b[1] >> b[2] >> b[3] >> a[0] >> a[1] >> a[2] >> a[3];
            SpriteFrame f;
            if (b[2] > b[0] && b[3] > b[1]) f.hurtbox = Rectangle{b[0], b[1], b[2] - b[0], b[3] - b[1]};
            if (a[2] > a[0] && a[3] > a[1]) f.hitbox = Rectangle{a[0], a[1], a[2] - a[0], a[3] - a[1]};
            frames.push_back(f);
        } else if (tag == "piece" && !frames.empty()) {
            int tex, sx, sy, sw, sh, x, y, flip;
            ls >> tex >> sx >> sy >> sw >> sh >> x >> y >> flip;
            FramePiece p;
            p.tex = tex;
            p.src = Rectangle{(float)sx, (float)sy, (float)sw, (float)sh};
            p.x = (float)x;
            p.y = (float)y;
            frames.back().pieces.push_back(p);
        } else if (tag == "clip") {
            std::string name, fd;
            int loop, n;
            ls >> name >> loop >> n;
            AnimationClip c{0, 0, 1.0f / 30.0f, loop != 0};
            while (ls >> fd) {
                const auto colon = fd.find(':');
                if (colon == std::string::npos) continue;
                c.frames.push_back(std::stoi(fd.substr(0, colon)));
                c.durations.push_back(std::stof(fd.substr(colon + 1)));
            }
            if (c.frames.empty()) continue;
            c.startFrame = c.frames.front();
            c.endFrame = c.frames.back();
            r.templ.namedClips[name] = c;
            r.clipNames.push_back(name);
        } else if (tag == "ability") {
            std::string clip, label;
            ls >> clip;
            std::getline(ls >> std::ws, label);
            r.abilityClips.push_back(clip);
            r.abilityNames.push_back(label);
        }
    }
    r.templ.frames = frames;
    return r;
}

bool Has(const KfReference& r, const char* clip) { return r.templ.namedClips.count(clip) != 0; }
const AnimationClip& C(const KfReference& r, const char* clip) { return r.templ.namedClips.at(clip); }

}  // namespace

int main() {
    // ---- RONIN ----
    {
        KfReference r = LoadHeadless("ronin");
        const auto walkBefore = C(r, "walk").frames;
        const auto airBefore = C(r, "air").frames;
        const std::size_t clipsBefore = r.templ.namedClips.size();
        DeriveWarriors(r, "ronin");
        // Caminar deja de ser la carrera (6 cuadros de guardia): ahora idle+dash.
        assert(C(r, "walk").frames != walkBefore);
        assert(C(r, "walk").loop);
        // Aire/derribo = cuadro(s) del salto, distintos del cuadro erguido original.
        assert(Has(r, "air") && Has(r, "airborne"));
        assert(C(r, "air").frames != airBefore);
        assert(C(r, "airborne").frames == C(r, "air").frames);
        // Recuperacion = 1 cuadro (guardia), no un gesto de dolor.
        assert(C(r, "recovery").frames.size() == 1);
        // Movimientos propios con cuadros validos.
        for (const char* n : {"rk_fly", "rk_launch", "rk_sweep"}) {
            assert(Has(r, n));
            assert(!C(r, n).frames.empty());
        }
        // Idempotencia: un betaId ajeno no toca nada.
        KfReference other = LoadHeadless("ronin");
        const std::size_t n = other.templ.namedClips.size();
        DeriveWarriors(other, "esqueleto");
        assert(other.templ.namedClips.size() == n);
        assert(clipsBefore < r.templ.namedClips.size());   // se agregaron clips
    }

    // ---- GUERRERO / RONIN-KRATOS (misma estructura de 35 clips) ----
    for (const char* id : {"kratos_espadas", "kratos_cesto", "kratos_rayo", "kratos_garras",
                           "ronin_kratos_espadas", "ronin_kratos_cesto", "ronin_kratos_rayo",
                           "ronin_kratos_garras"}) {
        KfReference r = LoadHeadless(id);
        const auto hitBefore = C(r, "hit").frames;
        const auto airBefore = C(r, "air").frames;
        const auto runDur = C(r, "run").durations;
        DeriveWarriors(r, id);
        // Sin dolor fantasma: recovery = 1 cuadro, distinto del clip de dolor.
        assert(C(r, "recovery").frames.size() == 1 && C(r, "recovery").frames != hitBefore);
        // Aire = primeros cuadros del derribo, no la pose de dolor de pie.
        assert(Has(r, "air") && C(r, "air").frames == C(r, "airborne").frames);
        assert(C(r, "air").frames != airBefore);
        assert(C(r, "air").frames == std::vector<int>(C(r, "knockdown").frames.begin(),
                                                      C(r, "knockdown").frames.begin() + 3));
        // Caminar = correr ralentizado: mismos cuadros, duraciones mayores.
        assert(C(r, "walk").frames == C(r, "run").frames);
        assert(C(r, "walk").durations != runDur);
        assert(C(r, "walk").durations.front() > runDur.front());
        // Subrangos de ataque propios.
        for (const char* n : {"gk_dash_a", "gk_habA_short", "gk_habB", "gk_habC", "gk_combo1_short",
                              "gk_combo2", "gk_sweep"}) {
            assert(Has(r, n));
            assert(!C(r, n).frames.empty());
        }
        // Idempotencia con un betaId ajeno (una bestia: ni la rama RONIN ni la Kratos).
        KfReference other = LoadHeadless(id);
        const std::size_t n = other.templ.namedClips.size();
        DeriveWarriors(other, "esqueleto");
        assert(other.templ.namedClips.size() == n);
    }

    // ---- MovesetFor_Warriors: nullptr fuera de sus skins (sin ventana) ----
    for (int skin : {0, 6, 21, 27, 37, 42}) assert(MovesetFor_Warriors(skin) == nullptr);

    std::puts("WarriorsDeriveTests OK");
    return 0;
}
