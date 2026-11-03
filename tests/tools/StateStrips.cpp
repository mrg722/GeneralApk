// Tira de estados por personaje: dibuja el Player real en cada estado.
// Uso: state_strips <dir_salida> [id] [--fixed-time[=T]] [--bbox] [--test-override] [--feel-light]
// (correr desde la raiz del repo). Escribe <dir_salida>/states_<id>.png para cada
// personaje (o solo `id`) y nada en la raiz del repo. Copiada de
// docs/auditoria_movimientos/herramientas/states.cpp.
//
// Reloj: el dibujo del jugador usa GetTime() (aura de transformacion, pulso del
// escudo, poses sueltas). CMake enlaza esta herramienta con -Wl,--wrap=GetTime,
// asi que las llamadas del juego llegan a __wrap_GetTime:
// - por defecto, un reloj simulado que avanza 1/60 s por cada Update del jugador
//   (vuelve a 0 por personaje): dos corridas dan los mismos pixeles;
// - con --fixed-time[=T], GetTime() devuelve SIEMPRE T (0 por defecto): el
//   dibujo no depende del reloj de pared ni del orden de los pasos, para
//   comparar pixel a pixel dos versiones del motor.
// --bbox: ademas mide el alto del cuerpo (pixeles opacos, alfa >= 250, sobre
//   fondo transparente) en cada captura y lo compara con el reposo ("idle a").
//   Imprime "bbox <id> max <ratio> (<captura>)" y cada captura que pasa 1.15x.
// --test-override: instala en el DIBUJO (VisualMoveSet, no en los archivos de
//   personaje) un MoveSet de prueba cuyo Punch1 usa GIRO(1) + ESTELA(3) + un FX,
//   para ver el evaluador PoseFx en cualquier personaje.
// --feel-light: igual, con MoveSet.feel = Light (feel ligero de KF/BETA).
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include "game/player/PoseFxEval.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
using namespace district_fury;
namespace { double gFakeTime = 0.0; bool gFixed = false; double gFixedValue = 0.0; }
extern "C" double __wrap_GetTime(void) { return gFixed ? gFixedValue : gFakeTime; }
struct Step { const char* label; std::function<void(PlayerInput&,Player&,int)> act; int frames; std::vector<int> shots; };

// Alto (px) de los pixeles opacos de una imagen RGBA: fila mas baja - mas alta + 1.
static int OpaqueHeight(const Image& img) {
  const Color* px = static_cast<const Color*>(img.data);
  int top = -1, bottom = -1;
  for (int y = 0; y < img.height; ++y)
    for (int x = 0; x < img.width; ++x)
      if (px[y * img.width + x].a >= 250) { if (top < 0) top = y; bottom = y; break; }
  return top < 0 ? 0 : bottom - top + 1;
}

static MoveSet MakeTestOverrideSet(bool light) {
  static MoveOverride ov[1];
  ov[0].id = static_cast<int>(AttackId::Punch1);
  PoseFx& fx = ov[0].visual.fx;
  fx.presets[fx.presetCount++] = posefx::ANTICIPA(1.0f);
  fx.presets[fx.presetCount++] = posefx::GIRO(1.0f, '*');
  fx.ghostCount = 3; fx.ghostInterval = 0.03f; fx.ghostTint = {255, 120, 220, 255};
  fx.overlay[0].first = 8; fx.overlay[0].count = 4; fx.overlay[0].x = 50; fx.overlay[0].y = -70; fx.overlay[0].phase = 'A';
  fx.overlayCount = 1;
  MoveSet set;
  set.name = "prueba GIRO(1)+ESTELA(3)";
  set.overrides = ov; set.overrideCount = 1;
  if (light) set.feel = FeelLevel::Light;
  return set;
}

