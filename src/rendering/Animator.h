#pragma once
#include "raylib.h"
#include "rendering/SpriteFrame.h"
#include "rendering/SpriteManifest.h"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace district_fury {

struct AnimationClip {
    int startFrame;
    int endFrame;
    float frameDuration;
    bool loop;
    std::vector<int> frames = {};
    // Duracion por cuadro de `frames` (asincrona). Vacio = frameDuration fijo.
    std::vector<float> durations = {};
    // BETA: sonidos por cuadro del clip (indice en `frames`, clave del sonido).
    std::vector<std::pair<int, std::string>> sounds = {};
};

class Animator {
public:
    Texture2D texture;
    int cols;
    int rows;
    int currentFrame;
    float timer;
    AnimationClip currentClip;
    bool isPlaying;
    bool isFinished;
    bool normalizedAtlas;
    std::vector<SpriteFrame> frames;
    std::size_t clipFrameIndex;
    // Clips cargados del manifiesto, indexados por nombre ("punch1", "hit_high"...).
    std::unordered_map<std::string, AnimationClip> namedClips;
    std::string currentClipName;
    // BETA: imagenes originales de las piezas (compartidas entre copias del animador).
    std::shared_ptr<const std::vector<Texture2D>> pieceTextures;

    Animator()
        : texture{0}, cols(1), rows(1), currentFrame(0), timer(0.0f),
          currentClip{0, 0, 0.1f, true}, isPlaying(false), isFinished(true),
          normalizedAtlas(false), frames(), clipFrameIndex(0) {}

    void Init(Texture2D tex, int columns, int rws, bool normalized = false) {
        texture = tex;
        cols = std::max(1, columns);
        rows = std::max(1, rws);
        currentFrame = 0;
        timer = 0.0f;
        isPlaying = false;
        isFinished = true;
        normalizedAtlas = normalized;
        frames.clear();
        namedClips.clear();
        currentClipName.clear();
        clipFrameIndex = 0;
    }

    // --- Perfil dinamico (Animator.cpp) ---
    // Aplica celda/pivote/clips de un perfil sin tocar la textura.
    bool ApplyProfile(const AtlasProfile& profile, const ClipMap& clips, int textureWidth, int textureHeight);
    // Busca `atlasId` en data/sprite_manifest.json y lo aplica a `tex`.
    bool InitFromManifest(const std::string& atlasId, Texture2D tex);
    bool HasClips() const { return !namedClips.empty(); }
    bool HasClip(const std::string& name) const { return namedClips.count(name) != 0; }
    // Reproduce un clip por nombre. False si no existe (el llamador decide el respaldo).
    bool PlayNamed(const std::string& name);
    void ScaleCurrentClipToDuration(float targetSeconds) {
        targetSeconds=std::max(0.016f,targetSeconds); float current=0.0f;
        if(!currentClip.durations.empty()) for(float d:currentClip.durations) current+=std::max(0.016f,d);
        else {const std::size_t n=currentClip.frames.empty()?static_cast<std::size_t>(std::max(1,currentClip.endFrame-currentClip.startFrame+1)):currentClip.frames.size();current=static_cast<float>(n)*std::max(0.016f,currentClip.frameDuration);}
        if(current<=0.0f)return; const float s=targetSeconds/current; currentClip.frameDuration=std::max(0.016f,currentClip.frameDuration*s); for(float& d:currentClip.durations)d=std::max(0.016f,d*s); timer*=s;
    }
    std::size_t CurrentClipFrameIndex() const{return clipFrameIndex;}
    std::size_t CurrentClipFrameCount() const{return currentClip.frames.empty()?static_cast<std::size_t>(std::max(1,currentClip.endFrame-currentClip.startFrame+1)):currentClip.frames.size();}

    void SetFrames(std::vector<SpriteFrame> metadata) {
        frames = std::move(metadata);
        if (!frames.empty()) {
            currentFrame = std::clamp(currentFrame, 0, static_cast<int>(frames.size()) - 1);
        }
    }

    void Play(AnimationClip clip) {
        const int frameCount = frames.empty()
            ? std::max(1, cols * rows)
            : static_cast<int>(frames.size());
        currentClip = clip;
        if (!clip.frames.empty()) {
            currentClip.startFrame = clip.frames.front();
            currentClip.endFrame = clip.frames.back();
        }
        currentClip.startFrame = std::clamp(currentClip.startFrame, 0, frameCount - 1);
        currentClip.endFrame = std::clamp(std::max(currentClip.startFrame, clip.endFrame), 0, frameCount - 1);
        if (!clip.frames.empty()) {
            currentClip.endFrame = std::clamp(clip.frames.back(), currentClip.startFrame, frameCount - 1);
        }
        currentClip.frameDuration = std::max(0.016f, clip.frameDuration);
        clipFrameIndex = 0;
        currentFrame = currentClip.frames.empty() ? currentClip.startFrame : currentClip.frames.front();
        timer = 0.0f;
        isPlaying = true;
        isFinished = false;
    }

