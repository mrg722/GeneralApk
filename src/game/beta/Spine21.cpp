#include "game/beta/Spine21.h"
#include "core/Json.h"
#include "core/Platform.h"
#include "rendering/AssetManager.h"
#include "rlgl.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace district_fury {
namespace spine21 {
namespace {

std::string ReadAsset(const std::string& rel) {
    std::string text;
    for (const std::string& p : {rel, "../" + rel, "../../" + rel})
        if (platform::LoadTextFile(p, text)) return text;
    return {};
}

float Num(const JsonValue* v, float fallback) { return v ? (float)v->NumberOr(fallback) : fallback; }

void ParseColor(const std::string& hex, float out[4]) {
    if (hex.size() < 8) return;
    for (int i = 0; i < 4; ++i) out[i] = (float)std::stoi(hex.substr((size_t)i * 2, 2), nullptr, 16) / 255.0f;
}

Curve ParseCurve(const JsonValue& key) {
    Curve c;
    const JsonValue* v = key.Find("curve");
    if (!v) return c;
    if (v->IsString() && v->string == "stepped") c.type = 1;
    else if (v->IsArray() && v->array.size() == 4) {
        c.type = 2;
        for (int i = 0; i < 4; ++i) c.c[i] = (float)v->array[(size_t)i].NumberOr(0);
    }
    return c;
}

float CurvePercent(const Curve& c, float p) {
    if (c.type == 1) return 0.0f;
    if (c.type != 2) return p;
    // x(t) = p por biseccion; devuelve y(t).
    float lo = 0, hi = 1;
    for (int i = 0; i < 24; ++i) {
        const float t = (lo + hi) * 0.5f;
        const float x = 3 * (1 - t) * (1 - t) * t * c.c[0] + 3 * (1 - t) * t * t * c.c[2] + t * t * t;
        if (x < p) lo = t; else hi = t;
    }
    const float t = (lo + hi) * 0.5f;
    return 3 * (1 - t) * (1 - t) * t * c.c[1] + 3 * (1 - t) * t * t * c.c[3] + t * t * t;
}

// Clave anterior, siguiente (o -1) y porcentaje para `time`.
template <class K> int Sample(const std::vector<K>& keys, float time, float& pct) {
    pct = 0;
    if (keys.empty()) return -1;
    if (time <= keys.front().time) return 0;
    if (time >= keys.back().time) return (int)keys.size() - 1;
    for (size_t i = 0; i + 1 < keys.size(); ++i)
        if (keys[i].time <= time && time < keys[i + 1].time) {
            const float p = (time - keys[i].time) / std::max(1e-6f, keys[i + 1].time - keys[i].time);
            pct = CurvePercent(keys[i].curve, p);
            return (int)i;
        }
    return (int)keys.size() - 1;
}

}  // namespace

int SlotAttachments::Find(const std::string& name) const {
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i].name == name) return (int)i;
    return -1;
}

int SkeletonData::FindAnimation(const std::string& name) const {
    for (size_t i = 0; i < animations.size(); ++i)
        if (animations[i].name == name) return (int)i;
    return -1;
}

