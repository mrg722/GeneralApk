#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include "rendering/AssetManager.h"
#include "audio/AudioSystem.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace district_fury {
namespace {

Color ThreatColor(StreetEnemyType type) {
    return GetEnemyProfile(type).fallbackTint;
}

// Compatibilidad con el codigo de dibujo heredado, que esperaba un struct Stats.
struct Stats {
    int hp; float speed; float damage; float range; float depth; float attackDuration;
    float scale; Color fallbackTint; float bodyWidth; float bodyHeight;
};

Stats GetStats(StreetEnemyType type) {
    const EnemyProfile& p = GetEnemyProfile(type);
    return {p.hp, p.moveSpeed, p.attackDamage, p.attackRange, p.attackDepth,
            p.attackDuration, p.visualScale, p.fallbackTint, p.bodyWidth, p.bodyHeight};
}

float DepthScale(float y) { return DepthScaleFor(y); }

struct EnemyAnimationLayout { int columns; int rows; int idleStart; int idleEnd; int walkStart; int walkEnd; int attackStart; int attackEnd; int hitFrame; int deathStart; int deathEnd; };
EnemyAnimationLayout LayoutFor(StreetEnemyType){ return {4,3,0,3,0,3,4,7,8,10,11}; }

const char* TextureKeyFor(StreetEnemyType type){
    switch(type){
        case StreetEnemyType::Brute:return "brute_clean";
        case StreetEnemyType::Charger:return "charger_clean";
        case StreetEnemyType::Enforcer:return "enforcer_clean";
        case StreetEnemyType::ChemicalSoldier:return "chemical_soldier_clean";
        case StreetEnemyType::UrbanNinja:return "urban_ninja_clean";
        case StreetEnemyType::Mutant:return "mutant_clean";
        case StreetEnemyType::ArmoredGuard:return "armored_guard_clean";
        default:return "punk_clean";
    }
}


// Mide cada celda del atlas directamente desde su alpha. Esto evita mantener una
// tabla de bounds dependiente del enemigo y elimina el riesgo de reutilizar la
// geometria de otro personaje.
std::vector<SpriteFrame> BuildSpriteFrames(Texture2D texture){
    std::vector<SpriteFrame> result;
    if(texture.id==0||texture.width!=512||texture.height!=384)return result;

    Image image=LoadImageFromTexture(texture);
    if(image.data==nullptr)return result;
    ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if(image.data==nullptr||image.width!=512||image.height!=384){UnloadImage(image);return result;}

    const Color* pixels=static_cast<const Color*>(image.data);
    result.reserve(12);
    for(int frameIndex=0;frameIndex<12;++frameIndex){
        const int cellX=(frameIndex%4)*128;
        const int cellY=(frameIndex/4)*128;
        int minX=128,minY=128,maxX=-1,maxY=-1;
        std::vector<int> lowerXs;
        lowerXs.reserve(128*16);

        for(int y=0;y<128;++y){
            for(int x=0;x<128;++x){
                const Color& pixel=pixels[(cellY+y)*image.width+(cellX+x)];
                if(pixel.a<8)continue;
                minX=std::min(minX,x);minY=std::min(minY,y);
                maxX=std::max(maxX,x);maxY=std::max(maxY,y);
            }
        }
        if(maxX<minX||maxY<minY){
            TraceLog(LOG_WARNING,"Distrito Fury: frame %i del atlas enemigo no contiene pixeles alpha",frameIndex);
            continue;
        }

        // El ancla horizontal se calcula con la zona inferior del personaje,
        // no con toda la silueta. Así un arma, cadena o efecto que sobresalga
        // durante un ataque no mueve el punto de apoyo de los pies.
        const int lowerStart=std::max(minY,maxY-15);
        for(int y=lowerStart;y<=maxY;++y){
            for(int x=minX;x<=maxX;++x){
                const Color& pixel=pixels[(cellY+y)*image.width+(cellX+x)];
                if(pixel.a>=8)lowerXs.push_back(x);
            }
        }
        if(lowerXs.empty()){
            for(int y=minY;y<=maxY;++y){
                for(int x=minX;x<=maxX;++x){
                    const Color& pixel=pixels[(cellY+y)*image.width+(cellX+x)];
                    if(pixel.a>=8)lowerXs.push_back(x);
                }
            }
        }
        std::sort(lowerXs.begin(),lowerXs.end());
        const float pivotX=static_cast<float>(lowerXs[lowerXs.size()/2]);
        const float pivotY=static_cast<float>(maxY+1);

        SpriteFrame frame;
        frame.source={(float)(cellX+minX),(float)(cellY+minY),(float)(maxX-minX+1),(float)(maxY-minY+1)};
        frame.width=(float)(maxX-minX+1);
        frame.height=(float)(maxY-minY+1);
        frame.pivotX=pivotX-(float)minX;
        frame.pivotY=pivotY-(float)minY;
        frame.duration=(frameIndex<4)?0.12f:(frameIndex<8?0.09f:0.10f);
        frame.visualBounds={0,0,frame.width,frame.height};
        result.push_back(frame);
    }
    UnloadImage(image);
    return result.size()==12?result:std::vector<SpriteFrame>{};
}

void EnsureAnimator(Animator& a,StreetEnemyType type){
    if(a.texture.id!=0)return;
    const EnemyAnimationLayout layout=LayoutFor(type);
    Texture2D t=AssetManager::Get().GetTexture(TextureKeyFor(type));
    if(t.id!=0 && t.width==512 && t.height==384){
        std::vector<SpriteFrame> measured=BuildSpriteFrames(t);
        if(measured.size()==12){
            a.Init(t,layout.columns,layout.rows,false);
            a.SetFrames(std::move(measured));
            a.Play({layout.idleStart,layout.idleEnd,0.12f,true});
            return;
        }
        TraceLog(LOG_WARNING,"Distrito Fury: atlas enemigo rechazado para %s porque sus 12 frames no pudieron medirse",TextureKeyFor(type));
        return;
    }
    if(t.id!=0)TraceLog(LOG_WARNING,"Distrito Fury: atlas rechazado para %s; se esperaba 512x384 y llegó %ix%i",TextureKeyFor(type),t.width,t.height);
}
void PlayIdle(Animator& animator,StreetEnemyType type){const EnemyAnimationLayout layout=LayoutFor(type);if(animator.isFinished||!animator.isPlaying||animator.currentFrame<layout.idleStart||animator.currentFrame>layout.idleEnd)animator.Play({layout.idleStart,layout.idleEnd,.12f,true});}
void PlayWalk(Animator& animator,StreetEnemyType type){const EnemyAnimationLayout layout=LayoutFor(type);if(animator.isFinished||!animator.isPlaying||animator.currentFrame<layout.walkStart||animator.currentFrame>layout.walkEnd)animator.Play({layout.walkStart,layout.walkEnd,.10f,true});}

void DrawFallbackEnemy(Vector2 p,const Stats&s,StreetEnemyType type,float scale,Color tint){
    const float w=s.bodyWidth*scale,h=s.bodyHeight*scale;
    const float cx=p.x, top=p.y-h, shoulder=top+h*.31f, hip=top+h*.64f;
    const float headR=w*.18f, torsoW=w*.42f, legW=w*.14f;
    const Color dark={20,23,27,255};
    DrawEllipse((int)p.x,(int)p.y,(int)(w*.34f),(int)(h*.07f),{0,0,0,145});
    if(type==StreetEnemyType::Punk){
        DrawCircle((int)cx,(int)(top+h*.18f),(int)headR,tint);
        DrawRectangle((int)(cx-torsoW*.5f),(int)shoulder,(int)torsoW,(int)(h*.34f),tint);
        DrawRectangle((int)(cx-torsoW*.72f),(int)(shoulder+h*.03f),(int)(torsoW*.24f),(int)(h*.30f),tint);
        DrawRectangle((int)(cx+torsoW*.48f),(int)(shoulder+h*.03f),(int)(torsoW*.24f),(int)(h*.30f),tint);
        DrawRectangle((int)(cx-legW*1.45f),(int)hip,(int)legW,(int)(h*.31f),dark);
        DrawRectangle((int)(cx+legW*.45f),(int)hip,(int)legW,(int)(h*.31f),dark);
        DrawRectangle((int)(cx-legW*1.75f),(int)(p.y-h*.04f),(int)(legW*1.7f),(int)(h*.05f),tint);
        DrawRectangle((int)(cx+legW*.15f),(int)(p.y-h*.04f),(int)(legW*1.7f),(int)(h*.05f),tint);
    } else if(type==StreetEnemyType::Brute||type==StreetEnemyType::Mutant||type==StreetEnemyType::ArmoredGuard){
        DrawCircle((int)cx,(int)(top+h*.17f),(int)(headR*1.12f),tint);
        DrawRectangle((int)(cx-torsoW*.62f),(int)shoulder,(int)(torsoW*1.24f),(int)(h*.37f),tint);
        DrawRectangle((int)(cx-torsoW*.92f),(int)shoulder,(int)(torsoW*.30f),(int)(h*.30f),tint);
        DrawRectangle((int)(cx+torsoW*.62f),(int)shoulder,(int)(torsoW*.30f),(int)(h*.30f),tint);
        DrawRectangle((int)(cx-legW*1.7f),(int)hip,(int)(legW*1.25f),(int)(h*.31f),dark);
        DrawRectangle((int)(cx+legW*.45f),(int)hip,(int)(legW*1.25f),(int)(h*.31f),dark);
        DrawRectangle((int)(cx-legW*1.95f),(int)(p.y-h*.04f),(int)(legW*1.8f),(int)(h*.05f),tint);
        DrawRectangle((int)(cx+legW*.25f),(int)(p.y-h*.04f),(int)(legW*1.8f),(int)(h*.05f),tint);
        if(type==StreetEnemyType::Mutant){DrawCircle((int)(cx-headR*.55f),(int)(top+h*.17f),3,{180,255,80,255});DrawCircle((int)(cx+headR*.55f),(int)(top+h*.17f),3,{180,255,80,255});}
    } else {
        DrawCircle((int)cx,(int)(top+h*.17f),(int)headR,tint);
        DrawRectangle((int)(cx-torsoW*.5f),(int)shoulder,(int)torsoW,(int)(h*.34f),tint);
        DrawRectangle((int)(cx-torsoW*.95f),(int)shoulder,(int)(torsoW*.30f),(int)(h*.28f),tint);
        DrawRectangle((int)(cx+torsoW*.65f),(int)shoulder,(int)(torsoW*.30f),(int)(h*.28f),tint);
        DrawRectangle((int)(cx-legW*1.4f),(int)hip,(int)legW,(int)(h*.31f),dark);
        DrawRectangle((int)(cx+legW*.4f),(int)hip,(int)legW,(int)(h*.31f),dark);
        DrawRectangle((int)(cx-legW*1.7f),(int)(p.y-h*.04f),(int)(legW*1.6f),(int)(h*.05f),tint);
        DrawRectangle((int)(cx+legW*.1f),(int)(p.y-h*.04f),(int)(legW*1.6f),(int)(h*.05f),tint);
    }
}

}  // namespace