    void Update(float dt) {
        if (!isPlaying || isFinished) return;
        timer += std::max(0.0f, dt);
        while (true) {
            const bool perStep = !currentClip.frames.empty() && !currentClip.durations.empty();
            const float duration = perStep
                ? std::max(0.016f, currentClip.durations[std::min(clipFrameIndex, currentClip.durations.size() - 1)])
                : frames.empty()
                    ? currentClip.frameDuration
                    : std::max(0.016f, frames[static_cast<std::size_t>(currentFrame)].duration);
            if (timer < duration) break;
            timer -= duration;
            const bool explicitSequence = !currentClip.frames.empty();
            if (explicitSequence) {
                ++clipFrameIndex;
            } else {
                ++currentFrame;
            }
            const bool clipEnded = explicitSequence
                ? clipFrameIndex >= currentClip.frames.size()
                : currentFrame > currentClip.endFrame;
            if (clipEnded) {
                if (currentClip.loop) {
                    clipFrameIndex = 0;
                    currentFrame = explicitSequence
                        ? currentClip.frames.front() : currentClip.startFrame;
                } else {
                    currentFrame = explicitSequence
                        ? currentClip.frames.back() : currentClip.endFrame;
                    isFinished = true;
                    isPlaying = false;
                    break;
                }
            } else if (explicitSequence) {
                currentFrame = currentClip.frames[clipFrameIndex];
            }
        }
    }

    // BETA: arma el cuadro con sus piezas. Escala uniforme (no deforma) salvo
    // que el llamador pida otra; espejo horizontal reflejando cada pieza.
    void DrawPieces(const SpriteFrame& frame, Vector2 feet, float scaleX, float scaleY, bool flipX, Color tint,
                    float angle = 0.0f) const {
        if (!pieceTextures) return;
        for (const FramePiece& p : frame.pieces) {
            if (p.tex < 0 || p.tex >= static_cast<int>(pieceTextures->size())) continue;
            const Texture2D& t = (*pieceTextures)[static_cast<std::size_t>(p.tex)];
            if (t.id == 0) continue;
            const bool fx = p.flipX != flipX;
            const Rectangle src{p.src.x, p.src.y, fx ? -p.src.width : p.src.width, p.flipY ? -p.src.height : p.src.height};
            const float w = p.src.width * scaleX, h = p.src.height * scaleY;
            const float relX = flipX ? -(p.x + p.src.width) * scaleX : p.x * scaleX;
            const float relY = p.y * scaleY;
            // origen = pies: la rotacion (si hay) es alrededor de los pies.
            DrawTexturePro(t, src, {feet.x, feet.y, w, h}, {-relX, -relY}, angle, tint);
        }
    }

    // Dibuja un frame concreto del atlas (sin tocar el estado de reproduccion).
    void DrawFrame(int frameIndex, Vector2 pivotPosition, float scale, bool flipX, Color tint = WHITE) const {
        if (texture.id == 0 || frames.empty()) return;
        const SpriteFrame& frame = frames[static_cast<std::size_t>(std::clamp(frameIndex, 0, static_cast<int>(frames.size()) - 1))];
        if (!frame.pieces.empty()) { DrawPieces(frame, pivotPosition, scale, scale, flipX, tint); return; }
        const float width = frame.width * scale, height = frame.height * scale;
        const Rectangle source = {frame.source.x, frame.source.y, flipX ? -frame.source.width : frame.source.width, frame.source.height};
        const float px = flipX ? width - frame.pivotX * scale : frame.pivotX * scale;
        DrawTexturePro(texture, source, {pivotPosition.x - px, pivotPosition.y - frame.pivotY * scale, width, height}, {0.0f, 0.0f}, 0.0f, tint);
    }

    // Frame actual con metadatos (nullptr en atlas de rejilla sin metadatos).
    const SpriteFrame* CurrentFrameData() const {
        if (frames.empty()) return nullptr;
        return &frames[static_cast<std::size_t>(std::clamp(currentFrame, 0, static_cast<int>(frames.size()) - 1))];
    }

    // Duracion total de un clip con nombre (0 si no existe).
    float ClipSeconds(const std::string& name) const {
        const auto it = namedClips.find(name);
        if (it == namedClips.end()) return 0.0f;
        const AnimationClip& c = it->second;
        if (!c.durations.empty()) {
            float t = 0.0f;
            for (float d : c.durations) t += std::max(0.016f, d);
            return t;
        }
        const int n = c.frames.empty() ? std::max(1, c.endFrame - c.startFrame + 1) : static_cast<int>(c.frames.size());
        return n * std::max(0.016f, c.frameDuration);
    }

