#include "rendering/AssetManager.h"
#include <algorithm>
#include <queue>
#include <string>
#include <vector>

namespace district_fury {
namespace {
std::string ResolveAssetPath(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) if (FileExists(path.c_str())) return path;
    return candidates.empty()?std::string{}:candidates.front();
}
Texture2D LoadRequiredTexture(const char* key,const std::vector<std::string>& candidates,TextureFilter filter){
    const std::string path=ResolveAssetPath(candidates);
    if(path.empty()||!FileExists(path.c_str())){TraceLog(LOG_WARNING,"District Fury asset missing: %s",key);return Texture2D{0};}
    Texture2D texture=LoadTexture(path.c_str());
    if(texture.id==0){TraceLog(LOG_WARNING,"District Fury asset failed to load: %s (%s)",key,path.c_str());return Texture2D{0};}
    SetTextureFilter(texture,filter);return texture;
}
bool NearWhite(const Color& c){const int mx=std::max({(int)c.r,(int)c.g,(int)c.b});const int mn=std::min({(int)c.r,(int)c.g,(int)c.b});return c.a>0&&mn>=235&&(mx-mn)<=18;}

// DF-013: corrige el "aura blanca" / "mal cortados" reportado en los enemigos.
// Diagnostico (verificado sobre los PNG reales en assets/enemies/): los atlas
// YA vienen con canal alfa parcial en los bordes (anti-aliasing), pero el color
// RGB de esos pixeles de alfa bajo esta contaminado hacia blanco -- es decir,
// fueron generados/compuestos sobre un lienzo blanco y el color se guardo sin
// "des-mezclar" (straight alpha sin decontaminar). Al dibujarlos con blending
// normal sobre el fondo oscuro del juego, esa contaminacion se ve como un halo
// blanco alrededor de la silueta (y en huecos internos como axilas o entre
// piernas). El flood-fill de borde de mas abajo no lo arregla porque esos
// pixeles no son "blanco puro": son una mezcla progresiva.
//
// La correccion es la tecnica estandar de "unpremultiply contra un fondo
// conocido": para cada pixel con alfa parcial, se asume
//   color_guardado = color_real * a + blanco * (1 - a)
// y se despeja color_real = (color_guardado - blanco*(1-a)) / a.
// Los pixeles casi invisibles (alfa muy bajo) se descartan directamente:
// despejar la ecuacion ahi es numericamente inestable y no aportan nada visible.
void DecontaminateWhiteMatte(Image& image, int alphaFloor = 14) {
    const int w = image.width, h = image.height;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Color c = GetImageColor(image, x, y);
            if (c.a == 0) continue;
            if (c.a < alphaFloor) { ImageDrawPixel(&image, x, y, {0, 0, 0, 0}); continue; }
            if (c.a >= 255) continue;  // opaco: nada que decontaminar
            const float alpha = static_cast<float>(c.a) / 255.0f;
            const float bleed = 255.0f * (1.0f - alpha);
            const auto fix = [&](unsigned char channel) -> unsigned char {
                const float trueValue = (static_cast<float>(channel) - bleed) / alpha;
                return static_cast<unsigned char>(std::clamp(trueValue, 0.0f, 255.0f));
            };
            ImageDrawPixel(&image, x, y, {fix(c.r), fix(c.g), fix(c.b), c.a});
        }
    }
}

// DF-013.2 (19-09): detecta si un atlas todavia trae el matte blanco. Desde
// esta version los PNG del repo ya vienen corregidos en disco (ver
// tools/fix_white_matte.py), asi que volver a decontaminarlos oscureceria el
// contorno de mas. Se mantiene la correccion en runtime para cualquier asset
// nuevo que llegue sucio, pero solo se aplica si de verdad hace falta.
bool HasWhiteMatte(const Image& image){
    const int w=image.width,h=image.height; int bright=0,sampled=0;
    for(int y=1;y<h-1;++y)for(int x=1;x<w-1;++x){
        const Color c=GetImageColor(image,x,y);
        if(c.a==0)continue;
        bool touchesTransparent=false;
        for(int dy=-1;dy<=1&&!touchesTransparent;++dy)for(int dx=-1;dx<=1;++dx){
            if(dx==0&&dy==0)continue;
            if(GetImageColor(image,x+dx,y+dy).a==0){touchesTransparent=true;break;}
        }
        if(!touchesTransparent)continue;
        ++sampled;
        if(((int)c.r+(int)c.g+(int)c.b)/3>=175)++bright;
    }
    return sampled>0&&(float)bright/(float)sampled>0.18f;
}

