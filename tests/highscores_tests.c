#include "highscores.h"

#include <stdio.h>
#include <string.h>

static int test_submission_and_order(void) {
    TetrisHighScores scores;
    const TetrisHighScoreEntry *entry;
    tetris_high_scores_init(&scores);

    if (!tetris_high_scores_submit(&scores, TETRIS_MODE_A,
                                   "FIRST", 25000, 42, 1, 0)) return 1;
    if (!tetris_high_scores_submit(&scores, TETRIS_MODE_A,
                                   "SECOND", 40000, 77, 3, 0)) return 1;
    if (!tetris_high_scores_submit(&scores, TETRIS_MODE_A,
                                   "THIRD", 10000, 12, 0, 0)) return 1;

    entry = &scores.entries[0][0];
    if (strcmp(entry->name, "SECOND") != 0 ||
        entry->score != 40000 || entry->lines != 77 || entry->level != 3)
        return 1;
    entry = &scores.entries[0][1];
    if (strcmp(entry->name, "FIRST-") != 0 ||
        entry->score != 25000 || entry->lines != 42 || entry->level != 1)
        return 1;
    entry = &scores.entries[0][2];
    if (strcmp(entry->name, "THIRD-") != 0 ||
        entry->score != 10000 || entry->lines != 12) return 1;
    return 0;
}

static int test_v2_round_trip(void) {
    static const char path[] = "highscores-v2-test.txt";
    TetrisHighScores source;
    TetrisHighScores loaded;
    int result = 1;
    tetris_high_scores_init(&source);
    if (!tetris_high_scores_submit(&source, TETRIS_MODE_B,
                                   "PLAYER", 123456, 25, 19, 5)) goto done;
    if (!tetris_high_scores_save(&source, path)) goto done;
    if (!tetris_high_scores_load(&loaded, path)) goto done;
    if (loaded.entries[1][0].score != 123456 ||
        loaded.entries[1][0].lines != 25 ||
        loaded.entries[1][0].level != 19 ||
        loaded.entries[1][0].height != 5 ||
        strcmp(loaded.entries[1][0].name, "PLAYER") != 0) goto done;
    result = 0;
done:
    (void)remove(path);
    return result;
}

static int test_v1_migration(void) {
    static const char path[] = "highscores-v1-test.txt";
    static const char *rows[] = {
        "A LEGACY 9000 4 0\n",
        "A ------ 0 0 0\n",
        "A ------ 0 0 0\n",
        "B OLD--- 8000 9 5\n",
        "B ------ 0 0 0\n",
        "B ------ 0 0 0\n"
    };
    TetrisHighScores loaded;
    FILE *file = fopen(path, "w");
    int result = 1;
    int index;
    if (!file) return 1;
    fputs("TETRIS_PC_SCORES_V1\n", file);
    for (index = 0; index < 6; ++index) fputs(rows[index], file);
    if (fclose(file) != 0) goto done;
    if (!tetris_high_scores_load(&loaded, path)) goto done;
    if (loaded.entries[0][0].score != 9000 ||
        loaded.entries[0][0].lines != 0 ||
        loaded.entries[0][0].level != 4) goto done;
    if (loaded.entries[1][0].score != 8000 ||
        loaded.entries[1][0].lines != 0 ||
        loaded.entries[1][0].height != 5) goto done;
    result = 0;
done:
    (void)remove(path);
    return result;
}

int main(void) {
    if (test_submission_and_order() != 0) {
        fputs("high-score ordering or line count failed\n", stderr);
        return 1;
    }
    if (test_v2_round_trip() != 0) {
        fputs("high-score V2 round trip failed\n", stderr);
        return 1;
    }
    if (test_v1_migration() != 0) {
        fputs("high-score V1 migration failed\n", stderr);
        return 1;
    }
    puts("High-score score/lines/persistence tests passed.");
    return 0;
}
