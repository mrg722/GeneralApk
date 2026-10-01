// Jugador: dibujo del personaje, aura de furia, escudo y proyectil de energia.
#include "game/player/PlayerCommon.h"

namespace district_fury {

void Player::DrawRageAura(Vector2 screen, float scale) const {
    if (rageState == RageState::Normal) return;

    float intensity = 1.0f;
    if (rageState == RageState::Starting) {
        intensity = 1.0f - std::clamp(rageStateTimer / kRageStartDuration, 0.0f, 1.0f);
    } else if (rageState == RageState::Ending) {
        intensity = std::clamp(rageStateTimer / kRageEndDuration, 0.0f, 1.0f);
    }
    if (intensity <= 0.01f) return;

    const float pulse = std::sin(rageAuraPhase * 9.0f) * 0.5f + 0.5f;
    const float bodyHeight = 112.0f * scale;
    const float bodyWidth = 40.0f * scale;

    // El aura envuelve el cuerpo: se compone de tres elipses apiladas
    // (piernas / torso / cabeza) en vez de un unico circulo plano.
    struct Blob { float offsetY; float rx; float ry; };
    const Blob blobs[3] = {
        {-bodyHeight * 0.16f, bodyWidth * 0.95f, bodyHeight * 0.22f},
        {-bodyHeight * 0.50f, bodyWidth * 1.05f, bodyHeight * 0.33f},
        {-bodyHeight * 0.84f, bodyWidth * 0.72f, bodyHeight * 0.22f},
    };

    for (int layer = 2; layer >= 0; --layer) {
        const float grow = 1.0f + layer * 0.16f + pulse * 0.07f;
        const unsigned char alpha = static_cast<unsigned char>(
            std::clamp((52.0f - layer * 14.0f) * intensity, 0.0f, 255.0f));
        const Color glow = {255, static_cast<unsigned char>(120 + layer * 30),
                            static_cast<unsigned char>(40 + layer * 20), alpha};
        for (const Blob& blob : blobs) {
            DrawEllipse(static_cast<int>(screen.x),
                        static_cast<int>(screen.y + blob.offsetY),
                        blob.rx * grow, blob.ry * grow, glow);
        }
    }

    // Lenguas de energia ascendentes ancladas a la silueta.
    const int tongues = 9;
    for (int i = 0; i < tongues; ++i) {
        const float phase = rageAuraPhase * 4.2f + static_cast<float>(i) * 1.17f;
        const float t = std::fmod(phase, 1.0f);
        const float side = (i % 2 == 0) ? -1.0f : 1.0f;
        const float x = screen.x + side * bodyWidth * (0.45f + 0.42f * std::sin(phase * 1.7f));
        const float y = screen.y - bodyHeight * (0.06f + t * 0.98f);
        const float size = (5.0f - t * 4.0f) * scale * (0.8f + pulse * 0.4f);
        if (size <= 0.4f) continue;
        const unsigned char alpha = static_cast<unsigned char>(
            std::clamp(215.0f * (1.0f - t) * intensity, 0.0f, 255.0f));
        DrawCircle(static_cast<int>(x), static_cast<int>(y), size,
                   {255, static_cast<unsigned char>(170 + 60 * (1.0f - t)), 70, alpha});
    }

    const unsigned char rimAlpha = static_cast<unsigned char>(
        std::clamp(150.0f * intensity, 0.0f, 255.0f));
    DrawEllipseLines(static_cast<int>(screen.x),
                     static_cast<int>(screen.y - bodyHeight * 0.50f),
                     bodyWidth * (1.22f + pulse * 0.08f),
                     bodyHeight * (0.60f + pulse * 0.04f),
                     {255, 205, 110, rimAlpha});
}

namespace {
Texture2D FxTexture() { return AssetManager::Get().GetTextureByPath("assets/fx/efectos_azules.png"); }
// Cuadro i de la rejilla 8x5 (celdas 128x96), centrado en `c`.
void DrawFx(const Texture2D& t, int i, Vector2 c, float scale, bool flip, Color tint = WHITE) {
    const Rectangle src{(float)(i % 8) * 128.0f, (float)(i / 8) * 96.0f, flip ? -128.0f : 128.0f, 96.0f};
    DrawTexturePro(t, src, {c.x - 64.0f * scale, c.y - 48.0f * scale, 128.0f * scale, 96.0f * scale}, {0, 0}, 0, tint);
}
}  // namespace

void Player::SpawnFx(int first, int count, Vector2 pos, float dur, float scale) {
    if (effects.size() > 12) effects.erase(effects.begin());
    effects.push_back({first, count, pos, 0.0f, dur, scale});
}

void Player::UpdateMotionFeel(float dt) {
    animClock += dt;
    // Efectos: chispa de impacto en cuanto un golpe conecta.
    for (Fx& e : effects) e.t += dt;
    effects.erase(std::remove_if(effects.begin(), effects.end(), [](const Fx& e) { return e.t >= e.dur; }), effects.end());
    if (hasHit && !prevHasHit && state == PlayerState::Attack) {   // impacto: sonido segun el golpe
        const bool heavyHit = GetAttack(currentAttack).heavy || activeSkill >= 0;
        AudioSystem::Get().Play(activeSkill >= 0 ? Sfx::EnergyImpact : heavyHit ? Sfx::HeavyHit
                                : attackPhase == AttackPhase::Kick ? Sfx::Kick : Sfx::Punch);
    }
    if (hasHit && !prevHasHit && state == PlayerState::Attack && !IsKfCharacter()) {
        const CombatBox b = GetAttackHitbox();
        const float dir = facing == Facing::Right ? 1.0f : -1.0f;
        const Vector2 at = b.width > 0 ? Vector2{position.x + dir * std::min(b.width, 90.0f), position.y - 70.0f}
                                   : Vector2{position.x + dir * 60.0f, position.y - 70.0f};
        const bool heavy = GetAttack(currentAttack).heavy || activeSkill >= 0;
        SpawnFx(heavy ? 10 : 8, heavy ? 4 : 3, at, heavy ? 0.30f : 0.20f, heavy ? 1.1f : 0.8f);
    }
    prevHasHit = hasHit;
    if (facing != lastFacing) { turnTimer = 0.10f; lastFacing = facing; }
    turnTimer = std::max(0.0f, turnTimer - dt);
    const bool wasDown = lastState == PlayerState::Airborne || lastState == PlayerState::Knockdown;
    const bool isDown = state == PlayerState::Airborne || state == PlayerState::Knockdown;
    if (wasDown && !isDown) { landTimer = 0.14f; AudioSystem::Get().Play(Sfx::Land); }
    if (lastState == PlayerState::Dash && state != PlayerState::Dash) landTimer = std::max(landTimer, 0.08f);
    if (state == PlayerState::Dash && lastState != PlayerState::Dash && !IsKfCharacter())   // estela de esquiva (efectos 49-56)
        SpawnFx(42, 4, {position.x - (facing == Facing::Right ? 40.0f : -40.0f), position.y - 55.0f}, 0.28f, 1.2f);
    landTimer = std::max(0.0f, landTimer - dt);
    if (state == PlayerState::Walk) walkPhase += dt * GetMoveSpeed() / 34.0f;
    lastState = state;

    // Avance real durante el golpe (root motion): en King Fighter cada golpe
    // da un paso; aqui el personaje se desliza mientras arranca y pega.
    if(!IsKfCharacter()&&state==PlayerState::Attack&&!clipDriven&&UsesAttackAnimationProfile(skin)){
        const auto& def=GetAttack(currentAttack); const auto& profile=GetAttackAnimationProfile(skin,currentAttack);
        const float target=profile.rootMotion*AttackRootMotionProgress(attackElapsed,def); const float delta=target-attackRootMotionApplied;
        if(std::fabs(delta)>0.0001f){position.x+=(facing==Facing::Right?1.0f:-1.0f)*delta;position.x=std::clamp(position.x,kStageStartX,kStageEndX-90.0f);attackRootMotionApplied+=delta;}
    }else if(state!=PlayerState::Attack)attackRootMotionApplied=0.0f;

    // Estelas (afterimages) en dash, golpes fuertes, habilidades y transformacion.
    for (Ghost& g : ghosts) g.life = std::max(0.0f, g.life - dt);
    const bool trail = state == PlayerState::Dash ||
                       (state == PlayerState::Attack && (activeSkill >= 0 || GetAttack(currentAttack).heavy)) ||
                       (IsTransformed() && state == PlayerState::Walk);
    ghostTimer -= dt;
    if (trail && ghostTimer <= 0.0f && animator.texture.id != 0) {
        for (int i = kGhostCount - 1; i > 0; --i) ghosts[i] = ghosts[i - 1];
        ghosts[0] = {position.ToScreen(), animator.currentFrame, facing == Facing::Left, 0.22f};
        ghostTimer = 0.035f;
    }
}

float Player::SpriteScale() const {
    const float scale = DepthScaleFor(position.y);
    const CharacterVisual& cv = GetCharacterVisual(skin);
    // KF y personajes con atlas propio (Rayder) usan su escala; el resto, 1.
    const float charScale = (cv.kfRoster >= 0 || cv.atlasId != nullptr) ? cv.scale : 1.0f;
    return (animator.normalizedAtlas ? 1.20f * scale : 0.76f * scale) * charScale;
}

void Player::Draw() const {
    Vector2 p = position.ToScreen();
    const float scale = DepthScaleFor(position.y);
    const float spriteScale = SpriteScale();

    DrawEllipse(static_cast<int>(p.x), static_cast<int>(position.y), 30 * scale, 8 * scale,
                {0, 0, 0, 145});

    DrawRageAura(p, scale);
    if (IsTransformed()) {
        // Transformacion: aura dorada que late (se apaga en los ultimos 2 s).
        const float t = static_cast<float>(GetTime());
        const float fade = std::clamp(transformTimer / 2.0f, 0.0f, 1.0f);
        const float pulse = 0.5f + 0.5f * std::sin(t * 7.0f);
        for (int layer = 0; layer < 3; ++layer) {
            const float grow = 1.0f + layer * 0.22f + pulse * 0.08f;
            DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y - 62 * scale), 42 * scale * grow, 82 * scale * grow,
                        {255, 210, 80, static_cast<unsigned char>((46 - layer * 13) * fade)});
        }
        for (int i = 0; i < 8; ++i) {
            const float ph = std::fmod(t * 1.6f + i * 0.37f, 1.0f);
            const float x = p.x + std::sin(i * 2.1f + t * 3.0f) * 34 * scale;
            DrawCircle(static_cast<int>(x), static_cast<int>(p.y - ph * 140 * scale), (4.0f - ph * 3.0f) * scale,
                       {255, 236, 140, static_cast<unsigned char>(220 * (1.0f - ph) * fade)});
        }
    }

    const Texture2D fxTex = FxTexture();
    const bool domeShield = fxTex.id != 0 && IsBlocking() && !IsKfCharacter();
    if (!domeShield && (IsBlocking() || shield < maxShield)) {
        const float ratio = std::clamp(static_cast<float>(shield) / maxShield, 0.f, 1.f);
        const float pulse = (1.f - ratio) * 3.f +
            std::sin(static_cast<float>(GetTime()) * 10.f) * (ratio < 0.3f ? 2.f : 0.5f);
        const Color shieldColor = ratio < 0.3f ? Color{255, 70, 70, 34} : Color{45, 180, 255, 28};
        DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y - 63 * scale),
                    (38 + pulse) * scale, (76 + pulse) * scale, shieldColor);
        DrawEllipseLines(static_cast<int>(p.x), static_cast<int>(p.y - 63 * scale),
                         (38 + pulse) * scale, (76 + pulse) * scale,
                         ratio < 0.3f ? Color{255, 90, 80, 230} : Color{80, 215, 255, 210});
        DrawEllipseLines(static_cast<int>(p.x), static_cast<int>(p.y - 63 * scale),
                         (33 + pulse) * scale, (70 + pulse) * scale, {180, 245, 255, 125});
    }

    Color spriteTint = WHITE;
    if (state == PlayerState::Hit || state == PlayerState::Knockdown) spriteTint = {255, 190, 190, 255};
    else if (state == PlayerState::Block) spriteTint = {175, 220, 255, 255};
    else if (isRageMode) spriteTint = {255, 214, 188, 255};
    else if (damageBuffTimer > 0.0f) spriteTint = {255, 226, 190, 255};
    if (IsTransformed() && state != PlayerState::Hit) spriteTint = {255, 238, 170, 255};

    // DF-013.2: personajes adicionales (clon y bosses jugables en VS). El
    // Rayden original (skin 0) conserva su atlas y su ruta de dibujo.
    const CharacterVisual& cv = GetCharacterVisual(skin);
    Texture2D altTex{};
    if (skin != 0 && cv.folder != nullptr && cv.atlasId == nullptr) {
        const char* pose = CharacterPose(skin, state, attackType, isRageMode, GetTime());
        // Rayden clon jugable: refuerza anticipacion/impacto/recuperacion usando
        // exclusivamente sus poses reales. No modifica KF ni Rayder Cruz.
        if (skin == 1 && state == PlayerState::Attack) {
            // El clon usa poses sueltas, pero sus transiciones ahora respetan el
            // mismo startup/active/recovery de AttackData que el atlas de Rayder.
            const AttackDef& ad=GetAttack(currentAttack);
            const float startup=ad.startup, activeEnd=ad.startup+ad.active;
            if (attackType == AttackType::Energy) {
                pose = attackElapsed < startup ? "ready" : attackElapsed < activeEnd ? "release_orb" : "idle2";
            } else if (attackType == AttackType::Kick) {
                pose = attackElapsed < startup ? "ready" : attackElapsed < activeEnd ? "kick" : "idle3";
            } else {
                pose = attackElapsed < startup ? "ready" : attackElapsed < activeEnd ? "punch" : "idle1";
            }
        }
        altTex = AssetManager::Get().GetTexture(std::string(cv.folder) + "_" + pose);
        if (altTex.id == 0) altTex = AssetManager::Get().GetTexture(std::string(cv.folder) + (cv.uniformCanvas ? "_idle" : "_idle1"));
    }
    if (altTex.id != 0) {
        const bool flip = cv.facesRightByDefault ? (facing == Facing::Left) : (facing == Facing::Right);
        if (cv.uniformCanvas) DrawSpriteUniform(altTex, p, cv.scale * scale, flip, spriteTint, cv.footInset);
        else DrawBossPose(altTex, p, cv.targetHeight * scale, flip, spriteTint);
    } else if (animator.texture.id != 0) {
        const bool flip = facing == Facing::Left;
        // Estelas: copias del cuadro anterior que se desvanecen.
        for (int i = kGhostCount - 1; i >= 0; --i) {
            const Ghost& g = ghosts[i];
            if (g.life <= 0.0f) continue;
            const unsigned char a = static_cast<unsigned char>(std::clamp(g.life / 0.22f, 0.0f, 1.0f) * 110.0f);
            const Color gc = IsTransformed() ? Color{255, 220, 120, a} : Color{110, 190, 255, a};
            animator.DrawFrame(g.frame, g.pos, spriteScale, g.flip, gc);
        }
        if (IsKfCharacter()) {
            // El arte de la APK ya trae su propio movimiento cuadro a cuadro.
            animator.Draw(p, spriteScale, flip, spriteTint);
        } else {
            const float dir = flip ? -1.0f : 1.0f;
            float sx = 1.0f, sy = 1.0f, angle = 0.0f;
            Vector2 at = p;
            switch (state) {
                case PlayerState::Idle:
                case PlayerState::Recovery: {
                    const float b = std::sin(animClock * 3.4f);
                    sy = 1.0f + 0.016f * b;
                    sx = 1.0f - 0.008f * b;
                    if (skin == 6) sy += 0.008f * std::sin(animClock * 6.8f);
                    break;
                }
                case PlayerState::Walk: {
                    const float st = std::sin(walkPhase);
                    at.y -= std::fabs(st) * 3.0f * scale;
                    sy = 1.0f + 0.012f * std::fabs(st);
                    angle = 3.5f * dir;
                    if (skin == 6) {
                        at.x += dir * 2.0f * std::sin(walkPhase);
                        angle = 4.5f * dir * std::sin(walkPhase);
                        sy += 0.008f * std::fabs(std::sin(walkPhase * 0.5f));
                    }
                    break;
                }
                case PlayerState::Dash:
                    angle = 9.0f * dir; sx = 1.06f; sy = 0.96f;
                    if (skin == 6) { sx = 1.10f; sy = 0.93f; angle = 11.0f * dir; }
                    break;
                case PlayerState::Attack: {
                    const AttackDef& ad=GetAttack(currentAttack); const float activeStart=ad.startup,activeEnd=ad.startup+ad.active;
                    const float t=std::clamp(attackElapsed/std::max(.01f,activeEnd),0.0f,1.0f); const float impact=std::sin(t*3.14159f);
                    angle=5.0f*dir*impact;sx=1.0f+.05f*impact;sy=1.0f-.025f*impact;
                    if(UsesAttackAnimationProfile(skin)){const auto& pr=GetAttackAnimationProfile(skin,currentAttack);
                        if(attackElapsed<activeStart){const float q=std::clamp(attackElapsed/std::max(.01f,activeStart),0.0f,1.0f);sx=1-.035f*pr.anticipationScale*(1-q);sy=1+.020f*pr.anticipationScale*(1-q);angle=-3*dir*(1-q);}
                        else if(attackElapsed<activeEnd){const float q=std::sin(std::clamp((attackElapsed-activeStart)/std::max(.01f,ad.active),0.0f,1.0f)*3.14159f);sx=1+.060f*pr.impactStretch*q;sy=1-.032f*pr.impactStretch*q;angle=5.5f*dir*q;at.x+=dir*2*q;}
                        else{const float q=std::clamp((attackElapsed-activeEnd)/std::max(.01f,ad.recovery),0.0f,1.0f);sx=1+.025f*(1-q)*pr.recoveryRecoil;sy=1-.015f*(1-q);angle=2*dir*(1-q)*pr.recoveryRecoil;}
                    }
                    break;
                }
                case PlayerState::Hit:
                case PlayerState::GuardBreak:  // retrocede y tiembla
                    angle = -7.0f * dir;
                    at.x += std::sin(animClock * 90.0f) * 2.0f * scale;
                    break;
                case PlayerState::Block:
                    sy = 0.98f; angle = -2.0f * dir;
                    break;
                default: break;
            }
            if (turnTimer > 0.0f) sx *= 0.72f + 0.28f * (1.0f - turnTimer / 0.10f);   // giro
            if (landTimer > 0.0f) {                                                 // aterrizaje
                const float k = landTimer / 0.14f;
                sy *= 1.0f - 0.10f * k;
                sx *= 1.0f + 0.08f * k;
            }
            animator.DrawScaled(at, spriteScale * sx * cv.widthScale, spriteScale * sy, flip, spriteTint, angle);
        }
    } else {
        Color tint = state == PlayerState::Hit ? RED
                   : state == PlayerState::Attack ? YELLOW
                   : state == PlayerState::Block ? Color{80, 190, 255, 255} : BLUE;
        DrawRectangle(static_cast<int>(p.x - 18 * scale), static_cast<int>(p.y - 68 * scale),
                      static_cast<int>(36 * scale), static_cast<int>(68 * scale), tint);
    }

    if (domeShield) {
        // Escudo en domo (efectos azules 17-19), mas tenue si el escudo esta bajo.
        const float ratio = std::clamp(static_cast<float>(shield) / maxShield, 0.f, 1.f);
        const unsigned char a = static_cast<unsigned char>(110 + 120 * ratio);
        const int f = 16 + (static_cast<int>(GetTime() * 10.0f) % 3);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawFx(fxTex, f, {p.x, p.y - 62.0f * scale}, 1.35f * scale, false,
               ratio < 0.3f ? Color{255, 120, 120, a} : Color{255, 255, 255, a});
        EndBlendMode();
    }
    if (fxTex.id != 0 && !effects.empty()) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (const Fx& e : effects) {
            const int k = std::min(e.count - 1, static_cast<int>(e.t / e.dur * e.count));
            const Vector2 sp = Vector3D{e.pos.x, e.pos.y, 0}.ToScreen();
            DrawFx(fxTex, e.first + k, sp, e.scale * scale, false);
        }
        EndBlendMode();
    }
    if(animationDebug&&state==PlayerState::Attack&&UsesAttackAnimationProfile(skin))DrawAnimationDebug(p);
    if (IsBlocking()) {
        const char* label = shield == 0 ? "ESCUDO ROTO" : "BLOQUEO";
        DrawText(label, static_cast<int>(p.x) - MeasureText(label, 12) / 2,
                 static_cast<int>(p.y) - 154, 12, {120, 215, 255, 230});
    }

    // Carga visible de la Energy Wave en las manos.
    const AttackDef& def = GetAttack(currentAttack);
    if (state == PlayerState::Attack && def.spawnsProjectile) {
        const float charge = std::clamp(attackElapsed / std::max(0.01f, def.startup), 0.f, 1.f);
        const float handX = p.x + (facing == Facing::Right ? 30 : -30);
        const float handY = p.y - 76 * scale;
        DrawCircle(static_cast<int>(handX), static_cast<int>(handY), 6 + 16 * charge,
                   {50, 210, 255, static_cast<unsigned char>(60 + 120 * charge)});
        DrawCircleLines(static_cast<int>(handX), static_cast<int>(handY), 12 + 20 * charge,
                        {120, 235, 255, 170});
        for (int i = 0; i < 5; ++i) {
            const float angle = rageAuraPhase * 7.0f + static_cast<float>(i) * 1.25f;
            DrawCircle(static_cast<int>(handX + std::cos(angle) * (16 + 14 * charge)),
                       static_cast<int>(handY + std::sin(angle) * (12 + 10 * charge)),
                       2.5f, {180, 245, 255, 200});
        }
    }
}

