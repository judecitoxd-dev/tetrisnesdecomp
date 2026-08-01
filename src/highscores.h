#ifndef TETRIS_HIGHSCORES_H
#define TETRIS_HIGHSCORES_H

#include "game.h"

#include <stdbool.h>
#include <stddef.h>

#define TETRIS_HIGH_SCORE_COUNT 3
#define TETRIS_HIGH_SCORE_NAME_LENGTH 6

typedef struct TetrisHighScoreEntry {
    char name[TETRIS_HIGH_SCORE_NAME_LENGTH + 1];
    int score;
    int level;
    int height;
} TetrisHighScoreEntry;

typedef struct TetrisHighScores {
    TetrisHighScoreEntry entries[2][TETRIS_HIGH_SCORE_COUNT];
} TetrisHighScores;

void tetris_high_scores_init(TetrisHighScores *scores);
bool tetris_high_scores_load(TetrisHighScores *scores, const char *path);
bool tetris_high_scores_save(const TetrisHighScores *scores, const char *path);
int tetris_high_scores_rank(const TetrisHighScores *scores, TetrisMode mode,
                            int score);
bool tetris_high_scores_submit(TetrisHighScores *scores, TetrisMode mode,
                               const char *name, int score,
                               int level, int height);
const TetrisHighScoreEntry *tetris_high_scores_top(const TetrisHighScores *scores,
                                                   TetrisMode mode);

#endif
