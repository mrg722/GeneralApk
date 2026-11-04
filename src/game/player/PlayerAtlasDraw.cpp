// Jugador: cuerpo dibujado desde el atlas del Animator (Rayden original, MAX,
// KF, BETA, RONIN...). Sin visual propio en el MoveSet el dibujo es EXACTAMENTE
// el de antes de F0.3 (estiramiento procedural sx/sy/angulo + giro +
// aterrizaje; KF/BETA su clip tal cual). Con MoveVisual (clips y/o PoseFx del
// MoveSet) usa el evaluador PoseFx (F0.3).
#include "game/player/PlayerCommon.h"
#include "game/player/PoseFxEval.h"

namespace district_fury {
namespace {

// El estiramiento de siempre (switch historico), sin giro ni aterrizaje.
void HistoricFeel(const Player& pl, float scale, float dir, float& sx, float& sy, float& angle, Vector2& at,
                  bool skipAttack) {
    const int skin = pl.skin;
    switch (pl.state) {
        case PlayerState::Idle:
        case PlayerState::Recovery: {
            const float b = std::sin(pl.animClock * 3.4f);
            sy = 1.0f + 0.016f * b;
            sx = 1.0f - 0.008f * b;
            if (skin == 6) sy += 0.008f * std::sin(pl.animClock * 6.8f);
            break;
        }
        case PlayerState::Walk: {
            const float st = std::sin(pl.walkPhase);
            at.y -= std::fabs(st) * 3.0f * scale;
            sy = 1.0f + 0.012f * std::fabs(st);
            angle = 3.5f * dir;
            if (skin == 6) {
                at.x += dir * 2.0f * std::sin(pl.walkPhase);
                angle = 4.5f * dir * std::sin(pl.walkPhase);
                sy += 0.008f * std::fabs(std::sin(pl.walkPhase * 0.5f));
            }
            break;
        }
        case PlayerState::Dash:
            angle = 9.0f * dir; sx = 1.06f; sy = 0.96f;
            if (skin == 6) { sx = 1.10f; sy = 0.93f; angle = 11.0f * dir; }
            break;
        case PlayerState::Attack: {
            if (skipAttack) break;
            const float attackElapsed = pl.attackElapsed;
            const AttackDef& ad=pl.CurrentAttackDef(); const float activeStart=ad.startup,activeEnd=ad.startup+ad.active;
            const float t=std::clamp(attackElapsed/std::max(.01f,activeEnd),0.0f,1.0f); const float impact=std::sin(t*3.14159f);
            angle=5.0f*dir*impact;sx=1.0f+.05f*impact;sy=1.0f-.025f*impact;
            if(UsesAttackAnimationProfile(skin)){const auto& pr=GetAttackAnimationProfile(skin,pl.currentAttack);
                if(attackElapsed<activeStart){const float q=std::clamp(attackElapsed/std::max(.01f,activeStart),0.0f,1.0f);sx=1-.035f*pr.anticipationScale*(1-q);sy=1+.020f*pr.anticipationScale*(1-q);angle=-3*dir*(1-q);}
                else if(attackElapsed<activeEnd){const float q=std::sin(std::clamp((attackElapsed-activeStart)/std::max(.01f,ad.active),0.0f,1.0f)*3.14159f);sx=1+.060f*pr.impactStretch*q;sy=1-.032f*pr.impactStretch*q;angle=5.5f*dir*q;at.x+=dir*2*q;}
                else{const float q=std::clamp((attackElapsed-activeEnd)/std::max(.01f,ad.recovery),0.0f,1.0f);sx=1+.025f*(1-q)*pr.recoveryRecoil;sy=1-.015f*(1-q);angle=2*dir*(1-q)*pr.recoveryRecoil;}
            }
            break;
        }
        case PlayerState::Hit:
        case PlayerState::GuardBreak:  // retrocede y tiembla
            angle = -7.0f * dir;
            at.x += std::sin(pl.animClock * 90.0f) * 2.0f * scale;
            break;
        case PlayerState::Block:
            sy = 0.98f; angle = -2.0f * dir;
            break;
        default: break;
    }
}

void TurnAndLand(const Player& pl, float& sx, float& sy) {
    if (pl.turnTimer > 0.0f) sx *= 0.72f + 0.28f * (1.0f - pl.turnTimer / 0.10f);   // giro
    if (pl.landTimer > 0.0f) {                                                    // aterrizaje
        const float k = pl.landTimer / 0.14f;
        sy *= 1.0f - 0.10f * k;
        sx *= 1.0f + 0.08f * k;
    }
}

}  // namespace

// Comparte la deteccion de "visual propio" con la toma de estelas de Update.
bool ComputeAtlasCustom(const Player& pl, Vector2 p, float spriteScale, Color spriteTint, BodyCmd& out) {
    if (pl.animator.texture.id == 0) return false;
    const MoveSet& ms = VisualMoveSet(pl.skin);
    if (ms.overrides == nullptr && !HasVisual(ms.hit) && !HasVisual(ms.knockdown) && !HasVisual(ms.getup) &&
        !HasVisual(ms.victory) && !HasVisual(ms.jumpRise) && !HasVisual(ms.jumpFall) && !HasVisual(ms.walk) &&
        !HasVisual(ms.block))
        return false;   // camino rapido: el MoveSet no tiene nada visual
    const bool kf = pl.IsKfCharacter();
    const FeelLevel feel = EffectiveFeel(ms, kf);
    if (kf && feel == FeelLevel::None) return false;
    const MoveVisual* mv = nullptr;
    const bool attack = pl.state == PlayerState::Attack;
    if (attack) {
        const MoveOverride* ov = FindOverride(ms, static_cast<int>(pl.currentAttack));
        if (ov && HasVisual(ov->visual)) mv = &ov->visual;
    } else {
        mv = StateVisual(ms, pl.state, pl.velocity.z);
        if ((pl.state == PlayerState::Knockdown) && pl.animator.currentClipName == "getup" && HasVisual(ms.getup)) mv = &ms.getup;
    }
    if (mv == nullptr) return false;

    const CharacterVisual& cv = GetCharacterVisual(pl.skin);
    const bool flip = pl.facing == Facing::Left;
    const float dir = flip ? -1.0f : 1.0f;
    const float scale = DepthScaleFor(pl.position.y);
    out = BodyCmd{};
    out.dir = dir;
    out.scale = spriteScale;
    out.bodyK = scale * ((cv.atlasId != nullptr) ? cv.scale : 1.0f);
    out.clock = PlayerFxClock(pl, mv);
    out.fx = mv->fx;
    out.hasFx = HasFx(mv->fx);

    // Cuadro: la secuencia de clips del MoveVisual (con respaldo en la forma
    // base si la transformada no tiene el clip) o el cuadro actual del animador.
    out.anim = &pl.animator;
    out.frame = pl.animator.currentFrame;
    out.clip = &pl.animator.currentClip;
    out.step = pl.animator.clipFrameIndex;
    Color formTint = WHITE;
    if (mv->clipCount > 0) {
        AtlasPick pick;
        const Animator* base = (pl.usingAlt && pl.altAnimator.texture.id != 0) ? &pl.altAnimator : nullptr;
        if (PickAtlasFrame(*mv, out.clock.u, out.clock.total, !attack, pl.animator, base, pick)) {
            out.anim = pick.anim; out.frame = pick.frame; out.clip = pick.clip; out.step = static_cast<std::size_t>(pick.step);
            if (pick.fromSecondary) formTint = {200, 225, 255, 255};   // forma transformada sin ese clip: base blanco-azulada
        }
    }

    float sx = 1.0f, sy = 1.0f, angle = 0.0f;
    Vector2 at = p;
    if (!kf && feel == FeelLevel::Full) HistoricFeel(pl, scale, dir, sx, sy, angle, at, attack && out.hasFx);
    TurnAndLand(pl, sx, sy);
    const FxXform xf = EvalPoseFx(mv->fx, out.clock);
    FxXform hop;
    if (attack && pl.position.z < 0.5f)
        if (const MoveOverride* ov = FindOverride(ms, static_cast<int>(pl.currentAttack))) hop = EvalRootHop(ov->root, out.clock.u);
    out.feet = {at.x + xf.dx * out.bodyK, at.y + (xf.dy + hop.dy) * out.bodyK};
    out.sx = sx * xf.sx * (cv.beta ? 1.0f : cv.widthScale);
    out.sy = sy * xf.sy;
    out.angle = angle + xf.angle;
    out.flip = flip != xf.flip;
    out.tint = MultiplyColor(MultiplyColor(spriteTint, formTint), xf.tint);
    out.tint.a = static_cast<unsigned char>(std::clamp(out.tint.a * xf.alpha, 0.0f, 255.0f));
    return true;
}

void Player::DrawAtlasBody(Vector2 p, float scale, float spriteScale, Color spriteTint) const {
    const CharacterVisual& cv = GetCharacterVisual(skin);
    const bool flip = facing == Facing::Left;
    // Estelas: copias del cuerpo (textura + recorte) que se desvanecen.
    DrawBodyTrails(*this, spriteScale);

    BodyCmd cmd;
    if (ComputeAtlasCustom(*this, p, spriteScale, spriteTint, cmd)) {
        cmd.anim->DrawFrameScaled(cmd.frame, cmd.feet, cmd.scale * cmd.sx, cmd.scale * cmd.sy, cmd.flip, cmd.tint,
                                  cmd.angle, cmd.clip, cmd.step);
        DrawBodyOverlays(cmd);
        return;
    }
    if (IsKfCharacter()) {
        const MoveSet& ms = VisualMoveSet(skin);
        if (EffectiveFeel(ms, true) != FeelLevel::Light) {
            // El arte de la APK ya trae su propio movimiento cuadro a cuadro.
            animator.Draw(p, spriteScale, flip, spriteTint);
            return;
        }
        // Feel ligero (F0.3): inclinacion y temblor al recibir golpe, caida
        // rotada si el clip de derribo es casi fijo, rebote de victoria.
        const float dir = flip ? -1.0f : 1.0f;
        float angle = 0.0f;
        Vector2 at = p;
        switch (state) {
            case PlayerState::Hit:
            case PlayerState::GuardBreak:
                angle = -7.0f * dir;
                at.x += posefx::Temblor(2.0f * scale, 28.0f, visual.stateClock).dx;
                break;
            case PlayerState::Knockdown:
            case PlayerState::Airborne:
            case PlayerState::Defeat:
                if (animator.CurrentClipFrameCount() <= 2) {
                    const FxXform x = posefx::CaidaRot(85.0f, visual.stateClock / 0.30f, dir);
                    angle = x.angle; at.y += x.dy * scale;
                }
                break;
            case PlayerState::Victory:
                at.y -= std::fabs(std::sin(visual.stateClock * 2.0f * posefx::kPi * 1.4f)) * 6.0f * scale;
                break;
            default: break;
        }
        animator.DrawScaled(at, spriteScale, spriteScale, flip, spriteTint, angle);
        return;
    }
    const float dir = flip ? -1.0f : 1.0f;
    float sx = 1.0f, sy = 1.0f, angle = 0.0f;
    Vector2 at = p;
    HistoricFeel(*this, scale, dir, sx, sy, angle, at, false);
    TurnAndLand(*this, sx, sy);
    if (cv.beta) { sx = sy = 1.0f; angle = 0.0f; }   // BETA: forma y tamano originales, sin estirar
    animator.DrawScaled(at, spriteScale * sx * cv.widthScale, spriteScale * sy, flip, spriteTint, angle);
}

}  // namespace district_fury
