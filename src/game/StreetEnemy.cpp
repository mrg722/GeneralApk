#include "game/StreetEnemy.h"
#include "game/combat/CombatWorld.h"
#include "rendering/AssetManager.h"
#include "game/combat/CombatMath.h"
#include "game/events/GameEvents.h"
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

// V8: tiempos en el suelo.
constexpr float kDownSeconds = 0.55f;
constexpr float kWakeUpSeconds = 0.35f;

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

    attackIndex = 0; attackPrevElapsed = 0; lungeApplied = 0; attackHitsDone = 0;
    telegraphFlash = 0; footstepTimer = 0;
    juggleUsed = 0; airHits = 0; hitCombo = 0; juggleExhausted = false; otgUsed = false;
    pendingGroundBounce = groundBounced = pendingWallBounce = wallBounced = false;
    grabEscapeTimer = 0; throwTimer = 0; throwDuration = 0.6f; throwDamage = 8; throwerSkin = -1;
    bodyHits.clear();

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
    GameEvent e;
    e.type = GameEventType::EnemyGuardBreak;
    e.x = position.x; e.y = position.y; e.heavy = true; e.weight = 3; e.kind = HitKind::GuardBreak;
    EmitGameEvent(e);
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
        case StreetEnemyState::Launched: return "LAUNCHED";
        case StreetEnemyState::Down:     return "DOWN";
        case StreetEnemyState::WakeUp:   return "WAKEUP";
        case StreetEnemyState::WallBounce: return "WALLBOUNCE";
        case StreetEnemyState::Grabbed:  return "GRABBED";
        case StreetEnemyState::Thrown:   return "THROWN";
        case StreetEnemyState::Defeat:   return "DEAD";
    }
    return "DEAD";
}

namespace {

// Telegrafia (F1.9): evento Telegraph para el audio (solo al pasar a true) y
// destello rojo de 0.15 s en el sprite.
void BeginTelegraph(StreetEnemy& e, float seconds, bool special) {
    if (!e.telegraphing || e.telegraphTimer <= 0.0f) {
        GameEvent ev;
        ev.type = GameEventType::Telegraph;
        ev.x = e.position.x; ev.y = e.position.y;
        ev.fromEnemy = true;
        ev.heavy = special || e.Profile().hasArmor;
        ev.weight = ev.heavy ? 3 : 1;
        ev.value = static_cast<int>(e.type);
        EmitGameEvent(ev);
        e.telegraphFlash = 0.15f;
    }
    e.telegraphing = true;
    e.telegraphTimer = seconds;
}

int AfterBlockAttack(StreetEnemyType type) {
    const EnemyMoveSet& m = GetEnemyMoveSet(type);
    for (std::size_t i = 0; i < m.attacks.size(); ++i)
        if (m.attacks[i].afterBlock) return static_cast<int>(i);
    return -1;
}

// Eleccion del ataque de datos por distancia y estado del jugador (pesos del
// perfil): golpe bajo / alto x3 contra la guardia, nada bajo si salta.
int ChooseAttack(const StreetEnemy& e, const Player& player, float horizontal) {
    const EnemyMoveSet& m = GetEnemyMoveSet(e.type);
    if (m.attacks.empty()) return 0;
    float w[8] = {};
    float total = 0.0f;
    const bool guarding = player.IsBlocking();
    const bool airborne = player.position.z > 4.0f;
    for (std::size_t i = 0; i < m.attacks.size() && i < 8; ++i) {
        const EnemyAttackDef& a = m.attacks[i];
        if (a.afterBlock) continue;
        if (horizontal < a.minDist || horizontal > a.maxDist) continue;
        if (airborne && (a.low || a.kind == EnemyMoveKind::Roar)) continue;
        float k = a.weight;
        if (guarding && a.vsGuard) k *= 3.0f;
        w[i] = k;
        total += k;
    }
    if (total <= 0.0f) return 0;
    float pick = static_cast<float>(GetRandomValue(0, 999)) / 1000.0f * total;
    for (std::size_t i = 0; i < m.attacks.size() && i < 8; ++i) {
        if (w[i] <= 0.0f) continue;
        if (pick < w[i]) return static_cast<int>(i);
        pick -= w[i];
    }
    return 0;
}

}  // namespace

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
        BeginTelegraph(*this, profile.special == EnemySpecial::Charge ? 0.55f
                            : profile.special == EnemySpecial::HeavySlam ? 0.62f : 0.35f,
                       true);
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
        StartAttack(ChooseAttack(*this, player, horizontal));
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

bool StreetEnemy::PrepareAtlasAnimator(Animator& animator, StreetEnemyType type) {
    EnsureAnimator(animator, type);
    return animator.texture.id != 0 && !animator.frames.empty();
}

