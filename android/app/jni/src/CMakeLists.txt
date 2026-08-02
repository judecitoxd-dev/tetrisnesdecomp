cmake_minimum_required(VERSION 3.22.1)
project(TetrisNESMain LANGUAGES C)

set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

add_library(main SHARED
    "${ROOT_DIR}/src/main.c"
    "${ROOT_DIR}/src/app.c"
    "${ROOT_DIR}/src/audio.c"
    "${ROOT_DIR}/src/rom_picker.c"
    "${ROOT_DIR}/src/android_bridge.c"
    "${ROOT_DIR}/src/touch_controls.c"
    "${ROOT_DIR}/src/rom_screens.c"
    "${ROOT_DIR}/src/rom_sprites.c"
    "${ROOT_DIR}/src/rom_endings.c"
    "${ROOT_DIR}/src/rom_concert_animation.c"
    "${ROOT_DIR}/src/rom_cathedral_sprites.c"
    "${ROOT_DIR}/src/rom_type_a.c"
    "${ROOT_DIR}/src/rom_type_a_background.c"
    "${ROOT_DIR}/src/game.c"
    "${ROOT_DIR}/src/rom.c"
    "${ROOT_DIR}/src/highscores.c"
    "${ROOT_DIR}/src/name_entry.c"
    "${ROOT_DIR}/src/demo.c"
    "${ROOT_DIR}/src/settings.c"
    "${ROOT_DIR}/src/replay.c"
    "${ROOT_DIR}/src/cathedral.c"
)

target_include_directories(main PRIVATE "${ROOT_DIR}/src")
target_compile_definitions(main PRIVATE TETRIS_ANDROID=1)
target_compile_options(main PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(main PRIVATE SDL2 log android m)
