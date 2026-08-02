#ifndef TETRIS_LEVEL_SCORE_TABLE_FIX_H
#define TETRIS_LEVEL_SCORE_TABLE_FIX_H

#include "app.h"

void render_level_score_table_fix(
    SDL_Renderer *renderer, SDL_Texture *font, AppScreen screen,
    const AppMenuState *menu, const TetrisHighScores *scores);

#endif