void Player::DrawPortrait(Rectangle box) const {
    const CharacterVisual& cv = GetCharacterVisual(skin);
    if (skin != 0 && cv.folder != nullptr && cv.atlasId == nullptr) {
        const Texture2D tex = AssetManager::Get().GetTexture(std::string(cv.folder) + (cv.uniformCanvas ? "_idle" : "_idle1"));
        if (tex.id == 0) return;
        const float s = box.height / (tex.height * 0.5f);
        const float w = tex.width * s;
        const bool flip = !cv.facesRightByDefault;
        DrawTexturePro(tex, {0, 0, flip ? -(float)tex.width : (float)tex.width, (float)tex.height},
                       {box.x + box.width * 0.5f - w * 0.5f, box.y + 4.0f, w, tex.height * s}, {0, 0}, 0, WHITE);
        return;
    }
    if (animator.texture.id == 0 || animator.frames.empty()) return;
    int idx = animator.currentFrame;
    const auto it = animator.namedClips.find("idle");
    if (it != animator.namedClips.end()) idx = it->second.frames.empty() ? it->second.startFrame : it->second.frames.front();
    const SpriteFrame& f = animator.frames[static_cast<std::size_t>(std::clamp(idx, 0, (int)animator.frames.size() - 1))];
    // Estatura de la figura: los frames KF vienen recortados al dibujo; las
    // celdas del manifiesto tienen aire arriba (~26 %).
    const float figH = std::min(f.pivotY, f.height) * (GetCharacterVisual(skin).kfRoster >= 0 ? 1.0f : 0.62f);
    if (figH <= 1.0f) return;
    const float s = box.height / (figH * 0.55f);
    animator.DrawFrame(idx, {box.x + box.width * 0.5f, box.y + 3.0f + figH * s}, s, false);
}

