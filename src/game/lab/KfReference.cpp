#include "game/lab/KfReference.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>

namespace district_fury {
namespace {

constexpr int kMidp[8] = {0, 6, 3, 5, 2, 1, 7, 4};   // interno -> MIDP
constexpr float kTickSeconds = 0.05f;                 // el juego original corre a 20 ticks/s
constexpr float kHueShift = 200.0f;                   // "pintado": giro de tono, formas intactas

struct Reader {
    const std::vector<unsigned char>& d; size_t p = 0; bool bad = false;
    explicit Reader(const std::vector<unsigned char>& data) : d(data) {}
    bool need(size_t n) { if (p + n > d.size()) { bad = true; return false; } return true; }
    int u8() { return need(1) ? d[p++] : 0; }
    int s8() { int v = u8(); return v > 127 ? v - 256 : v; }
    int s16() { if (!need(2)) return 0; int v = (d[p] << 8) | d[p + 1]; p += 2; return v > 32767 ? v - 65536 : v; }
    int u16() { return s16() & 0xFFFF; }
    int32_t s32() { if (!need(4)) return 0; uint32_t v = (uint32_t(d[p]) << 24) | (d[p + 1] << 16) | (d[p + 2] << 8) | d[p + 3]; p += 4; return (int32_t)v; }
    void skip(size_t n) { if (need(n)) p += n; }
};

struct SpriteData {
    std::vector<int> modules;                         // (imgId<<16)|clip
    struct Frame { int start, count; };
    std::vector<Frame> frames;
    std::vector<uint32_t> pieces;
    struct Step { int frame, ticks; };
    std::vector<std::vector<Step>> actions;
};

struct ImageData { std::vector<Rectangle> clips; std::vector<unsigned char> png; };

SpriteData ParseSprite(Reader& r) {
    SpriteData s;
    const int n = r.s16();
    for (int i = 0; i < n; ++i) r.s16();                              // imagenes usadas
    for (int i = 0; i < n; ++i) { const int c = r.s16(); for (int k = 0; k < c; ++k) r.s16(); }
    for (int i = 0; i < n; ++i) { const int c = r.u8(); for (int k = 0; k < c; ++k) r.s16(); }
    const int nm = r.s16(); for (int i = 0; i < nm; ++i) s.modules.push_back(r.s32());
    const int nf = r.s16();
    r.skip((size_t)nf * 2);                                            // indices de cajas
    std::vector<int> count(nf); for (int i = 0; i < nf; ++i) count[i] = r.u8();
    std::vector<int> start(nf + 1); for (int i = 0; i <= nf; ++i) start[i] = r.s16();
    const int np = r.s16(); for (int i = 0; i < np; ++i) s.pieces.push_back((uint32_t)r.s32());
    for (int i = 0; i < nf; ++i) s.frames.push_back({start[i], count[i]});
    const int na = r.s16();
    r.skip((size_t)na);                                                // propiedad por accion
    std::vector<int> len(na); for (int i = 0; i < na; ++i) len[i] = r.u8();
    std::vector<int> astart(na + 1); for (int i = 0; i <= na; ++i) astart[i] = r.s16();
    const int nh = r.s16(); std::vector<int> h(nh * 2); for (auto& v : h) v = r.u16();
    for (int a = 0; a < na; ++a) {
        std::vector<SpriteData::Step> steps;
        for (int k = 0; k < len[a]; ++k) {
            const size_t idx = (size_t)astart[a] + 2 * k;
            if (idx >= h.size()) break;
            steps.push_back({h[idx] & 1023, (h[idx] >> 10) & 31});
        }
        s.actions.push_back(steps);
    }
    return s;   // cajas y propiedades no se usan en el bot de referencia
}

ImageData ParseImage(Reader& r) {
    ImageData im;
    const bool hasClips = r.s8() == 0;
    const int n = r.s16();
    if (hasClips) {
        std::vector<std::pair<int, int>> wh(n), xy(n);
        for (auto& v : wh) { v.first = r.s16(); v.second = r.s16(); }
        for (auto& v : xy) { v.first = r.s16(); v.second = r.s16(); }
        for (int i = 0; i < n; ++i) im.clips.push_back({(float)xy[i].first, (float)xy[i].second, (float)wh[i].first, (float)wh[i].second});
    }
    const int npal = r.s8(); const int palSize = r.s16();
    if (palSize > 0) r.skip((size_t)npal * palSize);
    const int len = r.s32();
    if (len > 0 && r.need((size_t)len)) { im.png.assign(r.d.begin() + (long)r.p, r.d.begin() + (long)r.p + len); r.p += len; }
    return im;
}

std::map<int, std::vector<Rectangle>> clipsOf;   // recortes por imagen APK

Image TransformPiece(Image piece, int midp) {
    switch (midp) {
        case 2: ImageFlipHorizontal(&piece); break;
        case 1: ImageFlipVertical(&piece); break;
        case 3: ImageFlipHorizontal(&piece); ImageFlipVertical(&piece); break;
        case 5: ImageRotateCW(&piece); break;
        case 6: ImageRotateCCW(&piece); break;
        case 7: ImageFlipHorizontal(&piece); ImageRotateCW(&piece); break;
        case 4: ImageFlipHorizontal(&piece); ImageRotateCCW(&piece); break;
        default: break;
    }
    return piece;
}

void HueShift(Image& img, float degrees) {
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* px = (Color*)img.data;
    for (int i = 0; i < img.width * img.height; ++i) {
        if (px[i].a == 0) continue;
        Vector3 hsv = ColorToHSV(px[i]);
        const unsigned char a = px[i].a;
        px[i] = ColorFromHSV(std::fmod(hsv.x + degrees, 360.0f), hsv.y, hsv.z);
        px[i].a = a;
    }
}

// Accion de la APK -> clip del bot (ver docs: clasificacion de acciones del sprite 0).
struct ClipMap { const char* name; int action; bool loop; };
constexpr ClipMap kClips[] = {
    {"idle", 0, true}, {"walk", 1, true}, {"run", 2, true}, {"dash", 3, false},
    {"atk1", 6, false}, {"atk2", 7, false}, {"atk3", 8, false}, {"atk4", 9, false},
    {"special", 10, false}, {"hit", 11, false}, {"knockdown", 13, false}, {"air", 14, false},
    {"getup", 16, false}, {"defeat", 17, false}, {"super", 19, false},
};

KfReference Load() {
    KfReference out;
    std::vector<unsigned char> data;
    for (const char* path : {"apk_reference/king_fighter_iii/bin/animation.bin", "../apk_reference/king_fighter_iii/bin/animation.bin",
                             "../../apk_reference/king_fighter_iii/bin/animation.bin"}) {
        std::ifstream in(path, std::ios::binary);
        if (in) { data.assign(std::istreambuf_iterator<char>(in), {}); break; }
    }
    if (data.empty()) { out.error = "no se encontro apk_reference/.../animation.bin"; return out; }
    if (!IsWindowReady()) { out.error = "sin ventana"; return out; }

    Reader r(data);
    const int ns = r.s16(), ni = r.s16();
    std::vector<int> offs(ns + 1); for (auto& v : offs) v = r.s32();
    const size_t base = r.p;
    r.p = base + offs[0];
    SpriteData sp = ParseSprite(r);
    r.p = base + offs[ns];
    std::vector<int> ioffs(ni + 1); for (auto& v : ioffs) v = r.s32();
    const size_t ibase = r.p;
    if (r.bad) { out.error = "animation.bin truncado"; return out; }

    std::map<int, Image> sheets;
    auto sheet = [&](int id) -> Image* {
        auto it = sheets.find(id);
        if (it != sheets.end()) return &it->second;
        if (id < 0 || id >= ni) return nullptr;
        Reader ir(data); ir.p = ibase + ioffs[id];
        ImageData im = ParseImage(ir);
        if (im.png.empty()) return nullptr;
        Image img = LoadImageFromMemory(".png", im.png.data(), (int)im.png.size());
        if (!img.data) return nullptr;
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        sheets[id] = img;
        clipsOf[id] = im.clips;
        return &sheets[id];
    };
    (void)sheet;

    // Frames usados por los clips.
    std::map<int, int> frameSlot;   // frame APK -> indice en el atlas
    std::vector<Image> composed; std::vector<Vector2> origin;
    for (const ClipMap& c : kClips) {
        if (c.action >= (int)sp.actions.size()) continue;
        for (const auto& st : sp.actions[(size_t)c.action]) {
            if (frameSlot.count(st.frame) || st.frame >= (int)sp.frames.size()) continue;
            const auto& f = sp.frames[(size_t)st.frame];
            struct Part { Image img; int x, y; };
            std::vector<Part> parts;
            for (int k = 0; k < f.count; ++k) {
                const size_t pi = (size_t)f.start + k;
                if (pi >= sp.pieces.size()) break;
                const uint32_t v = sp.pieces[pi];
                const int mod = (v >> 18) & 1023; int x = (v >> 9) & 511; int y = v & 511; const int tr = (v >> 28) & 7;
                if (x & 256) x -= 512;
                if (y & 256) y -= 512;
                if (mod >= (int)sp.modules.size()) continue;
                const int imgId = (sp.modules[(size_t)mod] >> 16) & 0xFFFF, clip = sp.modules[(size_t)mod] & 0xFFFF;
                Image* sh = sheet(imgId);
                if (!sh) continue;
                const auto& clips = clipsOf[imgId];
                const Rectangle src = clips.empty() ? Rectangle{0, 0, (float)sh->width, (float)sh->height}
                                                    : (clip < (int)clips.size() ? clips[(size_t)clip] : Rectangle{0, 0, 0, 0});
                if (src.width <= 0 || src.height <= 0) continue;
                parts.push_back({TransformPiece(ImageFromImage(*sh, src), kMidp[tr]), x, y});
            }
            if (parts.empty()) continue;
            int x0 = 1 << 20, y0 = 1 << 20, x1 = -(1 << 20), y1 = -(1 << 20);
            for (auto& p : parts) { x0 = std::min(x0, p.x); y0 = std::min(y0, p.y); x1 = std::max(x1, p.x + p.img.width); y1 = std::max(y1, p.y + p.img.height); }
            Image canvas = GenImageColor(x1 - x0, y1 - y0, BLANK);
            for (auto& p : parts) {
                ImageDraw(&canvas, p.img, {0, 0, (float)p.img.width, (float)p.img.height},
                          {(float)(p.x - x0), (float)(p.y - y0), (float)p.img.width, (float)p.img.height}, WHITE);
                UnloadImage(p.img);
            }
            HueShift(canvas, kHueShift);
            frameSlot[st.frame] = (int)composed.size();
            composed.push_back(canvas);
            origin.push_back({(float)-x0, (float)-y0});
        }
    }
    for (auto& kv : sheets) UnloadImage(kv.second);
    if (composed.empty()) { out.error = "sin frames"; return out; }

    // Empaquetado por estantes en un atlas de 2048 de ancho.
    const int atlasW = 2048; int x = 0, y = 0, rowH = 0;
    std::vector<Rectangle> place;
    for (auto& img : composed) {
        if (x + img.width > atlasW) { x = 0; y += rowH + 2; rowH = 0; }
        place.push_back({(float)x, (float)y, (float)img.width, (float)img.height});
        x += img.width + 2; rowH = std::max(rowH, img.height);
    }
    Image atlas = GenImageColor(atlasW, y + rowH, BLANK);
    std::vector<SpriteFrame> frames;
    for (size_t i = 0; i < composed.size(); ++i) {
        ImageDraw(&atlas, composed[i], {0, 0, place[i].width, place[i].height}, place[i], WHITE);
        SpriteFrame f;
        f.source = place[i]; f.width = place[i].width; f.height = place[i].height;
        f.pivotX = origin[i].x; f.pivotY = origin[i].y;
        f.visualBounds = {0, 0, f.width, f.height};
        frames.push_back(f);
        UnloadImage(composed[i]);
    }
    Texture2D tex = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);

