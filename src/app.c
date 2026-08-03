#include "app_score_overlay.h"
#include "app_v05_00.inc"
#include "app_v05_01.inc"
#include "app_v05_02.inc"
#include "app_v05_03.inc"
#include "app_v05_04.inc"
#include "app_v05_05.inc"

/*
 * app_score_overlay.inc historically presented the renderer itself. The main
 * loop also presents after touch controls and the final menu overlays, which
 * exposed two different images per iteration and made SCORE/LV and the level
 * table visibly blink. Keep the legacy renderer, but defer presentation until
 * the single SDL_RenderPresent in main.
 */
static void tetris_deferred_present(SDL_Renderer *renderer) {
    (void)renderer;
}

#define tetris_present_with_score_overlay tetris_present_with_score_overlay_legacy
#define SDL_RenderPresent tetris_deferred_present
#include "app_score_overlay.inc"
#undef SDL_RenderPresent
#undef tetris_present_with_score_overlay

#include "app_fidelity_overlay.inc"
