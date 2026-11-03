// Jugador: cuerpo dibujado con poses sueltas (clon y bosses jugables: skins
// 1-5 con carpeta en assets/bosses/ y sin atlas). Codigo movido tal cual desde
// PlayerDraw.cpp (F0.1); F0.3 le suma PoseFx/poses por segmento.
#include "game/player/PlayerCommon.h"

namespace district_fury {

// Devuelve true si dibujo el cuerpo con una pose suelta; false si este personaje
// no usa poses sueltas o no se encontro la textura (Draw sigue con el atlas).
bool Player::DrawPoseBody(Vector2 p, float scale, Color spriteTint) const {
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
        return true;
    }
    return false;
}

}  // namespace district_fury