const EnemyProfile& StreetEnemy::Profile() const { return GetEnemyProfile(type); }

StreetEnemy::StreetEnemy() { Init({900.0f, 560.0f, 0.0f}, StreetEnemyType::Punk); }

void StreetEnemy::Init(Vector3D startPos, StreetEnemyType enemyType) {
    const EnemyProfile& profile = GetEnemyProfile(enemyType);
    position = startPos;
    velocity = {0, 0, 0};
    facing = Facing::Left;
    state = StreetEnemyState::Idle;
    type = enemyType;
    active = false;
    hp = profile.hp;
    maxHp = profile.hp;
    moveSpeed = profile.moveSpeed;
    attackDamage = profile.attackDamage;
    attackRange = profile.attackRange;
    attackDepth = profile.attackDepth;
    attackDuration = profile.attackDuration;
    stateTimer = 0;
    attackElapsed = 0;
    attackCooldown = 0;
    hasHit = false;

    decisionTimer = profile.reactionTime;
    specialTimer = profile.specialCooldown * 0.6f;
    maxGuardHealth = profile.guardStrength;
    guardHealth = profile.guardStrength;
    hitstunTimer = 0;
    telegraphTimer = 0;
    flankSide = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
    comboHits = 0;
    telegraphing = false;
    pendingSpecial = EnemySpecial::None;

    animator = Animator{};
}

