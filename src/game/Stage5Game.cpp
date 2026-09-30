#include "game/Stage5Game.h"
#include "rendering/AssetManager.h"
#include "rendering/BossSprite.h"
#include "rendering/Backdrop.h"
#include "ui/GameHUD.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {
constexpr float kPi = 3.14159265359f;
Color Alpha(Color c,float a){c.a=(unsigned char)(std::clamp(a,0.0f,1.0f)*255);return c;}
}
Stage5Game::Stage5Game()=default;
void Stage5Game::Init(){player.Reset();player.position={340,585,0};enemies.clear();projectiles.clear();particles.clear();combatWorld.Reset();zone=1;zoneSpawned=false;flow=Flow::Intro;stageTime=0;combo=0;maxCombo=0;damageTaken=0;cameraX=640;boss=RayderClone{};kessler=KesslerCameo{};boss.pos={940,585,0};bannerTimer=2.2f;}
void Stage5Game::SpawnImpact(Vector3D pos,Color color,bool heavy){const int n=heavy?32:16;for(int i=0;i<n;++i){float a=(float)i/n*2*kPi;float s=(float)GetRandomValue(90,heavy?400:250);particles.push_back({pos,{std::cos(a)*s,std::sin(a)*s},heavy?.55f:.3f,heavy?.55f:.3f,(float)GetRandomValue(3,heavy?10:6),color});}}
void Stage5Game::SpawnEnergy(){float d=player.facing==Facing::Right?1.f:-1.f;projectiles.push_back({{player.position.x+d*72,player.position.y-72,0},d*760,.9f,player.isRageMode?32:23,true,false});SpawnImpact({player.position.x+d*55,player.position.y-72,0},{40,210,255,255},true);}
void Stage5Game::UpdateProjectiles(float dt){for(auto&p:projectiles){if(!p.active)continue;p.pos.x+=p.vx*dt;p.life-=dt;if(p.life<=0){p.active=false;continue;}CombatBox b{p.pos.x-18,p.pos.y-18,36,36};if(!p.fromBoss){if(flow==Flow::Boss&&!boss.defeated){CombatBox bb{boss.pos.x-72,boss.pos.y-142,144,142};if(b.Intersects(bb)&&boss.invuln<=0){boss.hp=std::max(0,boss.hp-p.damage);boss.invuln=.12f;p.active=false;++combo;comboTimer=1;maxCombo=std::max(maxCombo,combo);SpawnImpact(p.pos,{50,215,255,255},true);hitstop=.12f;shake=.16f;}}}else if(player.state!=PlayerState::Defeat&&player.dashInvulnerability<=0.0f&&b.Intersects(player.GetHurtbox())){int before=player.hp;player.TakeDamage(p.damage);damageTaken+=before-player.hp;combo=0;comboTimer=0;p.active=false;SpawnImpact(player.position,{255,60,90,255},true);hitstop=.09f;shake=.14f;}}projectiles.erase(std::remove_if(projectiles.begin(),projectiles.end(),[](const Projectile&p){return !p.active;}),projectiles.end());}
void Stage5Game::HandleBossHits(){if(flow!=Flow::Boss||boss.defeated||boss.invuln>0||!player.AttackIsActive()||player.hasHit||player.attackType==AttackType::Energy)return;if(player.GetAttackHitbox().Intersects({boss.pos.x-72,boss.pos.y-142,144,142})){int dmg=player.GetAttackDamage()+(player.isRageMode?8:0);boss.hp=std::max(0,boss.hp-dmg);boss.invuln=.1f;player.hasHit=true;++combo;comboTimer=1;maxCombo=std::max(maxCombo,combo);SpawnImpact(boss.pos,{255,80,90,255},true);hitstop=.12f;shake=.18f;if(boss.hp<=0)DefeatBoss();}}
void Stage5Game::DefeatBoss(){if(boss.defeated)return;boss.defeated=true;boss.attack=BossAttack::None;flow=Flow::Clear;SpawnImpact(boss.pos,{255,190,120,255},true);shake=.55f;}
void Stage5Game::UpdateParticles(float dt){for(auto&p:particles){p.life-=dt;p.pos.x+=p.vel.x*dt;p.pos.y+=p.vel.y*dt;p.vel.x*=.92f;p.vel.y*=.92f;}particles.erase(std::remove_if(particles.begin(),particles.end(),[](const Particle&p){return p.life<=0;}),particles.end());}
// DF-013.2: guardias del proyecto en las 3 salas previas al clon. Se usan
// los tipos de enemigo que ya existen (seguridad de elite: ArmoredGuard,
// UrbanNinja, ChemicalSoldier, Enforcer), con la misma IA de siempre.
void Stage5Game::SpawnZone(int id){
 enemies.clear();zoneSpawned=true;bannerTimer=1.6f;
 struct S{float x,y;StreetEnemyType t;};
 const S z1[]={{760,560,StreetEnemyType::ArmoredGuard},{980,610,StreetEnemyType::Enforcer},{1180,535,StreetEnemyType::ArmoredGuard}};
 const S z2[]={{780,545,StreetEnemyType::UrbanNinja},{1000,600,StreetEnemyType::ChemicalSoldier},{1220,565,StreetEnemyType::UrbanNinja},{1400,610,StreetEnemyType::Enforcer}};
 const S z3[]={{800,560,StreetEnemyType::ArmoredGuard},{1020,610,StreetEnemyType::UrbanNinja},{1240,540,StreetEnemyType::ChemicalSoldier},{1460,590,StreetEnemyType::ArmoredGuard}};
 const S* list=id==1?z1:id==2?z2:z3; const int n=id==1?3:4;
 for(int i=0;i<n;++i){StreetEnemy e;e.Init({list[i].x,list[i].y,0},list[i].t);e.Activate();e.active=true;enemies.push_back(e);}
}
void Stage5Game::HandlePlayerHits(){
 if(!player.AttackIsActive()||player.hasHit||player.attackType==AttackType::Energy)return;
 const CombatBox hit=player.GetAttackHitbox();
 for(auto&e:enemies)if(e.active&&!e.IsDefeated()&&hit.Intersects(e.GetHurtbox())){
  const float d=player.facing==Facing::Right?1.f:-1.f;
  e.TakeDamage(player.GetAttackDamage()+(player.isRageMode?6:0),{d*player.GetAttackKnockback(),0,0});
  player.hasHit=true;++combo;comboTimer=1;maxCombo=std::max(maxCombo,combo);
  SpawnImpact({e.position.x,e.position.y-70,0},{255,120,140,255},player.attackType==AttackType::Kick);
  hitstop=.08f;shake=.1f;break;}
}
void Stage5Game::HandleEnemyHits(){
 if(player.state==PlayerState::Defeat)return;
 for(auto&e:enemies)if(e.active&&!e.IsDefeated()&&!e.hasHit&&e.AttackIsActive()&&e.GetAttackHitbox().Intersects(player.GetHurtbox())){
  const int before=player.hp;player.TakeDamage((int)e.attackDamage);damageTaken+=before-player.hp;
  e.hasHit=true;combo=0;comboTimer=0;SpawnImpact(player.position,{255,90,110,255},false);hitstop=.085f;shake=.12f;break;}
}
void Stage5Game::UpdateCombat(float dt){if(comboTimer>0)comboTimer-=dt;else combo=0;player.Update(dt);if(player.state==PlayerState::Attack&&player.attackType==AttackType::Energy&&player.energyReleased){SpawnEnergy();player.energyReleased=false;}
 // Zonas 1-3: guardias del proyecto. Al limpiarlas se pasa a la siguiente y,
 // tras la tercera, empieza el duelo contra el clon.
 if(zone<=3){
  if(!zoneSpawned)SpawnZone(zone);
  for(auto&e:enemies)if(e.active)e.Update(dt,player,&combatWorld);
  HandlePlayerHits();HandleEnemyHits();
  combatWorld.ResolveHazards(player,enemies);
  const bool cleared=!enemies.empty()&&std::all_of(enemies.begin(),enemies.end(),[](const StreetEnemy&e){return e.IsDefeated();});
  if(cleared){
   ++zone;zoneSpawned=false;enemies.clear();player.position.x=340;bannerTimer=1.8f;
   if(zone>3){flow=Flow::BossIntro;bannerTimer=2.4f;boss=RayderClone{};boss.pos={940,585,0};
    kessler.Start(boss.pos.x+420,boss.pos.x+140,boss.pos.x+900,boss.pos.y-30,{"Mismo poder. Mismo dolor. Sin tus debilidades.","Tu eras el prototipo, Rayden. El es la version final.","Termina lo que empezamos... si puedes."});}
  }
 }
 UpdateProjectiles(dt);if(zone>3)HandleBossHits();combatWorld.Update(dt);float left=140.f,right=zone>3?1740.f:1700.f;player.position.x=std::clamp(player.position.x,left,right);player.position.y=std::clamp(player.position.y,kLaneMinY,kLaneMaxY);if(player.state==PlayerState::Defeat)flow=Flow::GameOver;}
