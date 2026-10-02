#pragma once
#include "raylib.h"
#include <memory>
#include <string>
#include <vector>

namespace district_fury {
namespace spine21 {

// Runtime minimo de Spine 2.1 (formato JSON de los jefes de nuevosSprites):
// huesos con rotate/translate/scale (curvas lineal, escalonada y bezier),
// slots (adjunto y color), adjuntos region / mesh / skinnedmesh, ffd
// (deformaciones) y drawOrder. Dibuja las mallas con rlgl directamente desde
// las hojas originales del atlas: tamano y forma exactos, sin hornear cuadros.
// Formulas de spine-runtimes 2.1 (Bone.updateWorldTransform, RegionAttachment,
// MeshAttachment, SkinnedMeshAttachment). Mismo algoritmo que tools/beta/spine21.py.

struct Curve { int type = 0; float c[4] = {0, 0, 1, 1}; };   // 0 lineal, 1 escalonada, 2 bezier
struct RotKey { float time = 0, angle = 0; Curve curve; };
struct VecKey { float time = 0, x = 0, y = 0; Curve curve; };
struct ColorKey { float time = 0; float rgba[4] = {1, 1, 1, 1}; Curve curve; };
struct AttKey { float time = 0; std::string name; };
struct FfdKey { float time = 0; std::vector<float> v; Curve curve; };

struct BoneData {
    std::string name;
    int parent = -1;
    float x = 0, y = 0, rotation = 0, scaleX = 1, scaleY = 1, length = 0;
    bool inheritScale = true, inheritRotation = true;
};
struct SlotData {
    std::string name;
    int bone = 0;
    std::string attachment;
    float color[4] = {1, 1, 1, 1};
};
struct Region {
    int page = 0;
    float u = 0, v = 0, u2 = 0, v2 = 0;
    bool rotate = false;
    float w = 0, h = 0, ow = 0, oh = 0, offx = 0, offy = 0;
};
struct Attachment {
    int type = 0;            // 0 region, 1 mesh, 2 skinnedmesh
    std::string name;
    int region = -1;
    float x = 0, y = 0, rotation = 0, scaleX = 1, scaleY = 1, width = 0, height = 0;
    std::vector<float> vertices;   // mesh: x,y por vertice; skinned: crudo (n, [hueso,x,y,peso]*n)
    std::vector<float> uvs;        // uv en la hoja (ya convertidas)
    std::vector<unsigned short> triangles;
    int vertexCount = 0;
    float color[4] = {1, 1, 1, 1};
};
struct SlotAttachments {
    std::vector<Attachment> list;
    int Find(const std::string& name) const;
};
// IK de dos huesos (padre -> hijo apuntando al hueso objetivo).
struct IkData { std::string name; int parent = -1, child = -1, target = -1; int bend = 1; float mix = 1; };
struct IkKey { float time = 0, mix = 1; int bend = 1; Curve curve; };
struct IkTimeline { int ik = 0; std::vector<IkKey> keys; };
struct BoneTimeline { int bone = 0; std::vector<RotKey> rotate; std::vector<VecKey> translate, scale; };
struct SlotTimeline { int slot = 0; std::vector<AttKey> attachment; std::vector<ColorKey> color; };
struct FfdTimeline { int slot = 0; int attachment = 0; std::vector<FfdKey> keys; };
struct DrawOrderKey { float time = 0; std::vector<int> order; };
struct Event { float time = 0; std::string name; };
struct Animation {
    std::string name;
    float duration = 0;
    std::vector<BoneTimeline> bones;
    std::vector<SlotTimeline> slots;
    std::vector<FfdTimeline> ffd;
    std::vector<DrawOrderKey> drawOrder;
    std::vector<Event> events;
    std::vector<IkTimeline> ik;
};

class SkeletonData {
public:
    // `dir`/`name`: dir/name.json + dir/name.atlas (+ hojas .png del atlas).
    bool Load(const std::string& dir, const std::string& name);
    int FindAnimation(const std::string& name) const;
    const Animation* GetAnimation(int i) const { return i >= 0 && i < (int)animations.size() ? &animations[(size_t)i] : nullptr; }

    std::vector<BoneData> bones;
    std::vector<SlotData> slots;
    std::vector<IkData> iks;
    std::vector<SlotAttachments> skin;   // por slot
    std::vector<Animation> animations;
    std::vector<Region> regions;
    std::vector<std::string> regionNames;
    std::vector<Texture2D> pages;
    std::string error;
};

// Una instancia que reproduce animaciones de un SkeletonData compartido.
class Skeleton {
public:
    void SetData(std::shared_ptr<const SkeletonData> d) { data = std::move(d); }
    bool Valid() const { return data != nullptr; }
    bool Play(const std::string& anim, bool loop);
    void Update(float dt);
    bool Finished() const;
    float Time() const { return time; }
    const std::string& Current() const { return current; }
    // Dibuja con el origen del esqueleto en `pos`; flipX refleja horizontalmente.
    void Draw(Vector2 pos, float scale, bool flipX, Color tint) const;
    // Limites del dibujo en la pose actual (relativos al origen, y hacia abajo).
    Rectangle Bounds() const;
    // Eventos cruzados en el ultimo Update (nombres).
    const std::vector<std::string>& FiredEvents() const { return fired; }

private:
    struct Tri { int page; Vector2 p[3]; Vector2 uv[3]; float rgba[4]; };
    void BuildTriangles(std::vector<Tri>& out) const;
    std::shared_ptr<const SkeletonData> data;
    int anim = -1;
    std::string current;
    float time = 0;
    bool loop = false;
    std::vector<std::string> fired;
};

}  // namespace spine21
}  // namespace district_fury
