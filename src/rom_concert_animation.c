#include "rom_endings.h"
#include "rom_cathedral_sprites.h"

static void render_if_available(SDL_Renderer *renderer, unsigned sprite,
                                int x, int y) {
    if (tetris_rom_concert_sprite_available(sprite))
        tetris_rom_concert_sprite_render(renderer, sprite, x, y);
}

void tetris_rom_concert_render(SDL_Renderer *renderer,
                               int start_height, unsigned frame) {
    static const int mario_y[4] = {0x97, 0x8f, 0x87, 0x8f};
    static const int luigi_y[10] = {
        0x97, 0x8f, 0x87, 0x87, 0x8f,
        0x97, 0x8f, 0x87, 0x87, 0x8f
    };
    static const unsigned luigi_sprite[10] = {
        0x29, 0x29, 0x29, 0x2a, 0x2a,
        0x2a, 0x2a, 0x2a, 0x29, 0x29
    };
    const unsigned slow_frame = (frame >> 4) & 1u;
    if (!renderer) return;
    if (start_height < 0) start_height = 0;
    if (start_height > 5) start_height = 5;

    /* The 6502 switch falls through from the selected height. */
    if (start_height >= 5) {
        const unsigned peach = 0x21u + ((frame >> 3) & 1u);
        const unsigned mario_step = (frame >> 3) & 3u;
        const int mario_position_y = mario_y[mario_step];
        const unsigned mario = mario_position_y == 0x97 ? 0x27u : 0x28u;
        const unsigned luigi_step = (frame >> 3) % 10u;
        render_if_available(renderer, peach, 0xc8, 0x47);
        render_if_available(renderer, mario, 0xa0, mario_position_y);
        render_if_available(renderer, luigi_sprite[luigi_step],
                            0xc0, luigi_y[luigi_step]);
    }
    if (start_height >= 4)
        render_if_available(renderer, 0x1fu + slow_frame, 0x30, 0xa7);
    if (start_height >= 3)
        render_if_available(renderer, 0x1du + slow_frame, 0x40, 0x77);
    if (start_height >= 2)
        render_if_available(renderer, 0x1au + slow_frame, 0xa8, 0xd7);
    if (start_height >= 1)
        render_if_available(renderer, 0x18u + slow_frame, 0xc8, 0xd7);
    render_if_available(renderer, 0x16u + slow_frame, 0x28, 0x77);
}

void tetris_rom_cathedral_render(SDL_Renderer *renderer,
                                 int level, int start_height,
                                 unsigned frame) {
    TetrisCathedralSnapshot snapshot;
    if (!renderer ||
        !tetris_rom_cathedral_snapshot(level, start_height, frame, &snapshot))
        return;
    for (int index = 0; index < snapshot.sprite_count; ++index) {
        if (snapshot.visible[index] &&
            tetris_rom_cathedral_sprite_available(snapshot.sprite_index)) {
            tetris_rom_cathedral_sprite_render(renderer, snapshot.sprite_index,
                                               snapshot.x[index],
                                               snapshot.y[index]);
        }
    }
}