// DF-013.2: el clon "espejea" el kit del jugador (combo cuerpo a cuerpo,
// Energy Wave oscura via proyectil, dash y teletransporte) en vez de
// inventar un moveset nuevo — asi el diseno narrativo ("el mismo poder,
// sin humanidad") queda reflejado en el propio combate, no solo en texto.
void Stage5Game::UpdateBoss(float dt){if(boss.invuln>0)boss.invuln-=dt;if(boss.teleportCooldown>0)boss.teleportCooldown-=dt;if(flow==Flow::BossIntro){bannerTimer-=dt;kessler.Update(dt);if(IsKeyPressed(KEY_ENTER))kessler.Skip();player.position.x=std::min(player.position.x,boss.pos.x-220);if(bannerTimer<=0&&kessler.IsDone())flow=Flow::Boss;return;}if(boss.defeated)return;float ratio=(float)boss.hp/boss.maxHp;int ph=ratio<=.3f?3:ratio<=.65f?2:1;if(ph!=boss.phase){boss.phase=ph;boss.attack=BossAttack::None;boss.elapsed=0;SpawnImpact(boss.pos,{255,60,90,255},true);shake=.28f;}
 boss.facingRight=player.position.x>boss.pos.x;
 boss.attackTimer-=dt;boss.elapsed+=dt;float dx=player.position.x-boss.pos.x;
 if(boss.attack==BossAttack::None&&boss.teleportCooldown<=0&&std::abs(dx)<140&&GetRandomValue(0,99)<45){boss.attack=BossAttack::Teleport;boss.elapsed=0;boss.teleportCooldown=boss.phase>=3?2.2f:3.0f;}
 else if(boss.attack==BossAttack::None&&boss.attackTimer<=0){int pick=GetRandomValue(0,boss.phase>=3?3:2);boss.attack=pick==0?BossAttack::MirrorCombo:pick==1?BossAttack::DarkWave:pick==2?BossAttack::Dash:BossAttack::Finisher;boss.elapsed=0;boss.attackTimer=boss.phase>=3?1.1f:boss.phase==2?1.4f:1.75f;}
 if(boss.attack==BossAttack::None&&std::abs(dx)>170)boss.pos.x+=(dx>0?1:-1)*(boss.phase>=3?150.f:boss.phase==2?115.f:88.f)*dt;
 if(boss.attack==BossAttack::Teleport){if(boss.elapsed>=.28f){boss.pos.x=std::clamp(player.position.x-(boss.facingRight?190.f:-190.f),200.f,1680.f);SpawnImpact(boss.pos,{255,60,90,255},false);boss.attack=BossAttack::None;boss.elapsed=0;}}
 else{
  float tele=boss.attack==BossAttack::Dash?.4f:boss.attack==BossAttack::DarkWave?.5f:boss.attack==BossAttack::Finisher?.65f:.28f;
  CombatBox hit{};bool active=false;
  if(boss.attack!=BossAttack::None&&boss.elapsed>=tele){
   if(boss.attack==BossAttack::MirrorCombo){hit={boss.pos.x-95,boss.pos.y-120,190,120};active=boss.elapsed<=tele+.32f;}
   else if(boss.attack==BossAttack::Dash){boss.pos.x+=(dx>0?1:-1)*600*dt;hit={boss.pos.x-95,boss.pos.y-115,190,118};active=boss.elapsed<=tele+.4f;}
   else if(boss.attack==BossAttack::DarkWave){if(boss.elapsed<tele+.05f){float d=dx>0?1.f:-1.f;projectiles.push_back({{boss.pos.x+d*70,boss.pos.y-75,0},d*640.f,1.1f,boss.phase>=3?26:19,true,true});SpawnImpact({boss.pos.x+d*55,boss.pos.y-75},{255,60,120,255},true);}}
   else if(boss.attack==BossAttack::Finisher){hit={boss.pos.x-230,boss.pos.y-150,460,155};active=boss.elapsed<=tele+.28f;}
  }
  if(active&&hit.Intersects(player.GetHurtbox())&&player.state!=PlayerState::Hit){int dmg=boss.attack==BossAttack::Finisher?(boss.phase>=3?42:34):boss.phase>=3?28:boss.phase==2?23:18;int before=player.hp;player.TakeDamage(dmg);damageTaken+=before-player.hp;combo=0;comboTimer=0;SpawnImpact(player.position,{255,60,90,255},true);hitstop=boss.attack==BossAttack::Finisher?.16f:.1f;shake=boss.attack==BossAttack::Finisher?.3f:.2f;}
  if(boss.attack!=BossAttack::None&&boss.elapsed>(boss.attack==BossAttack::Dash?.95f:boss.attack==BossAttack::Finisher?1.05f:.85f)){boss.attack=BossAttack::None;boss.elapsed=0;}
 }
 boss.pos.x=std::clamp(boss.pos.x,200.f,1680.f);boss.pos.y=std::clamp(boss.pos.y,kLaneMinY,kLaneMaxY);HandleBossHits();if(boss.hp<=0)DefeatBoss();}
