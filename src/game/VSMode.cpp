#include "game/VSMode.h"
#include "rendering/AssetManager.h"
#include "ui/GameHUD.h"
#include "game/CharacterVisual.h"
#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace district_fury {
namespace {
constexpr float kMinX=180.0f,kMaxX=1120.0f,kMinY=505.0f,kMaxY=625.0f;
constexpr int kFieldCount=9;
// DF-013.2: opcion BOSS del selector VS. NONE deja el flujo de enemigos de
// calle intacto; los demas usan la clase Boss compartida (BossDefinition).

const char* kBossNames[]={"NINGUNO (ENEMIGOS)","BRAKK","GRINDER","TITAN-X","TITAN-X MEJORADO","RAYDER CLONE"};
constexpr int kBossOptionCount=6;  // NINGUNO + 5 bosses
const char* kStageNames[]={"STAGE 1 // SLUM DISTRICT","STAGE 2 // OLD STEEL YARD","STAGE 3 // ASTRA TOWER","STAGE 4 // KESSLER TOWER","STAGE 5 // CAMARA DEL CLON"};
const int kScenarioCounts[]={4,5,3,2,1};
constexpr int kStageCount=5;
const char* kStage1Scenarios[]={
 "BARRIO BAJO // BLOQUE 17",
 "MERCADO ANTIGUO // LINEA DEL CANAL",
 "PUERTA DE ACERO // RUTA DE CARGA",
 "ASTILLERO DE CADENAS // TERRITORIO DE BRAKK"};
const char* kStage2Scenarios[]={
 "DEEP LINE // ENTRADA",
 "OLD STEEL YARD // FUNDICION",
 "ZONA QUIMICA // PROCESAMIENTO",
 "OLD STEEL YARD // DEPOSITOS",
 "CAMARA DE GRINDER"};
const char* kStage3Scenarios[]={
 "PUBLIC ATRIUM",
 "RESEARCH FLOOR",
 "EXECUTIVE CORE"};
// DF-013.2: Stage4/Stage5 agregados al VS (antes solo 3 stages).
const char* kStage4Scenarios[]={
 "KESSLER TOWER // SEGURIDAD",
 "NUCLEO EJECUTIVO // TITAN-X MEJORADO"};
const char* kStage5Scenarios[]={
 "CAMARA DEL CLON"};
const char* kEnemyNames[]={"PUNK","BRUTE","CHARGER","ENFORCER","CHEMICAL SOLDIER","URBAN NINJA","MUTANT","ARMORED GUARD"};
StreetEnemyType NextEnemyType(StreetEnemyType t,int dir){int i=(static_cast<int>(t)+dir+8)%8;return static_cast<StreetEnemyType>(i);}
}
VSMode::VSMode()=default;
void VSMode::Init(){flow=VSFlow::Select;stage=0;scenario=0;enemyCount=1;selectedBoss=-1;selectedCharacter=0;cursor=0;exitRequested=false;playerDefeated=false;enemyTypes={StreetEnemyType::Punk,StreetEnemyType::Brute,StreetEnemyType::Charger,StreetEnemyType::Enforcer};enemies.clear();bossProjectiles.clear();player.Reset();player.position={360,585,0};hitstop=0;shake=0;}
bool VSMode::ShouldExit()const{return exitRequested;}
void VSMode::ClearExit(){exitRequested=false;}
int VSMode::ScenarioCount()const{return kScenarioCounts[std::clamp(stage,0,kStageCount-1)];}
const char* VSMode::StageName()const{return kStageNames[std::clamp(stage,0,kStageCount-1)];}
const char* VSMode::ScenarioText()const{
 if(stage==0)return kStage1Scenarios[std::clamp(scenario,0,3)];
 if(stage==1)return kStage2Scenarios[std::clamp(scenario,0,4)];
 if(stage==2)return kStage3Scenarios[std::clamp(scenario,0,2)];
 if(stage==3)return kStage4Scenarios[std::clamp(scenario,0,1)];
 return kStage5Scenarios[0];
}
const char* VSMode::BackgroundKey()const{
 if(stage==0&&scenario==1)return "bg_mercado_antiguo";
 if(stage==1&&scenario==2)return "bg_zona_quimica";
 if(stage==3)return "bg_zona_quimica";
 return stage==0?"bg_industrial":"bg_steel_deep";
}
const char* VSMode::EnemyTypeName(StreetEnemyType t){return kEnemyNames[static_cast<int>(t)];}
void VSMode::ResetFight(){
 player.Reset();player.ApplyCharacter(selectedCharacter);player.position={360,585,0};playerDefeated=false;hitstop=0;shake=0;enemies.clear();bossProjectiles.clear();combatWorld.Reset();
 if(selectedBoss>=0){boss.Reset(static_cast<BossId>(selectedBoss),{900,585,0});return;}
 const std::array<float,4> xs{700,835,970,1105};
 const std::array<float,4> ys{575,535,610,555};
 for(int i=0;i<enemyCount;++i){StreetEnemy e;e.Init({xs[(size_t)i],ys[(size_t)i],0},enemyTypes[(size_t)i]);e.active=true;enemies.push_back(e);}
}
void VSMode::StartFight(){flow=VSFlow::Fight;ResetFight();}
void VSMode::Update(float dt){
 dt=std::min(dt,.033f);
 if(flow==VSFlow::Select){
  if(IsKeyPressed(KEY_ESCAPE)){exitRequested=true;return;}
  if(IsKeyPressed(KEY_UP))cursor=(cursor+kFieldCount-1)%kFieldCount;
  if(IsKeyPressed(KEY_DOWN))cursor=(cursor+1)%kFieldCount;
  if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_RIGHT)){
   int dir=IsKeyPressed(KEY_RIGHT)?1:-1;
   if(cursor==0){stage=(stage+dir+kStageCount)%kStageCount;scenario=std::clamp(scenario,0,ScenarioCount()-1);}
   else if(cursor==1)scenario=std::clamp(scenario+dir,0,ScenarioCount()-1);
   else if(cursor==2)enemyCount=std::clamp(enemyCount+dir,1,4);
   else if(cursor==3)selectedCharacter=(selectedCharacter+dir+CharacterCount())%CharacterCount();
   else if(cursor==4)selectedBoss=((selectedBoss+1+dir+kBossOptionCount)%kBossOptionCount)-1;
   else{int slot=cursor-5;enemyTypes[(size_t)slot]=NextEnemyType(enemyTypes[(size_t)slot],dir);}
  }
  if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_J))StartFight();
  return;
 }
 if(IsKeyPressed(KEY_ESCAPE)){flow=VSFlow::Select;enemies.clear();return;}
 if(IsKeyPressed(KEY_R)){ResetFight();return;}
 if(playerDefeated||(selectedBoss>=0&&boss.IsDefeated())){if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_J))ResetFight();return;}
 if(hitstop>0){hitstop-=dt;return;}
 player.Update(dt);player.position.x=std::clamp(player.position.x,kMinX,kMaxX);player.position.y=std::clamp(player.position.y,kMinY,kMaxY);
 if(selectedBoss>=0){
  // DF-013.2: pelea de boss en VS via la clase Boss compartida (primer
  // consumidor real de BossDefinition — ver src/game/combat/Boss.h).
  boss.Update(dt,player,&combatWorld,bossProjectiles,&hitstop,&shake);
  combatWorld.Update(dt);
  if(player.AttackIsActive()&&!player.hasHit&&player.attackType!=AttackType::Energy&&boss.CanBeHit()){
   const CombatBox hit=player.GetAttackHitbox();
   if(hit.Intersects(boss.GetHurtbox())){boss.ApplyDamage(player.GetAttackDamage()+(player.isRageMode?8:0));player.hasHit=true;shake=.12f;}
  }
  for(auto& p:bossProjectiles){if(!p.active)continue;p.pos.x+=p.vx*dt;p.life-=dt;if(p.life<=0){p.active=false;continue;}
   const CombatBox b{p.pos.x-18,p.pos.y-18,36,36};
   if(p.fromBoss&&player.state!=PlayerState::Defeat&&player.dashInvulnerability<=0.0f&&b.Intersects(player.GetHurtbox())){player.TakeDamage(p.damage);p.active=false;shake=.14f;}}
  bossProjectiles.erase(std::remove_if(bossProjectiles.begin(),bossProjectiles.end(),[](const BossProjectile&p){return !p.active;}),bossProjectiles.end());
 }else{
  for(auto& e:enemies)if(e.active&&!e.IsDefeated()){e.Update(dt,player,&combatWorld);e.position.x=std::clamp(e.position.x,kMinX,kMaxX);e.position.y=std::clamp(e.position.y,kMinY,kMaxY);}
  combatWorld.Update(dt);combatWorld.ResolveHazards(player,enemies);
  if(player.AttackIsActive()&&!player.hasHit&&player.attackType!=AttackType::Energy){const CombatBox hit=player.GetAttackHitbox();for(auto& e:enemies)if(e.active&&!e.IsDefeated()&&hit.Intersects(e.GetHurtbox())){const float d=player.facing==Facing::Right?1.f:-1.f;e.TakeDamage(player.GetAttackDamage()+(player.isRageMode?5:0),{d*player.GetAttackKnockback(),0,0});player.hasHit=true;shake=.08f;break;}}
  if(player.state!=PlayerState::Defeat)for(auto& e:enemies)if(e.active&&!e.IsDefeated()&&!e.hasHit&&e.AttackIsActive()&&e.GetAttackHitbox().Intersects(player.GetHurtbox())){player.TakeDamage((int)e.attackDamage);e.hasHit=true;shake=.10f;break;}
 }
 playerDefeated=player.state==PlayerState::Defeat;
}
void VSMode::DrawBackground()const{
 DrawRectangle(0,0,1280,720,{5,8,12,255});
 const Texture2D bg=AssetManager::Get().GetTexture(BackgroundKey());
 if(bg.id)DrawTexturePro(bg,{0,0,(float)bg.width,(float)bg.height},{0,0,1280,720},{0,0},0,WHITE);
 DrawRectangle(0,625,1280,95,{7,10,13,220});
 for(int x=0;x<1280;x+=160){DrawRectangle(x,617,108,8,{55,60,61,235});DrawRectangle(x+25,647,68,5,{94,78,48,190});}
}
void VSMode::DrawHud()const{
 // DF-013.2: VS ya tenia el retrato (fue la referencia para el resto de
 // modos); ahora usa el mismo componente compartido en vez de su propia
 // copia, para que un cambio futuro al panel se haga en un solo lugar.
 {ui::PlayerVitals vitals{};vitals.hp=player.hp;vitals.maxHp=player.maxHp;vitals.shield=player.shield;vitals.maxShield=player.maxShield;vitals.sp=player.sp;vitals.maxSp=player.maxSp;vitals.rage=player.rage;vitals.maxRage=player.maxRage;vitals.isRageMode=player.isRageMode;vitals.combo=0;vitals.title=TextFormat("%s // MODO VS",GetCharacterVisual(selectedCharacter).name);vitals.x=16;vitals.y=16;vitals.width=530;vitals.panelHeight=122;ui::DrawPlayerVitals(vitals);}
 DrawText(TextFormat("STAGE %d  //  %s",stage+1,ScenarioText()),294,100,11,{175,200,210,230});
 DrawRectangle(818,16,446,122,{3,7,11,220});
 if(selectedBoss>=0){
  DrawText(boss.Def().displayName,840,25,18,{255,150,150,255});
  DrawText(TextFormat("FASE %d",boss.GetPhase()),840,52,13,WHITE);
  DrawRectangle(840,72,400,12,{28,18,20,255});
  DrawRectangle(840,72,(int)(400.f*std::max(0,boss.GetHp())/std::max(1,boss.GetMaxHp())),12,{225,60,90,255});
  DrawText(TextFormat("%d / %d HP",boss.GetHp(),boss.GetMaxHp()),840,92,12,{200,210,215,230});
 }else{
  DrawText(StageName(),840,25,18,{255,205,75,255});DrawText(TextFormat("ENEMIGOS %d/4",enemyCount),840,52,13,WHITE);
  for(int i=0;i<enemyCount;++i){const auto& e=enemies[(size_t)i];DrawText(TextFormat("%d  %s",i+1,EnemyTypeName(e.type)),840,72+i*15,11,e.IsDefeated()?Color{110,120,125,180}:WHITE);}
 }
}
void VSMode::DrawSelection()const{
 DrawBackground();DrawRectangle(205,48,870,610,{3,7,11,242});DrawRectangleLines(205,48,870,610,{55,90,105,170});DrawRectangle(205,48,6,610,{60,205,240,230});DrawRectangle(1069,48,6,610,{255,205,75,210});
 DrawText("MODO VS // LABORATORIO",405,76,36,{225,235,240,255});DrawText("PRUEBA DIRECTA DE SPRITES, ESCENARIOS Y COMBATE",335,121,13,{120,185,205,240});
 const int y[]={140,178,216,254,292,330,368,406,444};const Color active={255,215,80,255};
 const char* labels[]={"STAGE","ESCENARIO","CANTIDAD","PERSONAJE","BOSS","ENEMIGO 1","ENEMIGO 2","ENEMIGO 3","ENEMIGO 4"};
 for(int i=0;i<kFieldCount;++i){bool selected=cursor==i;DrawRectangle(335,y[i]-8,610,34,selected?Color{20,28,34,230}:Color{8,15,21,190});DrawRectangleLines(335,y[i]-8,610,34,selected?Color{255,205,75,210}:Color{70,95,105,90});DrawText(labels[i],360,y[i],14,selected?active:WHITE);}
 DrawText(StageName(),600,y[0],14,{190,220,230,255});DrawText(ScenarioText(),600,y[1],14,{190,220,230,255});DrawText(TextFormat("%d ENEMIGO%s",enemyCount,enemyCount==1?"":"S"),600,y[2],14,selectedBoss>=0?Color{85,95,100,130}:Color{190,220,230,255});
 DrawText(GetCharacterVisual(selectedCharacter).name,600,y[3],14,selectedCharacter==1?Color{255,160,170,255}:Color{190,220,230,255});
 DrawText(kBossNames[selectedBoss+1],600,y[4],14,selectedBoss>=0?Color{255,150,150,255}:Color{190,220,230,255});
 for(int i=0;i<4;++i){bool enabled=i<enemyCount&&selectedBoss<0;DrawText(EnemyTypeName(enemyTypes[(size_t)i]),600,y[5+i],14,enabled?Color{190,220,230,255}:Color{85,95,100,130});}
 if(selectedBoss>=0)DrawText("BOSS ACTIVO: los campos de enemigos se ignoran (1 vs 1).",335,y[8]+26,12,{255,180,120,220});
 DrawText("↑/↓ CAMPO    ←/→ CAMBIAR    ENTER/J INICIAR",391,540,14,{170,195,205,245});DrawText("ESC VOLVER AL MENU",485,568,13,{130,155,165,220});
}
void VSMode::DrawFight()const{
 DrawBackground();combatWorld.DrawGround();
 if(selectedBoss>=0){
  if(player.position.y<boss.GetPos().y){player.Draw();boss.Draw(player.position.x);}else{boss.Draw(player.position.x);player.Draw();}
  for(const auto&p:bossProjectiles){if(!p.active)continue;Vector2 s=p.pos.ToScreen();DrawCircle((int)s.x,(int)s.y-70,20,{255,80,120,90});DrawCircle((int)s.x,(int)s.y-70,12,{255,80,120,255});}
 }else{
  std::array<std::pair<float,int>,5> order{};int n=0;order[n++]={player.position.y,-1};for(int i=0;i<(int)enemies.size();++i)if(enemies[(size_t)i].active)order[n++]={enemies[(size_t)i].position.y,i};std::sort(order.begin(),order.begin()+n,[](auto&a,auto&b){return a.first<b.first;});for(int i=0;i<n;++i)if(order[i].second<0)player.Draw();else enemies[(size_t)order[i].second].Draw();
  for(int i=0;i<(int)enemies.size();++i){const auto&e=enemies[(size_t)i];Vector2 p=e.position.ToScreen();const int w=96,x=(int)p.x-w/2,y=(int)p.y-125;DrawRectangle(x,y,w,7,{18,20,23,220});if(!e.IsDefeated())DrawRectangle(x+1,y+1,(int)((w-2)*e.hp/std::max(1.f,(float)e.maxHp)),5,{220,65,65,255});DrawText(TextFormat("%d %s",i+1,EnemyTypeName(e.type)),x,y-16,10,e.IsDefeated()?Color{120,130,135,170}:WHITE);}
 }
 combatWorld.DrawEffects();DrawHud();DrawRectangle(260,657,760,45,{3,7,11,225});DrawText("J GOLPE   K PATADA   L ENERGIA   B BLOQUEO   SHIFT DASH   SPACE FURIA",296,669,12,{170,200,210,240});DrawText("R REINICIAR   ESC CONFIGURACION",489,687,11,{135,155,165,220});
 if(playerDefeated){DrawRectangle(0,0,1280,720,{0,0,0,160});DrawText("RAYDEN DERROTADO",438,288,46,{240,80,80,255});DrawText("ENTER/J REINICIAR   ESC CONFIGURACION",430,357,18,WHITE);}
 else if(selectedBoss>=0&&boss.IsDefeated()){DrawRectangle(0,0,1280,720,{0,0,0,150});DrawText("BOSS DERROTADO",470,288,42,{120,240,160,255});DrawText("ENTER/J REINICIAR   ESC CONFIGURACION",430,350,18,WHITE);}
}
void VSMode::Draw()const{if(flow==VSFlow::Select)DrawSelection();else DrawFight();}
}
