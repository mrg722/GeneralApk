#include "rendering/SpriteManifest.h"
#include "core/Json.h"
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
    std::ifstream in(path, std::ios::binary);
    if (!in) { error_ = "no se pudo abrir " + path; return false; }
    std::stringstream ss;
    ss << in.rdbuf();
    return LoadFromString(ss.str());
}

bool SpriteManifest::LoadFromString(const std::string& json) {
    atlases_.clear();
    clips_.clear();
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