    // Igual que Draw pero con escala horizontal y vertical separadas (respiracion,
    // estiramiento en saltos y golpes, o un personaje mas delgado). Gira `angle`
    // grados alrededor de los pies.
    void DrawScaled(Vector2 feetPosition, float scaleX, float scaleY, bool flipX, Color tint = WHITE,
                    float angle = 0.0f) const {
        const SpriteFrame* frame = CurrentFrameData();
        if (!frame || texture.id == 0) { Draw(feetPosition, scaleY, flipX, tint); return; }
        if (!frame->pieces.empty()) { DrawPieces(*frame, feetPosition, scaleX, scaleY, flipX, tint, angle); return; }
        const float width = frame->width * scaleX, height = frame->height * scaleY;
        if (width <= 0.0f || height <= 0.0f || frame->source.width <= 0.0f || frame->source.height <= 0.0f) return;
        const Rectangle source = {frame->source.x, frame->source.y, flipX ? -frame->source.width : frame->source.width,
                                  frame->source.height};
        const float drawPivotX = flipX ? width - frame->pivotX * scaleX : frame->pivotX * scaleX;
        const float drawPivotY = frame->pivotY * scaleY;
        // El origen de rotacion es el pivote (pies), asi el personaje se inclina sin despegarse del suelo.
        DrawTexturePro(texture, source, {feetPosition.x, feetPosition.y, width, height}, {drawPivotX, drawPivotY}, angle, tint);
    }

    void Draw(Vector2 feetPosition, float scale, bool flipX, Color tint = WHITE) const {
        if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;

        const int frameCount = frames.empty()
            ? std::max(1, cols * rows)
            : static_cast<int>(frames.size());
        const int safeFrame = std::clamp(currentFrame, 0, frameCount - 1);

        if (!frames.empty()) {
            const SpriteFrame& frame = frames[static_cast<std::size_t>(safeFrame)];
            if (!frame.pieces.empty()) { DrawPieces(frame, feetPosition, scale, scale, flipX, tint); return; }
            const float width = frame.width * scale;
            const float height = frame.height * scale;
            if (width <= 0.0f || height <= 0.0f || frame.source.width <= 0.0f || frame.source.height <= 0.0f) return;

            const Rectangle source = {
                frame.source.x,
                frame.source.y,
                flipX ? -frame.source.width : frame.source.width,
                frame.source.height
            };

            // SpriteFrame ya almacena el pivote relativo al recorte. Al invertir
            // horizontalmente solo hay que reflejar la distancia al borde izquierdo.
            const float drawPivotX = flipX
                ? width - frame.pivotX * scale
                : frame.pivotX * scale;
            const float drawPivotY = frame.pivotY * scale;

            const Rectangle dest = {
                feetPosition.x - drawPivotX,
                feetPosition.y - drawPivotY,
                width,
                height
            };
            DrawTexturePro(texture, source, dest, {0.0f, 0.0f}, 0.0f, tint);
            return;
        }

        const int col = safeFrame % cols;
        const int row = safeFrame / cols;

        const int x0 = (col * texture.width) / cols;
        const int x1 = ((col + 1) * texture.width) / cols;
        const int y0 = (row * texture.height) / rows;
        const int y1 = ((row + 1) * texture.height) / rows;
        const float frameWidth = static_cast<float>(x1 - x0);
        const float frameHeight = static_cast<float>(y1 - y0);
        if (frameWidth <= 1.0f || frameHeight <= 1.0f) return;

        Rectangle source = {
            static_cast<float>(x0),
            static_cast<float>(y0),
            flipX ? -frameWidth : frameWidth,
            frameHeight
        };

        if (normalizedAtlas) {
            const float width = frameWidth * scale;
            const float height = frameHeight * scale;
            const Rectangle dest = {
                feetPosition.x - width * 0.5f,
                feetPosition.y - height,
                width,
                height
            };
            DrawTexturePro(texture, source, dest, {0.0f, 0.0f}, 0.0f, tint);
            return;
        }

        const int inset = (x1 - x0 > 4 && y1 - y0 > 4) ? 1 : 0;
        const float safeWidth = frameWidth - inset * 2.0f;
        const float safeHeight = frameHeight - inset * 2.0f;
        const Rectangle safeSource = {
            static_cast<float>(x0 + inset),
            static_cast<float>(y0 + inset),
            flipX ? -safeWidth : safeWidth,
            safeHeight
        };
        const Rectangle dest = {
            feetPosition.x - safeWidth * scale * 0.5f,
            feetPosition.y - safeHeight * scale,
            safeWidth * scale,
            safeHeight * scale
        };
        DrawTexturePro(texture, safeSource, dest, {0.0f, 0.0f}, 0.0f, tint);
    }
};

}
