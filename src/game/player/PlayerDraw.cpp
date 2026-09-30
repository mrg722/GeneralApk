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

void Player::Draw() const {
    Vector2 p = position.ToScreen();
    const float scale = DepthScaleFor(position.y);
    const float kfScale = GetCharacterVisual(skin).kfRoster >= 0 ? GetCharacterVisual(skin).scale : 1.0f;
    const float spriteScale = (animator.normalizedAtlas ? 1.20f * scale : 0.76f * scale) * kfScale;

    DrawEllipse(static_cast<int>(p.x), static_cast<int>(position.y), 30 * scale, 8 * scale,
                {0, 0, 0, 145});

    DrawRageAura(p, scale);

    if (IsBlocking() || shield < maxShield) {
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

    // DF-013.2: personajes adicionales (clon y bosses jugables en VS). El
    // Rayden original (skin 0) conserva su atlas y su ruta de dibujo.
    const CharacterVisual& cv = GetCharacterVisual(skin);
    Texture2D altTex{};
    if (skin != 0 && cv.folder != nullptr && cv.atlasId == nullptr) {
        const char* pose = CharacterPose(skin, state, attackType, isRageMode, GetTime());
        altTex = AssetManager::Get().GetTexture(std::string(cv.folder) + "_" + pose);
        if (altTex.id == 0) altTex = AssetManager::Get().GetTexture(std::string(cv.folder) + (cv.uniformCanvas ? "_idle" : "_idle1"));
    }
    if (altTex.id != 0) {
        const bool flip = cv.facesRightByDefault ? (facing == Facing::Left) : (facing == Facing::Right);
        if (cv.uniformCanvas) DrawSpriteUniform(altTex, p, cv.scale * scale, flip, spriteTint, cv.footInset);
        else DrawBossPose(altTex, p, cv.targetHeight * scale, flip, spriteTint);
    } else if (animator.texture.id != 0) {
        animator.Draw(p, spriteScale, facing == Facing::Left, spriteTint);
    } else {
        Color tint = state == PlayerState::Hit ? RED
                   : state == PlayerState::Attack ? YELLOW
                   : state == PlayerState::Block ? Color{80, 190, 255, 255} : BLUE;
        DrawRectangle(static_cast<int>(p.x - 18 * scale), static_cast<int>(p.y - 68 * scale),
                      static_cast<int>(36 * scale), static_cast<int>(68 * scale), tint);
    }

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

bool Player::DrawEnergyProjectile(Vector2 center, bool movingLeft, float time) const {
    const auto it = animator.namedClips.find("projectile");
    if (it == animator.namedClips.end() || it->second.frames.empty()) return false;
    const auto& frames = it->second.frames;
    const int idx = frames[static_cast<std::size_t>(static_cast<int>(time / 0.06f)) % frames.size()];
    // El pivote del proyectil esta 40 px (atlas) bajo su centro visual.
    const float scale = 1.20f * DepthScaleFor(position.y);
    animator.DrawFrame(idx, {center.x, center.y + 40.0f * scale}, scale, movingLeft);
    return true;
}

}  // namespace district_fury