bool SkeletonData::Load(const std::string& dir, const std::string& name) {
    // ---- atlas (formato libgdx)
    const std::string atlas = ReadAsset(dir + "/" + name + ".atlas");
    if (atlas.empty()) { error = "falta " + dir + "/" + name + ".atlas"; return false; }
    {
        std::istringstream in(atlas);
        std::string line;
        bool inPage = false;
        Region cur;
        std::string curName;
        bool haveRegion = false;
        auto flush = [&]() {
            if (!haveRegion) return;
            const Texture2D& t = pages[(size_t)cur.page];
            const float pw = (float)std::max(1, t.width), ph = (float)std::max(1, t.height);
            // u,v,u2,v2 a partir de xy/size (cur.u/v guardan xy en pixeles por ahora)
            const float x = cur.u, y = cur.v;
            cur.u = x / pw; cur.v = y / ph;
            if (cur.rotate) { cur.u2 = (x + cur.h) / pw; cur.v2 = (y + cur.w) / ph; }
            else { cur.u2 = (x + cur.w) / pw; cur.v2 = (y + cur.h) / ph; }
            regions.push_back(cur);
            regionNames.push_back(curName);
            haveRegion = false;
        };
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto first = line.find_first_not_of(" \t");
            if (first == std::string::npos) { flush(); inPage = false; continue; }
            const std::string t = line.substr(first);
            const auto colon = t.find(':');
            if (!inPage) {
                // nombre de la hoja
                pages.push_back(AssetManager::Get().GetTextureByPath(dir + "/" + t));
                inPage = true;
                continue;
            }
            if (colon == std::string::npos) {
                flush();
                cur = Region{};
                cur.page = (int)pages.size() - 1;
                curName = t;
                haveRegion = true;
                continue;
            }
            if (!haveRegion) continue;   // propiedades de la hoja (size, format...)
            const std::string key = t.substr(0, colon);
            std::string val = t.substr(colon + 1);
            for (char& ch : val) if (ch == ',') ch = ' ';
            std::istringstream vs(val);
            if (key == "rotate") cur.rotate = val.find("true") != std::string::npos;
            else if (key == "xy") vs >> cur.u >> cur.v;
            else if (key == "size") vs >> cur.w >> cur.h;
            else if (key == "orig") vs >> cur.ow >> cur.oh;
            else if (key == "offset") vs >> cur.offx >> cur.offy;
        }
        flush();
    }
    for (const Texture2D& t : pages)
        if (t.id == 0) { error = "no cargo una hoja del atlas de " + name; return false; }

    // ---- esqueleto
    const std::string text = ReadAsset(dir + "/" + name + ".json");
    JsonValue root;
    if (text.empty() || !ParseJson(text, root, error)) { if (error.empty()) error = "falta json"; return false; }
    auto boneIndex = [&](const std::string& n) {
        for (size_t i = 0; i < bones.size(); ++i) if (bones[i].name == n) return (int)i;
        return -1;
    };
    auto slotIndex = [&](const std::string& n) {
        for (size_t i = 0; i < slots.size(); ++i) if (slots[i].name == n) return (int)i;
        return -1;
    };
    if (const JsonValue* bs = root.Find("bones"))
        for (const JsonValue& b : bs->array) {
            BoneData d;
            d.name = b.Find("name") ? b.Find("name")->string : "";
            if (const JsonValue* p = b.Find("parent")) d.parent = boneIndex(p->string);
            d.x = Num(b.Find("x"), 0); d.y = Num(b.Find("y"), 0);
            d.rotation = Num(b.Find("rotation"), 0);
            d.scaleX = Num(b.Find("scaleX"), 1); d.scaleY = Num(b.Find("scaleY"), 1);
            d.length = Num(b.Find("length"), 0);
            if (const JsonValue* v = b.Find("inheritScale")) d.inheritScale = v->BoolOr(true);
            if (const JsonValue* v = b.Find("inheritRotation")) d.inheritRotation = v->BoolOr(true);
            bones.push_back(d);
        }
    if (const JsonValue* ss = root.Find("slots"))
        for (const JsonValue& s : ss->array) {
            SlotData d;
            d.name = s.Find("name")->string;
            d.bone = std::max(0, boneIndex(s.Find("bone")->string));
            if (const JsonValue* a = s.Find("attachment")) d.attachment = a->StringOr("");
            if (const JsonValue* c = s.Find("color")) ParseColor(c->string, d.color);
            slots.push_back(d);
        }
    if (const JsonValue* ik = root.Find("ik"))
        for (const JsonValue& k : ik->array) {
            IkData d;
            d.name = k.Find("name") ? k.Find("name")->string : "";
            if (const JsonValue* bs = k.Find("bones"); bs && bs->array.size() == 2) {
                d.parent = boneIndex(bs->array[0].string);
                d.child = boneIndex(bs->array[1].string);
            }
            if (const JsonValue* t = k.Find("target")) d.target = boneIndex(t->string);
            if (const JsonValue* bp = k.Find("bendPositive")) d.bend = bp->BoolOr(true) ? 1 : -1;
            d.mix = Num(k.Find("mix"), 1);
            if (d.parent >= 0 && d.child >= 0 && d.target >= 0) iks.push_back(d);
        }
    skin.assign(slots.size(), {});
    const JsonValue* skins = root.Find("skins");
    const JsonValue* def = skins ? skins->Find("default") : nullptr;
    if (def)
        for (const auto& [slotName, atts] : def->object) {
            const int si = slotIndex(slotName);
            if (si < 0) continue;
            for (const auto& [attName, a] : atts.object) {
                Attachment at;
                at.name = attName;
                const std::string type = a.Find("type") ? a.Find("type")->string : "region";
                at.type = type == "mesh" ? 1 : type == "skinnedmesh" ? 2 : type == "region" ? 0 : -1;
                if (at.type < 0) continue;
                const std::string path = a.Find("path") ? a.Find("path")->string : attName;
                for (size_t r = 0; r < regionNames.size(); ++r) if (regionNames[r] == path) at.region = (int)r;
                if (at.region < 0) continue;
                const Region& reg = regions[(size_t)at.region];
                at.x = Num(a.Find("x"), 0); at.y = Num(a.Find("y"), 0);
                at.rotation = Num(a.Find("rotation"), 0);
                at.scaleX = Num(a.Find("scaleX"), 1); at.scaleY = Num(a.Find("scaleY"), 1);
                at.width = Num(a.Find("width"), reg.ow); at.height = Num(a.Find("height"), reg.oh);
                if (const JsonValue* c = a.Find("color")) ParseColor(c->string, at.color);
                if (at.type != 0) {
                    for (const JsonValue& v : a.Find("vertices")->array) at.vertices.push_back((float)v.number);
                    for (const JsonValue& v : a.Find("triangles")->array) at.triangles.push_back((unsigned short)v.number);
                    const auto& ru = a.Find("uvs")->array;
                    const float w = reg.u2 - reg.u, h = reg.v2 - reg.v;
                    for (size_t i = 0; i + 1 < ru.size(); i += 2) {
                        const float ux = (float)ru[i].number, uy = (float)ru[i + 1].number;
                        if (reg.rotate) { at.uvs.push_back(reg.u + uy * w); at.uvs.push_back(reg.v + h - ux * h); }
                        else { at.uvs.push_back(reg.u + ux * w); at.uvs.push_back(reg.v + uy * h); }
                    }
                    at.vertexCount = (int)ru.size() / 2;
                }
                skin[(size_t)si].list.push_back(std::move(at));
            }
        }
    // ---- animaciones
    if (const JsonValue* anims = root.Find("animations"))
        for (const auto& [animName, a] : anims->object) {
            Animation an;
            an.name = animName;
            auto upd = [&](float t) { an.duration = std::max(an.duration, t); };
            if (const JsonValue* bs = a.Find("bones"))
                for (const auto& [bn, tls] : bs->object) {
                    BoneTimeline tl;
                    tl.bone = boneIndex(bn);
                    if (tl.bone < 0) continue;
                    if (const JsonValue* r = tls.Find("rotate"))
                        for (const JsonValue& k : r->array) {
                            RotKey key{Num(k.Find("time"), 0), Num(k.Find("angle"), 0), ParseCurve(k)};
                            tl.rotate.push_back(key); upd(key.time);
                        }
                    for (const char* kind : {"translate", "scale"})
                        if (const JsonValue* r = tls.Find(kind))
                            for (const JsonValue& k : r->array) {
                                const float def0 = std::string(kind) == "scale" ? 1.0f : 0.0f;
                                VecKey key{Num(k.Find("time"), 0), Num(k.Find("x"), def0), Num(k.Find("y"), def0), ParseCurve(k)};
                                (std::string(kind) == "scale" ? tl.scale : tl.translate).push_back(key);
                                upd(key.time);
                            }
                    an.bones.push_back(std::move(tl));
                }
            if (const JsonValue* ss = a.Find("slots"))
                for (const auto& [sn, tls] : ss->object) {
                    SlotTimeline tl;
                    tl.slot = slotIndex(sn);
                    if (tl.slot < 0) continue;
                    if (const JsonValue* r = tls.Find("attachment"))
                        for (const JsonValue& k : r->array) {
                            AttKey key;
                            key.time = Num(k.Find("time"), 0);
                            if (const JsonValue* n = k.Find("name")) key.name = n->StringOr("");
                            tl.attachment.push_back(key); upd(key.time);
                        }
                    if (const JsonValue* r = tls.Find("color"))
                        for (const JsonValue& k : r->array) {
                            ColorKey key;
                            key.time = Num(k.Find("time"), 0);
                            if (const JsonValue* c = k.Find("color")) ParseColor(c->string, key.rgba);
                            key.curve = ParseCurve(k);
                            tl.color.push_back(key); upd(key.time);
                        }
                    an.slots.push_back(std::move(tl));
                }
            if (const JsonValue* ff = a.Find("ffd"))
                if (const JsonValue* dskin = ff->Find("default"))
                    for (const auto& [sn, atts] : dskin->object) {
                        const int si = slotIndex(sn);
                        if (si < 0) continue;
                        for (const auto& [an2, keys] : atts.object) {
                            FfdTimeline tl;
                            tl.slot = si;
                            tl.attachment = skin[(size_t)si].Find(an2);
                            if (tl.attachment < 0) continue;
                            const Attachment& at = skin[(size_t)si].list[(size_t)tl.attachment];
                            size_t n = at.vertices.size();
                            if (at.type == 2) {   // skinned: 2 valores por (hueso, vertice)
                                n = 0;
                                for (size_t i = 0; i < at.vertices.size();) {
                                    const int c = (int)at.vertices[i];
                                    n += (size_t)c * 2;
                                    i += 1 + (size_t)c * 4;
                                }
                            }
                            for (const JsonValue& k : keys.array) {
                                FfdKey key;
                                key.time = Num(k.Find("time"), 0);
                                key.curve = ParseCurve(k);
                                key.v.assign(n, 0.0f);
                                const int off = (int)Num(k.Find("offset"), 0);
                                if (const JsonValue* vs = k.Find("vertices"))
                                    for (size_t i = 0; i < vs->array.size() && off + i < n; ++i) key.v[(size_t)off + i] = (float)vs->array[i].number;
                                tl.keys.push_back(std::move(key));
                                upd(tl.keys.back().time);
                            }
                            an.ffd.push_back(std::move(tl));
                        }
                    }
            const JsonValue* dor = a.Find("drawOrder");
            if (!dor) dor = a.Find("draworder");
            if (dor)
                for (const JsonValue& k : dor->array) {
                    DrawOrderKey key;
                    key.time = Num(k.Find("time"), 0);
                    upd(key.time);
                    if (const JsonValue* offs = k.Find("offsets"); offs && !offs->array.empty()) {
                        const int n = (int)slots.size();
                        std::vector<int> draw((size_t)n, -1), unchanged;
                        int orig = 0;
                        for (const JsonValue& o : offs->array) {
                            const int si = slotIndex(o.Find("slot")->string);
                            if (si < 0) continue;
                            while (orig != si) unchanged.push_back(orig++);
                            const int at = orig + (int)Num(o.Find("offset"), 0);
                            if (at >= 0 && at < n) draw[(size_t)at] = orig;
                            ++orig;
                        }
                        while (orig < n) unchanged.push_back(orig++);
                        for (int j = n - 1; j >= 0; --j)
                            if (draw[(size_t)j] == -1 && !unchanged.empty()) { draw[(size_t)j] = unchanged.back(); unchanged.pop_back(); }
                        key.order = std::move(draw);
                    }
                    an.drawOrder.push_back(std::move(key));
                }
            if (const JsonValue* ikt = a.Find("ik"))
                for (const auto& [ikName, keys] : ikt->object) {
                    IkTimeline tl;
                    tl.ik = -1;
                    for (size_t i = 0; i < iks.size(); ++i) if (iks[i].name == ikName) tl.ik = (int)i;
                    if (tl.ik < 0) continue;
                    for (const JsonValue& k : keys.array) {
                        IkKey key;
                        key.time = Num(k.Find("time"), 0);
                        key.mix = Num(k.Find("mix"), 1);
                        if (const JsonValue* bp = k.Find("bendPositive")) key.bend = bp->BoolOr(true) ? 1 : -1;
                        key.curve = ParseCurve(k);
                        tl.keys.push_back(key);
                        upd(key.time);
                    }
                    an.ik.push_back(std::move(tl));
                }
            if (const JsonValue* evs = a.Find("events"))
                for (const JsonValue& k : evs->array) {
                    Event e{Num(k.Find("time"), 0), k.Find("name") ? k.Find("name")->string : ""};
                    an.events.push_back(e);
                    upd(e.time);
                }
            animations.push_back(std::move(an));
        }
    return !bones.empty() && !slots.empty();
}