void StreetEnemy::Activate() {
    active = true;
    if (state == StreetEnemyState::Defeat) return;
    state = StreetEnemyState::Idle;
    hasHit = false;
}

void StreetEnemy::EnterState(StreetEnemyState next) {
    if (state == next) return;
    state = next;
    const EnemyAnimationLayout layout = LayoutFor(type);
    switch (next) {
        case StreetEnemyState::Idle:
        case StreetEnemyState::Block:
            PlayIdle(animator, type);
            break;
        case StreetEnemyState::Patrol:
        case StreetEnemyState::Chase:
        case StreetEnemyState::Position:
        case StreetEnemyState::Retreat:
            PlayWalk(animator, type);
            break;
        case StreetEnemyState::Attack:
        case StreetEnemyState::Special:
            animator.Play({layout.attackStart, layout.attackEnd, 0.09f, false});
            break;
        default:
            break;
    }
}

bool StreetEnemy::IsDefeated() const { return state == StreetEnemyState::Defeat; }
bool StreetEnemy::IsGuarding() const { return state == StreetEnemyState::Block && guardHealth > 0.0f; }
bool StreetEnemy::IsTelegraphing() const { return telegraphing && telegraphTimer > 0.0f; }

void StreetEnemy::AbsorbGuard(int damage) {
    guardHealth -= static_cast<float>(damage) * 2.6f;
    if (guardHealth <= 0.0f) BreakGuard();
}

