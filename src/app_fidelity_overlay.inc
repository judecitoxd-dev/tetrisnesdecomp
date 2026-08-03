/*
 * Final ROM-fidelity pass.
 *
 * This file is included after app_score_overlay.inc, so it can reuse that
 * module's private virtual nametables and CHR atlases without duplicating ROM
 * data or committing extracted assets.
 */

#define FIDELITY_PPU_GAME_TYPE 0x2083u

static uint16_t fidelity_ppu_address(int tile_x, int tile_y) {
    return (uint16_t)(0x2000u + (unsigned)tile_y * 32u + (unsigned)tile_x);
}

static void fidelity_draw_level_table_frame(SDL_Renderer *renderer,
                                            const OverlayScreenState *state) {
    int x;
    int y;
    if (!renderer || !state) return;

    /* Top edge: TL, twenty horizontal tiles, TR. */
    overlay_draw_tile(renderer, state, fidelity_ppu_address(5, 17), 0x28u);
    for (x = 6; x <= 25; ++x)
        overlay_draw_tile(renderer, state, fidelity_ppu_address(x, 17), 0x29u);
    overlay_draw_tile(renderer, state, fidelity_ppu_address(26, 17), 0x2au);

    /* Header side walls. */
    overlay_draw_tile(renderer, state, fidelity_ppu_address(5, 18), 0x2bu);
    overlay_draw_tile(renderer, state, fidelity_ppu_address(26, 18), 0x2cu);

    /* Header separator with the original left/right junction tiles. */
    overlay_draw_tile(renderer, state, fidelity_ppu_address(5, 19), 0x4cu);
    for (x = 6; x <= 25; ++x)
        overlay_draw_tile(renderer, state, fidelity_ppu_address(x, 19), 0x2eu);
    overlay_draw_tile(renderer, state, fidelity_ppu_address(26, 19), 0x4du);

    /* Record rows and bottom edge. */
    for (y = 20; y <= 24; ++y) {
        overlay_draw_tile(renderer, state, fidelity_ppu_address(5, y), 0x2bu);
        overlay_draw_tile(renderer, state, fidelity_ppu_address(26, y), 0x2cu);
    }
    overlay_draw_tile(renderer, state, fidelity_ppu_address(5, 25), 0x2du);
    for (x = 6; x <= 25; ++x)
        overlay_draw_tile(renderer, state, fidelity_ppu_address(x, 25), 0x2eu);
    overlay_draw_tile(renderer, state, fidelity_ppu_address(26, 25), 0x2fu);
}

static void fidelity_draw_level_table(SDL_Renderer *renderer,
                                      const NesRom *rom,
                                      const AppMenuState *menu) {
    OverlayScreenState *state;
    enum OverlayExactScreen exact;
    if (!renderer || !rom || !menu) return;
    exact = menu->mode == TETRIS_MODE_B
        ? OVERLAY_EXACT_LEVEL_B : OVERLAY_EXACT_LEVEL_A;
    state = overlay_get_screen(renderer, rom, exact);
    if (!state) return;

    fidelity_draw_level_table_frame(renderer, state);
    overlay_draw_text(renderer, state, TETRIS_PPU_RECORD_HEADER_NAME, "NAME");
    overlay_draw_text(renderer, state, TETRIS_PPU_RECORD_HEADER_SCORE, "SCORE");
    overlay_draw_text(renderer, state, TETRIS_PPU_RECORD_HEADER_LEVEL, "LV");
}

static void fidelity_draw_game_type(SDL_Renderer *renderer,
                                    const NesRom *rom,
                                    const TetrisGame *game) {
    OverlayScreenState *state;
    if (!renderer || !rom || !game ||
        !tetris_rom_screen_available(TETRIS_ROM_SCREEN_GAME)) return;
    state = overlay_get_screen(renderer, rom, OVERLAY_EXACT_GAME);
    if (!state) return;

    /*
     * The old generic A/B glyph started four logical pixels below the NES tile
     * and leaked into the following row. Restore both affected tiles, then use
     * the game CHR bank and its attribute-selected palette.
     */
    overlay_restore_rectangle(renderer, state, 3, 4, 1, 2);
    overlay_draw_text(renderer, state, FIDELITY_PPU_GAME_TYPE,
                      game->mode == TETRIS_MODE_A ? "A" : "B");
}

