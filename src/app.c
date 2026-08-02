#include "app_v05_00.inc"
#include "app_v05_01.inc"
#include "app_v05_02.inc"
#include "app_v05_03.inc"
#include "app_score_overlay.inc"
#define SDL_RenderPresent(renderer) \
    tetris_present_with_score_overlay((renderer), font, screen, game, result, scores)
#include "app_v05_04.inc"
#undef SDL_RenderPresent
#include "app_v05_05.inc"
