#include "game/beta/BetaCharacter.h"
#include "core/Platform.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <memory>
#include <sstream>

namespace district_fury {
namespace {

// Los personajes por piezas viven en dos sitios: Kratos es compartido (lo usan
// el Modo VS de District Fury y GowApk) y el resto del reparto solo existe en
// GowApk. Devuelve la primera ruta que existe de verdad.
std::string FirstExisting(const std::vector<std::string>& candidates) {
    for (const std::string& c : candidates)
        for (const std::string& p : {c, "../" + c, "../../" + c})
            if (platform::AssetExists(p)) return c;
    return candidates.empty() ? std::string{} : candidates.front();
}

}  // namespace

KfReference LoadBetaCharacter(const std::string& id) {
    KfReference out;
    std::string text;
    const std::string rel = FirstExisting({"data/kratos/" + id + ".txt",
                                           "GowApk/data/characters/" + id + ".txt"});
    for (const std::string& p : {rel, "../" + rel, "../../" + rel})
        if (platform::LoadTextFile(p, text)) break;
    if (text.empty()) { out.error = "no se encontro " + rel; return out; }

    // Imagen de cada indice: "N" = una imagen suelta de actor; si no, una ruta
    // dentro de los assets de GowApk (jefes Spine: "spine/Titan/Titan").
    std::vector<std::string> imagePaths;
    std::vector<SpriteFrame> frames;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "name") {
            std::getline(ls >> std::ws, out.displayName);
        } else if (tag == "character") {
            std::string cid, kind;
            ls >> cid >> kind >> out.maxHp;
            out.betaHero = kind == "hero";
        } else if (tag == "image") {
            int idx = 0;
            std::string img;
            ls >> idx >> img;
            if (idx < 0 || img.empty()) continue;
            if (idx >= (int)imagePaths.size()) imagePaths.resize((size_t)idx + 1);
            const bool numeric = img.find_first_not_of("0123456789") == std::string::npos;
            imagePaths[(size_t)idx] =
                numeric ? FirstExisting({"assets/kratos/actor/" + img + ".png", "GowApk/assets/actor/" + img + ".png"})
                        : FirstExisting({"GowApk/assets/" + img + ".png", "assets/kratos/" + img + ".png"});
        } else if (tag == "frame") {
            int idx = 0, n = 0;
            float b[4] = {}, a[4] = {};
            ls >> idx >> n >> b[0] >> b[1] >> b[2] >> b[3] >> a[0] >> a[1] >> a[2] >> a[3];
            SpriteFrame f;
            if (b[2] > b[0] && b[3] > b[1]) f.hurtbox = Rectangle{b[0], b[1], b[2] - b[0], b[3] - b[1]};
            if (a[2] > a[0] && a[3] > a[1]) f.hitbox = Rectangle{a[0], a[1], a[2] - a[0], a[3] - a[1]};
            frames.push_back(f);
        } else if (tag == "piece" && !frames.empty()) {
            int tex = 0, sx = 0, sy = 0, sw = 0, sh = 0, x = 0, y = 0, flip = 0;
            ls >> tex >> sx >> sy >> sw >> sh >> x >> y >> flip;
            FramePiece p;
            p.tex = tex;
            p.src = Rectangle{(float)sx, (float)sy, (float)sw, (float)sh};
            p.x = (float)x;
            p.y = (float)y;
            p.flipX = (flip & 1) != 0;
            p.flipY = (flip & 2) != 0;
            p.rot90 = (flip & 4) != 0;
            frames.back().pieces.push_back(p);
        } else if (tag == "clip") {
            std::string name;
            int loop = 0, n = 0;
            ls >> name >> loop >> n;
            AnimationClip clip{0, 0, 1.0f / 30.0f, loop != 0};
            std::string fd;
            while (ls >> fd) {
                const auto colon = fd.find(':');
                if (colon == std::string::npos) continue;
                clip.frames.push_back(std::stoi(fd.substr(0, colon)));
                clip.durations.push_back(std::stof(fd.substr(colon + 1)));
            }
            if (clip.frames.empty()) continue;
            clip.startFrame = clip.frames.front();
            clip.endFrame = clip.frames.back();
            out.templ.namedClips[name] = clip;
            out.clipNames.push_back(name);
        } else if (tag == "clipsound") {
            std::string name, snd;
            int idx = 0;
            ls >> name >> idx >> snd;
            auto it = out.templ.namedClips.find(name);
            if (it != out.templ.namedClips.end()) it->second.sounds.push_back({idx, snd});
        } else if (tag == "ability") {
            std::string clip, label;
            ls >> clip;
            std::getline(ls >> std::ws, label);
            out.abilityClips.push_back(clip);
            out.abilityNames.push_back(label);
        } else if (tag == "sound") {
            std::string ev, snd;
            ls >> ev >> snd;
            out.sounds[ev] = snd;
        }
    }
    if (frames.empty()) { out.error = "sin cuadros"; return out; }

    // Medidas de cada cuadro (las usan sombra, retrato y respaldo de cajas).
    for (SpriteFrame& f : frames) {
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        bool first = true;
        for (const FramePiece& p : f.pieces) {
            const float w = p.rot90 ? p.src.height : p.src.width, h = p.rot90 ? p.src.width : p.src.height;
            if (first) { x0 = p.x; y0 = p.y; x1 = p.x + w; y1 = p.y + h; first = false; }
            x0 = std::min(x0, p.x); y0 = std::min(y0, p.y);
            x1 = std::max(x1, p.x + w); y1 = std::max(y1, p.y + h);
        }
        f.width = std::max(1.0f, x1 - x0);
        f.height = std::max(1.0f, y1 - y0);
        f.pivotX = -x0;
        f.pivotY = -y0;
        f.source = Rectangle{0, 0, f.width, f.height};
        f.visualBounds = Rectangle{0, 0, f.width, f.height};
    }

    auto textures = std::make_shared<std::vector<Texture2D>>();
    for (const std::string& path : imagePaths)
        textures->push_back(path.empty() ? Texture2D{0} : AssetManager::Get().GetTextureByPath(path));
    Texture2D firstTex{0};
    for (const Texture2D& t : *textures)
        if (t.id != 0) { firstTex = t; break; }
    if (firstTex.id == 0) { out.error = "no cargaron las imagenes de actor de " + id; return out; }

    auto clips = out.templ.namedClips;
    out.templ.Init(firstTex, 1, 1, true);
    out.templ.SetFrames(frames);
    out.templ.namedClips = clips;
    out.templ.pieceTextures = textures;
    if (!out.templ.HasClip("idle")) { out.error = "sin clip idle"; return out; }
    out.templ.PlayNamed("idle");
    out.scale = 1.0f;
    out.beta = true;
    out.loaded = true;
    return out;
}

}  // namespace district_fury