void StreetEnemy::BreakGuard() {
    guardHealth = 0.0f;
    if (state == StreetEnemyState::Defeat) return;
    state = StreetEnemyState::Stun;
    stateTimer = 0.85f;
    telegraphing = false;
    AudioSystem::Get().Play(Sfx::HeavyHit);
}

void StreetEnemy::ApplyHitstun(float duration) {
    const EnemyProfile& profile = Profile();
    if (profile.hasArmor && duration < 0.30f) return;  // armor: aguanta ligeros
    hitstunTimer = std::max(hitstunTimer, duration);
}

const char* StreetEnemy::GetTypeName() const { return Profile().displayName; }

const char* StreetEnemy::GetStateName() const {
    switch (state) {
        case StreetEnemyState::Idle:     return "IDLE";
        case StreetEnemyState::Patrol:   return "PATROL";
        case StreetEnemyState::Chase:    return "CHASE";
        case StreetEnemyState::Position: return "POSITION";
        case StreetEnemyState::Attack:   return "ATTACK";
        case StreetEnemyState::Block:    return "BLOCK";
        case StreetEnemyState::Hit:      return "HIT";
        case StreetEnemyState::Stun:     return "STUN";
        case StreetEnemyState::Retreat:  return "RETREAT";
        case StreetEnemyState::Special:  return "SPECIAL";
        default:                          return "DEAD";
    }
}

