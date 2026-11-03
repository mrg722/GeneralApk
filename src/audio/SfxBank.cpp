// Banco de sonidos: logica pura de mezcla (ver SfxBank.h).
#include "audio/SfxBank.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

int VoicesFor(SoundCat cat) {
    switch (cat) {
        case SoundCat::Hit:    return 4;
        case SoundCat::Move:   return 3;
        case SoundCat::Step:   return 3;
        case SoundCat::Voice:  return 2;
        case SoundCat::Ui:     return 2;
        case SoundCat::Jingle: return 1;
        case SoundCat::Ambient:return 1;
    }
    return 1;
}

float TargetSt50Db(SoundCat cat) {
    switch (cat) {
        case SoundCat::Hit:    return -10.0f;
        case SoundCat::Move:   return -16.0f;
        case SoundCat::Step:   return -24.0f;
        case SoundCat::Ui:     return -16.0f;
        case SoundCat::Jingle: return -12.0f;
        case SoundCat::Voice:  return -12.0f;
        case SoundCat::Ambient:return -28.0f;
    }
    return -12.0f;
}

// ---- VoiceLimiter -----------------------------------------------------------
int VoiceLimiter::Acquire(int key, int priority, double now, bool* stolen) {
    if (stolen) *stolen = false;
    for (int i = 0; i < kMaxSfxVoices; ++i)
        if (!slots[i].used) {
            slots[i] = {true, key, priority, now};
            return i;
        }
    int victim = 0;
    for (int i = 1; i < kMaxSfxVoices; ++i) {
        const VoiceSlot& a = slots[i];
        const VoiceSlot& b = slots[victim];
        if (a.priority < b.priority || (a.priority == b.priority && a.start < b.start)) victim = i;
    }
    if (priority < slots[victim].priority) return -1;   // se descarta el nuevo
    if (stolen) *stolen = true;
    slots[victim] = {true, key, priority, now};
    return victim;
}

void VoiceLimiter::Release(int slot) {
    if (slot >= 0 && slot < kMaxSfxVoices) slots[slot] = VoiceSlot{};
}

int VoiceLimiter::Active() const {
    int n = 0;
    for (const VoiceSlot& s : slots) n += s.used ? 1 : 0;
    return n;
}

void VoiceLimiter::Clear() {
    for (VoiceSlot& s : slots) s = VoiceSlot{};
}

// ---- BurstGuard -------------------------------------------------------------
bool BurstGuard::Allow(int key, double now, double window) {
    if (key < 0 || key >= kMaxKeys) return true;
    if (seen[key] && now - last[key] < window) return false;
    seen[key] = true;
    last[key] = now;
    return true;
}

void BurstGuard::Clear() {
    for (int i = 0; i < kMaxKeys; ++i) { seen[i] = false; last[i] = 0.0; }
}

// ---- Azar / variantes -------------------------------------------------------
uint32_t MixRng::Next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

float MixRng::Uniform(float lo, float hi) {
    const float u = static_cast<float>(Next() >> 8) / 16777216.0f;
    return lo + (hi - lo) * u;
}

int PickVariant(int count, int last, MixRng& rng) {
    if (count <= 1) return 0;
    if (last < 0 || last >= count) return static_cast<int>(rng.Next() % static_cast<uint32_t>(count));
    // Elige entre las count-1 restantes: nunca repite la anterior.
    const int r = static_cast<int>(rng.Next() % static_cast<uint32_t>(count - 1));
    return r >= last ? r + 1 : r;
}

int PickAlias(const bool* playing, const double* start, int count) {
    if (count <= 0) return -1;
    int oldest = 0;
    for (int i = 0; i < count; ++i) {
        if (!playing[i]) return i;
        if (start[i] < start[oldest]) oldest = i;
    }
    return oldest;
}

float PanForX(float x, float cameraCenterX) {
    return 0.5f + std::clamp((x - cameraCenterX) / 1280.0f, -0.35f, 0.35f);
}

float DbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
float LinearToDb(float g) { return g <= 1e-9f ? -180.0f : 20.0f * std::log10(g); }

// ---- Ducker -----------------------------------------------------------------
void Ducker::Trigger(float depthDb, float holdSec) {
    const float g = DbToLinear(-std::fabs(depthDb));
    gain = std::min(gain, g);
    hold = std::max(hold, holdSec);
}

void Ducker::Update(float dt) {
    if (dt <= 0.0f) return;
    if (hold > 0.0f) {
        hold -= dt;
        if (hold >= 0.0f) return;
        dt = -hold;   // lo que sobra del frame ya se recupera
        hold = 0.0f;
    }
    constexpr float kTau = 0.035f;
    gain = 1.0f - (1.0f - gain) * std::exp(-dt / kTau);
    if (1.0f - gain < 0.005f) gain = 1.0f;
}

// ---- Limitador --------------------------------------------------------------
float SoftClip(float x) {
    static const float kNorm = 1.0f / std::tanh(1.2f);
    const float y = std::tanh(1.2f * x) * kNorm;
    return y > 1.0f ? 1.0f : (y < -1.0f ? -1.0f : y);
}

void SoftClipBuffer(float* samples, unsigned int count) {
    for (unsigned int i = 0; i < count; ++i) samples[i] = SoftClip(samples[i]);
}

// ---- Normalizacion ----------------------------------------------------------
float St50Db(const float* samples, unsigned int frames, int channels, int sampleRate) {
    if (!samples || frames == 0 || channels <= 0 || sampleRate <= 0) return -180.0f;
    const unsigned int win = std::max(1u, static_cast<unsigned int>(sampleRate / 20));   // 50 ms
    const unsigned int hop = std::max(1u, win / 2);
    double best = 0.0;
    for (unsigned int f0 = 0; f0 < frames; f0 += hop) {
        const unsigned int f1 = std::min(frames, f0 + win);
        double acc = 0.0;
        for (unsigned int f = f0; f < f1; ++f)
            for (int c = 0; c < channels; ++c) {
                const double v = samples[static_cast<std::size_t>(f) * channels + c];
                acc += v * v;
            }
        // Ventana corta al final: se promedia sobre la ventana completa (no infla).
        const double ms = acc / (static_cast<double>(win) * channels);
        best = std::max(best, ms);
        if (f1 == frames) break;
    }
    return best <= 1e-18 ? -180.0f : static_cast<float>(10.0 * std::log10(best));
}

float NormalizeGainDb(float st50Db, float targetDb) {
    if (st50Db <= -120.0f) return 0.0f;   // silencio: no se toca
    return std::clamp(targetDb - st50Db, -18.0f, 12.0f);
}

}  // namespace district_fury