bool Player::DrawEnergyProjectile(Vector2 center, bool movingLeft, float time) const {
    // Proyectil azul animado (efectos 5-8) para nuestros personajes.
    const Texture2D fxTex = FxTexture();
    if (fxTex.id != 0 && !IsKfCharacter()) {
        const int f = 4 + static_cast<int>(time / 0.07f) % 4;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawFx(fxTex, f, center, 1.5f * DepthScaleFor(position.y), movingLeft);
        EndBlendMode();
        return true;
    }
    const auto it = animator.namedClips.find("projectile");
    if (it == animator.namedClips.end() || it->second.frames.empty()) return false;
    const auto& frames = it->second.frames;
    const int idx = frames[static_cast<std::size_t>(static_cast<int>(time / 0.06f)) % frames.size()];
    // El pivote del proyectil esta 40 px (atlas) bajo su centro visual.
    const float scale = 1.20f * DepthScaleFor(position.y);
    animator.DrawFrame(idx, {center.x, center.y + 40.0f * scale}, scale, movingLeft);
    return true;
}

void Player::DrawAnimationDebug(Vector2 screen) const {
    if(state!=PlayerState::Attack||!UsesAttackAnimationProfile(skin))return;
    const auto& def=GetAttack(currentAttack);const auto& pr=GetAttackAnimationProfile(skin,currentAttack);const float total=AttackTotalDuration(def),ae=def.startup+def.active;
    const int bx=14,by=470,bw=360;DrawRectangle(bx,by,bw,174,{5,5,9,224});DrawRectangleLines(bx,by,bw,174,{255,205,110,220});
    DrawText(TextFormat("ANIM DEBUG %s / %s",GetCharacterVisual(skin).name,pr.clip),bx+10,by+8,14,WHITE);
    DrawText(TextFormat("TIME %.3f/%.3f FRAME %d/%d IMPACT %d",attackElapsed,total,animator.currentFrame,(int)animator.CurrentClipFrameCount(),pr.impactFrameIndex),bx+10,by+29,12,WHITE);
    DrawText(TextFormat("PHASE %s HITBOX %s",AttackPhaseLabel(attackElapsed,def),AttackIsActive()?"ON":"OFF"),bx+10,by+47,12,AttackIsActive()?GREEN:LIGHTGRAY);
    const int x=bx+10,y=by+70,w=338;DrawRectangle(x,y,w,10,{35,35,42,255});const int sx=x+(int)(w*def.startup/total),ax=x+(int)(w*ae/total);
    DrawRectangle(x,y,std::max(1,sx-x),10,{90,90,110,255});DrawRectangle(sx,y,std::max(1,ax-sx),10,{230,120,70,255});DrawRectangle(ax,y,std::max(1,x+w-ax),10,{90,130,170,255});DrawLine(sx,y-6,sx,y+16,YELLOW);
    DrawText(TextFormat("ROOT %.1fpx | IMPACT %s | F3",attackRootMotionApplied,attackImpactTriggered?"TRIGGERED":"WAITING"),bx+10,by+104,12,WHITE);
}

}  // namespace district_fury
