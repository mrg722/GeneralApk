#pragma once
// Capa unica de entrada: teclado + teclas virtuales (controles tactiles).
// Todo el juego lee con input::Pressed / input::Down en vez de IsKeyPressed /
// IsKeyDown, asi cualquier boton en pantalla funciona en menus y combate.
namespace district_fury {
namespace input {

// Teclas virtuales sin equivalente de teclado (comandos especiales directos).
constexpr int kVirtualSpecialWave = 400;   // "ONDA": abajo, abajo-adelante, adelante + J
constexpr int kVirtualSpecialRise = 401;   // "GANCHO": adelante, abajo, abajo-adelante + K
// Habilidades 1..6 (boton tactil; teclado: 1..6). 402..407.
constexpr int kVirtualSkill0 = 402;
constexpr int kSkillKeys = 6;
constexpr int kKeyCount = 512;

// La UI tactil marca teclas virtuales pulsadas este frame y luego llama a Commit().
void ClearNext();
void SetVirtual(int key, bool down);
void Commit();

bool Pressed(int key);   // flanco de bajada (teclado o virtual)
bool Down(int key);      // mantenida (teclado o virtual)

}  // namespace input
}  // namespace district_fury
