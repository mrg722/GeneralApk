#pragma once
#include <array>
#include <cstddef>

namespace district_fury {

// Comandos de ataque que se registran en el buffer.
// Punch/Kick = J/K. Los especiales se reconocen por comando de direccion al
// estilo King Fighter (ver MotionHistory): SpecialWave = abajo, abajo-adelante,
// adelante + J; SpecialRise = adelante, abajo, abajo-adelante + K.
enum class InputCommand { Punch, Kick, SpecialWave, SpecialRise };

// Cola circular de tamano fijo (sin asignaciones dinamicas) con caducidad
// estricta en frames de 1/60 s. Un comando que supera kExpiryFrames sin ser
// consumido se elimina solo.
class InputBuffer {
public:
    static constexpr int kExpiryFrames = 15;
    static constexpr std::size_t kCapacity = 8;
    static constexpr float kFrameSeconds = 1.0f / 60.0f;

    struct Entry {
        InputCommand command = InputCommand::Punch;
        int frame = 0;  // frame global en el que se pulso
        int dir = 0;    // -1/+1: direccion del comando especial (0 = sin direccion)
    };

    // Avanza el reloj interno en frames enteros y limpia lo caducado.
    void Tick(float dt) {
        accumulator_ += dt;
        while (accumulator_ >= kFrameSeconds) {
            accumulator_ -= kFrameSeconds;
            ++frame_;
        }
        PruneExpired();
    }

    // Registra un comando. Con la cola llena descarta el mas antiguo.
    void Push(InputCommand command, int dir = 0) {
        if (count_ == kCapacity) PopFront();
        entries_[(head_ + count_) % kCapacity] = {command, frame_, dir};
        ++count_;
    }

    // Elimina del frente todo comando con edad > kExpiryFrames.
    void PruneExpired() {
        while (count_ > 0 && frame_ - entries_[head_].frame > kExpiryFrames) PopFront();
    }

    bool Empty() const { return count_ == 0; }
    std::size_t Size() const { return count_; }

    // Comando mas antiguo vigente (nullptr si esta vacia).
    const Entry* Peek() const { return count_ == 0 ? nullptr : &entries_[head_]; }

    // Extrae el comando mas antiguo. Devuelve false si esta vacia.
    bool Pop(InputCommand& out) {
        if (count_ == 0) return false;
        out = entries_[head_].command;
        PopFront();
        return true;
    }

    void Clear() { head_ = 0; count_ = 0; accumulator_ = 0.0f; }
    int Frame() const { return frame_; }

private:
    void PopFront() { head_ = (head_ + 1) % kCapacity; --count_; }

    std::array<Entry, kCapacity> entries_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    int frame_ = 0;
    float accumulator_ = 0.0f;
};

// Historial de direcciones (en X absoluta) para reconocer comandos especiales.
// Solo guarda cambios de direccion; ventana de 15 frames como el buffer.
class MotionHistory {
public:
    static constexpr int kWindowFrames = 15;
    static constexpr std::size_t kCapacity = 16;

    struct Step { bool down = false; int h = 0; int frame = 0; };

    void Record(bool down, int h, int frame) {
        if (count_ > 0) {
            const Step& last = steps_[(head_ + count_ - 1) % kCapacity];
            if (last.down == down && last.h == h) return;
        }
        if (count_ == kCapacity) { head_ = (head_ + 1) % kCapacity; --count_; }
        steps_[(head_ + count_) % kCapacity] = {down, h, frame};
        ++count_;
    }
    void Clear() { head_ = 0; count_ = 0; }

    // abajo -> (abajo-adelante) -> adelante. Devuelve la direccion (+1/-1) o 0.
    int QuarterCircle(int frame) const {
        if (count_ < 2) return 0;
        const Step& last = At(count_ - 1);
        if (last.h == 0 || frame - last.frame > kWindowFrames) return 0;
        for (int i = static_cast<int>(count_) - 2; i >= 0; --i) {
            const Step& s = At(static_cast<std::size_t>(i));
            if (frame - s.frame > kWindowFrames) break;
            if (s.down && s.h == 0) return last.h;          // abajo puro encontrado
            if (s.h != last.h && s.h != 0) break;            // cambio de lado: invalido
        }
        return 0;
    }

    // adelante -> abajo -> abajo-adelante ("shoryuken"). Devuelve +1/-1 o 0.
    int DragonPunch(int frame) const {
        if (count_ < 3) return 0;
        const Step& last = At(count_ - 1);
        if (!last.down || last.h == 0 || frame - last.frame > kWindowFrames) return 0;
        bool sawDown = false;
        for (int i = static_cast<int>(count_) - 2; i >= 0; --i) {
            const Step& s = At(static_cast<std::size_t>(i));
            if (frame - s.frame > kWindowFrames) break;
            if (s.down && s.h == 0) sawDown = true;
            else if (!s.down && s.h == last.h) return sawDown ? last.h : 0;
        }
        return 0;
    }

private:
    const Step& At(std::size_t i) const { return steps_[(head_ + i) % kCapacity]; }
    std::array<Step, kCapacity> steps_{};
    std::size_t head_ = 0, count_ = 0;
};

}  // namespace district_fury
