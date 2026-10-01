#include "game/lab/KfReference.h"
#include "core/Platform.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <cstdio>
#include <unordered_map>
#include <cstdlib>
#include <string>
#include <vector>

namespace district_fury {
namespace {

constexpr int kMidp[8] = {0, 6, 3, 5, 2, 1, 7, 4};   // interno -> MIDP
constexpr float kTickSeconds = 0.05f;                 // el juego original corre a 20 ticks/s
// Colores originales. La copia "Rayder" (tint 1) cambia acentos rojos/naranjas
// por azul electrico y la ropa gris por negro; las formas no se tocan.

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

// Copia "Rayder clon BETA" (tint 2): cada color exacto de las piezas del heroe
// se cambia por el material del clon (data/kf_clone_palette.json, generado por
// tools/build_kf_clone_palette.py). Los efectos fuera de la tabla pasan a morado.
const std::unordered_map<uint32_t, uint32_t>& ClonePalette() {
    static std::unordered_map<uint32_t, uint32_t> table;
    static bool loaded = false;
    if (loaded) return table;
    loaded = true;
    std::string text;
    for (const char* path : {"data/kf_clone_palette.json", "../data/kf_clone_palette.json", "../../data/kf_clone_palette.json"})
        if (platform::LoadTextFile(path, text)) break;
    auto hex = [](const std::string& s, size_t at) { return (uint32_t)std::strtoul(s.substr(at, 6).c_str(), nullptr, 16); };
    for (size_t p = text.find('"'); p != std::string::npos; p = text.find('"', p + 1)) {
        // pares "rrggbb": "rrggbb"
        if (p + 7 < text.size() && text[p + 7] == '"') {
            const size_t q = text.find('"', p + 8);
            if (q != std::string::npos && q + 7 < text.size() && text[q + 7] == '"') {
                table[hex(text, p + 1)] = hex(text, q + 1);
                p = q + 7;
            }
        }
    }
    return table;
}

void CloneTint(Image& img) {
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto& table = ClonePalette();
    Color* px = (Color*)img.data;
    for (int i = 0; i < img.width * img.height; ++i) {
        if (px[i].a == 0) continue;
        const uint32_t key = (uint32_t(px[i].r) << 16) | (uint32_t(px[i].g) << 8) | px[i].b;
        const auto it = table.find(key);
        if (it != table.end()) {
            px[i].r = (it->second >> 16) & 255; px[i].g = (it->second >> 8) & 255; px[i].b = it->second & 255;
            continue;
        }
        const Vector3 hsv = ColorToHSV(px[i]);
        if ((hsv.x < 66.0f || hsv.x > 320.0f) && hsv.y > 0.35f) {   // fuego, cortes y destellos (rojo a amarillo) -> morado
            const unsigned char a = px[i].a;
            px[i] = ColorFromHSV(282.0f, hsv.y * 0.62f, std::min(1.0f, hsv.z * 1.2f + 0.1f));
            px[i].a = a;
        }
    }
}

// Escala x2 para pixel art (EPX/Scale2x): suaviza diagonales sin difuminar.
Image Scale2x(const Image& src) {
    Image dst = GenImageColor(src.width * 2, src.height * 2, BLANK);
    const Color* s = (const Color*)src.data;
    Color* d = (Color*)dst.data;
    auto at = [&](int x, int y) {
        x = std::clamp(x, 0, src.width - 1); y = std::clamp(y, 0, src.height - 1);
        return s[y * src.width + x];
    };
    auto eq = [](Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; };
    for (int y = 0; y < src.height; ++y)
        for (int x = 0; x < src.width; ++x) {
            const Color P = at(x, y), A = at(x, y - 1), B = at(x + 1, y), C = at(x - 1, y), D = at(x, y + 1);
            Color e0 = P, e1 = P, e2 = P, e3 = P;
            if (eq(C, A) && !eq(C, D) && !eq(A, B)) e0 = A;
            if (eq(A, B) && !eq(A, C) && !eq(B, D)) e1 = B;
            if (eq(D, C) && !eq(D, B) && !eq(C, A)) e2 = C;
            if (eq(B, D) && !eq(B, A) && !eq(D, C)) e3 = D;
            const int w = dst.width;
            d[(2 * y) * w + 2 * x] = e0; d[(2 * y) * w + 2 * x + 1] = e1;
            d[(2 * y + 1) * w + 2 * x] = e2; d[(2 * y + 1) * w + 2 * x + 1] = e3;
        }
    return dst;
}

// Poderes del Rayder clon (tint 3): todo color saturado pasa a rojo carmesi.
void RedFx(Image& img) {
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* px = (Color*)img.data;
    for (int i = 0; i < img.width * img.height; ++i) {
        if (px[i].a == 0) continue;
        const Vector3 hsv = ColorToHSV(px[i]);
        if (hsv.y < 0.28f) continue;   // blancos, grises y sombras quedan
        const unsigned char a = px[i].a;
        px[i] = ColorFromHSV(352.0f, std::min(1.0f, hsv.y * 1.05f), hsv.z);
        px[i].a = a;
    }
}

// Despiece del Rayder clon (tools/build_kf_clone_rig_canva.py): una pieza por
// cada pieza del cuerpo del heroe KF, a RES veces su resolucion.
struct Rig { bool ok = false; int res = 1, pad = 0; Image img{}; std::vector<Rectangle> rects; };
Rig LoadRig() {
    Rig r;
    std::string text;
    std::vector<unsigned char> bytes;
    const std::string base = "assets/characters/rayder/kf_clone/rig_rojo";
    for (const std::string& pre : {std::string(""), std::string("../"), std::string("../../")}) {
        if (!platform::LoadTextFile(pre + base + ".txt", text)) continue;
        if (!platform::LoadBinaryFile(pre + base + ".png", bytes)) continue;
        break;
    }
    if (text.empty() || bytes.empty()) return r;
    const char* c = text.c_str();
    char* end = nullptr;
    r.res = (int)std::strtol(c, &end, 10); c = end;
    r.pad = (int)std::strtol(c, &end, 10); c = end;
    while (true) {
        const long a = std::strtol(c, &end, 10); if (end == c) break; c = end;
        const long b = std::strtol(c, &end, 10); c = end;
        const long w = std::strtol(c, &end, 10); c = end;
        const long h = std::strtol(c, &end, 10); c = end;
        r.rects.push_back({(float)a, (float)b, (float)w, (float)h});
    }
    r.img = LoadImageFromMemory(".png", bytes.data(), (int)bytes.size());
    if (!r.img.data || r.res < 1) return r;
    ImageFormat(&r.img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    r.ok = true;
    return r;
}

void RayderTint(Image& img) {
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* px = (Color*)img.data;
    for (int i = 0; i < img.width * img.height; ++i) {
        if (px[i].a == 0) continue;
        const Vector3 hsv = ColorToHSV(px[i]);   // h 0-360, s 0-1, v 0-1
        const unsigned char a = px[i].a;
        const bool warm = hsv.x < 60.0f || hsv.x > 320.0f;
        if (hsv.y > 0.55f && warm && !(hsv.x > 12.0f && hsv.x < 38.0f && hsv.y < 0.7f && hsv.z > 0.6f)) {
            px[i] = ColorFromHSV(218.0f, std::min(1.0f, hsv.y + 0.1f), hsv.z);   // acentos -> azul
        } else if (hsv.y < 0.18f && hsv.z > 0.2f && hsv.z < 0.82f) {
            px[i] = ColorFromHSV(220.0f, 0.12f, hsv.z * 0.42f);                  // ropa gris -> negra
        }
        px[i].a = a;
    }
}

// Accion de la APK -> nombre de clip. Plantillas identificadas en
// docs/estudio_kf/FORMATO_Y_ACCIONES.md (heroe: sprite 0/2; enemigo: 6,10,...).
struct ClipMap { const char* name; int action; bool loop; };
constexpr ClipMap kHeroClips[] = {
    {"idle", 0, true}, {"walk", 1, true}, {"run", 2, true}, {"dash", 3, false},
    {"atk1", 6, false}, {"atk2", 7, false}, {"atk3", 8, false}, {"atk4", 9, false},
    {"special", 10, false}, {"hit", 11, false}, {"knockdown", 13, false}, {"air", 14, false},
    {"getup", 16, false}, {"defeat", 17, false}, {"super", 19, false},
    {"punch1", 6, false}, {"punch2", 7, false}, {"punch3", 8, false}, {"kick", 9, false},
    {"energy", 10, false}, {"dash_attack", 3, false}, {"rage_attack", 19, false}, {"finisher", 10, false},
    {"block", 15, true}, {"hit_high", 11, false}, {"hit_low", 11, false}, {"airborne", 14, false},
    {"recovery", 15, false},
};
constexpr ClipMap kEnemyClips[] = {
    {"idle", 0, true}, {"walk", 1, true}, {"run", 2, true}, {"dash", 2, false},
    {"atk1", 3, false}, {"atk2", 4, false}, {"atk3", 5, false}, {"atk4", 6, false},
    {"special", 7, false}, {"hit", 8, false}, {"knockdown", 13, false}, {"air", 14, false},
    {"getup", 20, false}, {"defeat", 11, false},
    {"punch1", 3, false}, {"punch2", 4, false}, {"punch3", 5, false}, {"kick", 6, false},
    {"energy", 7, false}, {"dash_attack", 2, false}, {"rage_attack", 6, false}, {"finisher", 7, false},
    {"block", 0, true}, {"hit_high", 8, false}, {"hit_low", 12, false}, {"airborne", 14, false},
    {"recovery", 0, false},
};
// Clips que el juego necesita siempre; si la accion no existe se usa "idle".
constexpr const char* kRequired[] = {"idle", "walk", "dash", "atk1", "atk2", "atk3", "atk4", "special", "hit",
    "knockdown", "air", "getup", "defeat", "punch1", "punch2", "punch3", "kick", "energy", "dash_attack",
    "rage_attack", "finisher", "block", "hit_high", "hit_low", "airborne", "recovery"};

constexpr KfRosterEntry kRoster[] = {
    {0, "KF HEROE (PELO BLANCO)", true, 110, 1.8f},
    {2, "KF HEROINA (PELIRROJA)", true, 100, 1.8f},
    {6, "KF MATON", false, 90, 1.8f},
    {10, "KF NAVAJERA", false, 80, 1.8f},
    {11, "KF SOLDADO", false, 95, 1.8f},
    {13, "KF RUBIA", false, 80, 1.8f},
    {14, "KF GORRA", false, 85, 1.8f},
    {16, "KF PELEADOR", false, 95, 1.8f},
    {18, "KF CUCHILLERO", false, 85, 1.8f},
    {12, "KF JEFE GARRA", false, 160, 1.3f},   // sprite de jefe: el doble de grande de origen
    {15, "KF BUFONA", false, 140, 1.8f},
    {1, "KF HEROE TRANSFORMADO", true, 120, 1.8f},     // el heroe transformado (llamas rojas, otros golpes)
    {3, "KF HEROINA TRANSFORMADA", true, 100, 1.8f},   // la heroina transformada (lanza y rayos)
    // COPIA del heroe (normal y transformado) vestida como el Rayder clon BETA:
    // mismas piezas, cuadros, tiempos y habilidades; el heroe original no se toca.
    {0, "RAYDER CLON BETA", true, 115, 0.9f, 2},
    {1, "RAYDER CLON BETA TRANSFORMADO", true, 125, 0.9f, 2},
    // Rayder clon (rojo): todos los cuadros y habilidades del heroe KF armados con
    // su propio despiece (hojas de Canva), a 3x; poderes en rojo.
    {0, "RAYDER CLON", true, 120, 0.6f, 3},
    {1, "RAYDER CLON TRANSFORMADO", true, 130, 0.6f, 3},
    // El sprite 33 no es luchador (vendedor/puesto del escenario): excluido.
};

KfReference Load(const KfRosterEntry& who) {
    KfReference out;
    std::vector<unsigned char> data;
    for (const char* path : {"apk_reference/king_fighter_iii/bin/animation.bin", "../apk_reference/king_fighter_iii/bin/animation.bin",
                             "../../apk_reference/king_fighter_iii/bin/animation.bin"}) {
        if (platform::AssetExists(path) && platform::LoadBinaryFile(path, data)) break;
    }
    if (data.empty()) { out.error = "no se encontro apk_reference/.../animation.bin"; return out; }
    if (!IsWindowReady()) { out.error = "sin ventana"; return out; }

    Reader r(data);
    const int ns = r.s16(), ni = r.s16();
    std::vector<int> offs(ns + 1); for (auto& v : offs) v = r.s32();
    const size_t base = r.p;
    if (who.sprite < 0 || who.sprite >= ns) { out.error = "sprite fuera de rango"; return out; }
    r.p = base + offs[(size_t)who.sprite];
    SpriteData sp = ParseSprite(r);
    std::vector<ClipMap> clipList = who.hero ? std::vector<ClipMap>(std::begin(kHeroClips), std::end(kHeroClips))
                                             : std::vector<ClipMap>(std::begin(kEnemyClips), std::end(kEnemyClips));
    // Habilidades propias (botones 1-6), con su fuego/estela dibujados dentro
    // de cada frame. Heroes: A20..A24 + transformacion A31 (aura dorada).
    // Enemigos/jefes: sus ataques completos (A3..A7 y extras largos).
    static const char* kSkillNames[] = {"skill1", "skill2", "skill3", "skill4", "skill5"};
    std::vector<int> skillActs;
    auto usable = [&](int a) { return a < (int)sp.actions.size() && sp.actions[(size_t)a].size() >= 4; };
    if (who.hero) {
        // Heroes: sus 5 habilidades (A20..A24). Las variantes que no las traen
        // completas usan sus ataques largos (A8, A9, A10, A3, A5...).
        for (int a : {20, 21, 22, 23, 24}) if (usable(a)) skillActs.push_back(a);
        for (int a : {9, 8, 10, 3, 5, 7, 6}) if (skillActs.size() < 5 && usable(a)) skillActs.push_back(a);
    } else {
        for (int a : {3, 4, 5, 6, 7}) if (usable(a)) skillActs.push_back(a);
        for (int a = 21; a < (int)sp.actions.size() && skillActs.size() < 5; ++a) if (usable(a)) skillActs.push_back(a);
        for (size_t k = 0; !skillActs.empty() && skillActs.size() < 5; ++k) skillActs.push_back(skillActs[k]);
    }
    for (size_t k = 0; k < skillActs.size() && k < 5; ++k) clipList.push_back({kSkillNames[k], skillActs[k], false});
    if (who.hero && usable(31)) clipList.push_back({"transform", 31, false});
    // Todas las habilidades y tecnicas (no solo 5): especiales con fuego, super,
    // despertar y acrobacias. Nombres en espanol por accion del heroe.
    struct Ab { int act; const char* name; };
    static const Ab kHeroAbilities[] = {
        {20, "SOMBRAS"}, {21, "ESTALLIDO"}, {22, "PILAR"}, {23, "LLAMARADA"}, {24, "FENIX"}, {25, "TORNADO"},
        {27, "BARRIDA"}, {30, "CIRCULO FUEGO"}, {46, "ERUPCION"}, {47, "MEDIA LUNA"}, {50, "TAJO"}, {51, "INFIERNO"},
        {19, "SUPER"}, {42, "DESPERTAR"}, {36, "VOLTERETA"}, {37, "GIRO"}, {38, "RODADA"}, {39, "PATADA AEREA"},
        {40, "CAIDA"}, {41, "TORBELLINO"}, {45, "MORTAL"}, {55, "CONTRA"}, {57, "SALTO TIGRE"},
        // formas transformadas y otras variantes: sus ataques largos
        {8, "COMBO 3"}, {9, "VOLTERETA"}, {10, "ESPECIAL"}, {34, "TECNICA"}, {35, "TECNICA"}};
    std::vector<int> abActs;
    for (const Ab& ab : kHeroAbilities) {
        if (!usable(ab.act)) continue;
        if (!who.hero && ab.act < 19) continue;
        if (ab.act < 19 && abActs.size() >= 10) continue;   // golpes normales: solo si faltan habilidades
        bool dup = false;
        for (int a : abActs) dup |= a == ab.act;
        if (dup) continue;
        abActs.push_back(ab.act);
        out.abilityNames.push_back(ab.name);
    }
    if (!who.hero) {   // enemigos/jefes: todos sus ataques largos
        for (int a : skillActs) {
            bool dup = false;
            for (int b : abActs) dup |= a == b;
            if (dup) continue;
            abActs.push_back(a);
            out.abilityNames.push_back("TECNICA");
        }
    }
    static const char* kAbClip[] = {"ab0", "ab1", "ab2", "ab3", "ab4", "ab5", "ab6", "ab7", "ab8", "ab9", "ab10", "ab11",
                                    "ab12", "ab13", "ab14", "ab15", "ab16", "ab17", "ab18", "ab19", "ab20", "ab21",
                                    "ab22", "ab23", "ab24", "ab25", "ab26", "ab27", "ab28", "ab29"};
    const size_t nAb = std::min(abActs.size(), sizeof(kAbClip) / sizeof(kAbClip[0]));
    out.abilityNames.resize(nAb);
    for (size_t k = 0; k < nAb; ++k) {
        out.abilityClips.push_back(kAbClip[k]);
        clipList.push_back({kAbClip[k], abActs[k], false});
    }
    const ClipMap* clips = clipList.data();
    const size_t nclips = clipList.size();
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
        if (who.tint == 2) {
            // Rayder clon BETA: hoja de piezas armada con los recortes del clon
            // (tools/build_kf_clone_pieces.py), mismas medidas y posiciones que la
            // del KF. Las demas imagenes (efectos, poderes) se pintan de morado.
            const std::string rel = "assets/characters/rayder/kf_clone/img_" + std::to_string(id) + ".png";
            std::vector<unsigned char> bytes;
            bool replaced = false;
            for (const std::string& path : {rel, "../" + rel, "../../" + rel}) {
                if (!platform::AssetExists(path) || !platform::LoadBinaryFile(path, bytes)) continue;
                Image rep = LoadImageFromMemory(".png", bytes.data(), (int)bytes.size());
                if (rep.data && rep.width == img.width && rep.height == img.height) {
                    ImageFormat(&rep, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    UnloadImage(img);
                    img = rep;
                    replaced = true;
                } else if (rep.data) {
                    UnloadImage(rep);
                }
                break;
            }
            if (!replaced) CloneTint(img);
        }
        sheets[id] = img;
        clipsOf[id] = im.clips;
        return &sheets[id];
    };
    (void)sheet;

    Rig rig;
    if (who.tint == 3) rig = LoadRig();
    const int R = rig.ok ? rig.res : 1;
    // Frames usados por los clips.
    std::map<int, int> frameSlot;   // frame APK -> indice en el atlas
    std::vector<Image> composed; std::vector<Vector2> origin;
    for (size_t ci = 0; ci < nclips; ++ci) {
        const ClipMap& c = clips[ci];
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
                if (rig.ok) {
                    // Rayder clon: piezas del cuerpo del despiece propio; el resto
                    // (efectos y poderes) ampliado y en rojo. Posiciones x RES.
                    if (imgId == 1 && clip < (int)rig.rects.size()) {
                        parts.push_back({TransformPiece(ImageFromImage(rig.img, rig.rects[(size_t)clip]), kMidp[tr]),
                                         x * R - rig.pad, y * R - rig.pad});
                    } else {
                        Image pc = ImageFromImage(*sh, src);
                        ImageResizeNN(&pc, pc.width * R, pc.height * R);
                        RedFx(pc);
                        parts.push_back({TransformPiece(pc, kMidp[tr]), x * R, y * R});
                    }
                    continue;
                }
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
            if (who.tint == 1) RayderTint(canvas);
            float upscale = 1.0f;
            if (rig.ok) {   // recorte al dibujo (el atlas a 3x no debe crecer de mas)
                const Rectangle bb = GetImageAlphaBorder(canvas, 0.02f);
                if (bb.width > 0 && bb.height > 0) {
                    ImageCrop(&canvas, bb);
                    x0 += (int)bb.x; y0 += (int)bb.y;
                }
            }
            if (who.tint == 2) {
                Image big = Scale2x(canvas);
                UnloadImage(canvas);
                canvas = big;
                upscale = 2.0f;
            }
            frameSlot[st.frame] = (int)composed.size();
            composed.push_back(canvas);
            origin.push_back({(float)-x0 * upscale, (float)-y0 * upscale});
        }
    }
    for (auto& kv : sheets) UnloadImage(kv.second);
    if (rig.ok) UnloadImage(rig.img);
    if (composed.empty()) { out.error = "sin frames"; return out; }

    // Empaquetado por estantes en un atlas de 2048 de ancho.
    const int atlasW = who.tint >= 2 ? 4096 : 2048; int x = 0, y = 0, rowH = 0;   // x2/x3: atlas mas ancho (<= 4096)
    std::vector<Rectangle> place;
    // Medidas logicas de cada cuadro (las que usa el dibujo); si el atlas no cabe
    // en 4096 px de alto se reducen las imagenes, no el tamano en pantalla.
    std::vector<Vector2> logical;
    for (auto& img : composed) logical.push_back({(float)img.width, (float)img.height});
    for (float shrink = 1.0f; shrink > 0.3f; shrink *= 0.85f) {
        place.clear(); x = 0; y = 0; rowH = 0;
        for (auto& lg : logical) {
            const int w = std::max(1, (int)(lg.x * shrink)), h = std::max(1, (int)(lg.y * shrink));
            if (x + w > atlasW) { x = 0; y += rowH + 2; rowH = 0; }
            place.push_back({(float)x, (float)y, (float)w, (float)h});
            x += w + 2; rowH = std::max(rowH, h);
        }
        if (y + rowH <= 4096) {
            if (shrink < 1.0f)
                for (size_t i = 0; i < composed.size(); ++i) ImageResize(&composed[i], (int)place[i].width, (int)place[i].height);
            break;
        }
    }
    Image atlas = GenImageColor(atlasW, y + rowH, BLANK);
    std::vector<SpriteFrame> frames;
    for (size_t i = 0; i < composed.size(); ++i) {
        ImageDraw(&atlas, composed[i], {0, 0, place[i].width, place[i].height}, place[i], WHITE);
        SpriteFrame f;
        f.source = place[i]; f.width = logical[i].x; f.height = logical[i].y;
        f.pivotX = origin[i].x; f.pivotY = origin[i].y;
        f.visualBounds = {0, 0, f.width, f.height};
        frames.push_back(f);
        UnloadImage(composed[i]);
    }
    Texture2D tex = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    // Si la GPU no acepta el atlas (celulares con limite de textura), se informa
    // en vez de dejar el personaje invisible.
    if (tex.id == 0) { out.error = "la GPU no cargo la textura del personaje"; return out; }
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);