void Stage5Game::Update(float dt){dt=std::min(dt,.033f);if(IsKeyPressed(KEY_ESCAPE)){if(flow==Flow::Combat||flow==Flow::Boss||flow==Flow::BossIntro)flow=Flow::Pause;else if(flow==Flow::Pause)flow=boss.defeated?Flow::Clear:Flow::Boss;}if(flow==Flow::Pause){if(IsKeyPressed(KEY_R))Init();return;}if(flow==Flow::GameOver){if(IsKeyPressed(KEY_R))Init();return;}if(flow==Flow::Clear){if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_J))advanceRequested=true;return;}if(hitstop>0){hitstop-=dt;return;}shake=std::max(0.f,shake-dt);bannerTimer=std::max(0.f,bannerTimer-dt);UpdateParticles(dt);if(flow==Flow::Intro){if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_J)){flow=Flow::Combat;zone=1;zoneSpawned=false;bannerTimer=1.8f;}return;}stageTime+=dt;UpdateCombat(dt);if(flow==Flow::BossIntro||flow==Flow::Boss)UpdateBoss(dt);float target=zone>3?std::clamp((player.position.x+boss.pos.x)*0.5f,640.f,1240.f):std::clamp(player.position.x,640.f,1240.f);cameraX+=(target-cameraX)*(1-std::pow(.001f,dt));}
// DF-013.2: silueta roja/negra corrupta del clon — mismas proporciones que
// un personaje humano (no un boss gigante), con "grietas" de energia roja
// en vez de piel, coherente con el diseno ("mismo poder, sin humanidad").
// DF-013.2: Rayder Clone ahora se dibuja con el sprite real del usuario
// (espejo oscuro de Rayden) en vez de la silueta primitiva original. El
// aura roja pulsante y el destello de Finisher se conservan como overlay
// porque son feedback de combate, no la silueta del personaje.
void Stage5Game::DrawBoss() const{
    if(boss.defeated)return;
    Vector2 p=boss.pos.ToScreen();
    const bool tp=boss.attack==BossAttack::Teleport&&boss.elapsed<.28f;
    const bool flip=!boss.facingRight;
    const char* pose="idle1";
    if(boss.attack==BossAttack::MirrorCombo)pose="punch";
    else if(boss.attack==BossAttack::DarkWave)pose="release_orb";
    else if(boss.attack==BossAttack::Dash)pose="dash";
    else if(boss.attack==BossAttack::Finisher)pose="kick";
    else if(boss.attack==BossAttack::Teleport)pose="ready";
    else{
        const int cycle=((int)(boss.elapsed*1.5f))%4;
        pose=cycle==0?"idle1":cycle==1?"idle2":cycle==2?"idle3":"idle4";
    }
    Texture2D tex=AssetManager::Get().GetTexture(std::string("rayder_clone_")+pose);
    Color tint=tp?Color{255,255,255,90}:(boss.phase>=3?Color{255,190,195,255}:WHITE);
    // Rayder Clone es un espejo del jugador: debe verse igual de humano,
    // no un monstruo (a diferencia de los Titan-X). Altura objetivo similar
    // a la de Rayden en pantalla.
    DrawBossPose(tex,{p.x,p.y},130.f,flip,tint);
    DrawCircleLines((int)p.x,(int)(p.y-70),56+std::sin((float)GetTime()*10)*6,Alpha({255,60,90,255},boss.phase>=3?.6f:.3f));
    if(boss.attack==BossAttack::Finisher)DrawCircleLines((int)p.x,(int)(p.y-70),90+std::sin((float)GetTime()*14)*10,{255,90,60,200});
}
void Stage5Game::DrawWorld() const{Camera2D c{};c.offset={640,360};c.target={cameraX,360};c.zoom=1;if(shake>0){c.target.x+=GetRandomValue(-100,100)*shake*9;c.target.y+=GetRandomValue(-100,100)*shake*6;}BeginMode2D(c);DrawRectangle(0,0,1920,720,{6,4,9,255});
 // DF-013.2: la camara del clon cambia de sala segun la fase del combate
 // (los 4 fondos del Stage 5 se usan como progresion del enfrentamiento).
 DrawScenarioBackdrop(AssetManager::Get().GetTexture(TextFormat("bg_s5_%d",std::clamp(zone,1,4))),cameraX,0.30f);for(int x=-100;x<1900;x+=220){DrawRectangle(x,180,10,460,{22,14,20,220});DrawCircle(x+5,175,5,{255,60,90,160});}DrawRectangle(0,635,1920,100,{4,3,7,255});for(int x=0;x<1900;x+=160){DrawRectangle(x,618,110,10,{35,22,28,255});}for(auto&p:projectiles){Vector2 s=p.pos.ToScreen();Color col=p.fromBoss?Color{255,60,120,255}:Color{50,220,255,255};DrawCircle((int)s.x,(int)s.y,20,Alpha(col,.32f));DrawCircle((int)s.x,(int)s.y,12,col);}for(auto&p:particles){Vector2 s=p.pos.ToScreen();float a=p.life/p.maxLife;DrawCircle((int)s.x,(int)s.y,p.size*a,Alpha(p.color,a));}kessler.DrawWorld();if(zone<=3){
  std::vector<std::pair<float,int>> order{{player.position.y,-1}};
  for(size_t i=0;i<enemies.size();++i)if(enemies[i].active)order.push_back({enemies[i].position.y,(int)i});
  std::sort(order.begin(),order.end(),[](auto&a,auto&b){return a.first<b.first;});
  for(auto&o:order){if(o.second<0)player.Draw();else enemies[(size_t)o.second].Draw();}
 }
 else if(player.position.y<boss.pos.y){player.Draw();DrawBoss();}else{DrawBoss();player.Draw();}combatWorld.DrawEffects();EndMode2D();}
