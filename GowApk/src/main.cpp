// GowApk: aplicacion aparte, construida con el contenido del paquete original.
// Comparte el motor con District Fury (district_fury_core: Player, Animator,
// audio, entrada, controles tactiles) pero tiene sus propios datos, personajes,
// escenarios e interfaz. District Fury no depende de esta aplicacion.
#include "raylib.h"
#include "audio/AudioSystem.h"
#include "core/ApplicationState.h"
#include "core/DisplaySettings.h"
#include "core/InputMap.h"
#include "rendering/AssetManager.h"
#include "ui/TouchControls.h"
#include "BetaMode.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

// Pantalla de titulo: logo del juego original sobre el fondo de carga.
void DibujaTitulo(float t) {
    using namespace district_fury;
    ClearBackground({6, 8, 12, 255});
    const Texture2D fondo = AssetManager::Get().GetTextureByPath("GowApk/assets/ui/fondo_carga.png");
    if (fondo.id)
        DrawTexturePro(fondo, {0, 0, (float)fondo.width, (float)fondo.height}, {0, 0, 1280, 720}, {0, 0}, 0, WHITE);
    const Texture2D logo = AssetManager::Get().GetTextureByPath("GowApk/assets/ui/logo.png");
    if (logo.id) {
        const float k = std::min(760.0f / logo.width, 300.0f / logo.height);
        DrawTexturePro(logo, {0, 0, (float)logo.width, (float)logo.height},
                       {640 - logo.width * k / 2, 110, logo.width * k, logo.height * k}, {0, 0}, 0, WHITE);
    } else {
        const char* n = "GOW APK";
        DrawText(n, 640 - MeasureText(n, 72) / 2, 180, 72, {230, 190, 110, 255});
    }
    const unsigned char a = (unsigned char)(170 + 85 * std::sin(t * 3.0f));
    const char* msg = touch::Enabled() ? "TOCA LA PANTALLA PARA EMPEZAR" : "PULSA ENTER PARA EMPEZAR";
    DrawText(msg, 640 - MeasureText(msg, 24) / 2, 560, 24, {255, 235, 200, a});
    const char* pie = "Version de prueba. Contenido del paquete original, separado de District Fury.";
    DrawText(pie, 640 - MeasureText(pie, 13) / 2, 686, 13, {150, 140, 130, 200});
}

}  // namespace

int main() {
    constexpr int ancho = 1280, alto = 720;
#if defined(PLATFORM_ANDROID)
    InitWindow(0, 0, "GowApk");
    district_fury::touch::SetEnabled(true);
#else
    InitWindow(ancho, alto, "GowApk");
    district_fury::touch::SetEnabled(std::getenv("DF_TOUCH") != nullptr);
#endif
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    RenderTexture2D lienzo = LoadRenderTexture(ancho, alto);
    SetTextureFilter(lienzo.texture, TEXTURE_FILTER_BILINEAR);
    district_fury::display::Load();
    district_fury::AudioSystem::Get().Init();

    district_fury::BetaMode juego;
    bool enTitulo = true;
    float t = 0.0f;
    namespace touch = district_fury::touch;
    auto estado = district_fury::core::ApplicationState::Running;

    while (district_fury::core::shouldContinue(estado)) {
        const float escala = std::min(GetScreenWidth() / (float)ancho, GetScreenHeight() / (float)alto);
        const float escalaX = escala * district_fury::display::WidthPercent() / 100.0f;
        const float offX = (GetScreenWidth() - ancho * escalaX) * 0.5f;
        const float offY = (GetScreenHeight() - alto * escala) * 0.5f;
        touch::SetScreenTransform(escalaX, escala, offX, offY);
        const touch::Context ctx = enTitulo ? touch::Context::Menu
                                            : (juego.TouchContext() == 1   ? touch::Context::Menu
                                               : juego.TouchContext() == 2 ? touch::Context::EndScreen
                                                                           : touch::Context::Warrior);
        touch::Update(ctx);
        if (WindowShouldClose()) { estado = district_fury::core::ApplicationState::ExitRequested; continue; }

        const float dt = GetFrameTime();
        t += dt;
        if (enTitulo) {
            if (district_fury::input::Pressed(KEY_ENTER) || district_fury::input::Pressed(KEY_J) ||
                district_fury::input::Pressed(KEY_SPACE)) {
                juego.Init();
                enTitulo = false;
            }
            if (district_fury::input::Pressed(KEY_ESCAPE))
                estado = district_fury::core::ApplicationState::ExitRequested;
        } else {
            juego.Update(dt);
            if (juego.ShouldExit()) {   // salir del juego devuelve al titulo
                juego.ClearExit();
                juego.Shutdown();
                enTitulo = true;
            }
        }

        BeginTextureMode(lienzo);
        if (enTitulo) DibujaTitulo(t);
        else { ClearBackground({8, 11, 11, 255}); juego.Draw(); }
        touch::Draw(ctx, enTitulo ? nullptr : &juego.PlayerRef());
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(lienzo.texture, {0, 0, (float)ancho, -(float)alto},
                       {offX, offY, ancho * escalaX, alto * escala}, {0, 0}, 0, WHITE);
        EndDrawing();
    }
    juego.Shutdown();
    UnloadRenderTexture(lienzo);
    district_fury::AudioSystem::Get().Shutdown();
    district_fury::AssetManager::Get().UnloadAll();
    CloseWindow();
    return 0;
}
