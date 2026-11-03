// Tira de estados por personaje: dibuja el Player real en cada estado.
// Uso: state_strips <dir_salida> [id]   (correr desde la raiz del repo)
// Escribe <dir_salida>/states_<id>.png para cada personaje (o solo `id`) y nada
// en la raiz del repo. Copiada de docs/auditoria_movimientos/herramientas/states.cpp.
//
// Reloj fijo: el dibujo del jugador usa GetTime() (aura de transformacion, pulso
// del escudo, poses sueltas). CMake enlaza esta herramienta con
// -Wl,--wrap=GetTime, asi que las llamadas del juego llegan a __wrap_GetTime, que
// devuelve un reloj simulado que avanza 1/60 s por cada Update del jugador (y
// vuelve a 0 por personaje). Dos corridas dan los mismos pixeles.
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
using namespace district_fury;
namespace { double gFakeTime = 0.0; }
extern "C" double __wrap_GetTime(void) { return gFakeTime; }
struct Step { const char* label; std::function<void(PlayerInput&,Player&,int)> act; int frames; std::vector<int> shots; };
int main(int argc,char**argv){
  if(argc<2){ std::fprintf(stderr,"uso: state_strips <dir_salida> [id]\n"); return 2; }
  const std::string out=argv[1]; int only=argc>2?atoi(argv[2]):-1;
  std::error_code ec; std::filesystem::create_directories(out,ec);
  SetTraceLogLevel(LOG_WARNING); InitWindow(640,360,"states"); AssetManager::Get().LoadAll();
  const int CW=230,CH=300;
  for(int id=0;id<CharacterCount();++id){
    if(only>=0&&id!=only) continue;
    gFakeTime=0.0;
    Player p; p.ApplyCharacter(id); p.position={640,560,0}; p.debugInvulnerable=false;
    PlayerInput in; p.scriptedInput=&in;
    struct Shot{std::string label; RenderTexture2D rt;};
    std::vector<std::pair<std::string,Image>> shots;
    auto snap=[&](const std::string& lab){
      RenderTexture2D rt=LoadRenderTexture(CW,CH);
      BeginTextureMode(rt); ClearBackground({70,78,92,255});
      Vector2 s=p.position.ToScreen(); Camera2D cam{}; cam.target={s.x,p.position.y}; cam.offset={CW/2.0f,CH-30.0f}; cam.zoom=1.0f;
      BeginMode2D(cam); DrawLine((int)s.x-110,(int)p.position.y,(int)s.x+110,(int)p.position.y,{255,255,0,160}); p.Draw(); EndMode2D();
      DrawRectangle(0,0,CW,18,{0,0,0,160}); DrawText(lab.c_str(),4,3,12,WHITE);
      const char* f=p.facing==Facing::Right?"->":"<-"; DrawText(f,CW-22,3,12,YELLOW);
      EndTextureMode(); Image img=LoadImageFromTexture(rt.texture); ImageFlipVertical(&img); UnloadRenderTexture(rt); shots.push_back({lab,img}); };
    auto run=[&](int n){ for(int i=0;i<n;++i){ p.Update(1.0f/60.0f); gFakeTime+=1.0/60.0;} };
    auto reset=[&](){ in=PlayerInput{}; run(40); p.position.x=640; p.position.y=560; };
    // idle
    reset(); snap("idle a"); run(13); snap("idle b");
    // walk right
    in=PlayerInput{}; in.moveX=1; run(8); snap("walk-> t1"); run(5); snap("walk-> t2"); run(5); snap("walk-> t3");
    in.moveX=-1; run(10); snap("walk<- t1"); run(6); snap("walk<- t2"); reset();
    in.moveX=1; run(2); in=PlayerInput{}; run(2);
    // dash
    in.dash=true; run(1); in.dash=false; run(3); snap("dash"); reset();
    // punch chain
    auto press=[&](int which){ in=PlayerInput{}; if(which==0) in.punch=true; else if(which==1) in.kick=true; else if(which==2) in.energy=true; run(1); in=PlayerInput{}; };
    press(0); run(6); snap("punch1 early"); run(6); snap("punch1 hit"); press(0); run(8); snap("punch2"); press(0); run(10); snap("punch3"); reset();
    press(1); run(12); snap("kick"); reset();
    p.sp=p.maxSp; press(2); run(6); snap("energy start"); run(14); snap("energy"); reset();
    // skills
    for(int k=0;k<5;++k){ in=PlayerInput{}; in.skill=k; run(1); in=PlayerInput{}; run(18); char b[32]; snprintf(b,32,"skill%d",k+1); snap(b); run(200); reset(); }
    in=PlayerInput{}; in.skill=5; run(1); in=PlayerInput{}; run(30); snap("transform"); run(120); snap("transformed idle"); reset();
    // block
    in.block=true; run(10); snap("block"); in=PlayerInput{}; reset(); run(800);
    // hit
    p.TakeDamage(10); run(3); snap("hit"); run(60); reset();
    p.TakeDamage(40); run(6); snap("airborne"); run(30); snap("knockdown"); run(25); snap("getup?"); run(60); reset();
    p.hp=1; p.TakeDamage(50); run(10); snap("defeat"); run(60); snap("defeat end");
    // compose
    const int cols=8; int rows=(int)(shots.size()+cols-1)/cols;
    Image sheet=GenImageColor(cols*CW,rows*CH+24,{20,20,26,255});
    ImageDrawText(&sheet,TextFormat("%d %s",id,GetCharacterVisual(id).name),6,4,18,YELLOW);
    for(size_t i=0;i<shots.size();++i){ ImageDraw(&sheet,shots[i].second,{0,0,(float)CW,(float)CH},{(float)(i%cols*CW),(float)(24+i/cols*CH),(float)CW,(float)CH},WHITE); UnloadImage(shots[i].second);} 
    std::string fn=out+"/states_"+std::to_string(id)+".png"; ExportImage(sheet,fn.c_str()); UnloadImage(sheet); std::printf("%d ok\n",id); fflush(stdout);
  }
  CloseWindow();
}
