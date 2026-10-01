#include "game/combat/AttackAnimation.h"
#include <cassert>
#include <string>
int main(){using namespace district_fury;for(int skin:{2,6})for(AttackId id:{AttackId::Punch1,AttackId::Punch2,AttackId::Punch3,AttackId::Kick,AttackId::EnergyWave,AttackId::DashAttack,AttackId::RageAttack,AttackId::Finisher}){auto a=GetAttackAnimationProfile(skin,id);assert(a.clip&&a.impactFrameIndex>=0&&a.rootMotion>=0);auto& d=GetAttack(id);assert(AttackTotalDuration(d)>0);assert(AttackRootMotionProgress(0,d)==0);assert(AttackRootMotionProgress(AttackTotalDuration(d),d)<=1.0001f);assert(std::string(AttackPhaseLabel(0,d))=="STARTUP");assert(std::string(AttackPhaseLabel(d.startup,d))=="ACTIVE");}assert(!UsesAttackAnimationProfile(0)&&!UsesAttackAnimationProfile(1));}
