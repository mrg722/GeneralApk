#pragma once
// Bot de prueba del Nivel 1: produce la PlayerInput de un frame a partir del
// estado visible del juego (la misma interfaz que el teclado).
#include "game/Stage1StoryGame.h"
#include <cmath>

namespace district_fury {

struct Stage1Bot {
    bool invulnerable = false;
    int attackTick = 0, comboStep = 0;

    static float Sign(float v) { return v > 0 ? 1.f : (v < 0 ? -1.f : 0.f); }

    // Devuelve false si la partida termino (StageClear o GameOver).
    bool Think(Stage1StoryGame& game, PlayerInput& in) {
        Player& p = game.PlayerRef();
        in = PlayerInput{};
        const StoryFlow flow = game.Flow();
        if (flow == StoryFlow::StageClear || flow == StoryFlow::GameOver) return false;

        // Objetivo: enemigo vivo mas cercano, o Brakk en su pelea.
        bool hasTarget = false;
        float tx = 0, ty = 0;
        if (flow == StoryFlow::Boss) {
            hasTarget = !game.Boss().defeated;
            tx = game.Boss().position.x; ty = game.Boss().position.y;
        } else {
            float best = 1e9f;
            for (const StreetEnemy& e : game.Enemies()) {
                if (!e.active || e.IsDefeated()) continue;
                const float d = std::fabs(e.position.x - p.position.x) + std::fabs(e.position.y - p.position.y);
                if (d < best) { best = d; hasTarget = true; tx = e.position.x; ty = e.position.y; }
            }
        }

        if (!hasTarget) {
            in.moveX = 1.0f;                                  // avanzar (GO >>)
            in.moveY = Sign(575.f - p.position.y);
        } else {
            const float dx = tx - p.position.x, dy = ty - p.position.y;
            // Pararse delante del objetivo, del lado donde ya esta el jugador.
            const float side = dx >= 0 ? -1.f : 1.f;
            const float wantX = tx + side * (flow == StoryFlow::Boss ? 105.f : 82.f);
            if (std::fabs(dy) > 14.f) in.moveY = Sign(dy);
            // Pegado al objetivo (p. ej. ambos contra el borde): golpear sin moverse.
            const bool overlapping = std::fabs(dx) < 40.f;
            if (!overlapping && std::fabs(wantX - p.position.x) > 20.f) in.moveX = Sign(wantX - p.position.x);
            const bool facingOk = std::fabs(dx) < 40.f || (dx >= 0) == (p.facing == Facing::Right);
            if (!facingOk && !overlapping && std::fabs(in.moveX) < 0.5f) in.moveX = Sign(dx);
            const bool inRange = std::fabs(dx) < 140.f && std::fabs(dy) < 32.f;
            if (inRange && facingOk && ++attackTick % 9 == 0) {
                // Cadena J J J K a traves del Input Buffer.
                if (comboStep++ % 4 == 3) in.kick = true; else in.punch = true;
            }
            if (flow == StoryFlow::Boss && p.sp >= 20 && std::fabs(dx) > 180.f && std::fabs(dy) < 30.f && facingOk)
                in.energy = true;
            if (p.rage >= p.maxRage) in.rage = true;

            // Defensa: bloquear (B) ante un ataque anunciado cercano.
            bool threat = false;
            if (flow == StoryFlow::Boss) {
                threat = game.Boss().attack != StoryBossAttack::None && std::fabs(dx) < 330.f;
            } else {
                for (const StreetEnemy& e : game.Enemies())
                    if (e.active && !e.IsDefeated() && (e.IsTelegraphing() || e.AttackIsActive()) &&
                        std::fabs(e.position.x - p.position.x) < 170.f && std::fabs(e.position.y - p.position.y) < 45.f)
                        threat = true;
            }
            if (flow == StoryFlow::Boss && p.hp < 50) {
                // Vida baja: mantener distancia y usar la Onda de Energia (L).
                in = PlayerInput{};
                const float away = dx >= 0 ? -1.f : 1.f;
                if (std::fabs(dx) < 300.f && !((away < 0 && p.position.x < 4990.f) || (away > 0 && p.position.x > 5880.f))) in.moveX = away;
                else if (!facingOk) in.moveX = Sign(dx);
                if (std::fabs(dy) > 10.f) in.moveY = Sign(dy);
                if (facingOk && std::fabs(dy) < 30.f && p.sp >= 20) in.energy = true;
                if (game.Boss().attack != StoryBossAttack::None && std::fabs(dx) < 330.f && p.shield > 10) { in = PlayerInput{}; in.block = true; }
            } else if (flow == StoryFlow::Boss) {
                // Contra Brakk: golpear en sus pausas y alejarse cuando prepara un ataque.
                const StoryBoss& b = game.Boss();
                const bool danger = !invulnerable && (b.attack != StoryBossAttack::None || b.attackTimer < 0.20f);
                if (danger && std::fabs(dx) < 360.f) {
                    in = PlayerInput{};
                    const float away = dx >= 0 ? -1.f : 1.f;
                    const bool cornered = (away < 0 && p.position.x < 4990.f) || (away > 0 && p.position.x > 5880.f);
                    // Con escudo: bloquear (B). Sin escudo: alejarse.
                    if (p.shield > 25 || cornered) in.block = true; else in.moveX = away;
                    in.moveY = Sign(ty - p.position.y) * (std::fabs(dy) > 60.f ? 0.f : -1.f);   // salir del carril
                }
            } else if (threat && p.shield > 15 && p.state != PlayerState::Attack) {
                in = PlayerInput{};
                in.block = true;
            }
        }

        return true;
    }
};

}  // namespace district_fury
