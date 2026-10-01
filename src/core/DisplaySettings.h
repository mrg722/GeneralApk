#pragma once
// Ajuste de pantalla (OPCIONES): ancho del area de juego en porcentaje.
// Solo reduce el ancho (la altura no cambia), para celulares muy alargados.
namespace district_fury {
namespace display {

constexpr int kMinWidthPercent = 70;
constexpr int kMaxWidthPercent = 100;
constexpr int kWidthStep = 5;

int WidthPercent();
void SetWidthPercent(int percent);   // se acota a [70, 100] y se guarda
void Load();                         // lee display_settings.txt (si existe)

}  // namespace display
}  // namespace district_fury