void StreetEnemy::RunSpecial(float dt, const Player& player, CombatWorld* world) {
    const EnemyProfile& profile = Profile();
    const float direction = facing == Facing::Right ? 1.0f : -1.0f;

    switch (pendingSpecial) {
        case EnemySpecial::Charge:
            position.x += direction * 520.0f * dt;
            break;

        case EnemySpecial::ChemicalCloud:
            if (world != nullptr && !hasHit) {
                world->AddHazard({position.x + direction * 150.0f, position.y, 0.0f},
                                 HazardKind::Acid, 140.0f, 82.0f, profile.specialDamage,
                                 3.0f, 1.6f);
                world->SpawnImpact({position.x + direction * 60.0f, position.y - 70.0f, 0.0f},
                                   {120, 235, 90, 255}, false);
                hasHit = true;
            }
            break;

        case EnemySpecial::Flank: {
            const float targetY = player.position.y + flankSide * 52.0f;
            const float dy = targetY - position.y;
            position.y += (dy > 0 ? 1.0f : -1.0f) * std::min(std::abs(dy), profile.repositionSpeed * dt);
            position.x += (player.position.x - position.x > 0 ? 1.0f : -1.0f) *
                          profile.repositionSpeed * 1.1f * dt;
            break;
        }

        case EnemySpecial::HeavySlam:
            if (world != nullptr && !hasHit && stateTimer < attackDuration * 0.5f) {
                world->SpawnImpact({position.x + direction * 70.0f, position.y, 0.0f},
                                   {235, 175, 90, 255}, true);
                world->DoShake(0.16f);
                hasHit = true;
            }
            break;

        case EnemySpecial::GuardCounter:
            // El contraataque se ejecuta como un ataque normal con mas alcance.
            break;

        case EnemySpecial::Frenzy:
            if (stateTimer <= 0.0f && comboHits < 3) {
                ++comboHits;
                stateTimer = attackDuration * 0.55f;
                attackElapsed = 0;
                hasHit = false;
            }
            break;

        default:
            break;
    }
}

void StreetEnemy::Decide(float dt, const Player& player) {
    const EnemyProfile& profile = Profile();

    const float dx = player.position.x - position.x;
    const float dy = player.position.y - position.y;
    const float horizontal = std::abs(dx);
    const float depth = std::abs(dy);
    const float distance = std::sqrt(dx * dx + dy * dy);

    facing = dx >= 0 ? Facing::Right : Facing::Left;

    const float hpRatio = maxHp > 0 ? static_cast<float>(hp) / maxHp : 1.0f;
    const bool playerThreatening = player.AttackIsActive() ||
                                   (player.state == PlayerState::Attack && horizontal < attackRange * 1.3f);

    // BLOCK: reactivo, solo tipos con guardia.
    if (profile.guardChance > 0.0f && guardHealth > 0.0f && playerThreatening &&
        horizontal < attackRange * 1.25f && attackCooldown <= 0.0f) {
        if (GetRandomValue(0, 999) < static_cast<int>(profile.guardChance * 1000.0f)) {
            EnterState(StreetEnemyState::Block);
            stateTimer = 0.55f;
            return;
        }
    }

    // SPECIAL: cuando toca y esta en rango.
    if (profile.special != EnemySpecial::None && specialTimer <= 0.0f &&
        distance < profile.specialRange && attackCooldown <= 0.0f) {
        pendingSpecial = profile.special;
        specialTimer = profile.specialCooldown;
        telegraphing = true;
        telegraphTimer = profile.special == EnemySpecial::Charge ? 0.55f
                       : profile.special == EnemySpecial::HeavySlam ? 0.62f : 0.35f;
        comboHits = 0;
        hasHit = false;
        attackElapsed = 0;
        stateTimer = attackDuration * 1.25f;
        EnterState(StreetEnemyState::Special);
        return;
    }

    // RETREAT: con poca vida, los tipos tacticos se alejan a recuperar espacio.
    if (profile.retreatHpRatio > 0.0f && hpRatio < profile.retreatHpRatio &&
        horizontal < profile.preferredDistance * 0.75f) {
        EnterState(StreetEnemyState::Retreat);
        stateTimer = 0.8f;
        return;
    }

    // ATTACK: dentro de rango efectivo.
    const float stopDistance = std::max(72.0f, attackRange * 0.60f);
    if (horizontal < attackRange && depth < attackDepth && horizontal > stopDistance * 0.55f &&
        attackCooldown <= 0.0f) {
        EnterState(StreetEnemyState::Attack);
        stateTimer = attackDuration;
        attackElapsed = 0;
        hasHit = false;
        telegraphing = true;
        telegraphTimer = attackDuration * 0.32f;
        const float frameTime = type == StreetEnemyType::UrbanNinja ? 0.055f
                              : type == StreetEnemyType::ChemicalSoldier ? 0.09f : 0.10f;
        const EnemyAnimationLayout layout = LayoutFor(type);
        animator.Play({layout.attackStart, layout.attackEnd, frameTime, false});
        return;
    }

    // POSITION: los tipos que quieren mantener distancia ajustan en vez de pegarse.
    if (horizontal < profile.preferredDistance * 0.70f && profile.aggression < 0.6f) {
        EnterState(StreetEnemyState::Position);
        stateTimer = 0.45f;
        return;
    }

    if (distance < 820.0f) {
        EnterState(StreetEnemyState::Chase);
        return;
    }

    EnterState(StreetEnemyState::Patrol);
}

