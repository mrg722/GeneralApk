// Controles tactiles -> teclas virtuales -> jugador (sin ventana).
#include "core/InputMap.h"
#include "game/Player.h"
#include "ui/TouchControls.h"
#include <cassert>
#include <cstdio>

using namespace district_fury;

int main() {
    touch::SetEnabled(true);
    touch::SetScreenTransform(1.0f, 0.0f, 0.0f);

    // Boton GOLPE: flanco en el primer frame, mantenido mientras dura el toque.
    touch::InjectTouchForTest({1165, 612});
    touch::Update(touch::Context::Combat);
    assert(input::Pressed(KEY_J) && input::Down(KEY_J));
    touch::InjectTouchForTest({1165, 612});
    touch::Update(touch::Context::Combat);
    assert(!input::Pressed(KEY_J) && input::Down(KEY_J));
    touch::Update(touch::Context::Combat);
    assert(!input::Down(KEY_J));

    // Joystick a la derecha + GOLPE a la vez (multitouch).
    touch::InjectTouchForTest({175 + 90, 560});
    touch::InjectTouchForTest({1165, 612});
    touch::Update(touch::Context::Combat);
    assert(input::Down(KEY_D) && !input::Down(KEY_A) && input::Pressed(KEY_J));
    assert(!input::Down(KEY_RIGHT));                 // en combate no se tocan las flechas

    // Menu: cruceta a la derecha + OK = ENTER; el joystick no existe en menus.
    touch::Update(touch::Context::Menu);
    touch::InjectTouchForTest({1010, 530});
    touch::InjectTouchForTest({1180, 612});
    touch::InjectTouchForTest({175 + 90, 560});
    touch::Update(touch::Context::Menu);
    assert(input::Pressed(KEY_UP) && input::Pressed(KEY_ENTER) && !input::Down(KEY_D));

    // Recompensa: botones 1/2/3.
    touch::InjectTouchForTest({640, 640});
    touch::Update(touch::Context::Reward);
    assert(input::Pressed(KEY_TWO) && !input::Pressed(KEY_ONE));

    // Fin de partida: REINTENTAR = R.
    touch::InjectTouchForTest({1050, 660});
    touch::Update(touch::Context::EndScreen);
    assert(input::Pressed(KEY_R));

    // Habilidad 1 (ONDA en nuestros personajes): lanza la onda sin gastar energia.
    Player p;
    touch::Update(touch::Context::Combat);
    touch::InjectTouchForTest({961, 633});
    touch::Update(touch::Context::Combat);
    const int spBefore = p.sp;
    p.Update(1.0f / 60.0f);
    assert(p.state == PlayerState::Attack && p.currentAttack == AttackId::EnergyWave);
    assert(p.sp == spBefore && !p.SkillReady(0));
    assert(p.skillCooldown[0] > Player::kSkillCooldown - 0.1f);
    // En espera: volver a pulsar no la lanza otra vez.
    for (int i = 0; i < 120; ++i) { touch::Update(touch::Context::Combat); p.Update(1.0f / 60.0f); }
    touch::InjectTouchForTest({961, 633});
    touch::Update(touch::Context::Combat);
    p.Update(1.0f / 60.0f);
    assert(p.currentAttack != AttackId::EnergyWave || p.state != PlayerState::Attack);

    // Habilidad 2 (GANCHO) -> gancho ascendente (Punch3).
    Player q;
    touch::Update(touch::Context::Combat);
    touch::InjectTouchForTest({965, 568});
    touch::Update(touch::Context::Combat);
    q.Update(1.0f / 60.0f);
    assert(q.currentAttack == AttackId::Punch3);

    // ESPECIAL: el especial por boton (energia), como antes la ONDA.
    Player e;
    touch::Update(touch::Context::Combat);
    touch::InjectTouchForTest({1245, 485});
    touch::Update(touch::Context::Combat);
    e.Update(1.0f / 60.0f);
    assert(e.currentAttack == AttackId::EnergyWave);

    // Habilidad 6: transformacion (dano y velocidad extra durante 12 s).
    Player t;
    touch::Update(touch::Context::Combat);
    touch::InjectTouchForTest({1152, 406});
    touch::Update(touch::Context::Combat);
    t.Update(1.0f / 60.0f);
    assert(t.IsTransformed() && t.activeSkill == Player::kSkillCount - 1);

    // Desactivado: los toques no hacen nada.
    touch::SetEnabled(false);
    touch::InjectTouchForTest({1165, 612});
    touch::Update(touch::Context::Combat);
    assert(!input::Down(KEY_J));

    std::puts("touch_controls_tests OK");
    return 0;
}
