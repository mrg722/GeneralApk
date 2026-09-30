#include "rendering/Animator.h"
#include "rendering/SpriteManifest.h"
#include <cassert>
#include <cstdio>

using namespace district_fury;

int main() {
    // Manifiesto real del repositorio (el test corre desde la raiz).
    SpriteManifest real;
    const bool loaded = real.LoadFromFile("data/sprite_manifest.json");
    if (!loaded) std::fprintf(stderr, "manifest: %s\n", real.Error().c_str());
    assert(loaded);

    const AtlasProfile* rayden = real.FindAtlas("rayden");
    assert(rayden);
    assert(rayden->cellW == 128 && rayden->cellH == 128);
    assert(rayden->pivotX == 64 && rayden->pivotY == 126);   // centro inferior

    // Las celdas de otro personaje pueden ser distintas: no hay tamano global.
    assert(real.FindAtlas("rayden_legacy_96")->cellW == 96);
    assert(real.FindAtlas("punk")->cellW == 128);

    const ClipMap* clips = real.FindClips("rayden");
    assert(clips && clips->count("punch1") && clips->count("hit_high") && clips->count("knockdown"));

    Animator anim;
    assert(anim.ApplyProfile(*rayden, *clips, 512, 512));
    assert(anim.frames.size() == 16);
    for (const SpriteFrame& f : anim.frames) assert(f.pivotX == 64 && f.pivotY == 126 && f.width == 128);

    // Duraciones asincronas por cuadro: kick = [0.11, 0.31].
    assert(anim.PlayNamed("kick"));
    assert(anim.currentFrame == 10);
    anim.Update(0.10f); assert(anim.currentFrame == 10);
    anim.Update(0.02f); assert(anim.currentFrame == 11);
    anim.Update(0.25f); assert(anim.currentFrame == 11);
    anim.Update(0.10f); assert(anim.isFinished);
    assert(!anim.PlayNamed("no_existe"));

    // Celda asimetrica y hoja de otro tamano, con clip de largo libre (20 frames).
    SpriteManifest custom;
    const char* json = R"({"atlases":{"heroe":{"path":"x.png","cell":[96,160],"pivot":[48,158]}},
        "clips":{"heroe":{"combo":{"frames":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],"fps":20,"loop":false}}}})";
    assert(custom.LoadFromString(json));
    Animator wide;
    assert(wide.ApplyProfile(*custom.FindAtlas("heroe"), *custom.FindClips("heroe"), 96 * 5, 160 * 4));
    assert(wide.frames.size() == 20 && wide.frames[7].source.x == 96 * 2 && wide.frames[7].source.y == 160);
    assert(wide.frames[0].height == 160 && wide.frames[0].pivotY == 158);

    // Validaciones: pivote fuera de la celda y clip fuera de la hoja se rechazan.
    SpriteManifest bad;
    assert(!bad.LoadFromString(R"({"atlases":{"a":{"cell":[64,64],"pivot":[64,70]}}})"));
    Animator strict;
    ClipMap oob; oob["x"] = ClipDef{{99}, {0.1f}, true};
    assert(!strict.ApplyProfile(*rayden, oob, 512, 512));

    std::puts("sprite_manifest_tests OK");
    return 0;
}