// ------------------------------------------------------------------ instancia

bool Skeleton::Play(const std::string& name, bool shouldLoop) {
    if (!data) return false;
    const int i = data->FindAnimation(name);
    if (i < 0) return false;
    anim = i;
    current = name;
    time = 0;
    loop = shouldLoop;
    fired.clear();
    return true;
}

void Skeleton::Update(float dt) {
    fired.clear();
    const Animation* a = data ? data->GetAnimation(anim) : nullptr;
    if (!a) return;
    const float before = time;
    time += dt;
    for (const Event& e : a->events)
        if (e.time >= before && e.time < time) fired.push_back(e.name);
    if (loop && a->duration > 0 && time >= a->duration) time = std::fmod(time, a->duration);
}

bool Skeleton::Finished() const {
    const Animation* a = data ? data->GetAnimation(anim) : nullptr;
    return !a || (!loop && time >= a->duration);
}

void Skeleton::BuildTriangles(std::vector<Tri>& out) const {
    out.clear();
    if (!data) return;
    const SkeletonData& d = *data;
    const Animation* a = d.GetAnimation(anim);
    const float t = a ? std::min(time, a->duration) : 0.0f;
    struct Local { float x, y, rot, sx, sy; };
    struct World { float x, y, sx, sy, rot, m00, m01, m10, m11; };
    std::vector<Local> local(d.bones.size());
    for (size_t i = 0; i < d.bones.size(); ++i) {
        const BoneData& b = d.bones[i];
        local[i] = {b.x, b.y, b.rotation, b.scaleX, b.scaleY};
    }
    float pct = 0;
    if (a)
        for (const BoneTimeline& tl : a->bones) {
            const BoneData& b = d.bones[(size_t)tl.bone];
            Local& L = local[(size_t)tl.bone];
            int k = Sample(tl.rotate, t, pct);
            if (k >= 0) {
                float ang = tl.rotate[(size_t)k].angle;
                if ((size_t)k + 1 < tl.rotate.size() && t > tl.rotate[(size_t)k].time) {
                    float diff = tl.rotate[(size_t)k + 1].angle - ang;
                    diff = std::fmod(diff + 180.0f, 360.0f);
                    if (diff < 0) diff += 360.0f;
                    diff -= 180.0f;
                    ang += diff * pct;
                }
                L.rot = b.rotation + ang;
            }
            k = Sample(tl.translate, t, pct);
            if (k >= 0) {
                float x = tl.translate[(size_t)k].x, y = tl.translate[(size_t)k].y;
                if ((size_t)k + 1 < tl.translate.size() && t > tl.translate[(size_t)k].time) {
                    x += (tl.translate[(size_t)k + 1].x - x) * pct;
                    y += (tl.translate[(size_t)k + 1].y - y) * pct;
                }
                L.x = b.x + x; L.y = b.y + y;
            }
            k = Sample(tl.scale, t, pct);
            if (k >= 0) {
                float x = tl.scale[(size_t)k].x, y = tl.scale[(size_t)k].y;
                if ((size_t)k + 1 < tl.scale.size() && t > tl.scale[(size_t)k].time) {
                    x += (tl.scale[(size_t)k + 1].x - x) * pct;
                    y += (tl.scale[(size_t)k + 1].y - y) * pct;
                }
                L.sx = b.scaleX * x; L.sy = b.scaleY * y;
            }
        }
    std::vector<World> world(d.bones.size());
    auto updateWorld = [&]() {
        for (size_t i = 0; i < d.bones.size(); ++i) {
            const BoneData& b = d.bones[i];
            const Local& L = local[i];
            World w{};
            if (b.parent >= 0) {
                const World& P = world[(size_t)b.parent];
                w.x = L.x * P.m00 + L.y * P.m01 + P.x;
                w.y = L.x * P.m10 + L.y * P.m11 + P.y;
                w.sx = b.inheritScale ? P.sx * L.sx : L.sx;
                w.sy = b.inheritScale ? P.sy * L.sy : L.sy;
                w.rot = b.inheritRotation ? P.rot + L.rot : L.rot;
            } else {
                w.x = L.x; w.y = L.y; w.sx = L.sx; w.sy = L.sy; w.rot = L.rot;
            }
            const float r = w.rot * DEG2RAD, c = std::cos(r), s = std::sin(r);
            w.m00 = c * w.sx; w.m10 = s * w.sx; w.m01 = -s * w.sy; w.m11 = c * w.sy;
            world[i] = w;
        }
    };
    updateWorld();
    // IK de dos huesos (IkConstraint.apply2 de spine-runtimes 2.1).
    for (size_t ci = 0; ci < d.iks.size(); ++ci) {
        const IkData& ik = d.iks[ci];
        float mix = ik.mix;
        int bend = ik.bend;
        if (a)
            for (const IkTimeline& tl : a->ik) {
                if (tl.ik != (int)ci || tl.keys.empty()) continue;
                const int k = Sample(tl.keys, t, pct);
                mix = tl.keys[(size_t)k].mix;
                if ((size_t)k + 1 < tl.keys.size() && t > tl.keys[(size_t)k].time) mix += (tl.keys[(size_t)k + 1].mix - mix) * pct;
                bend = tl.keys[(size_t)k].bend;
            }
        if (mix <= 0.0f || d.bones[(size_t)ik.child].parent != ik.parent) continue;
        Local& LP = local[(size_t)ik.parent];
        Local& LC = local[(size_t)ik.child];
        const World& T = world[(size_t)ik.target];
        float tx = T.x, ty = T.y;
        const int pp = d.bones[(size_t)ik.parent].parent;
        if (pp >= 0) {
            const World& W = world[(size_t)pp];
            const float dx = tx - W.x, dy = ty - W.y;
            const float det = W.m00 * W.m11 - W.m01 * W.m10;
            if (std::fabs(det) < 1e-9f) continue;
            const float lx = (W.m11 * dx - W.m01 * dy) / det, ly = (W.m00 * dy - W.m10 * dx) / det;
            tx = (lx - LP.x) * W.sx;
            ty = (ly - LP.y) * W.sy;
        } else {
            tx -= LP.x;
            ty -= LP.y;
        }
        const World& WP = world[(size_t)ik.parent];
        const float childX = LC.x * WP.sx, childY = LC.y * WP.sy;
        const float offset = std::atan2(childY, childX);
        const float len1 = std::sqrt(childX * childX + childY * childY);
        const float len2 = d.bones[(size_t)ik.child].length * world[(size_t)ik.child].sx;
        const float denom = 2 * len1 * len2;
        auto wrap = [](float r) { while (r > 180) r -= 360; while (r < -180) r += 360; return r; };
        if (denom < 0.0001f) {
            LC.rot += (std::atan2(ty, tx) * RAD2DEG - LP.rot - LC.rot) * mix;
        } else {
            const float cosv = std::clamp((tx * tx + ty * ty - len1 * len1 - len2 * len2) / denom, -1.0f, 1.0f);
            const float childAngle = std::acos(cosv) * (float)bend;
            const float adjacent = len1 + len2 * cosv, opposite = len2 * std::sin(childAngle);
            const float parentAngle = std::atan2(ty * adjacent - tx * opposite, tx * adjacent + ty * opposite);
            const float rp = wrap((parentAngle - offset) * RAD2DEG - LP.rot);
            const float rc = wrap((childAngle + offset) * RAD2DEG - LC.rot);
            LP.rot += rp * mix;
            LC.rot += rc * mix;
        }
        updateWorld();
    }
    // Slots: adjunto y color de la animacion.
    std::vector<int> att(d.slots.size(), -1);
    std::vector<std::array<float, 4>> col(d.slots.size());
    for (size_t i = 0; i < d.slots.size(); ++i) {
        att[i] = d.skin[i].Find(d.slots[i].attachment);
        for (int c = 0; c < 4; ++c) col[i][(size_t)c] = d.slots[i].color[c];
    }
    if (a)
        for (const SlotTimeline& tl : a->slots) {
            if (!tl.attachment.empty() && t >= tl.attachment.front().time) {
                const AttKey* key = &tl.attachment.front();
                for (const AttKey& k : tl.attachment) if (k.time <= t) key = &k;
                att[(size_t)tl.slot] = key->name.empty() ? -1 : d.skin[(size_t)tl.slot].Find(key->name);
            }
            const int k = Sample(tl.color, t, pct);
            if (k >= 0) {
                for (int c = 0; c < 4; ++c) {
                    float v = tl.color[(size_t)k].rgba[c];
                    if ((size_t)k + 1 < tl.color.size() && t > tl.color[(size_t)k].time) v += (tl.color[(size_t)k + 1].rgba[c] - v) * pct;
                    col[(size_t)tl.slot][(size_t)c] = v;
                }
            }
        }
    std::vector<int> order(d.slots.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = (int)i;
    if (a) {
        const DrawOrderKey* key = nullptr;
        for (const DrawOrderKey& k : a->drawOrder) if (k.time <= t) key = &k;
        if (key && key->order.size() == order.size()) order = key->order;
    }
    std::vector<float> deform;
    std::vector<Vector2> verts;
    for (int si : order) {
        const int ai = att[(size_t)si];
        if (ai < 0) continue;
        const Attachment& at = d.skin[(size_t)si].list[(size_t)ai];
        const Region& reg = d.regions[(size_t)at.region];
        const World& B = world[(size_t)d.slots[(size_t)si].bone];
        float rgba[4];
        for (int c = 0; c < 4; ++c) rgba[c] = col[(size_t)si][(size_t)c] * at.color[c];
        verts.clear();
        std::vector<float> uv;
        std::vector<unsigned short> tris;
        if (at.type == 0) {
            const float rsx = at.width / std::max(1.0f, reg.ow) * at.scaleX;
            const float rsy = at.height / std::max(1.0f, reg.oh) * at.scaleY;
            const float lx = -at.width / 2 * at.scaleX + reg.offx * rsx, ly = -at.height / 2 * at.scaleY + reg.offy * rsy;
            const float lx2 = lx + reg.w * rsx, ly2 = ly + reg.h * rsy;
            const float r = at.rotation * DEG2RAD, c = std::cos(r), s = std::sin(r);
            const Vector2 lp[4] = {{lx * c - ly * s + at.x, ly * c + lx * s + at.y},
                                   {lx * c - ly2 * s + at.x, ly2 * c + lx * s + at.y},
                                   {lx2 * c - ly2 * s + at.x, ly2 * c + lx2 * s + at.y},
                                   {lx2 * c - ly * s + at.x, ly * c + lx2 * s + at.y}};
            for (const Vector2& p : lp) verts.push_back({p.x * B.m00 + p.y * B.m01 + B.x, p.x * B.m10 + p.y * B.m11 + B.y});
            if (reg.rotate) uv = {reg.u2, reg.v2, reg.u, reg.v2, reg.u, reg.v, reg.u2, reg.v};
            else uv = {reg.u, reg.v2, reg.u, reg.v, reg.u2, reg.v, reg.u2, reg.v2};
            tris = {0, 1, 2, 2, 3, 0};
        } else {
            deform.clear();
            if (a)
                for (const FfdTimeline& tl : a->ffd) {
                    if (tl.slot != si || tl.attachment != ai || tl.keys.empty()) continue;
                    const int k = Sample(tl.keys, t, pct);
                    deform = tl.keys[(size_t)k].v;
                    if ((size_t)k + 1 < tl.keys.size() && t > tl.keys[(size_t)k].time)
                        for (size_t i = 0; i < deform.size() && i < tl.keys[(size_t)k + 1].v.size(); ++i)
                            deform[i] += (tl.keys[(size_t)k + 1].v[i] - deform[i]) * pct;
                    break;
                }
            if (at.type == 1) {
                for (size_t i = 0; i + 1 < at.vertices.size(); i += 2) {
                    float x = at.vertices[i], y = at.vertices[i + 1];
                    if (i + 1 < deform.size()) { x += deform[i]; y += deform[i + 1]; }
                    verts.push_back({x * B.m00 + y * B.m01 + B.x, x * B.m10 + y * B.m11 + B.y});
                }
            } else {
                size_t f = 0;
                for (size_t i = 0; i < at.vertices.size();) {
                    const int n = (int)at.vertices[i++];
                    float wx = 0, wy = 0;
                    for (int j = 0; j < n && i + 3 < at.vertices.size() + 1; ++j) {
                        const int bi = (int)at.vertices[i];
                        float vx = at.vertices[i + 1], vy = at.vertices[i + 2];
                        const float w = at.vertices[i + 3];
                        i += 4;
                        if (f + 1 < deform.size()) { vx += deform[f]; vy += deform[f + 1]; }
                        f += 2;
                        if (bi < 0 || bi >= (int)world.size()) continue;
                        const World& W = world[(size_t)bi];
                        wx += (vx * W.m00 + vy * W.m01 + W.x) * w;
                        wy += (vx * W.m10 + vy * W.m11 + W.y) * w;
                    }
                    verts.push_back({wx, wy});
                }
            }
            uv = at.uvs;
            tris = at.triangles;
        }
        for (size_t i = 0; i + 2 < tris.size(); i += 3) {
            Tri tr{};
            tr.page = reg.page;
            bool ok = true;
            for (int k = 0; k < 3; ++k) {
                const unsigned short id = tris[i + (size_t)k];
                if (id >= verts.size() || (size_t)id * 2 + 1 >= uv.size()) { ok = false; break; }
                tr.p[k] = {verts[id].x, -verts[id].y};   // y hacia abajo
                tr.uv[k] = {uv[(size_t)id * 2], uv[(size_t)id * 2 + 1]};
            }
            if (!ok) continue;
            for (int c = 0; c < 4; ++c) tr.rgba[c] = rgba[c];
            out.push_back(tr);
        }
    }
}

void Skeleton::Draw(Vector2 pos, float scale, bool flipX, Color tint) const {
    std::vector<Tri> tris;
    BuildTriangles(tris);
    if (tris.empty()) return;
    // Los triangulos se emiten en cualquier sentido (espejos, escalas negativas):
    // sin descarte de caras mientras se dibujan.
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    const float sx = flipX ? -scale : scale;
    int page = -1;
    for (const Tri& tr : tris) {
        if (tr.page != page) {
            if (page >= 0) rlEnd();
            page = tr.page;
            rlCheckRenderBatchLimit(3 * 64);
            // rlSetTexture abre una llamada nueva (con el modo anterior) y rlBegin
            // con otro modo reinicia la textura: textura, modo y textura otra vez.
            rlSetTexture(data->pages[(size_t)page].id);
            rlBegin(RL_TRIANGLES);
            rlSetTexture(data->pages[(size_t)page].id);
        } else {
            rlCheckRenderBatchLimit(3);   // si vacia el lote, rlgl restaura modo y textura
        }
        rlColor4ub((unsigned char)(tr.rgba[0] * tint.r), (unsigned char)(tr.rgba[1] * tint.g),
                   (unsigned char)(tr.rgba[2] * tint.b), (unsigned char)(tr.rgba[3] * tint.a));
        for (int k = 0; k < 3; ++k) {
            rlTexCoord2f(tr.uv[k].x, tr.uv[k].y);
            rlVertex2f(pos.x + tr.p[k].x * sx, pos.y + tr.p[k].y * scale);
        }
    }
    if (page >= 0) rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
}

Rectangle Skeleton::Bounds() const {
    std::vector<Tri> tris;
    BuildTriangles(tris);
    if (tris.empty()) return {0, 0, 0, 0};
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (const Tri& tr : tris)
        for (const Vector2& p : tr.p) { x0 = std::min(x0, p.x); y0 = std::min(y0, p.y); x1 = std::max(x1, p.x); y1 = std::max(y1, p.y); }
    return {x0, y0, x1 - x0, y1 - y0};
}

}  // namespace spine21
}  // namespace district_fury
