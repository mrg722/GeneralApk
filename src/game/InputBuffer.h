#pragma once
#include <array>
#include <cstddef>

namespace district_fury {

// Comandos de ataque que se registran en el buffer.
enum class InputCommand { Punch, Kick };

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
    void Push(InputCommand command) {
        if (count_ == kCapacity) PopFront();
        entries_[(head_ + count_) % kCapacity] = {command, frame_};
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

private:
    void PopFront() { head_ = (head_ + 1) % kCapacity; --count_; }

    std::array<Entry, kCapacity> entries_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    int frame_ = 0;
    float accumulator_ = 0.0f;
};

}  // namespace district_fury