void StreetEnemy::Update(float dt, const Player& player, CombatWorld* world) {
    if (!active) return;
    EnsureAnimator(animator, type);
    animator.Update(dt);

    const EnemyProfile& profile = Profile();

    attackCooldown = std::max(0.0f, attackCooldown - dt);
    specialTimer = std::max(0.0f, specialTimer - dt);
    hitstunTimer = std::max(0.0f, hitstunTimer - dt);
    if (telegraphTimer > 0.0f) {
        telegraphTimer -= dt;
        if (telegraphTimer <= 0.0f) telegraphing = false;
    }
    if (guardHealth < maxGuardHealth && state != StreetEnemyState::Block &&
        state != StreetEnemyState::Stun) {
        guardHealth = std::min(maxGuardHealth, guardHealth + 16.0f * dt);
    }

    if (state == StreetEnemyState::Defeat) {
        stateTimer -= dt;
        if (stateTimer <= 0) active = false;
        return;
    }

    if (state == StreetEnemyState::Stun) {
        stateTimer -= dt;
        if (stateTimer <= 0) { guardHealth = maxGuardHealth * 0.4f; EnterState(StreetEnemyState::Idle); }
        return;
    }

    if (state == StreetEnemyState::Hit) {
        stateTimer -= dt;
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        velocity.x *= 0.84f;
        velocity.y *= 0.84f;
        if (stateTimer <= 0 && hitstunTimer <= 0.0f) EnterState(StreetEnemyState::Idle);
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        return;
    }

    if (state == StreetEnemyState::Block) {
        stateTimer -= dt;
        // El bloqueo tiene coste: el guardia se acerca despacio mientras cubre.
        const float dx = player.position.x - position.x;
        position.x += (dx > 0 ? 1.0f : -1.0f) * profile.repositionSpeed * 0.25f * dt;
        if (stateTimer <= 0) {
            if (profile.special == EnemySpecial::GuardCounter && specialTimer <= 0.0f) {
                pendingSpecial = EnemySpecial::GuardCounter;
                specialTimer = profile.specialCooldown;
                stateTimer = attackDuration;
                attackElapsed = 0;
                hasHit = false;
                EnterState(StreetEnemyState::Attack);
            } else {
                EnterState(StreetEnemyState::Idle);
            }
        }
        return;
    }

    if (state == StreetEnemyState::Attack) {
        stateTimer -= dt;
        attackElapsed += dt;
        if (stateTimer <= 0 || animator.isFinished) {
            attackCooldown = profile.attackCooldown;
            pendingSpecial = EnemySpecial::None;
            EnterState(StreetEnemyState::Idle);
        }
        return;
    }

    if (state == StreetEnemyState::Special) {
        stateTimer -= dt;
        attackElapsed += dt;
        if (!telegraphing) RunSpecial(dt, player, world);
        if (stateTimer <= 0) {
            attackCooldown = profile.attackCooldown;
            pendingSpecial = EnemySpecial::None;
            comboHits = 0;
            EnterState(StreetEnemyState::Idle);
        }
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        return;
    }

    if (player.state == PlayerState::Defeat) {
        EnterState(StreetEnemyState::Idle);
        return;
    }

    // --- movimiento segun el estado decidido ---
    const float dx = player.position.x - position.x;
    const float dy = player.position.y - position.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    if (state == StreetEnemyState::Chase) {
        float chaseSpeed = moveSpeed;
        if (type == StreetEnemyType::Mutant && distance > 280) chaseSpeed *= 0.82f;
        if (type == StreetEnemyType::ChemicalSoldier && std::abs(dx) < 360) chaseSpeed *= 0.78f;
        if (distance > 0.001f) {
            position.x += (dx / distance) * chaseSpeed * dt;
            const float depthBias = profile.flankBias > 0.4f ? flankSide * 0.35f : 0.0f;
            position.y += ((dy / distance) + depthBias) * chaseSpeed * 0.72f * dt;
        }
    } else if (state == StreetEnemyState::Position) {
        const float target = profile.preferredDistance;
        const float toward = std::abs(dx) < target ? -1.0f : 1.0f;
        position.x += (dx > 0 ? 1.0f : -1.0f) * toward * profile.repositionSpeed * dt;
        position.y += (dy > 0 ? 1.0f : -1.0f) * profile.repositionSpeed * 0.35f * dt;
        stateTimer -= dt;
    } else if (state == StreetEnemyState::Retreat) {
        position.x -= (dx > 0 ? 1.0f : -1.0f) * profile.repositionSpeed * dt;
        stateTimer -= dt;
        if (stateTimer <= 0) EnterState(StreetEnemyState::Idle);
    } else if (state == StreetEnemyState::Patrol) {
        position.x += flankSide * profile.repositionSpeed * 0.25f * dt;
        if (GetRandomValue(0, 400) == 0) flankSide = -flankSide;
    }

    // --- decision periodica ---
    decisionTimer -= dt;
    if (decisionTimer <= 0.0f && hitstunTimer <= 0.0f) {
        decisionTimer = profile.reactionTime;
        Decide(dt, player);
    }

    position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
    position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
}

