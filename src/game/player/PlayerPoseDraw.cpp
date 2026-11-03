// Jugador: cuerpo dibujado con poses sueltas (clon y bosses jugables: skins
// 1-5 con carpeta en assets/bosses/ y sin atlas). F0.3:
// - escala UNIFORME por personaje (targetHeight / alto de idle1; Grinder la de
//   siempre), la misma para todas sus poses: no cambia de tamano entre poses;
// - cuadro por ResolvePoseFrame (MoveSet o tablas Poses_*.cpp): 1-2 piezas con
//   su propio desfase y orientacion;
// - el mismo feel que la rama atlas (respiracion, paso, dash, impacto, retroceso,
//   aterrizaje, giro) + PoseFx del movimiento, rotando alrededor de los pies;
// - estelas con la textura del propio personaje.
#include "game/player/PlayerCommon.h"
#include "game/player/PoseFxEval.h"
#include "game/visual/PoseTables.h"

namespace district_fury {
namespace {

Texture2D PoseTexture(const char* folder, const char* pose) {
    if (folder == nullptr || pose == nullptr) return Texture2D{0};
    return AssetManager::Get().GetTexture(std::string(folder) + "_" + pose);
}

// El feel de la rama atlas para un cuerpo de poses (sin los extras de MAX).
void PoseFeel(const Player& pl, float scale, float dir, bool skipAttack, float& sx, float& sy, float& angle, Vector2& at) {
    switch (pl.state) {
        case PlayerState::Idle:
        case PlayerState::Recovery: {
            const float b = std::sin(pl.animClock * 3.4f);
            sy = 1.0f + 0.016f * b;
            sx = 1.0f - 0.008f * b;
            break;
        }
        case PlayerState::Walk: {
            const float st = std::sin(pl.walkPhase);
            at.y -= std::fabs(st) * 3.0f * scale;
            sy = 1.0f + 0.012f * std::fabs(st);
            angle = 3.5f * dir;
            break;
        }
        case PlayerState::Dash:
            angle = 9.0f * dir; sx = 1.06f; sy = 0.96f;
            break;
        case PlayerState::Attack: {
            if (skipAttack) break;
            const AttackDef& ad = pl.CurrentAttackDef();
            const float activeEnd = ad.startup + ad.active;
            const float t = std::clamp(pl.attackElapsed / std::max(.01f, activeEnd), 0.0f, 1.0f);
            const float impact = std::sin(t * 3.14159f);
            angle = 5.0f * dir * impact; sx = 1.0f + .05f * impact; sy = 1.0f - .025f * impact;
            break;
        }
        case PlayerState::Hit:
        case PlayerState::GuardBreak:   // retrocede y tiembla
            angle = -7.0f * dir;
            at.x += std::sin(pl.animClock * 90.0f) * 2.0f * scale;
            break;
        case PlayerState::Block:
            sy = 0.98f; angle = -2.0f * dir;
            break;
        default: break;
    }
    if (pl.turnTimer > 0.0f) sx *= 0.72f + 0.28f * (1.0f - pl.turnTimer / 0.10f);
    if (pl.landTimer > 0.0f) {
        const float k = pl.landTimer / 0.14f;
        sy *= 1.0f - 0.10f * k;
        sx *= 1.0f + 0.08f * k;
    }
}

}  // namespace

FxClock PlayerFxClock(const Player& pl, const MoveVisual* mv) {
    const float dir = pl.facing == Facing::Left ? -1.0f : 1.0f;
    if (pl.state == PlayerState::Attack) {
        const AttackDef& d = pl.CurrentAttackDef();
        return AttackClock(pl.attackElapsed, d.startup, d.active, d.recovery, dir);
    }
    const float dur = (mv && mv->duration > 0) ? mv->duration : 0.35f;
    return StateClock(pl.visual.stateClock, dur, mv ? mv->loop : false, dir);
}

bool ComputePoseBody(const Player& pl, Vector2 p, float scale, Color spriteTint, BodyCmd& out) {
    const CharacterVisual& cv = GetCharacterVisual(pl.skin);
    if (pl.skin == 0 || cv.folder == nullptr || cv.atlasId != nullptr) return false;
    PoseContext ctx;
    ctx.state = pl.state;
    ctx.attack = static_cast<int>(pl.currentAttack);
    ctx.attackType = pl.attackType;
    const FxClock attackClock = PlayerFxClock(pl, nullptr);
    if (pl.state == PlayerState::Attack) { ctx.phase = attackClock.phase; ctx.v = attackClock.v; }
    ctx.stateTime = pl.visual.stateClock;
    ctx.walkPhase = pl.walkPhase;
    ctx.rage = pl.isRageMode;
    ctx.transformed = pl.IsTransformed();
    ctx.z = pl.position.z;
    ctx.time = GetTime();
    ctx.gettingUp = pl.state == PlayerState::Knockdown && pl.animator.currentClipName == "getup";
    PoseFrame fr;
    if (!ResolvePoseFrame(pl.skin, ctx, fr) || fr.pieceCount <= 0) return false;

    out = BodyCmd{};
    const char* idleName = cv.uniformCanvas ? "idle" : "idle1";
    const Texture2D idle = PoseTexture(cv.folder, idleName);
    out.pieceCount = 0;
    for (int i = 0; i < std::min(fr.pieceCount, 2); ++i) {
        Texture2D t = PoseTexture(cv.folder, fr.pieces[i].pose);
        if (t.id == 0 && i == 0) t = idle;   // pose sin textura: el reposo (como antes)
        if (t.id == 0) continue;
        const int k = out.pieceCount++;
        out.tex[k] = t;
        out.src[k] = fr.pieces[i].src;
        out.offset[k] = fr.pieces[i].offset;
        if (cv.uniformCanvas) out.offset[k].y += cv.footInset;
        out.pieceFlip[k] = fr.pieces[i].facesRight ? (pl.facing == Facing::Left) : (pl.facing == Facing::Right);
    }
    if (out.pieceCount == 0) return false;

    // Escala uniforme por personaje (la misma en todas sus poses).
    if (cv.uniformCanvas) out.scale = cv.scale * scale;
    else {
        const float k = PoseSetScale(idle.id != 0 ? idle : out.tex[0], cv.targetHeight);
        out.scale = k * scale;
    }
    const float bodyPx = cv.uniformCanvas ? (idle.id != 0 ? idle.height * cv.scale : 112.0f) : cv.targetHeight;
    out.bodyK = scale * bodyPx / 112.0f;

    const bool flip = pl.facing == Facing::Left;
    out.dir = flip ? -1.0f : 1.0f;
    const MoveSet& ms = VisualMoveSet(pl.skin);
    const MoveOverride* ov = pl.state == PlayerState::Attack ? FindOverride(ms, ctx.attack) : nullptr;
    const MoveVisual* mv = ov ? &ov->visual : StateVisual(ms, pl.state, pl.velocity.z);
    if (ctx.gettingUp && HasVisual(ms.getup)) mv = &ms.getup;
    out.clock = pl.state == PlayerState::Attack ? attackClock : PlayerFxClock(pl, mv);
    out.fx = fr.fx;
    out.hasFx = HasFx(fr.fx);

    float sx = 1.0f, sy = 1.0f, angle = 0.0f;
    Vector2 at = p;
    if (EffectiveFeel(ms, false) == FeelLevel::Full)
        PoseFeel(pl, scale, out.dir, pl.state == PlayerState::Attack && out.hasFx, sx, sy, angle, at);
    const FxXform xf = EvalPoseFx(fr.fx, out.clock);
    FxXform hop;
    if (ov && pl.position.z < 0.5f) hop = EvalRootHop(ov->root, out.clock.u);
    out.feet = {at.x + xf.dx * out.bodyK, at.y + (xf.dy + hop.dy) * out.bodyK};
    out.sx = sx * xf.sx;
    out.sy = sy * xf.sy;
    out.angle = angle + xf.angle;
    out.flip = xf.flip;   // espejo extra (GIRO): se combina con el de cada pieza
    if (xf.flip) for (int i = 0; i < out.pieceCount; ++i) out.pieceFlip[i] = !out.pieceFlip[i];
    out.tint = MultiplyColor(spriteTint, xf.tint);
    out.tint.a = static_cast<unsigned char>(std::clamp(out.tint.a * xf.alpha, 0.0f, 255.0f));
    return true;
}

// Devuelve true si dibujo el cuerpo con una pose suelta; false si este personaje
// no usa poses sueltas o no se encontro la textura (Draw sigue con el atlas).
bool Player::DrawPoseBody(Vector2 p, float scale, Color spriteTint) const {
    BodyCmd cmd;
    if (!ComputePoseBody(*this, p, scale, spriteTint, cmd)) return false;
    DrawBodyTrails(*this, cmd.scale);
    for (int i = 0; i < cmd.pieceCount; ++i)
        DrawBossPoseEx(cmd.tex[i], cmd.src[i], cmd.feet, cmd.scale, cmd.pieceFlip[i], cmd.angle, cmd.sx, cmd.sy,
                       cmd.tint, cmd.offset[i]);
    DrawBodyOverlays(cmd);
    return true;
}

}  // namespace district_fury
