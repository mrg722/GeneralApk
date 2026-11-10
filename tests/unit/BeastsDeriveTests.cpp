// F1.8: bestias BETA (skins 27-37). Sin ventana: carga los .txt de
// GowApk/data/characters en un KfReference sin texturas, deriva y comprueba
// - cada golpe de la cadena (punch1, punch2, punch_hook, punch3) con cuadros distintos y caja propia;
// - recovery != hit (sin dolor tras un golpe propio), bloqueo != idle, aire con el derribo;
// - centauros sin las piezas del circulo magico; 5 habilidades distintas;
// - MoveSet propio en 27-37 (>= 9 movimientos con override) y ninguno fuera.
#include "game/beta/BetaDerive.h"
#include "game/moves/MoveRegistry.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

using namespace district_fury;

namespace {

KfReference LoadHeadless(const std::string& id) {
    KfReference r;
    std::ifstream in("GowApk/data/characters/" + id + ".txt");
    assert(in.good());
    std::vector<SpriteFrame> frames;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "frame") {
            int idx, n; float b[4], a[4];
            ls >> idx >> n >> b[0] >> b[1] >> b[2] >> b[3] >> a[0] >> a[1] >> a[2] >> a[3];
            SpriteFrame f;
            if (b[2] > b[0] && b[3] > b[1]) f.hurtbox = Rectangle{b[0], b[1], b[2] - b[0], b[3] - b[1]};
            if (a[2] > a[0] && a[3] > a[1]) f.hitbox = Rectangle{a[0], a[1], a[2] - a[0], a[3] - a[1]};
            frames.push_back(f);
        } else if (tag == "piece" && !frames.empty()) {
            int tex, sx, sy, sw, sh, x, y, flip;
            ls >> tex >> sx >> sy >> sw >> sh >> x >> y >> flip;
            FramePiece p;
            p.tex = tex; p.src = Rectangle{(float)sx, (float)sy, (float)sw, (float)sh}; p.x = (float)x; p.y = (float)y;
            frames.back().pieces.push_back(p);
        } else if (tag == "clip") {
            std::string name, fd; int loop, n;
            ls >> name >> loop >> n;
            AnimationClip c{0, 0, 1.0f / 30.0f, loop != 0};
            while (ls >> fd) {
                const auto colon = fd.find(':');
                c.frames.push_back(std::stoi(fd.substr(0, colon)));
                c.durations.push_back(std::stof(fd.substr(colon + 1)));
            }
            c.startFrame = c.frames.front(); c.endFrame = c.frames.back();
            r.templ.namedClips[name] = c;
            r.clipNames.push_back(name);
        } else if (tag == "ability") {
            std::string clip, label;
            ls >> clip;
            std::getline(ls >> std::ws, label);
            r.abilityClips.push_back(clip); r.abilityNames.push_back(label);
        }
    }
    r.templ.frames = frames;
    return r;
}

bool HasHit(const KfReference& r, const char* clip) {
    for (int f : r.templ.namedClips.at(clip).frames) if (r.templ.frames[(size_t)f].hitbox) return true;
    return false;
}

}  // namespace

int main() {
    const char* ids[] = {"esqueleto", "excavador", "momia_escudo", "bestia_elefante", "ave_sanadora", "centauro",
                         "centauro_rojo", "bruto_cadena", "bruto_cadena_rojo", "medusa", "medusa_roja"};
    for (const char* id : ids) {
        KfReference r = LoadHeadless(id);
        const auto hitBefore = r.templ.namedClips.at("hit");
        const auto idleBefore = r.templ.namedClips.at("idle");
        DeriveBeasts(r, id);
        auto& c = r.templ.namedClips;
        std::set<std::vector<int>> chain;
        for (const char* n : {"punch1", "punch2", "punch_hook", "punch3"}) {
            assert(c.count(n));
            assert(HasHit(r, n));
            chain.insert(c.at(n).frames);
        }
        if (chain.size() != 4) std::fprintf(stderr, "cadena repetida en %s\n", id);
        assert(chain.size() == 4);   // cuadros distintos en cada golpe
        for (const char* n : {"kick", "kick2", "kick3", "dash_attack", "dash_kick", "fwd_punch", "fwd_kick", "spin_kick",
                              "sweep", "signature1", "signature2", "signature3", "special", "super"})
            assert(c.count(n) && HasHit(r, n));
        // Sin dolor tras golpe propio: recovery es 1 cuadro (el ultimo del ataque), no el clip de hit.
        assert(c.at("recovery").frames.size() == 1 && c.at("recovery").frames != hitBefore.frames);
        assert(c.at("block").frames.size() == 1 && c.at("block").frames != idleBefore.frames && c.at("block").loop);
        assert(c.at("air").frames.size() >= 1 && c.at("airborne").frames == c.at("air").frames);
        assert(r.abilityClips.size() == 5 && r.abilityNames.size() == 5);
        assert(std::set<std::string>(r.abilityClips.begin(), r.abilityClips.end()).size() == 5);
        for (const auto& s : r.abilityClips) assert(c.count(s));
        if (std::string(id).rfind("centauro", 0) == 0)
            for (const SpriteFrame& f : r.templ.frames)
                for (const FramePiece& p : f.pieces) assert(!(p.tex == 0 && p.y > -45 && p.y < 10 && p.src.width >= 110 && p.src.height <= 90));
        // Idempotencia con otro personaje: DeriveBeasts ignora lo que no es suyo.
        KfReference other = LoadHeadless(id);
        const std::size_t n = other.templ.namedClips.size();
        DeriveBeasts(other, "ronin");
        assert(other.templ.namedClips.size() == n);
    }
    for (int skin = 27; skin <= 37; ++skin) {
        const MoveSet* m = MovesetFor_Beasts(skin);
        assert(m != nullptr && m->overrideCount >= 9);
        assert(m->chainJCount == 4 && m->chainKCount == 3 && m->feel == FeelLevel::Light);
        assert(!(m->caps & (CapJump | CapAir)));
    }
    assert(MovesetFor_Beasts(26) == nullptr && MovesetFor_Beasts(38) == nullptr);
    std::puts("BeastsDeriveTests OK");
    return 0;
}
