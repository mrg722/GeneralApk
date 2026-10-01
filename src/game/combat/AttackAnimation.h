#pragma once
#include "game/combat/AttackData.h"
#include <algorithm>
namespace district_fury {
struct AttackAnimationProfile { const char* clip; float rootMotion; int impactFrameIndex; float anticipationScale; float impactStretch; float recoveryRecoil; };
inline bool UsesAttackAnimationProfile(int skin){return skin==2||skin==6;}
inline AttackAnimationProfile GetAttackAnimationProfile(int skin,AttackId id){
 if(skin==2)switch(id){case AttackId::Punch1:return{"punch1",12,1,1.05f,1.08f,.96f};case AttackId::Punch2:return{"punch2",16,1,1.05f,1.09f,.95f};case AttackId::Punch3:return{"punch3",24,2,1.12f,1.13f,.92f};case AttackId::Kick:return{"kick",22,1,1.10f,1.12f,.91f};case AttackId::EnergyWave:return{"energy",18,2,1.08f,1.10f,.93f};case AttackId::DashAttack:return{"dash_attack",48,1,1.02f,1.14f,.90f};case AttackId::RageAttack:return{"rage_attack",28,1,1.12f,1.16f,.89f};case AttackId::Finisher:return{"finisher",34,1,1.14f,1.18f,.87f};default:break;}
 if(skin==6)switch(id){case AttackId::Punch1:return{"punch1",14,1,1.03f,1.07f,.97f};case AttackId::Punch2:return{"punch2",18,1,1.04f,1.08f,.96f};case AttackId::Punch3:return{"punch3",26,1,1.07f,1.11f,.94f};case AttackId::Kick:return{"kick",22,1,1.06f,1.10f,.94f};case AttackId::EnergyWave:return{"energy",10,2,1.05f,1.09f,.95f};case AttackId::DashAttack:return{"dash_attack",46,1,1.02f,1.13f,.92f};case AttackId::RageAttack:return{"rage_attack",20,1,1.08f,1.15f,.91f};case AttackId::Finisher:return{"finisher",30,1,1.10f,1.17f,.88f};default:break;}
 return{"idle",0,0,1,1,1};
}
inline float AttackRootMotionProgress(float elapsed,const AttackDef& def){const float end=def.startup+def.active;if(elapsed<=0||end<=0)return 0;const float t=std::clamp(elapsed/end,0.0f,1.0f);if(t<.30f){const float u=t/.30f;return .18f*u*u;}const float u=(t-.30f)/.70f;return .18f+.82f*(1-(1-u)*(1-u));}
inline const char* AttackPhaseLabel(float elapsed,const AttackDef& def){if(elapsed<def.startup)return"STARTUP";if(elapsed<def.startup+def.active)return"ACTIVE";return"RECOVERY";}
}