void StreetEnemy::UseReferenceSkin(const Animator& templ, float scale, const char* label) {
    referenceSkin = true;
    skinAnimator = templ;
    skinScale = scale;
    skinLabel = label;
    skinClip.clear();
    skinState = StreetEnemyState::Defeat;
    if (state == StreetEnemyState::Defeat) skinState = StreetEnemyState::Idle;
}

namespace {
const char* SkinClipFor(StreetEnemyState s, int attackVariant) {
    static const char* kAttacks[] = {"atk1", "atk2", "atk3", "atk4"};
    switch (s) {
        case StreetEnemyState::Chase:
        case StreetEnemyState::Position:
        case StreetEnemyState::Retreat:  return "walk";
        case StreetEnemyState::Attack:   return kAttacks[attackVariant % 4];
        case StreetEnemyState::Special:  return "special";
        case StreetEnemyState::Hit:
        case StreetEnemyState::Stun:     return "hit";
        case StreetEnemyState::Launched:
        case StreetEnemyState::WallBounce:
        case StreetEnemyState::Thrown:   return "air";
        case StreetEnemyState::Grabbed:  return "hit";
        case StreetEnemyState::Down:     return "knockdown";
        case StreetEnemyState::WakeUp:   return "getup";
        case StreetEnemyState::Defeat:   return "defeat";
        default:                         return "idle";
    }
}
}  // namespace

