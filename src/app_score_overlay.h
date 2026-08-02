#ifndef TETRIS_APP_SCORE_OVERLAY_H
#define TETRIS_APP_SCORE_OVERLAY_H

#include "app.h"

void tetris_present_with_score_overlay(
    SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
    const TetrisGame *game, const AppResultState *result,
    const TetrisHighScores *scores);

#endif
