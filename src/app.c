#include "app_v05_00.inc"

static void tetris_present_with_score_overlay(
    SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
    const TetrisGame *game, const AppResultState *result,
    const TetrisHighScores *scores);

#include "app_v05_01.inc"
#include "app_v05_02.inc"
#include "app_v05_03.inc"
#include "app_v05_04.inc"
#include "app_v05_05.inc"
#include "app_score_overlay.inc"
