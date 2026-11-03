#include "rendering/SpriteManifest.h"
#include "core/Json.h"
#include "core/Platform.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace district_fury {
namespace {

bool ReadPair(const JsonValue* v, float& a, float& b) {
    if (!v || !v->IsArray() || v->array.size() != 2 || !v->array[0].IsNumber() || !v->array[1].IsNumber()) return false;
    a = static_cast<float>(v->array[0].number);
    b = static_cast<float>(v->array[1].number);
    return true;
}

bool ParseClip(const JsonValue& v, ClipDef& clip, std::string& err, const std::string& name) {
    const JsonValue* frames = v.Find("frames");
    if (!frames || !frames->IsArray() || frames->array.empty()) {
        err = "clip '" + name + "': 'frames' debe ser un arreglo no vacio";
        return false;
    }
    for (const JsonValue& f : frames->array) {
        if (!f.IsNumber() || f.number < 0) { err = "clip '" + name + "': indice de frame invalido"; return false; }
        clip.frames.push_back(static_cast<int>(f.number));
    }
    clip.loop = v.Find("loop") ? v.Find("loop")->BoolOr(true) : true;

    // Prioridad de duraciones: "durations" (por cuadro) > "duration" > "fps".
    float uniform = 0.1f;
    if (const JsonValue* fps = v.Find("fps")) if (fps->NumberOr(0) > 0) uniform = 1.0f / static_cast<float>(fps->number);
    if (const JsonValue* d = v.Find("duration")) if (d->NumberOr(0) > 0) uniform = static_cast<float>(d->number);
    clip.durations.assign(clip.frames.size(), uniform);
    if (const JsonValue* ds = v.Find("durations")) {
        if (!ds->IsArray() || ds->array.size() != clip.frames.size()) {
            err = "clip '" + name + "': 'durations' debe tener un valor por frame";
            return false;
        }
        for (std::size_t i = 0; i < ds->array.size(); ++i) {
            if (ds->array[i].NumberOr(0) <= 0) { err = "clip '" + name + "': duracion no positiva"; return false; }
            clip.durations[i] = static_cast<float>(ds->array[i].number);
        }
    }
    if (const JsonValue* sc = v.Find("scale")) {
        if (sc->NumberOr(0) <= 0) { err = "clip '" + name + "': 'scale' debe ser positivo"; return false; }
        clip.scales.assign(clip.frames.size(), static_cast<float>(sc->number));
    }
    if (const JsonValue* ss = v.Find("scales")) {
        if (!ss->IsArray() || ss->array.size() != clip.frames.size()) {
            err = "clip '" + name + "': 'scales' debe tener un valor por frame";
            return false;
        }
        clip.scales.clear();
        for (const JsonValue& x : ss->array) {
            if (x.NumberOr(0) <= 0) { err = "clip '" + name + "': escala no positiva"; return false; }
            clip.scales.push_back(static_cast<float>(x.number));
        }
    }
    if (const JsonValue* os = v.Find("offsets")) {
        if (!os->IsArray() || os->array.size() != clip.frames.size()) {
            err = "clip '" + name + "': 'offsets' debe tener un [dx,dy] por frame";
            return false;
        }
        for (const JsonValue& o : os->array) {
            float dx = 0, dy = 0;
            if (!ReadPair(&o, dx, dy)) { err = "clip '" + name + "': offset invalido"; return false; }
            clip.offsets.emplace_back(dx, dy);
        }
    }
    return true;
}

}  // namespace

const SpriteManifest& SpriteManifest::Get() {
    static const SpriteManifest instance = [] {
        SpriteManifest m;
        for (const char* path : {"data/sprite_manifest.json", "../data/sprite_manifest.json",
                                 "../../data/sprite_manifest.json"}) {
            if (m.LoadFromFile(path)) break;
        }
        return m;
    }();
    return instance;
}

bool SpriteManifest::LoadFromFile(const std::string& path) {
    std::string text;
    if (!platform::LoadTextFile(path, text)) { error_ = "no se pudo abrir " + path; return false; }
    return LoadFromString(text);
}