    out.templ.Init(tex, 1, 1, true);
    out.templ.SetFrames(frames);
    for (const ClipMap& c : kClips) {
        if (c.action >= (int)sp.actions.size()) continue;
        AnimationClip clip{0, 0, kTickSeconds, c.loop};
        for (const auto& st : sp.actions[(size_t)c.action]) {
            auto it = frameSlot.find(st.frame);
            if (it == frameSlot.end()) continue;
            const float dur = std::max(1, st.ticks) * kTickSeconds;
            if (!clip.frames.empty() && clip.frames.back() == it->second) clip.durations.back() += dur;
            else { clip.frames.push_back(it->second); clip.durations.push_back(dur); }
        }
        if (clip.frames.empty()) continue;
        clip.startFrame = clip.frames.front(); clip.endFrame = clip.frames.back();
        out.templ.namedClips[c.name] = clip;
        out.clipNames.push_back(c.name);
    }
    out.loaded = true;
    return out;
}

}  // namespace

const KfReference& GetKfReference() {
    static KfReference ref;
    static bool tried = false;
    if (!tried && IsWindowReady()) { tried = true; ref = Load(); if (!ref.loaded) TraceLog(LOG_WARNING, "KF referencia: %s", ref.error.c_str()); }
    return ref;
}

}  // namespace district_fury