static int fidelity_type_a_rocket_y(int initial_y,
                                    unsigned elapsed_frames) {
    int rocket_y = initial_y;
    unsigned frame;

    /* Original ending delay: F0 (240) updates before lift-off. */
    for (frame = 240u; frame < elapsed_frames && rocket_y > 0; ++frame) {
        const unsigned movement_frame = frame - 240u;
        if (rocket_y >= 0xb0 && (movement_frame & 1u) != 0u)
            continue;
        --rocket_y;
        if (rocket_y > 0 && rocket_y < 0x80)
            --rocket_y;
    }
    return rocket_y;
}

static bool fidelity_render_type_a_ending(SDL_Renderer *renderer,
                                          SDL_Texture *font,
                                          const AppResultState *result) {
    static const unsigned body_sprite[5] = {
        0x3eu, 0x41u, 0x44u, 0x47u, 0x4au
    };
    static const unsigned jet_sprite[5][2] = {
        {0x3fu, 0x40u}, {0x42u, 0x43u}, {0x45u, 0x46u},
        {0x48u, 0x49u}, {0x23u, 0x24u}
    };
    static const int rocket_x[5] = {0x54, 0x54, 0x50, 0x48, 0xa0};
    static const int initial_y[5] = {0xbf, 0xbf, 0xbf, 0xbf, 0xc7};
    unsigned elapsed_frames;
    unsigned jet;
    int rocket_class;
    int rocket_y;
    bool over_120k;
    char value[32];

    if (!renderer || !font || !result ||
        result->mode != TETRIS_MODE_A || result->score < 30000)
        return false;

    if (result->score < 50000) rocket_class = 0;
    else if (result->score < 70000) rocket_class = 1;
    else if (result->score < 100000) rocket_class = 2;
    else if (result->score < 120000) rocket_class = 3;
    else rocket_class = 4;

    over_120k = rocket_class == 4;
    if (!tetris_rom_type_a_background_available(over_120k) ||
        !tetris_rom_type_a_sprite_available(body_sprite[rocket_class]))
        return false;

    /* Redraw the complete ending last so the animated OAM is never hidden by
       the legacy fallback or by an intermediate presentation. */
    tetris_rom_type_a_background_render(renderer, over_120k);
    snprintf(value, sizeof(value), "%06d",
             result->score > 999999 ? 999999 : result->score);
    draw_text(renderer, font, 288, 64, 2, value);

    elapsed_frames = (unsigned)(((Uint32)(SDL_GetTicks() -
        result->ending_started_ms) * 60u) / 1000u);
    rocket_y = fidelity_type_a_rocket_y(initial_y[rocket_class],
                                        elapsed_frames);
    if (rocket_y > 0) {
        jet = jet_sprite[rocket_class][(elapsed_frames >> 1) & 1u];
        tetris_rom_type_a_sprite_render(renderer, body_sprite[rocket_class],
                                        rocket_x[rocket_class], rocket_y);
        if (tetris_rom_type_a_sprite_available(jet))
            tetris_rom_type_a_sprite_render(renderer, jet,
                                            rocket_x[rocket_class], rocket_y);
    }

    draw_centered(renderer, font, 450, 1,
                  result->rank >= 0 ? "ENTER NAME  H RECORDS" :
                                      "ENTER RETURNS  H RECORDS");
    return true;
}

void tetris_present_with_score_overlay(
    SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
    const TetrisGame *game, const AppMenuState *menu,
    const AppResultState *result, const NesRom *rom,
    const TetrisHighScores *scores) {
    /* Draw the existing ROM-native dynamic fields, but do not present yet. */
    tetris_present_with_score_overlay_legacy(renderer, font, screen,
                                             game, menu, result, rom, scores);

    switch (screen) {
        case SCREEN_LEVEL_SELECT:
            fidelity_draw_level_table(renderer, rom, menu);
            break;
        case SCREEN_GAME:
        case SCREEN_DEMO:
        case SCREEN_REPLAY:
            fidelity_draw_game_type(renderer, rom, game);
            break;
        case SCREEN_ENDING:
            (void)fidelity_render_type_a_ending(renderer, font, result);
            break;
        default:
            break;
    }
}
