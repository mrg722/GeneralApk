// F0.3: manifiesto extensible ("includes" -> data/manifest/*.json). Sin ventana.
#include "rendering/Animator.h"
#include "rendering/SpriteManifest.h"
#include <cassert>
#include <cstdio>
#include <string>

using namespace district_fury;

static bool HasWarning(const SpriteManifest& m, const std::string& part) {
    for (const std::string& w : m.Warnings()) if (w.find(part) != std::string::npos) return true;
    return false;
}

static const char* kBase = R"({"atlases":{
    "rayden":{"path":"r.png","cell":[128,128],"pivot":[64,126],"grid":[4,6]},
    "otro":{"path":"o.png","cell":[64,64],"pivot":[32,62],"grid":[2,2]}},
  "clips":{
    "rayden":{"idle":{"frames":[0,1],"fps":8}},
    "otro":{"idle":{"frames":[0],"fps":8},"golpe":{"frames":[1,2],"fps":10}}}})";

int main() {
    // 1) Un include AGREGA clips (tambien a un juego nuevo).
    {
        SpriteManifest m;
        assert(m.LoadFromString(kBase));
        assert(m.ApplyInclude(R"({"clips":{"rayden":{"giro":{"frames":[3,4,5],"durations":[0.05,0.06,0.07],"loop":false}},
                                          "nuevo":{"x":{"frames":[0]}}}})", "inc.json"));
        const ClipMap* r = m.FindClips("rayden");
        assert(r && r->size() == 2 && r->count("giro") && r->at("giro").fromInclude && !r->at("giro").loop);
        assert(r->at("idle").frames.size() == 2 && !r->at("idle").fromInclude);
        assert(m.FindClips("nuevo") && m.FindClips("nuevo")->count("x"));
        assert(m.Warnings().empty());
    }
    // 2) No pisa un clip existente (aviso) salvo override, y override PROHIBIDO en rayden.
    {
        SpriteManifest m;
        assert(m.LoadFromString(kBase));
        assert(m.ApplyInclude(R"({"clips":{"otro":{"idle":{"frames":[3]}}}})", "a.json"));
        assert(m.FindClips("otro")->at("idle").frames[0] == 0 && HasWarning(m, "ya existe"));
        assert(m.ApplyInclude(R"({"clips":{"otro":{"idle":{"frames":[3],"override":true}}}})", "b.json"));
        assert(m.FindClips("otro")->at("idle").frames[0] == 3);
        assert(m.ApplyInclude(R"({"clips":{"rayden":{"idle":{"frames":[9],"override":true}}}})", "c.json"));
        assert(m.FindClips("rayden")->at("idle").frames[0] == 0 && HasWarning(m, "prohibido en el juego rayden"));
        // Tampoco con override se puede AGREGAR a rayden un clip marcado override.
        assert(m.ApplyInclude(R"({"clips":{"rayden":{"nuevo":{"frames":[1],"override":true}}}})", "d.json"));
        assert(!m.FindClips("rayden")->count("nuevo"));
        // JSON roto: false, sin tocar lo cargado.
        assert(!m.ApplyInclude("{ roto", "e.json") && m.FindClips("rayden")->size() == 1);
    }
    // 3) Escala / desfase por cuadro; un clip mal formado se descarta solo.
    {
        SpriteManifest m;
        assert(m.LoadFromString(kBase));
        assert(m.ApplyInclude(R"({"clips":{"otro":{
            "a":{"frames":[0,1],"scale":0.62},
            "b":{"frames":[0,1],"scales":[1.0,0.5],"offsets":[[2,0],[-3,4]]},
            "mal":{"frames":[0,1],"scales":[1.0]}}}})", "s.json"));
        const ClipMap* o = m.FindClips("otro");
        assert(o->at("a").scales.size() == 2 && o->at("a").scales[1] == 0.62f);
        assert(o->at("b").offsets.size() == 2 && o->at("b").offsets[1].first == -3 && o->at("b").offsets[1].second == 4);
        assert(!o->count("mal") && HasWarning(m, "descartado"));
        Animator an;
        assert(an.ApplyProfile(*m.FindAtlas("otro"), *o, 128, 128));
        assert(an.PlayNamed("b"));
        float k = 0; Vector2 off{};
        an.currentClip.FrameAdjust(1, k, off);
        assert(k == 0.5f && off.x == -3 && off.y == 4);
        an.currentClip.FrameAdjust(0, k, off);
        assert(k == 1.0f && off.x == 2);
        assert(an.PlayNamed("idle") && !an.currentClip.HasFrameAdjust());
    }
    // 4) Indice fuera de la rejilla: el clip del include se descarta, el personaje carga.
    {
        SpriteManifest m;
        assert(m.LoadFromString(kBase));
        assert(m.ApplyInclude(R"({"clips":{"otro":{"lejos":{"frames":[0,99]},"bien":{"frames":[3]}}}})", "g.json"));
        Animator an;
        assert(an.ApplyProfile(*m.FindAtlas("otro"), *m.FindClips("otro"), 128, 128));   // 4 celdas
        assert(!an.HasClip("lejos") && an.HasClip("bien") && an.HasClip("golpe"));
        // Un clip del manifiesto principal fuera de la rejilla sigue invalidando el perfil.
        SpriteManifest strict;
        assert(strict.LoadFromString(R"({"atlases":{"otro":{"cell":[64,64],"pivot":[32,62]}},
                                        "clips":{"otro":{"idle":{"frames":[99]}}}})"));
        Animator s;
        assert(!s.ApplyProfile(*strict.FindAtlas("otro"), *strict.FindClips("otro"), 128, 128));
    }
    // 5) Manifiesto real: los includes existen y no avisan; los 19 clips de rayden
    //    quedan identicos a cargar sin includes.
    {
        SpriteManifest real;
        const bool ok = real.LoadFromFile("data/sprite_manifest.json");
        if (!ok) std::fprintf(stderr, "manifest: %s\n", real.Error().c_str());
        assert(ok);
        for (const std::string& w : real.Warnings()) std::fprintf(stderr, "aviso: %s\n", w.c_str());
        assert(real.Warnings().empty());
        std::string text;
        FILE* f = std::fopen("data/sprite_manifest.json", "rb");
        assert(f);
        char buf[4096]; size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
        std::fclose(f);
        SpriteManifest bare;
        assert(bare.LoadFromString(text, false));
        const ClipMap* a = real.FindClips("rayden");
        const ClipMap* b = bare.FindClips("rayden");
        assert(a && b && a->size() >= 19 && b->size() == 19);   // F1.3: el include agrega clips nuevos a rayden
        for (const auto& kv : *b) {
            const ClipDef& x = a->at(kv.first);
            assert(x.frames == kv.second.frames && x.durations == kv.second.durations && x.loop == kv.second.loop);
            assert(x.scales.empty() && x.offsets.empty() && !x.fromInclude);
        }
        assert(real.FindClips("brakk_v2") && bare.FindClips("brakk_v2") &&
               real.FindClips("brakk_v2")->size() == bare.FindClips("brakk_v2")->size());
    }
    // 6) Include que no existe: aviso, la carga sigue.
    {
        SpriteManifest m;
        assert(m.LoadFromString(R"({"includes":["data/manifest/no_existe.json"],"clips":{"x":{"a":{"frames":[0]}}}})"));
        assert(HasWarning(m, "no encontrado") && m.FindClips("x"));
    }
    std::puts("manifest_include_tests OK");
    return 0;
}