bool StreetEnemy::AttackIsActive() const {
    if (state == StreetEnemyState::Attack) {
        return attackElapsed >= attackDuration * 0.34f && attackElapsed <= attackDuration * 0.74f;
    }
    if (state == StreetEnemyState::Special && !telegraphing) {
        return pendingSpecial == EnemySpecial::Charge ||
               pendingSpecial == EnemySpecial::HeavySlam ||
               pendingSpecial == EnemySpecial::Frenzy;
    }
    return false;
}

CombatBox StreetEnemy::GetHurtbox() const {
    if (!active || IsDefeated()) return {};
    const EnemyProfile& profile = Profile();
    return {position.x - profile.bodyWidth * 0.5f, position.y - profile.bodyHeight,
            profile.bodyWidth, profile.bodyHeight};
}

CombatBox StreetEnemy::GetAttackHitbox() const {
    if (!AttackIsActive()) return {};
    const EnemyProfile& profile = Profile();
    const float direction = facing == Facing::Right ? 1.f : -1.f;
    float width = attackRange * 0.68f;
    float height = 52.f;
    float forward = attackRange * 0.52f;

    if (state == StreetEnemyState::Special) {
        if (pendingSpecial == EnemySpecial::HeavySlam) { width = 220.f; height = 78.f; forward = 80.f; }
        else if (pendingSpecial == EnemySpecial::Charge) { width = 110.f; height = 92.f; forward = 55.f; }
    } else if (pendingSpecial == EnemySpecial::GuardCounter) {
        width = attackRange * 0.95f;
        forward = attackRange * 0.60f;
    }
    (void)profile;

    const float cx = position.x + direction * forward;
    const float cy = position.y - 66.f;
    return {cx - width * 0.5f, cy - height * 0.5f, width, height};
}