int main(int argc,char**argv){
  std::string out; int only=-1; bool bbox=false, testOverride=false, feelLight=false;
  for(int i=1;i<argc;++i){
    const char* a=argv[i];
    if(std::strncmp(a,"--fixed-time",12)==0){ gFixed=true; if(a[12]=='=') gFixedValue=std::atof(a+13); }
    else if(std::strcmp(a,"--bbox")==0) bbox=true;
    else if(std::strcmp(a,"--test-override")==0) testOverride=true;
    else if(std::strcmp(a,"--feel-light")==0) { testOverride=true; feelLight=true; }
    else if(out.empty()) out=a;
    else only=std::atoi(a);
  }
  if(out.empty()){ std::fprintf(stderr,"uso: state_strips <dir_salida> [id] [--fixed-time[=T]] [--bbox] [--test-override] [--feel-light]\n"); return 2; }
  std::error_code ec; std::filesystem::create_directories(out,ec);
  SetTraceLogLevel(LOG_WARNING); InitWindow(640,360,"states"); AssetManager::Get().LoadAll();
  static const MoveSet kTestSet = MakeTestOverrideSet(feelLight);
  const int CW=230,CH=300;
  float worstAll=0; std::string worstAllLabel;
  for(int id=0;id<CharacterCount();++id){
    if(only>=0&&id!=only) continue;
    gFakeTime=0.0;
    if(testOverride) SetVisualMoveSetForTests(id,&kTestSet);
    Player p; p.ApplyCharacter(id); p.position={640,560,0}; p.debugInvulnerable=false;
    PlayerInput in; p.scriptedInput=&in;
    std::vector<std::pair<std::string,Image>> shots;
    std::vector<std::pair<std::string,int>> heights;
    auto snap=[&](const std::string& lab){
      RenderTexture2D rt=LoadRenderTexture(CW,CH);
      Vector2 s=p.position.ToScreen(); Camera2D cam{}; cam.target={s.x,p.position.y}; cam.offset={CW/2.0f,CH-30.0f}; cam.zoom=1.0f;
      if(bbox){
        BeginTextureMode(rt); ClearBackground({0,0,0,0}); BeginMode2D(cam); p.Draw(); EndMode2D(); EndTextureMode();
        Image m=LoadImageFromTexture(rt.texture); heights.push_back({lab,OpaqueHeight(m)}); UnloadImage(m);
      }
      BeginTextureMode(rt); ClearBackground({70,78,92,255});
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
    if(bbox&&!heights.empty()&&heights[0].second>0){
      const float ref=(float)heights[0].second; float worst=0; std::string wl;
      for(const auto& h:heights){ const float r=h.second/ref; if(r>worst){worst=r;wl=h.first;}
        if(r>1.15f) std::printf("  %d '%s' alto %d = %.2fx del reposo (%d)\n",id,h.first.c_str(),h.second,r,heights[0].second); }
      std::printf("bbox %d max %.2f (%s) reposo %d px\n",id,worst,wl.c_str(),heights[0].second);
      if(worst>worstAll){worstAll=worst;worstAllLabel=std::to_string(id)+" "+wl;}
    }
    // compose
    const int cols=8; int rows=(int)(shots.size()+cols-1)/cols;
    Image sheet=GenImageColor(cols*CW,rows*CH+24,{20,20,26,255});
    ImageDrawText(&sheet,TextFormat("%d %s",id,GetCharacterVisual(id).name),6,4,18,YELLOW);
    for(size_t i=0;i<shots.size();++i){ ImageDraw(&sheet,shots[i].second,{0,0,(float)CW,(float)CH},{(float)(i%cols*CW),(float)(24+i/cols*CH),(float)CW,(float)CH},WHITE); UnloadImage(shots[i].second);}
    std::string fn=out+"/states_"+std::to_string(id)+".png"; ExportImage(sheet,fn.c_str()); UnloadImage(sheet); std::printf("%d ok\n",id); fflush(stdout);
    if(testOverride) SetVisualMoveSetForTests(id,nullptr);
  }
  if(bbox) std::printf("bbox peor %.2f (%s)\n",worstAll,worstAllLabel.c_str());
  CloseWindow();
}
