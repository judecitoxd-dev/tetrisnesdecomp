cmake_minimum_required(VERSION 3.20)
project(tetris_nes_pc_port VERSION 0.18.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(TETRIS_FETCH_SDL2 "Download SDL2 automatically when it is not installed" ON)
option(TETRIS_ENABLE_OGG_AUDIO "Enable optional OGG audio packs through SDL2_mixer" ON)
option(TETRIS_FETCH_SDL2_MIXER "Download SDL2_mixer automatically when it is not installed" ON)
option(TETRIS_BUILD_APP "Build the SDL2 desktop application" ON)
option(TETRIS_BUILD_TESTS "Build gameplay unit tests" ON)
option(TETRIS_STATIC_MSVC_RUNTIME "Use the static MSVC runtime for portable Windows builds" ON)

if(MSVC AND TETRIS_STATIC_MSVC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

function(tetris_strict_c target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(tetris_core STATIC
    src/game.c
    src/rom.c
    src/highscores.c
    src/name_entry.c
    src/demo.c
    src/settings.c
    src/replay.c
    src/cathedral.c
    src/cpu6502_runtime.c
    src/cpu6502_irq.c
    src/nes_apu.c
    src/nes_apu_frame_counter.c
    src/rom_audio.c
)
target_include_directories(tetris_core PUBLIC src)
tetris_strict_c(tetris_core)
if(NOT MSVC)
    target_link_libraries(tetris_core PUBLIC m)
endif()

add_executable(tetris_replay_verify tools/replay_verify.c)
add_executable(tetris_demo_verify tools/demo_verify.c)
add_executable(tetris_apu_render tools/apu_render.c)
add_executable(tetris_apu_scenario tools/apu_scenario.c)
foreach(target IN ITEMS
        tetris_replay_verify tetris_demo_verify
        tetris_apu_render tetris_apu_scenario)
    target_link_libraries(${target} PRIVATE tetris_core)
    tetris_strict_c(${target})
endforeach()

if(TETRIS_BUILD_APP)
    find_package(SDL2 CONFIG QUIET)
    if(NOT TARGET SDL2::SDL2)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
        endif()
    endif()
    if(NOT TARGET SDL2::SDL2 AND NOT TARGET PkgConfig::SDL2 AND TETRIS_FETCH_SDL2)
        include(FetchContent)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.30.11
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(SDL2)
    endif()

    add_executable(tetris_pc
        src/main.c src/app.c src/audio.c src/rom_picker.c
        src/android_bridge.c src/touch_controls.c
        src/rom_screens.c src/rom_sprites.c src/rom_endings.c
        src/rom_concert_animation.c src/rom_cathedral_sprites.c
        src/rom_type_a.c src/rom_type_a_background.c
    )
    target_link_libraries(tetris_pc PRIVATE tetris_core)
    if(TARGET SDL2::SDL2)
        target_link_libraries(tetris_pc PRIVATE SDL2::SDL2)
    elseif(TARGET PkgConfig::SDL2)
        target_link_libraries(tetris_pc PRIVATE PkgConfig::SDL2)
    else()
        message(FATAL_ERROR "SDL2 was not found. Install SDL2 or enable TETRIS_FETCH_SDL2.")
    endif()
    if(TARGET SDL2::SDL2main)
        target_link_libraries(tetris_pc PRIVATE SDL2::SDL2main)
    endif()

    if(TETRIS_ENABLE_OGG_AUDIO)
        find_package(SDL2_mixer CONFIG QUIET)
        if(NOT TARGET SDL2_mixer::SDL2_mixer AND
           NOT TARGET SDL2_mixer::SDL2_mixer-static)
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(SDL2_MIXER QUIET IMPORTED_TARGET SDL2_mixer)
            endif()
        endif()
        if(NOT TARGET SDL2_mixer::SDL2_mixer AND
           NOT TARGET SDL2_mixer::SDL2_mixer-static AND
           NOT TARGET PkgConfig::SDL2_MIXER AND
           TETRIS_FETCH_SDL2_MIXER)
            include(FetchContent)
            set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_SAMPLES OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_VENDORED ON CACHE BOOL "" FORCE)
            set(SDL2MIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_CMD OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_GME OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_MP3 OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_MIDI OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)
            set(SDL2MIXER_VORBIS STB CACHE STRING "" FORCE)
            set(SDL2MIXER_WAVE ON CACHE BOOL "" FORCE)
            FetchContent_Declare(SDL2_mixer
                GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
                GIT_TAG release-2.8.1
                GIT_SHALLOW TRUE
            )
            FetchContent_MakeAvailable(SDL2_mixer)
        endif()
        if(TARGET SDL2_mixer::SDL2_mixer-static)
            target_link_libraries(tetris_pc PRIVATE SDL2_mixer::SDL2_mixer-static)
            target_compile_definitions(tetris_pc PRIVATE TETRIS_HAVE_SDL_MIXER=1)
        elseif(TARGET SDL2_mixer::SDL2_mixer)
            target_link_libraries(tetris_pc PRIVATE SDL2_mixer::SDL2_mixer)
            target_compile_definitions(tetris_pc PRIVATE TETRIS_HAVE_SDL_MIXER=1)
        elseif(TARGET PkgConfig::SDL2_MIXER)
            target_link_libraries(tetris_pc PRIVATE PkgConfig::SDL2_MIXER)
            target_compile_definitions(tetris_pc PRIVATE TETRIS_HAVE_SDL_MIXER=1)
        else()
            message(WARNING "SDL2_mixer was not found; OGG audio-pack support is disabled.")
        endif()
    endif()
    if(WIN32)
        target_link_libraries(tetris_pc PRIVATE comdlg32)
        set_target_properties(tetris_pc PROPERTIES WIN32_EXECUTABLE OFF)
    endif()
    tetris_strict_c(tetris_pc)
endif()

if(TETRIS_BUILD_TESTS)
    enable_testing()
    add_executable(tetris_game_tests tests/game_tests.c)
    add_executable(tetris_demo_tests tests/demo_rom_tests.c)
    add_executable(tetris_cathedral_tests tests/cathedral_tests.c)
    add_executable(tetris_apu_tests tests/apu_tests.c)
    add_executable(tetris_irq_timing_tests tests/irq_timing_tests.c)
    add_executable(tetris_call_frame_tests tests/call_frame_regression_tests.c)
    add_executable(tetris_highscores_tests tests/highscores_tests.c)
    foreach(target IN ITEMS
            tetris_game_tests tetris_demo_tests tetris_cathedral_tests
            tetris_apu_tests tetris_irq_timing_tests tetris_call_frame_tests
            tetris_highscores_tests)
        target_link_libraries(${target} PRIVATE tetris_core)
        tetris_strict_c(${target})
    endforeach()
    add_test(NAME gameplay COMMAND tetris_game_tests)
    add_test(NAME demo_rom COMMAND tetris_demo_tests)
    add_test(NAME cathedral COMMAND tetris_cathedral_tests)
    add_test(NAME apu COMMAND tetris_apu_tests)
    add_test(NAME irq_timing COMMAND tetris_irq_timing_tests)
    add_test(NAME call_frame COMMAND tetris_call_frame_tests)
    add_test(NAME highscores COMMAND tetris_highscores_tests)
    add_test(NAME apu_render_self_test COMMAND tetris_apu_render --self-test)
    add_test(NAME apu_scenario_self_test COMMAND tetris_apu_scenario --self-test)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME apu_matrix_self_test
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/apu_matrix.py self-test)
    endif()
endif()

include(GNUInstallDirs)
if(TETRIS_BUILD_APP)
    install(TARGETS tetris_pc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(TARGETS tetris_replay_verify tetris_demo_verify
        tetris_apu_render tetris_apu_scenario
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS
    tools/audio_pack.py tools/rom_assets_verify.py
    tools/type_a_ending_verify.py tools/trace_compare.py
    tools/apu_matrix.py tools/mesen_trace.lua
    DESTINATION tools)
install(FILES README.md DESTINATION .)
install(FILES
    docs/PORT_STATUS.md docs/ANDROID_PORT.md docs/ROM_MAP.md
    docs/ROM_SCREENS.md docs/ROM_DEMO_OAM.md docs/ROM_TYPE_A_ENDING.md
    docs/TRACE_COMPARISON.md docs/APU_RENDERING.md
    docs/REPLAY_FORMAT.md docs/AUDIO_PACK.md docs/DECOMP_TOOLS.md
    DESTINATION docs)

set(CPACK_PACKAGE_NAME "tetris-nes-pc-port")
set(CPACK_PACKAGE_VENDOR "YlPorts")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "tetris-nes-pc-port-v${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)