void StreetEnemy::Update(float dt, const Player& player, CombatWorld* world) {
    if (!active) return;
    if (referenceSkin) {
        if (state != skinState) {
            if (state == StreetEnemyState::Attack) ++skinAttackVariant;
            skinState = state;
            const char* clip = SkinClipFor(state, skinAttackVariant);
            if (skinClip != clip && skinAnimator.PlayNamed(clip)) skinClip = clip;
        }
        skinAnimator.Update(dt);
    }
    EnsureAnimator(animator, type);
    animator.Update(dt);

    const EnemyProfile& profile = Profile();

    attackCooldown = std::max(0.0f, attackCooldown - dt);
    specialTimer = std::max(0.0f, specialTimer - dt);
    hitstunTimer = std::max(0.0f, hitstunTimer - dt);
    hurtShakeT += dt;
    telegraphFlash = std::max(0.0f, telegraphFlash - dt);
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
        if (position.z > 0.0f) {   // derrotado en el aire: cae (antes se quedaba flotando)
            position.z = std::max(0.0f, position.z + velocity.z * dt - 750.0f * dt * dt);
            velocity.z -= 1500.0f * dt;
        }
        if (stateTimer <= 0) active = false;
        return;
    }

    if (state == StreetEnemyState::Stun) {
        stateTimer -= dt;
        if (stateTimer <= 0) { guardHealth = maxGuardHealth * 0.4f; EnterState(StreetEnemyState::Idle); }
        return;
    }

    // ---- F1.9: estados sin decision de IA ----
    if (state == StreetEnemyState::Launched || state == StreetEnemyState::WallBounce) {
        UpdateAirborne(dt);
        return;
    }
    if (state == StreetEnemyState::Down) {
        stateTimer -= dt;
        DampedStep(position.x, velocity.x, 0.84f, dt);
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        if (stateTimer <= 0.0f) {
            state = StreetEnemyState::WakeUp;
            stateTimer = kWakeUpSeconds;
            velocity = {0, 0, 0};
        }
        return;
    }
    if (state == StreetEnemyState::WakeUp) {
        stateTimer -= dt;
        if (stateTimer <= 0.0f) {
            juggleUsed = 0; airHits = 0; hitCombo = 0; juggleExhausted = false; otgUsed = false;
            pendingGroundBounce = groundBounced = pendingWallBounce = wallBounced = false;
            decisionTimer = profile.reactionTime;
            animator.isFinished = true;   // EnterState(Idle) vuelve a poner el idle
            EnterState(StreetEnemyState::Idle);
            PlayIdle(animator, type);
        }
        return;
    }
    if (state == StreetEnemyState::Grabbed) {
        grabEscapeTimer += dt;
        // Los que tienen armadura se zafan a los 0.6 s.
        if (profile.hasArmor && grabEscapeTimer >= 0.6f) SetGrabbed(false, position);
        return;
    }
    if (state == StreetEnemyState::Thrown) {
        throwTimer += dt;
        position.x += velocity.x * dt;
        position.z += velocity.z * dt - 750.0f * dt * dt;
        velocity.z -= 1500.0f * dt;
        if (position.x <= arenaMinX || position.x >= arenaMaxX) {
            position.x = std::clamp(position.x, arenaMinX, arenaMaxX);
            velocity.x = -0.3f * velocity.x;
        }
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        if (position.z <= 0.0f) {
            position.z = 0.0f;
            hp = std::max(0, hp - throwDamage);   // dano de impacto contra el suelo
            if (hp == 0) {
                state = StreetEnemyState::Defeat;
                stateTimer = 0.85f;
                const EnemyAnimationLayout layout = LayoutFor(type);
                if (animator.texture.id != 0) animator.Play({layout.deathStart, layout.deathEnd, .14f, false});
                GameEvent e;
                e.type = GameEventType::EnemyKilled;
                e.x = position.x; e.y = position.y; e.value = static_cast<int>(type);
                e.heavy = true; e.skin = throwerSkin;
                EmitGameEvent(e);
            } else {
                EnterDown();
            }
        }
        return;
    }

    if (state == StreetEnemyState::Hit) {
        stateTimer -= dt;
        // B11: deslizamiento independiente del frame rate (antes 0.84 por frame).
        DampedStep(position.x, velocity.x, 0.84f, dt);
        DampedStep(position.y, velocity.y, 0.84f, dt);
        if (stateTimer <= 0 && hitstunTimer <= 0.0f) {
            hitCombo = 0;   // se corto la racha: el escalado de combo vuelve a 0
            EnterState(StreetEnemyState::Idle);
        }
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
            const int counter = AfterBlockAttack(type);
            const bool canCounter = counter >= 0 &&
                (profile.special == EnemySpecial::GuardCounter ? specialTimer <= 0.0f
                                                               : GetRandomValue(0, 99) < 50);
            if (canCounter) {
                if (profile.special == EnemySpecial::GuardCounter) specialTimer = profile.specialCooldown;
                StartAttack(counter);
            } else {
                EnterState(StreetEnemyState::Idle);
            }
        }
        return;
    }

    if (state == StreetEnemyState::Attack) {
        const EnemyAttackDef& a = CurrentAttack();
        stateTimer -= dt;
        attackPrevElapsed = attackElapsed;
        attackElapsed += dt;
        const float dir = facing == Facing::Right ? 1.0f : -1.0f;
        if (a.lunge > 0.0f) {   // root motion: lunge con outCubic sobre S+A (frena x0.3 al conectar)
            const float u = std::clamp(attackElapsed / std::max(1e-4f, a.startup + a.active), 0.0f, 1.0f);
            const float want = a.lunge * CombatEase(Ease::OutCubic, u);
            float step = want - lungeApplied;
            if (hasHit) step *= 0.3f;
            position.x += dir * step;
            lungeApplied = want;
        }
        if (a.hits > 1 && attackElapsed >= a.startup) {   // multi-golpe: re-golpe por tramo del activo
            const int idx = static_cast<int>((attackElapsed - a.startup) / std::max(1e-4f, a.active / a.hits));
            if (idx > attackHitsDone && idx < a.hits) { attackHitsDone = idx; hasHit = false; }
        }
        if (attackElapsed >= a.Total()) {
            attackCooldown = profile.attackCooldown;
            pendingSpecial = EnemySpecial::None;
            EnterState(StreetEnemyState::Idle);
        }
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
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

    // Pasos de los pesados (audio: Footstep, peso 3).
    const bool walking = state == StreetEnemyState::Chase || state == StreetEnemyState::Position ||
                         state == StreetEnemyState::Retreat || state == StreetEnemyState::Patrol;
    if (walking && profile.hasArmor) {
        footstepTimer -= dt;
        if (footstepTimer <= 0.0f) {
            footstepTimer = 0.42f;
            GameEvent e;
            e.type = GameEventType::Footstep;
            e.x = position.x; e.y = position.y; e.weight = 3; e.heavy = true; e.fromEnemy = true;
            e.value = static_cast<int>(type);
            EmitGameEvent(e);
        }
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

const EnemyAttackDef& StreetEnemy::CurrentAttack() const {
    const EnemyMoveSet& m = GetEnemyMoveSet(type);
    static const EnemyAttackDef kNone{};
    if (m.attacks.empty()) return kNone;
    return m.attacks[static_cast<std::size_t>(std::clamp(attackIndex, 0, static_cast<int>(m.attacks.size()) - 1))];
}

float StreetEnemy::AttackScale() const { return EnemyAttackScale(type); }
int StreetEnemy::JuggleBudget() const { return GetEnemyMoveSet(type).juggleBudget; }

bool StreetEnemy::InAir() const {
    return state == StreetEnemyState::Launched || state == StreetEnemyState::WallBounce ||
           (position.z > 0.0f && state != StreetEnemyState::Grabbed && state != StreetEnemyState::Thrown);
}

void StreetEnemy::StartAttack(int index) {
    const EnemyMoveSet& m = GetEnemyMoveSet(type);
    attackIndex = m.attacks.empty() ? 0 : std::clamp(index, 0, static_cast<int>(m.attacks.size()) - 1);
    const EnemyAttackDef& a = CurrentAttack();
    EnterState(StreetEnemyState::Attack);
    stateTimer = a.Total();
    attackElapsed = 0.0f;
    attackPrevElapsed = 0.0f;
    lungeApplied = 0.0f;
    attackHitsDone = 0;
    hasHit = false;
    pendingSpecial = a.counterSpecial ? EnemySpecial::GuardCounter : EnemySpecial::None;
    BeginTelegraph(*this, a.startup, false);
    // Los 4 cuadros de ataque cubren arranque + activo; en la recuperacion queda
    // el ultimo.
    const EnemyAnimationLayout layout = LayoutFor(type);
    animator.Play({layout.attackStart, layout.attackEnd, std::max(0.03f, (a.startup + a.active) / 4.0f), false});
}

void StreetEnemy::EnterDown() {
    state = StreetEnemyState::Down;
    stateTimer = kDownSeconds;
    position.z = 0.0f;
    velocity = {velocity.x * 0.4f, 0.0f, 0.0f};
    telegraphing = false;
    pendingSpecial = EnemySpecial::None;
    pendingGroundBounce = false;
    pendingWallBounce = false;
    bodyHits.clear();
    GameEvent e;
    e.type = GameEventType::KnockdownLand;
    e.x = position.x; e.y = position.y; e.fromEnemy = true;
    e.heavy = Profile().hasArmor; e.weight = e.heavy ? 3 : 2;
    e.value = static_cast<int>(type);
    EmitGameEvent(e);
}

// V8: gravedad creciente con los golpes en el aire, integracion exacta por paso
// (misma altura maxima a 30 y a 60 fps), rebote en el borde y en el suelo.
void StreetEnemy::UpdateAirborne(float dt) {
    const float g = 1500.0f * (1.0f + 0.12f * static_cast<float>(airHits));
    position.z += velocity.z * dt - 0.5f * g * dt * dt;
    velocity.z -= g * dt;
    DampedStep(position.x, velocity.x, 0.95f, dt);   // B11: por dt
    if (position.x <= arenaMinX || position.x >= arenaMaxX) {
        position.x = std::clamp(position.x, arenaMinX, arenaMaxX);
        if (pendingWallBounce && !wallBounced) {
            wallBounced = true;
            pendingWallBounce = false;
            velocity.x = -0.5f * velocity.x;
            velocity.z = 260.0f;
            state = StreetEnemyState::WallBounce;
            stateTimer = 0.25f;
            GameEvent e;
            e.type = GameEventType::KnockdownLand;
            e.x = position.x; e.y = position.y; e.fromEnemy = true; e.heavy = true; e.weight = 3;
            EmitGameEvent(e);
        } else {
            velocity.x = 0.0f;
        }
    }
    position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
    if (state == StreetEnemyState::WallBounce) {
        stateTimer -= dt;
        if (stateTimer <= 0.0f) state = StreetEnemyState::Launched;
    }
    if (position.z <= 0.0f) {
        position.z = 0.0f;
        if (pendingGroundBounce && !groundBounced && !juggleExhausted) {
            groundBounced = true;
            pendingGroundBounce = false;
            velocity.z = 300.0f;
            velocity.x *= 0.6f;
            state = StreetEnemyState::Launched;
            GameEvent e;
            e.type = GameEventType::KnockdownLand;
            e.x = position.x; e.y = position.y; e.fromEnemy = true; e.weight = 2;
            EmitGameEvent(e);
        } else {
            EnterDown();
        }
    }
}

bool StreetEnemy::CanBeGrabbed() const {
    if (!active || IsDefeated()) return false;
    if (type == StreetEnemyType::Brute || type == StreetEnemyType::Mutant || type == StreetEnemyType::ArmoredGuard)
        return false;
    switch (state) {
        case StreetEnemyState::Launched: case StreetEnemyState::WallBounce: case StreetEnemyState::Down:
        case StreetEnemyState::WakeUp: case StreetEnemyState::Grabbed: case StreetEnemyState::Thrown:
            return false;
        case StreetEnemyState::Attack: {
            const EnemyAttackDef& a = CurrentAttack();
            return !(a.armor && attackElapsed <= a.startup + a.active);
        }
        default:
            return true;
    }
}

void StreetEnemy::SetGrabbed(bool on, Vector3D hand) {
    if (!active || IsDefeated()) return;
    if (on) {
        if (state != StreetEnemyState::Grabbed) {
            state = StreetEnemyState::Grabbed;
            grabEscapeTimer = 0.0f;
            telegraphing = false;
            pendingSpecial = EnemySpecial::None;
            velocity = {0, 0, 0};
        }
        position = hand;
        return;
    }
    if (state == StreetEnemyState::Grabbed) {
        position.z = 0.0f;
        velocity = {0, 0, 0};
        attackCooldown = std::max(attackCooldown, 0.3f);
        EnterState(StreetEnemyState::Idle);
    }
}

void StreetEnemy::Throw(const HitInfo& info, Vector3D vel) {
    if (!active || IsDefeated()) return;
    if (info.damage > 0) {
        hp = std::max(0, hp - info.damage);
        if (hp == 0) {
            state = StreetEnemyState::Defeat;
            stateTimer = 0.85f;
            velocity = vel;
            const EnemyAnimationLayout layout = LayoutFor(type);
            if (animator.texture.id != 0) animator.Play({layout.deathStart, layout.deathEnd, .14f, false});
            GameEvent e;
            e.type = GameEventType::EnemyKilled;
            e.x = position.x; e.y = position.y; e.value = static_cast<int>(type);
            e.heavy = true; e.attack = info.sourceAttack; e.skin = info.attackerSkin;
            EmitGameEvent(e);
            return;
        }
    }
    state = StreetEnemyState::Thrown;
    velocity = vel;
    position.z = std::max(position.z, 1.0f);
    throwTimer = 0.0f;
    // Tiempo de vuelo de la parabola (para el giro de 540 grados).
    const float disc = vel.z * vel.z + 2.0f * 1500.0f * position.z;
    throwDuration = std::max(0.2f, (vel.z + std::sqrt(std::max(0.0f, disc))) / 1500.0f);
    throwerSkin = info.attackerSkin;
    bodyHits.clear();
    telegraphing = false;
    pendingSpecial = EnemySpecial::None;
    juggleExhausted = true;
}

bool StreetEnemy::IsFlyingBody() const {
    if (!active || IsDefeated()) return false;
    if (state == StreetEnemyState::Thrown) return true;
    return (state == StreetEnemyState::Launched || state == StreetEnemyState::WallBounce) &&
           std::fabs(velocity.x) >= 420.0f;
}

bool StreetEnemy::AttackIsActive() const {
    if (state == StreetEnemyState::Attack) {
        const EnemyAttackDef& a = CurrentAttack();
        return attackElapsed >= a.startup && attackElapsed <= a.startup + a.active;
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
    if (state == StreetEnemyState::Down) return {position.x - 38.0f, position.y - 36.0f, 76.0f, 36.0f};
    return {position.x - profile.bodyWidth * 0.5f, position.y - profile.bodyHeight,
            profile.bodyWidth, profile.bodyHeight};
}

namespace {
// Caja historica de los specials (Charge / HeavySlam / Frenzy), en pantalla.
CombatBox SpecialBox(const StreetEnemy& e) {
    const float direction = e.facing == Facing::Right ? 1.f : -1.f;
    float width = e.attackRange * 0.68f;
    float height = 52.f;
    float forward = e.attackRange * 0.52f;
    if (e.pendingSpecial == EnemySpecial::HeavySlam) { width = 220.f; height = 78.f; forward = 80.f; }
    else if (e.pendingSpecial == EnemySpecial::Charge) { width = 110.f; height = 92.f; forward = 55.f; }
    const float cx = e.position.x + direction * forward;
    const float cy = e.position.y - 66.f;
    return {cx - width * 0.5f, cy - height * 0.5f, width, height};
}
}  // namespace

bool StreetEnemy::AttackShapeHits(const Rect2& hurt, Vector2* contact) const {
    if (!AttackIsActive()) return false;
    if (state == StreetEnemyState::Special) {
        const CombatBox b = SpecialBox(*this);
        const Rect2 r = ScreenBoxToPlane(b.x, b.y, b.width, b.height, position.y);
        if (!RectOverlap(r, hurt)) return false;
        if (contact) *contact = RectContact(r, hurt);
        return true;
    }
    const EnemyAttackDef& a = CurrentAttack();
    const float s = AttackScale();
    if (a.areaRadius > 0.0f) {
        const float r = a.areaRadius * s;
        const float top = a.kind == EnemyMoveKind::Roar ? 200.0f * s : 40.0f * s;
        const Rect2 area{position.x - r, position.z - 10.0f, position.x + r, position.z + top};
        if (!RectOverlap(area, hurt)) return false;
        if (contact) *contact = RectContact(area, hurt);
        return true;
    }
    const float act = std::max(1e-4f, a.active);
    const float u = std::clamp((attackElapsed - a.startup) / act, 0.0f, 1.0f);
    const float uPrev = std::clamp((attackPrevElapsed - a.startup) / act, 0.0f, 1.0f);
    const float f = facing == Facing::Right ? 1.0f : -1.0f;
    const ArcHit h = LimbArcVsRect(a.arc, position.x, position.z, f, s, uPrev, u, hurt);
    if (h.hit && contact) *contact = h.contact;
    return h.hit;
}

CombatBox StreetEnemy::GetAttackHitbox() const {
    if (!AttackIsActive()) return {};
    if (state == StreetEnemyState::Special) return SpecialBox(*this);
    // Caja envolvente (pantalla) de la forma real: depuracion y compatibilidad.
    const EnemyAttackDef& a = CurrentAttack();
    const float s = AttackScale();
    float x0, x1, h0, h1;
    if (a.areaRadius > 0.0f) {
        const float r = a.areaRadius * s;
        x0 = position.x - r; x1 = position.x + r;
        h0 = position.z; h1 = position.z + (a.kind == EnemyMoveKind::Roar ? 200.0f : 40.0f) * s;
    } else {
        const float u = std::clamp((attackElapsed - a.startup) / std::max(1e-4f, a.active), 0.0f, 1.0f);
        const LimbPose lp = EvalLimbArc(a.arc, position.x, position.z, facing == Facing::Right ? 1.0f : -1.0f, s, u);
        x0 = std::min(lp.base.x, lp.tip.x) - lp.radius; x1 = std::max(lp.base.x, lp.tip.x) + lp.radius;
        h0 = std::min(lp.base.y, lp.tip.y) - lp.radius; h1 = std::max(lp.base.y, lp.tip.y) + lp.radius;
    }
    return {x0, position.y - h1, x1 - x0, h1 - h0};
}

void StreetEnemy::TakeDamage(int damage, Vector3D knockback) {
    // Envoltorio historico: empuje = knockback tal cual, stun fijo 0.30 s, sin
    // guardia ni rompe-guardia.
    HitInfo info;
    info.damage = damage;
    info.dirX = knockback.x >= 0.0f ? 1.0f : -1.0f;
    info.knockback = std::fabs(knockback.x);
    info.launch = knockback.z;
    info.hitstun = 0.30f;
    info.unblockable = true;
    ApplyHit(info);
}

bool StreetEnemy::InAttackStartup() const {
    // Ataque de datos: el arranque lo dice su EnemyAttackDef (fiable frame a
    // frame); el resto (specials), la telegrafia.
    if (state == StreetEnemyState::Attack) return attackElapsed < CurrentAttack().startup;
    return IsTelegraphing();
}

int StreetEnemy::AttackPriority() const {
    if (state == StreetEnemyState::Special) return 3;
    if (state == StreetEnemyState::Attack && CurrentAttack().armor) return 2;
    return Profile().hasArmor ? 2 : 1;
}

HitResult StreetEnemy::ApplyHit(const HitInfo& info) {
    if (!active || state == StreetEnemyState::Defeat) return HitResult::Ignored;
    // F1.9: levantandose es invulnerable; en vuelo de proyeccion no se le golpea;
    // en el suelo solo entra un golpe otg por derribo.
    if (state == StreetEnemyState::WakeUp || state == StreetEnemyState::Thrown) return HitResult::Ignored;
    const bool down = state == StreetEnemyState::Down;
    if (down && (!info.otg || otgUsed)) return HitResult::Ignored;
    const EnemyProfile& profile = Profile();
    const bool inAir = InAir();
    const float dir = info.dirX >= 0.0f ? 1.0f : -1.0f;
    const float kb = std::fabs(info.knockback);

    // V8: escalado de dano y degradacion de hitstun por racha; presupuesto de juggle.
    int damage = info.damage;
    if (damage > 0) {
        const float k = std::max(0.35f, 1.0f - 0.08f * static_cast<float>(std::max(0, hitCombo - 3)));
        damage = std::max(1, static_cast<int>(std::lround(damage * k)));
    }
    const int cost = std::max(1, info.juggleCost);
    const bool juggleFail = inAir && (juggleExhausted || juggleUsed + cost > JuggleBudget());
    if (juggleFail && damage > 0) damage = std::max(1, static_cast<int>(std::lround(damage * 0.3f)));
    const float hitstun = info.hitstun * std::pow(0.92f, static_cast<float>(hitCombo));
    ++hitCombo;

    hurtShakeT = 0.0f;
    hurtShakeAmp = VictimShakeAmplitude(damage);

    // Guardia (Block): un golpe normal se queda en la guardia (dano reducido a la
    // mitad, la guardia se gasta x2.6); el que rompe guardia la rompe y entra.
    if (IsGuarding() && !info.unblockable) {
        if (!info.breaksGuard) {
            damage = std::max(1, damage / 2);
            AbsorbGuard(damage);
            hp = std::max(1, hp - damage);
            return HitResult::Blocked;
        }
        BreakGuard();
        hp = std::max(0, hp - damage);
        if (hp > 0) return HitResult::Hit;   // queda aturdido (Stun)
    } else {
        hp = std::max(0, hp - damage);
    }
    const EnemyAnimationLayout layout = LayoutFor(type);

    if (hp == 0) {
        velocity = {dir * std::min(kb, 300.0f), 0.0f, std::max(0.0f, info.launch)};
        state = StreetEnemyState::Defeat;
        stateTimer = 0.85f;
        telegraphing = false;
        if (animator.texture.id != 0) animator.Play({layout.deathStart, layout.deathEnd, .14f, false});
        else { animator.isFinished = true; animator.isPlaying = false; }
        GameEvent e;
        e.type = GameEventType::EnemyKilled;
        e.x = position.x; e.y = position.y; e.value = static_cast<int>(type);
        e.heavy = info.heavy; e.attack = info.sourceAttack; e.skin = info.attackerSkin;
        EmitGameEvent(e);
        return HitResult::Killed;
    }

    if (state == StreetEnemyState::Grabbed) return HitResult::Hit;   // rodillazos: solo dano

    // Armor: los pesados no pierden su turno con golpes ligeros; los ataques de
    // datos con superarmadura (carga corta del Brute) aguantan en arranque + activo.
    if (!inAir && !down) {
        if (profile.hasArmor && damage < profile.armorThreshold &&
            (state == StreetEnemyState::Attack || state == StreetEnemyState::Special)) {
            return HitResult::Armored;
        }
        if (state == StreetEnemyState::Attack) {
            const EnemyAttackDef& a = CurrentAttack();
            if (a.armor && attackElapsed <= a.startup + a.active) return HitResult::Armored;
        }
    }

    telegraphing = false;
    pendingSpecial = EnemySpecial::None;
    if (animator.texture.id != 0) animator.Play({layout.hitFrame, layout.hitFrame, .09f, false, {8, 9}});

    if (down) {   // OTG: rebote corto y vuelve al suelo (no reinicia el derribo)
        otgUsed = true;
        juggleExhausted = true;
        velocity = {dir * 60.0f, 0.0f, 150.0f};
        position.z = std::max(position.z, 0.5f);
        state = StreetEnemyState::Launched;
        return HitResult::Hit;
    }

    if (inAir) {
        if (juggleFail) {   // presupuesto agotado: no levanta, cae a Down
            juggleExhausted = true;
            pendingGroundBounce = false;
            velocity.z = std::min(velocity.z, 0.0f);
            velocity.x = dir * std::min(kb, 180.0f) * 0.5f;
        } else {
            juggleUsed += cost;
            ++airHits;
            velocity.z = info.launch < 0.0f ? info.launch
                                             : std::max(info.launch, 220.0f) * std::pow(0.9f, static_cast<float>(airHits));
            velocity.x = dir * std::min(kb, 180.0f);
            if (info.groundBounce && !groundBounced) pendingGroundBounce = true;
            if (info.wallBounce && !wallBounced) { pendingWallBounce = true; velocity.x = dir * kb; }
        }
        if (state != StreetEnemyState::WallBounce) state = StreetEnemyState::Launched;
        stateTimer = hitstun;
        return HitResult::Hit;
    }

    // En el suelo.
    const bool groundSlam = info.groundBounce && !groundBounced;   // picada sobre alguien de pie
    if (info.launch > 0.0f || groundSlam || (info.knockdown && !info.low)) {
        juggleUsed = cost;
        airHits = 0;
        juggleExhausted = false;
        otgUsed = false;
        groundBounced = groundSlam;
        wallBounced = false;
        pendingGroundBounce = false;
        pendingWallBounce = info.wallBounce;
        velocity.z = info.launch > 0.0f ? info.launch : groundSlam ? 300.0f : 240.0f;
        velocity.x = dir * (info.wallBounce ? kb : std::min(kb, 300.0f));
        position.z = std::max(position.z, 0.5f);
        state = StreetEnemyState::Launched;
        stateTimer = hitstun;
        bodyHits.clear();
        return HitResult::Hit;
    }
    if (info.knockdown && info.low) {   // barrida: al suelo directo
        otgUsed = false;
        velocity = {dir * std::min(kb, 200.0f), 0.0f, 0.0f};
        EnterDown();
        return HitResult::Hit;
    }

    velocity = {dir * kb, 0.0f, 0.0f};
    state = StreetEnemyState::Hit;
    stateTimer = std::max(0.05f, hitstun);   // hitstun de la tabla (B03/B17), degradado (V8)
    ApplyHitstun(hitstun);
    return HitResult::Hit;
}


void StreetEnemy::Draw() const {
    if (!active) return;
    const Stats s = GetStats(type);
    Vector2 p = position.ToScreen();
    if (hurtShakeT < 0.3f) p.x += VictimShakeOffset(hurtShakeAmp, hurtShakeT);   // temblor de victima (V7)
    const EnemyProfile& profile = Profile();
    const float ds = DepthScale(position.y);
    const bool authored = animator.texture.id != 0 && !animator.frames.empty();
    const float visualScale = authored ? s.scale * ds : ds;

    // La sombra queda en el suelo aunque el enemigo este en el aire.
    DrawEllipse((int)p.x, (int)position.y, 26 * s.scale * ds, 8.5f * ds, {0, 0, 0, 145});

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

    if (referenceSkin && skinAnimator.texture.id != 0) {
        Color tint = WHITE;
        if (state == StreetEnemyState::Hit) tint = {255, 225, 225, 255};
        if (state == StreetEnemyState::Defeat) tint = {180, 180, 185, 255};
        skinAnimator.Draw(p, skinScale * ds, facing == Facing::Left, tint);
        if (skinLabel) DrawText(skinLabel, (int)p.x - MeasureText(skinLabel, 10) / 2, (int)p.y + 8, 10, {255, 200, 90, 220});
    } else if (authored) {
        Color tint = WHITE;
        if (state == StreetEnemyState::Hit || state == StreetEnemyState::Launched ||
            state == StreetEnemyState::WallBounce || state == StreetEnemyState::Grabbed)
            tint = {255, 225, 225, 255};
        if (state == StreetEnemyState::Stun) tint = {215, 215, 160, 255};
        if (state == StreetEnemyState::Defeat) tint = {180, 180, 185, 255};
        if (telegraphFlash > 0.0f) {   // destello rojo al empezar a telegrafiar
            const float k = std::clamp(telegraphFlash / 0.15f, 0.0f, 1.0f) * 0.65f;
            tint.g = (unsigned char)(tint.g * (1.0f - k));
            tint.b = (unsigned char)(tint.b * (1.0f - k));
        }
        // F1.9: poses procedurales sin arte nuevo (cuadros 8-11 girados/aplastados).
        const float fs = facing == Facing::Right ? 1.0f : -1.0f;
        int frame = -1;
        float angle = 0.0f, sy = 1.0f;
        switch (state) {
            case StreetEnemyState::Launched:
            case StreetEnemyState::WallBounce: {
                frame = velocity.z >= 0.0f ? 8 : 9;
                const float deg = std::atan2(velocity.z, std::max(1.0f, std::fabs(velocity.x))) * 57.29578f * 0.6f;
                angle = -fs * deg;   // hacia atras (lejos de su frente)
                break;
            }
            case StreetEnemyState::Down:
                frame = (kDownSeconds - stateTimer) < 0.12f ? 10 : 11;
                sy = 0.55f;
                break;
            case StreetEnemyState::WakeUp: {
                const float t = kWakeUpSeconds - stateTimer;
                frame = t < 0.12f ? 11 : t < 0.24f ? 10 : 0;
                sy = 0.55f + 0.45f * std::clamp(t / kWakeUpSeconds, 0.0f, 1.0f);
                break;
            }
            case StreetEnemyState::Grabbed:
                frame = 8;
                break;
            case StreetEnemyState::Thrown: {
                frame = 8;
                const float u = std::clamp(throwTimer / std::max(0.05f, throwDuration), 0.0f, 1.0f);
                angle = (velocity.x >= 0.0f ? 1.0f : -1.0f) * 540.0f * u;
                break;
            }
            case StreetEnemyState::Attack:
                sy = CurrentAttack().sy;
                break;
            default:
                break;
        }
        if (frame >= 0 || sy != 1.0f || angle != 0.0f) {
            const int f = frame >= 0 ? frame : animator.currentFrame;
            Vector2 at = p;
            if (angle != 0.0f && (state == StreetEnemyState::Thrown || state == StreetEnemyState::Launched ||
                                  state == StreetEnemyState::WallBounce)) {
                // Giro alrededor del centro del cuerpo, no de los pies.
                const float half = s.bodyHeight * visualScale / s.scale * 0.5f;
                const float rad = angle * 0.0174533f;
                at.x -= std::sin(rad) * half;
                at.y += std::cos(rad) * half - half;
            }
            animator.DrawFrameScaled(f, at, visualScale, visualScale * sy, facing == Facing::Left, tint, angle);
        } else {
            animator.Draw(p, visualScale, facing == Facing::Left, tint);
        }
    } else {
        Color tint = state == StreetEnemyState::Hit ? WHITE
                   : state == StreetEnemyState::Defeat ? Color{110, 110, 115, 255}
                   : s.fallbackTint;
        DrawFallbackEnemy(p, s, type, ds, tint);
    }

    if (state != StreetEnemyState::Defeat) {
        const int width = s.scale >= 1.08f ? 82 : 68;
        const int x = (int)(p.x - width * .5f);
        // La barra va por encima del sprite dibujado (antes usaba la altura de
        // la hurtbox y quedaba sobre la cabeza de los enemigos grandes).
        float spriteTop = p.y - s.bodyHeight * ds;
        if (referenceSkin) spriteTop = p.y - 78.f * skinScale * ds;
        else if (authored) {
            const SpriteFrame& f = animator.frames[(std::size_t)std::clamp(animator.currentFrame, 0, (int)animator.frames.size() - 1)];
            spriteTop = std::min(spriteTop, p.y - (f.pivotY - f.visualBounds.y) * visualScale);
        }
        const int y = (int)(spriteTop - 12);
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