void Stage5Game::DrawHUD() const{ui::PlayerVitals vitals{};vitals.hp=player.hp;vitals.maxHp=player.maxHp;vitals.shield=player.shield;vitals.maxShield=player.maxShield;vitals.sp=player.sp;vitals.maxSp=player.maxSp;vitals.rage=player.rage;vitals.maxRage=player.maxRage;vitals.isRageMode=player.isRageMode;vitals.combo=combo;vitals.title="RAYDEN CRUZ // CAMARA DEL CLON";vitals.x=16;vitals.y=14;vitals.width=500;vitals.panelHeight=132;ui::DrawPlayerVitals(vitals);
 DrawText(TextFormat("ZONA %d/4",std::clamp(zone,1,4)),1060,24,18,WHITE);
 if(bannerTimer>0&&zone<=3){const char* z=zone==1?"ACCESO AL LABORATORIO":zone==2?"CAMARA DE KESSLER":"SALA DE CLONACION";DrawText(z,640-MeasureText(z,30)/2,190,30,{255,150,165,230});}if(flow==Flow::Boss||flow==Flow::BossIntro){DrawRectangle(280,18,720,48,{9,6,8,230});DrawText("RAYDER CLONE // ESPEJO OSCURO",420,21,20,{255,110,130,255});DrawRectangle(350,50,580,12,{28,18,20,255});DrawRectangle(350,50,(int)(580.f*boss.hp/boss.maxHp),12,{225,60,90,255});DrawText(TextFormat("FASE %d/3",boss.phase),940,49,15,WHITE);}if(bannerTimer>0&&flow==Flow::BossIntro){const char*t="RAYDER CLONE // EL ESPEJO OSCURO";DrawText(t,640-MeasureText(t,30)/2,190,30,{255,150,165,235});}}
