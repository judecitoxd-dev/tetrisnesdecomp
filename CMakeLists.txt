cmake_minimum_required(VERSION 3.20)
project(tetris_nes_pc_port VERSION 0.3.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(TETRIS_FETCH_SDL2 "Download SDL2 automatically when it is not installed" ON)
option(TETRIS_BUILD_APP "Build the SDL2 desktop application" ON)
option(TETRIS_BUILD_TESTS "Build gameplay unit tests" ON)

add_library(tetris_core STATIC
    src/game.c
    src/rom.c
    src/highscores.c
)
target_include_directories(tetris_core PUBLIC src)
if(MSVC)
    target_compile_options(tetris_core PRIVATE /W4)
else()
    target_compile_options(tetris_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

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

    add_executable(tetris_pc src/main.c src/app.c src/audio.c)
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

    if(WIN32)
        set_target_properties(tetris_pc PROPERTIES WIN32_EXECUTABLE OFF)
    endif()

    include(GNUInstallDirs)
    install(TARGETS tetris_pc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES README.md DESTINATION .)
endif()

if(TETRIS_BUILD_TESTS)
    enable_testing()
    add_executable(tetris_game_tests tests/game_tests.c)
    target_link_libraries(tetris_game_tests PRIVATE tetris_core)
    add_test(NAME gameplay COMMAND tetris_game_tests)
endif()