bool SpriteManifest::LoadFromString(const std::string& json, bool followIncludes) {
    atlases_.clear();
    clips_.clear();
    warnings_.clear();
    JsonValue root;
    if (!ParseJson(json, root, error_)) return false;
    if (!root.IsObject()) { error_ = "la raiz del manifiesto debe ser un objeto"; return false; }

    if (const JsonValue* atlases = root.Find("atlases")) {
        for (const auto& kv : atlases->object) {
            const JsonValue& a = kv.second;
            if (!a.Find("cell")) continue;   // fondos u otros recursos sin celda
            AtlasProfile p;
            p.id = kv.first;
            p.path = a.Find("path") ? a.Find("path")->StringOr("") : "";
            if (!ReadPair(a.Find("cell"), p.cellW, p.cellH) || p.cellW <= 0 || p.cellH <= 0) {
                error_ = "atlas '" + p.id + "': 'cell' invalido"; return false;
            }
            if (!ReadPair(a.Find("pivot"), p.pivotX, p.pivotY) ||
                p.pivotX < 0 || p.pivotX > p.cellW || p.pivotY < 0 || p.pivotY > p.cellH) {
                error_ = "atlas '" + p.id + "': 'pivot' invalido o fuera de la celda"; return false;
            }
            float cols = 0, rows = 0;
            if (a.Find("grid") && ReadPair(a.Find("grid"), cols, rows)) {
                p.columns = static_cast<int>(cols);
                p.rows = static_cast<int>(rows);
            }
            p.clipSet = a.Find("clips") ? a.Find("clips")->StringOr(p.id) : p.id;
            atlases_[p.id] = p;
        }
    }

    if (const JsonValue* sets = root.Find("clips")) {
        for (const auto& set : sets->object) {
            ClipMap map;
            for (const auto& c : set.second.object) {
                ClipDef clip;
                if (!ParseClip(c.second, clip, error_, set.first + "." + c.first)) return false;
                map[c.first] = std::move(clip);
            }
            clips_[set.first] = std::move(map);
        }
    }

    // F0.3: "includes" agrega clips desde data/manifest/*.json. Un include que no
    // se encuentra o no se puede leer solo deja un aviso.
    if (const JsonValue* inc = root.Find("includes"); inc && followIncludes) {
        if (!inc->IsArray()) { warnings_.push_back("'includes' debe ser un arreglo de rutas"); return true; }
        for (const JsonValue& item : inc->array) {
            const std::string rel = item.StringOr("");
            if (rel.empty()) { warnings_.push_back("include vacio o no es texto"); continue; }
            std::string text;
            bool found = false;
            for (const std::string& path : {rel, "../" + rel, "../../" + rel})
                if (platform::AssetExists(path) && platform::LoadTextFile(path, text)) { found = true; break; }
            if (!found) { warnings_.push_back("include no encontrado: " + rel); continue; }
            if (!ApplyInclude(text, rel)) warnings_.push_back("include ilegible: " + rel + " (" + error_ + ")");
        }
        error_.clear();
    }
    return true;
}

bool SpriteManifest::ApplyInclude(const std::string& json, const std::string& label) {
    JsonValue root;
    std::string err;
    if (!ParseJson(json, root, err) || !root.IsObject()) { error_ = label + ": " + (err.empty() ? "raiz no es objeto" : err); return false; }
    const JsonValue* sets = root.Find("clips");
    if (!sets) return true;
    if (!sets->IsObject()) { warnings_.push_back(label + ": 'clips' debe ser un objeto"); return true; }
    for (const auto& set : sets->object) {
        if (!set.second.IsObject()) { warnings_.push_back(label + ": juego '" + set.first + "' no es objeto"); continue; }
        ClipMap& map = clips_[set.first];
        for (const auto& c : set.second.object) {
            const std::string full = set.first + "." + c.first;
            const bool wantsOverride = c.second.Find("override") && c.second.Find("override")->BoolOr(false);
            if (wantsOverride && set.first == "rayden") {
                warnings_.push_back("ERROR " + label + ": 'override' prohibido en el juego rayden (" + full + " ignorado)");
                continue;
            }
            if (map.count(c.first) && !wantsOverride) {
                warnings_.push_back(label + ": " + full + " ya existe (ignorado; usa \"override\": true para reemplazar)");
                continue;
            }
            ClipDef clip;
            std::string clipErr;
            if (!ParseClip(c.second, clip, clipErr, full)) { warnings_.push_back(label + ": " + clipErr + " (descartado)"); continue; }
            clip.fromInclude = true;
            map[c.first] = std::move(clip);
        }
    }
    return true;
}

const AtlasProfile* SpriteManifest::FindAtlas(const std::string& id) const {
    const auto it = atlases_.find(id);
    return it == atlases_.end() ? nullptr : &it->second;
}

const ClipMap* SpriteManifest::FindClips(const std::string& set) const {
    const auto it = clips_.find(set);
    return it == clips_.end() ? nullptr : &it->second;
}

std::vector<SpriteFrame> BuildFrames(const AtlasProfile& p, int texW, int texH) {
    std::vector<SpriteFrame> out;
    if (p.cellW <= 0 || p.cellH <= 0) return out;
    const int cols = p.columns > 0 ? p.columns : static_cast<int>(texW / p.cellW);
    const int rows = p.rows > 0 ? p.rows : static_cast<int>(texH / p.cellH);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const float x = c * p.cellW;
            const float y = r * p.cellH;
            if (x + p.cellW > texW || y + p.cellH > texH) continue;   // celda fuera de la hoja
            SpriteFrame f;
            f.source = {x, y, p.cellW, p.cellH};
            f.width = p.cellW;
            f.height = p.cellH;
            f.pivotX = p.pivotX;
            f.pivotY = p.pivotY;
            f.visualBounds = {0, 0, p.cellW, p.cellH};
            out.push_back(f);
        }
    }
    return out;
}

}  // namespace district_fury
