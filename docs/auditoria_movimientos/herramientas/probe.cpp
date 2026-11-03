#include "game/Player.h"
#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include <cstdio>
using namespace district_fury;
constexpr float F=1.0f/60.0f;
int main(){
  // 1. energy wave spawns per stage-style loop
  { Player p; p.QueueCommand(InputCommand::SpecialWave); int spawns=0;
    for(int i=0;i<60;++i){ p.Update(F); if(p.state==PlayerState::Attack&&p.attackType==AttackType::Energy&&p.energyReleased){++spawns;p.energyReleased=false;} }
    printf("1) EnergyWave spawns in one cast (stage loop): %d\n",spawns); }
  // 2. Kick after combo window expiry
  { Player p; p.QueueCommand(InputCommand::Kick); p.Update(F);
    p.OnAttackConnected(GetAttack(AttackId::Kick)); // comboWindow=.18
    int noneFrames=0; for(int i=0;i<40&&p.state==PlayerState::Attack;++i){p.Update(F); if(p.currentAttack==AttackId::None) ++noneFrames;}
    printf("2) Kick with comboWindow .18: frames in Attack with currentAttack==None: %d\n",noneFrames);}
  // 3. depth tolerance Punch1 vs Punk
  { Player p; p.position={500,565,0}; p.state=PlayerState::Attack; p.currentAttack=AttackId::Punch1; p.attackElapsed=0.15f; p.facing=Facing::Right;
    CombatBox hb=p.GetAttackHitbox(); printf("3) Punch1 hitbox x=%.0f..%.0f y=%.0f..%.0f\n",hb.x,hb.x+hb.width,hb.y,hb.y+hb.height);
    int lo=999,hi=-999; for(int dy=-150;dy<=150;++dy){StreetEnemy e; e.Init({560,565.f+dy,0},StreetEnemyType::Punk); e.active=true; if(hb.Intersects(e.GetHurtbox())){lo=std::min(lo,dy);hi=std::max(hi,dy);} }
    printf("   lane offset that still hits Punk: [%d, %d] (def.depth=%.0f unused)\n",lo,hi,GetAttack(AttackId::Punch1).depth);
    int xl=999,xh=-999; for(int dx=-200;dx<=300;++dx){StreetEnemy e; e.Init({500.f+dx,565,0},StreetEnemyType::Punk); e.active=true; if(hb.Intersects(e.GetHurtbox())){xl=std::min(xl,dx);xh=std::max(xh,dx);} }
    printf("   x offset that hits: [%d, %d] (def.range=%.0f unused)\n",xl,xh,GetAttack(AttackId::Punch1).range);
    for(AttackId id:{AttackId::Kick,AttackId::RageAttack,AttackId::Finisher}){ p.currentAttack=id; p.attackElapsed=GetAttack(id).startup+0.01f; CombatBox b=p.GetAttackHitbox(); int a=999,c=-999; for(int dy=-150;dy<=150;++dy){StreetEnemy e; e.Init({p.position.x+GetAttack(id).boxForward,565.f+dy,0},StreetEnemyType::Punk); e.active=true; if(b.Intersects(e.GetHurtbox())){a=std::min(a,dy);c=std::max(c,dy);} } printf("   %s lane hit range [%d,%d]\n",GetAttack(id).name,a,c);} }
  // enemy -> player lane tolerance
  { Player p; p.position={500,565,0}; int lo=999,hi=-999; for(int dy=-150;dy<=150;++dy){StreetEnemy e; e.Init({600,565.f+dy,0},StreetEnemyType::Punk); e.active=true; e.facing=Facing::Left; e.state=StreetEnemyState::Attack; e.attackElapsed=e.attackDuration*0.5f; if(e.GetAttackHitbox().Intersects(p.GetHurtbox())){lo=std::min(lo,dy);hi=std::max(hi,dy);} } printf("3b) Punk attack lane hit range vs player [%d,%d] (attackDepth=38 used only by AI)\n",lo,hi);}
  // 4. block pushback accumulates
  { Player p; p.SetState(PlayerState::Block); for(int i=0;i<5;++i){p.dashInvulnerability=0; p.TakeDamage(5);} printf("4) velocity.x after 5 blocked hits: %.0f, x=%.1f\n",p.velocity.x,p.position.x);
    PlayerInput in; p.scriptedInput=&in; for(int i=0;i<30;++i)p.Update(F); printf("   after 30 frames idle (no block): velocity.x=%.0f x=%.1f\n",p.velocity.x,p.position.x);
    p.dashInvulnerability=0; p.TakeDamage(10); for(int i=0;i<10;++i)p.Update(F); printf("   after taking a normal hit: x=%.1f (slides with stale velocity)\n",p.position.x);}
  // 5. knockdown direction from behind
  { Player p; p.facing=Facing::Right; float x0=p.position.x=800; p.TakeDamage(30); for(int i=0;i<20;++i)p.Update(F); printf("5) heavy hit (attacker anywhere) facing Right -> moved dx=%.1f (always backward of facing)\n",p.position.x-x0);}
  // 6. juggle infinite
  { StreetEnemy e; e.Init({600,565,0},StreetEnemyType::Enforcer); e.active=true; Player tgt; e.TakeDamage(1,{200,0,180});
    int hits=1; for(int i=0;i<600;++i){ e.Update(F,tgt,nullptr); if(e.state==StreetEnemyState::Airborne && i%12==0){ e.TakeDamage(1,{50,0,0}); ++hits;} if(e.state!=StreetEnemyState::Airborne) break; }
    printf("6) juggle: enemy kept airborne for %d light hits (no juggle limit / gravity scaling), z=%.1f hurtbox.y=%.0f (ignores z)\n",hits,e.position.z,e.GetHurtbox().y);}
  // 7. multi-target
  { Player p; p.position={500,565,0}; p.state=PlayerState::Attack; p.currentAttack=AttackId::RageAttack; p.attackElapsed=0.15f; std::vector<StreetEnemy> es(3); for(int i=0;i<3;++i){es[i].Init({560.f+i*40,565,0},StreetEnemyType::Punk); es[i].active=true;} CombatWorld w; auto r=w.ResolvePlayerMelee(p,es); int hit=0; for(auto&e:es) if(e.hp<e.maxHp)++hit; printf("7) RageAttack vs 3 enemies in box: %d hit (report.hits=%d)\n",hit,r.hits);}
  // 8. walking chain J J J whiff timing
  { Player p; PlayerInput in; p.scriptedInput=&in; int f=0; const char* last=""; for(;f<120;++f){ in=PlayerInput{}; if(f==0||f==10||f==20||f==30) in.punch=true; p.Update(F); const char* n=p.AttackPhaseName(); if(std::string(n)!=last){printf("8) f=%d %s state=%d\n",f,n,(int)p.state); last=n;} } }
  // 9. finisher via cancel
  { Player p; p.isRageMode=true; p.comboCount=5; p.QueueCommand(InputCommand::Punch); p.Update(F); p.OnAttackConnected(GetAttack(AttackId::Punch1)); p.QueueCommand(InputCommand::Kick); for(int i=0;i<20&&p.currentAttack==AttackId::Punch1;++i)p.Update(F); printf("9) rage+combo5, K in cancel window of Punch1 -> %s (finisher needs non-cancel path)\n",GetAttack(p.currentAttack).name);}
  // 10. Kick -> J cancel goes to Punch1? and Punch3 -> J
  { Player p; p.QueueCommand(InputCommand::Punch); p.Update(F); p.QueueCommand(InputCommand::Punch); for(int i=0;i<60&&p.currentAttack!=AttackId::Punch2;++i)p.Update(F); p.QueueCommand(InputCommand::Punch); for(int i=0;i<60&&p.currentAttack!=AttackId::Punch3;++i)p.Update(F); p.QueueCommand(InputCommand::Punch); int fr=0; for(;fr<60&&p.currentAttack==AttackId::Punch3;++fr)p.Update(F); printf("10) whiff P1>P2>P3 then J: after P3 -> %s state=%d after %d frames\n",GetAttack(p.currentAttack).name,(int)p.state,fr);}
  // 11. player hit from attack during normal punch -> trade? damage while attacking Punch1
  { Player p; p.QueueCommand(InputCommand::Punch); p.Update(F); p.dashInvulnerability=0; p.TakeDamage(10); printf("11) hit during Punch1 -> state=%d (Hit=5)\n",(int)p.state);}
  // 12. Hit state knockback when not blocking
  { Player p; p.velocity={0,0,0}; float x0=p.position.x; p.TakeDamage(12); for(int i=0;i<12;++i)p.Update(F); printf("12) normal hit pushback dx=%.2f (no knockback on player)\n",p.position.x-x0);}
  return 0;}