void Stage5Game::DrawClear() const{DrawRectangle(0,0,1280,720,{6,4,9,245});DrawText("CAMPANA COMPLETA",370,90,54,{255,205,130,255});DrawText("El clon cae. Rayden es el unico que queda.",320,160,20,WHITE);DrawText(TextFormat("TIME        %6.1fs",stageTime),380,255,23,WHITE);DrawText(TextFormat("DAMAGE      %6d",damageTaken),380,295,23,WHITE);DrawText(TextFormat("MAX COMBO   %6d",maxCombo),380,335,23,WHITE);DrawText("BOSS        RAYDER CLONE",380,380,23,WHITE);DrawText("\"El mismo poder... pero sin humanidad.\" — el eco del clon se apaga.",190,440,16,{210,200,205,225});DrawText("ENTER — VOLVER AL MENU PRINCIPAL",390,625,22,WHITE);}
void Stage5Game::Draw() const{DrawWorld();if(flow==Flow::Intro){DrawRectangle(0,0,1280,720,{6,4,9,160});DrawText("CAMARA DEL CLON",420,265,42,{255,150,165,255});DrawText("Tres salas del proyecto y, al final, tu propio reflejo.",330,325,18,{210,200,205,230});DrawText("ENTER / J — ENTRAR AL LABORATORIO",395,375,22,WHITE);}if(flow==Flow::Combat||flow==Flow::BossIntro||flow==Flow::Boss)DrawHUD();kessler.DrawDialogue();if(flow==Flow::Pause){DrawHUD();DrawRectangle(0,0,1280,720,{0,0,0,165});DrawText("PAUSED",545,235,48,WHITE);DrawText("ESC — RESUME",500,330,22,WHITE);DrawText("R — RESTART",510,375,22,WHITE);}if(flow==Flow::GameOver){DrawHUD();DrawRectangle(0,0,1280,720,{0,0,0,185});DrawText("GAME OVER",440,250,60,{235,55,65,255});DrawText("R — RESTART",500,355,22,WHITE);}if(flow==Flow::Clear)DrawClear();}
}