Texture2D LoadEnemyTexture(const char* key,const std::vector<std::string>& candidates){
    const std::string path=ResolveAssetPath(candidates);
    if(path.empty()||!FileExists(path.c_str())){TraceLog(LOG_WARNING,"District Fury enemy asset missing: %s",key);return Texture2D{0};}
    Image image=LoadImage(path.c_str());
    if(image.data==nullptr){TraceLog(LOG_WARNING,"District Fury enemy asset failed to load: %s (%s)",key,path.c_str());return Texture2D{0};}
    ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if(image.data==nullptr){UnloadImage(image);return Texture2D{0};}
    const int w=image.width,h=image.height; std::vector<unsigned char> visited((size_t)w*h,0); std::queue<int> queue;
    const auto enqueue=[&](int x,int y){const int idx=y*w+x;if(!visited[(size_t)idx]&&NearWhite(GetImageColor(image,x,y))){visited[(size_t)idx]=1;queue.push(idx);}};
    // Paso 1: si existiera un fondo blanco solido (assets futuros sin alfa
    // previo), se sigue eliminando por flood-fill desde el borde.
    for(int x=0;x<w;++x){enqueue(x,0);enqueue(x,h-1);} for(int y=0;y<h;++y){enqueue(0,y);enqueue(w-1,y);}
    const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    while(!queue.empty()){const int idx=queue.front();queue.pop();const int x=idx%w,y=idx/w;ImageDrawPixel(&image,x,y,{0,0,0,0});for(int i=0;i<4;++i){const int nx=x+dx[i],ny=y+dy[i];if(nx>=0&&nx<w&&ny>=0&&ny<h)enqueue(nx,ny);}}
    // Paso 2: decontaminar el matte blanco de los bordes anti-aliasados, solo
    // si el atlas llega sucio (los del repo ya vienen limpios en disco).
    const bool dirty=HasWhiteMatte(image);
    if(dirty)DecontaminateWhiteMatte(image);
    // Paso 3: red de seguridad. Si tras decontaminar todavia queda algun
    // pixel casi blanco pegado a zona transparente (rincones muy comprimidos),
    // se recorta ese ultimo anillo.
    if(dirty)for(int y=0;y<h;++y)for(int x=0;x<w;++x){const Color c=GetImageColor(image,x,y);if(c.a==0||!NearWhite(c))continue;bool touchesTransparent=false;for(int i=0;i<4;++i){const int nx=x+dx[i],ny=y+dy[i];if(nx>=0&&nx<w&&ny>=0&&ny<h&&GetImageColor(image,nx,ny).a==0){touchesTransparent=true;break;}}if(touchesTransparent)ImageDrawPixel(&image,x,y,{c.r,c.g,c.b,0});}
    Texture2D texture=LoadTextureFromImage(image); UnloadImage(image); if(texture.id==0)return Texture2D{0}; SetTextureFilter(texture,TEXTURE_FILTER_POINT); return texture;
}
// DF-013.2: sprites reales de bosses (Titan-X, Titan-X Mejorado, Rayder
// Clone), aportados por el usuario como hojas de pose sueltas (no atlas de
// grilla fija como los enemigos). Cada pose es su propio PNG con alfa recto
// ya limpio (verificado sin matte blanco antes de recortar), cargado bajo
// la clave "<boss>_<pose>". Ver docs/AUTONOMOUS_PROGRESS.md.
void LoadBossPoseSet(std::unordered_map<std::string,Texture2D>& textures,const char* boss,const std::vector<std::string>& poses){
    for(const auto& pose:poses){
        const std::string key=std::string(boss)+"_"+pose;
        const std::vector<std::string> candidates={
            "assets/bosses/"+std::string(boss)+"/"+pose+".png",
            "../assets/bosses/"+std::string(boss)+"/"+pose+".png",
            "../../assets/bosses/"+std::string(boss)+"/"+pose+".png",
        };
        textures[key]=LoadRequiredTexture(key.c_str(),candidates,TEXTURE_FILTER_POINT);
    }
}
}
void AssetManager::LoadAll(){
    if(!textures.empty())return;
    textures["bg_industrial"]=LoadRequiredTexture("old_steel_yard_clean",{"assets/backgrounds/old_steel_yard_clean.png","../assets/backgrounds/old_steel_yard_clean.png","../../assets/backgrounds/old_steel_yard_clean.png"},TEXTURE_FILTER_BILINEAR);
    // DF-013.2 (auditoria 19-09): el PNG "deep" nunca existio en el repo y
    // Stage 2 / VS se quedaban con un color plano. Se mantiene la ruta
    // preferente y se agrega el fondo de astillero como respaldo real.
    textures["bg_steel_deep"]=LoadRequiredTexture("old_steel_yard_deep_clean",{"assets/backgrounds/old_steel_yard_deep_clean.png","../assets/backgrounds/old_steel_yard_deep_clean.png","../../assets/backgrounds/old_steel_yard_deep_clean.png","assets/backgrounds/old_steel_yard_clean.png","../assets/backgrounds/old_steel_yard_clean.png","../../assets/backgrounds/old_steel_yard_clean.png"},TEXTURE_FILTER_BILINEAR);
    textures["bg_mercado_antiguo"]=LoadRequiredTexture("mercado_antiguo_clean",{"assets/backgrounds/mercado_antiguo_clean.png","../assets/backgrounds/mercado_antiguo_clean.png","../../assets/backgrounds/mercado_antiguo_clean.png"},TEXTURE_FILTER_POINT);
    textures["bg_zona_quimica"]=LoadRequiredTexture("zona_quimica_clean",{"assets/backgrounds/zona_quimica_clean.png","../assets/backgrounds/zona_quimica_clean.png","../../assets/backgrounds/zona_quimica_clean.png"},TEXTURE_FILTER_POINT);
    textures["rayden_128"]=LoadRequiredTexture("rayden_128",{"assets/characters/rayden_128.png","../assets/characters/rayden_128.png","../../assets/characters/rayden_128.png"},TEXTURE_FILTER_POINT);
    textures["rayden_clean"]=LoadRequiredTexture("rayden_clean",{"assets/characters/rayden_clean.png","../assets/characters/rayden_clean.png","../../assets/characters/rayden_clean.png"},TEXTURE_FILTER_POINT);
    textures["punk_clean"]=LoadEnemyTexture("punk_clean",{"assets/enemies/punk_clean.png","../assets/enemies/punk_clean.png","../../assets/enemies/punk_clean.png"});
    textures["charger_clean"]=LoadEnemyTexture("charger_clean",{"assets/enemies/charger_clean.png","../assets/enemies/charger_clean.png","../../assets/enemies/charger_clean.png"});
    textures["brute_clean"]=LoadEnemyTexture("brute_clean",{"assets/enemies/brute_clean.png","../assets/enemies/brute_clean.png","../../assets/enemies/brute_clean.png"});
    textures["enforcer_clean"]=LoadEnemyTexture("enforcer_clean",{"assets/enemies/enforcer_clean.png","../assets/enemies/enforcer_clean.png","../../assets/enemies/enforcer_clean.png"});
    textures["chemical_soldier_clean"]=LoadEnemyTexture("chemical_soldier_clean",{"assets/enemies/chemical_soldier_clean.png","../assets/enemies/chemical_soldier_clean.png","../../assets/enemies/chemical_soldier_clean.png"});
    textures["urban_ninja_clean"]=LoadEnemyTexture("urban_ninja_clean",{"assets/enemies/urban_ninja_clean.png","../assets/enemies/urban_ninja_clean.png","../../assets/enemies/urban_ninja_clean.png"});
    textures["mutant_clean"]=LoadEnemyTexture("mutant_clean",{"assets/enemies/mutant_clean.png","../assets/enemies/mutant_clean.png","../../assets/enemies/mutant_clean.png"});
    textures["armored_guard_clean"]=LoadEnemyTexture("armored_guard_clean",{"assets/enemies/armored_guard_clean.png","../assets/enemies/armored_guard_clean.png","../../assets/enemies/armored_guard_clean.png"});
    textures["menu_main_art"]=LoadRequiredTexture("menu_main_art",{"assets/ui/menu_main_art.png","../assets/ui/menu_main_art.png","../../assets/ui/menu_main_art.png"},TEXTURE_FILTER_BILINEAR);
    // DF-013.2 (19-09): 20 fondos de escenario (5 stages x 4). 256x144, se
    // dibujan x5 con filtro POINT (pixel perfect) y parallax — ver
    // src/rendering/Backdrop.h.
    for(int stage=1;stage<=5;++stage)for(int sc=1;sc<=4;++sc){
        const std::string file=TextFormat("stage%d_scenario%02d.png",stage,sc);
        const std::string key=TextFormat("bg_s%d_%d",stage,sc);
        // 19-09: los fondos definitivos son arte pintado (no pixel art de rejilla):
        // se escalan a la altura de pantalla, asi que BILINEAR se ve mejor que POINT.
        textures[key]=LoadRequiredTexture(key.c_str(),{"assets/backgrounds/"+file,"../assets/backgrounds/"+file,"../../assets/backgrounds/"+file},TEXTURE_FILTER_BILINEAR);
    }
    LoadBossPoseSet(textures,"brakk",{"idle","walk","run","hurt","death","basic","heavy","grab","chain","chain_throw","charge","smash","fury","explosive"});
    LoadBossPoseSet(textures,"grinder",{"idle","hurt","death","saw","slam","ram","overdrive"});
    LoadBossPoseSet(textures,"titanx",{"idle1","idle2","idle3","idle4","lean","punch1","punch2","uppercut","slam","shoot_orb","recoil","death"});
    LoadBossPoseSet(textures,"titanx_mejorado",{"idle1","idle2","idle3","idle4","cannon_aim","slam","charge_orb","shoot_orb","dash","rage_aura","death"});
    // Dr. Kessler (NPC narrativo): PNG transparentes opcionales en assets/npc/kessler/.
    for(const char* pose:{"idle","walk1","walk2"}){const std::string f=std::string("npc/kessler/")+pose+".png";textures[std::string("kessler_")+pose]=LoadRequiredTexture(pose,{"assets/"+f,"../assets/"+f,"../../assets/"+f},TEXTURE_FILTER_POINT);}
    LoadBossPoseSet(textures,"rayder_clone",{"idle1","idle2","idle3","idle4","ready","punch","kick","dash","release_orb","hurt","death"});
}
void AssetManager::UnloadAll(){for(auto&pair:textures)if(pair.second.id!=0)UnloadTexture(pair.second);textures.clear();}
Texture2D AssetManager::GetTexture(const std::string&name){const auto it=textures.find(name);return it!=textures.end()?it->second:Texture2D{0};}
}