void StreetEnemy::TakeDamage(int damage, Vector3D knockback) {
    if (!active || state == StreetEnemyState::Defeat) return;
    const EnemyProfile& profile = Profile();

    hp = std::max(0, hp - damage);
    const EnemyAnimationLayout layout = LayoutFor(type);

    if (hp == 0) {
        velocity = knockback;
        state = StreetEnemyState::Defeat;
        stateTimer = 0.85f;
        telegraphing = false;
        if (animator.texture.id != 0) animator.Play({layout.deathStart, layout.deathEnd, .14f, false});
        else { animator.isFinished = true; animator.isPlaying = false; }
        return;
    }

    // Armor: los pesados no pierden su turno con golpes ligeros.
    if (profile.hasArmor && damage < profile.armorThreshold &&
        (state == StreetEnemyState::Attack || state == StreetEnemyState::Special)) {
        if (animator.texture.id != 0 && GetRandomValue(0, 1) == 0) {
            // solo parpadeo visual, el ataque continua
        }
        return;
    }

    velocity = knockback;
    state = StreetEnemyState::Hit;
    stateTimer = 0.30f;
    telegraphing = false;
    pendingSpecial = EnemySpecial::None;
    if (animator.texture.id != 0) animator.Play({layout.hitFrame, layout.hitFrame, .09f, false, {8, 9}});
}

void StreetEnemy::Draw() const {
    if (!active) return;
    const Stats s = GetStats(type);
    const EnemyProfile& profile = Profile();
    const Vector2 p = position.ToScreen();
    const float ds = DepthScale(position.y);
    const bool authored = animator.texture.id != 0 && !animator.frames.empty();
    const float visualScale = authored ? s.scale * ds : ds;

    DrawEllipse((int)p.x, (int)p.y, 26 * s.scale * ds, 8.5f * ds, {0, 0, 0, 145});

    // Telegraph: el jugador debe poder leer el ataque antes de que salga.
    if (IsTelegraphing()) {
        const float pulse = 0.4f + 0.6f * std::sin((float)GetTime() * 22.0f);
        const Color warn = state == StreetEnemyState::Special
            ? Color{255, 95, 60, (unsigned char)(120 + 80 * pulse)}
            : Color{255, 210, 90, (unsigned char)(80 + 70 * pulse)};
        DrawEllipseLines((int)p.x, (int)(p.y - s.bodyHeight * ds * 0.5f),
                         s.bodyWidth * ds * 0.85f, s.bodyHeight * ds * 0.6f, warn);
    }

    if (IsGuarding()) {
        DrawEllipseLines((int)p.x, (int)(p.y - s.bodyHeight * ds * 0.52f),
                         s.bodyWidth * ds * 0.78f, s.bodyHeight * ds * 0.58f,
                         {110, 190, 245, 225});
    }

    if (authored) {
        Color tint = WHITE;
        if (state == StreetEnemyState::Hit) tint = {255, 225, 225, 255};
        if (state == StreetEnemyState::Stun) tint = {215, 215, 160, 255};
        if (state == StreetEnemyState::Defeat) tint = {180, 180, 185, 255};
        animator.Draw(p, visualScale, facing == Facing::Left, tint);
    } else {
        Color tint = state == StreetEnemyState::Hit ? WHITE
                   : state == StreetEnemyState::Defeat ? Color{110, 110, 115, 255}
                   : s.fallbackTint;
        DrawFallbackEnemy(p, s, type, ds, tint);
    }

    if (state != StreetEnemyState::Defeat) {
        const int width = s.scale >= 1.08f ? 82 : 68;
        const int x = (int)(p.x - width * .5f);
        const int y = (int)(p.y - s.bodyHeight * ds - 11);
        DrawRectangle(x, y, width, 6, {8, 9, 11, 210});
        DrawRectangle(x, y, (int)(width * ((float)hp / maxHp)), 6, ThreatColor(type));
        if (maxGuardHealth > 0.0f && guardHealth < maxGuardHealth) {
            DrawRectangle(x, y - 5, width, 3, {8, 9, 11, 190});
            DrawRectangle(x, y - 5, (int)(width * (guardHealth / maxGuardHealth)), 3,
                          {110, 185, 240, 230});
        }
        if (state == StreetEnemyState::Stun) {
            DrawText("GUARDIA ROTA", (int)p.x - 40, y - 20, 11, {255, 210, 110, 235});
        }
    }
    (void)profile;
}

}  // namespace district_fury
