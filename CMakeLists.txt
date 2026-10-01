cmake_minimum_required(VERSION 3.21)

project(DistrictFury VERSION 0.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

set(RAYLIB_COMMIT c1ab645ca298a2801097931d1079b10ff7eb9df8)
set(RAYLIB_ZIP_URL "https://github.com/raysan5/raylib/archive/${RAYLIB_COMMIT}.zip")
set(RAYLIB_ZIP_SHA256 "94e2dc09267d8f05f3eca73f4c7f91b86a0d7a7a02e1befd26d15a31d2281d5a")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.raylib-src/CMakeLists.txt")
    FetchContent_Declare(
        raylib
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.raylib-src"
    )
else()
    FetchContent_Declare(
        raylib
        URL ${RAYLIB_ZIP_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
endif()

# Android (tools/android/build_apk.sh): raylib para NativeActivity + OpenGL ES2.
if(ANDROID)
    set(PLATFORM Android CACHE STRING "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(raylib)

add_library(district_fury_core
    src/audio/AudioSystem.cpp
    src/core/ApplicationState.cpp
    src/core/Json.cpp
    src/core/Platform.cpp
    src/core/InputMap.cpp
    src/core/DisplaySettings.cpp
    src/game/combat/AttackData.cpp
    src/game/combat/Boss.cpp
    src/game/combat/RayderCloneVisual.cpp
    src/game/RivalAI.cpp
    src/game/combat/BossDefinition.cpp
    src/game/combat/CombatWorld.cpp
    src/game/enemies/EnemyBrain.cpp
    src/game/Campaign.cpp
    src/game/CharacterVisual.cpp
    src/game/Player.cpp
    src/game/player/PlayerCombat.cpp
    src/game/player/PlayerDraw.cpp
    src/game/StreetEnemy.cpp
    src/game/stage/ArenaDirector.cpp
    src/game/lab/KfReference.cpp
    src/game/Stage1StoryGame.cpp
    src/game/stage1/Stage1Waves.cpp
    src/game/stage1/Stage1Combat.cpp
    src/game/stage1/Stage1Boss.cpp
    src/game/stage1/Stage1Draw.cpp
    src/game/Stage2Game.cpp
    src/game/Stage3Game.cpp
    src/game/Stage4Game.cpp
    src/game/Stage5Game.cpp
    src/game/VSMode.cpp
    src/game/npc/KesslerCameo.cpp
    src/rendering/Animator.cpp
    src/rendering/AssetManager.cpp
    src/rendering/SpriteManifest.cpp
    src/ui/MainMenu.cpp
    src/ui/GameHUD.cpp
    src/ui/TouchControls.cpp
)
target_include_directories(district_fury_core PUBLIC src)
target_link_libraries(district_fury_core PUBLIC raylib)

if(ANDROID)
    target_compile_definitions(district_fury_core PUBLIC PLATFORM_ANDROID)
    target_include_directories(district_fury_core PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
    # NativeActivity carga "libmain.so"; raylib define android_main() y llama a main().
    add_library(district_fury SHARED src/main.cpp)
    set_target_properties(district_fury PROPERTIES OUTPUT_NAME main)
    target_link_libraries(district_fury PRIVATE district_fury_core android log EGL GLESv2 OpenSLES m)
    target_link_options(district_fury PRIVATE "-u" "ANativeActivity_onCreate")
else()
    add_executable(district_fury src/main.cpp)
    target_link_libraries(district_fury PRIVATE district_fury_core)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(application_state_tests tests/ApplicationStateTests.cpp)
    target_link_libraries(application_state_tests PRIVATE district_fury_core)
    add_test(NAME application_state_tests COMMAND application_state_tests)

    add_executable(combat_smoke_tests tests/CombatSmokeTests.cpp)
    target_link_libraries(combat_smoke_tests PRIVATE district_fury_core)
    add_executable(attack_animation_timing_tests tests/AttackAnimationTimingTests.cpp)
    target_link_libraries(attack_animation_timing_tests PRIVATE district_fury_core)
    add_test(NAME attack_animation_timing_tests COMMAND attack_animation_timing_tests)
    add_test(NAME combat_smoke_tests COMMAND combat_smoke_tests)

    add_executable(input_buffer_tests tests/InputBufferTests.cpp)
    target_link_libraries(input_buffer_tests PRIVATE district_fury_core)
    add_test(NAME input_buffer_tests COMMAND input_buffer_tests)

    add_executable(touch_controls_tests tests/TouchControlsTests.cpp)
    target_link_libraries(touch_controls_tests PRIVATE district_fury_core)
    add_test(NAME touch_controls_tests COMMAND touch_controls_tests)

    add_executable(stage1_playthrough_test tests/Stage1PlaythroughTest.cpp)
    target_link_libraries(stage1_playthrough_test PRIVATE district_fury_core)
    add_test(NAME stage1_playthrough_test COMMAND stage1_playthrough_test)

    # Verificacion visual manual (necesita ventana; no se registra en ctest).
    add_executable(stage1_visual_check tests/Stage1VisualCheck.cpp)
    target_link_libraries(stage1_visual_check PRIVATE district_fury_core)

    add_executable(kf_bot_visual_check tests/KfBotVisualCheck.cpp)
    target_link_libraries(kf_bot_visual_check PRIVATE district_fury_core)
    add_executable(roster_sheets tests/RosterSheets.cpp)
    target_link_libraries(roster_sheets PRIVATE district_fury_core)
    add_executable(skills_visual_check tests/SkillsVisualCheck.cpp)
    target_link_libraries(skills_visual_check PRIVATE district_fury_core)

    add_executable(all_modes_check tests/AllModesCheck.cpp)
    target_link_libraries(all_modes_check PRIVATE district_fury_core)

    add_executable(vs_boss_visual_check tests/VsBossVisualCheck.cpp)
    target_link_libraries(vs_boss_visual_check PRIVATE district_fury_core)

    add_executable(sprite_manifest_tests tests/SpriteManifestTests.cpp)
    target_link_libraries(sprite_manifest_tests PRIVATE district_fury_core)
    add_test(NAME sprite_manifest_tests COMMAND sprite_manifest_tests
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Los tests usan assert(): en Release (CI) NDEBUG los apagaba y no
    # comprobaban nada. Se vuelven a encender solo en los tests.
    foreach(t application_state_tests combat_smoke_tests attack_animation_timing_tests input_buffer_tests
              touch_controls_tests stage1_playthrough_test sprite_manifest_tests)
        target_compile_options(${t} PRIVATE -UNDEBUG)
    endforeach()
endif()
