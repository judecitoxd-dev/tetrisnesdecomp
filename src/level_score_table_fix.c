#include "level_score_table_fix.h"

/*
 * Compatibility hook retained for the PC/Android main loop.
 *
 * The level-select table is now completed by app_fidelity_overlay.inc using
 * the ROM's own CHR bank, palette attributes and frame tiles. Drawing the old
 * grayscale pass here would cover that exact result and reintroduce the
 * SCORE/LV flicker caused by the former two-presentation pipeline.
 */
void render_level_score_table_fix(
    SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
    const AppMenuState *menu, const TetrisHighScores *scores) {
    (void)renderer;
    (void)font;
    (void)screen;
    (void)menu;
    (void)scores;
}
