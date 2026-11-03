// Jugador: cuerpo dibujado desde el atlas del Animator (Rayden original, MAX,
// KF, BETA, RONIN...): estelas, cuadro KF y el estiramiento procedural de sx/sy/
// angulo + giro + aterrizaje. Codigo movido tal cual desde PlayerDraw.cpp (F0.1);
// F0.3 le suma PoseFx.
#include "game/player/PlayerCommon.h"

namespace district_fury {

void Player::DrawAtlasBody(Vector2 p, float scale, float spriteScale, Color spriteTint) const {
    const CharacterVisual& cv = GetCharacterVisual(skin);
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
        if (cv.beta) { sx = sy = 1.0f; angle = 0.0f; }   // BETA: forma y tamano originales, sin estirar
        animator.DrawScaled(at, spriteScale * sx * cv.widthScale, spriteScale * sy, flip, spriteTint, angle);
    }
}

}  // namespace district_fury