    out.templ.Init(tex, 1, 1, true);
    out.templ.SetFrames(frames);
    for (size_t ci = 0; ci < nclips; ++ci) {
        const ClipMap& c = clips[ci];
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
    out.scale = who.scale;
    if (!out.templ.HasClip("idle")) { out.error = "sin clip idle"; return out; }
    for (const char* req : kRequired)
        if (!out.templ.HasClip(req)) { out.templ.namedClips[req] = out.templ.namedClips["idle"]; out.clipNames.push_back(req); }
    out.loaded = true;
    return out;
}

}  // namespace

int KfRosterCount() { return (int)(sizeof(kRoster) / sizeof(kRoster[0])); }

const KfRosterEntry& KfRoster(int index) {
    return kRoster[(size_t)std::clamp(index, 0, KfRosterCount() - 1)];
}

const KfReference& GetKfCharacter(int rosterIndex) {
    static std::map<int, KfReference> cache;
    const int idx = std::clamp(rosterIndex, 0, KfRosterCount() - 1);
    auto it = cache.find(idx);
    if (it != cache.end()) return it->second;
    static KfReference empty;
    if (!IsWindowReady()) return empty;
    KfReference& ref = cache[idx];
    ref = Load(kRoster[(size_t)idx]);
    if (!ref.loaded) TraceLog(LOG_WARNING, "KF %s: %s", kRoster[(size_t)idx].name, ref.error.c_str());
    return ref;
}

}  // namespace district_fury
