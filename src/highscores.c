#include "highscores.h"

#include <stdio.h>
#include <string.h>

static int mode_index(TetrisMode mode) {
    return mode == TETRIS_MODE_B ? 1 : 0;
}

static void copy_name(char destination[TETRIS_HIGH_SCORE_NAME_LENGTH + 1],
                      const char *name) {
    size_t length = 0;
    if (!name || !name[0]) name = "PLAYER";
    while (length < TETRIS_HIGH_SCORE_NAME_LENGTH && name[length]) {
        char c = name[length];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) c = '-';
        destination[length] = c;
        ++length;
    }
    while (length < TETRIS_HIGH_SCORE_NAME_LENGTH) destination[length++] = '-';
    destination[TETRIS_HIGH_SCORE_NAME_LENGTH] = '\0';
}

void tetris_high_scores_init(TetrisHighScores *scores) {
    memset(scores, 0, sizeof(*scores));
    for (int mode = 0; mode < 2; ++mode) {
        for (int rank = 0; rank < TETRIS_HIGH_SCORE_COUNT; ++rank) {
            copy_name(scores->entries[mode][rank].name, "------");
        }
    }
}

bool tetris_high_scores_submit(TetrisHighScores *scores, TetrisMode mode,
                               const char *name, int score,
                               int level, int height) {
    TetrisHighScoreEntry entry;
    const int index = mode_index(mode);
    int insert_at = TETRIS_HIGH_SCORE_COUNT;

    if (score < 0) score = 0;
    if (score > 999999) score = 999999;
    if (level < 0) level = 0;
    if (level > 99) level = 99;
    if (height < 0) height = 0;
    if (height > 5) height = 5;

    for (int rank = 0; rank < TETRIS_HIGH_SCORE_COUNT; ++rank) {
        if (score > scores->entries[index][rank].score) {
            insert_at = rank;
            break;
        }
    }
    if (insert_at == TETRIS_HIGH_SCORE_COUNT) return false;

    copy_name(entry.name, name);
    entry.score = score;
    entry.level = level;
    entry.height = height;
    for (int rank = TETRIS_HIGH_SCORE_COUNT - 1; rank > insert_at; --rank) {
        scores->entries[index][rank] = scores->entries[index][rank - 1];
    }
    scores->entries[index][insert_at] = entry;
    return true;
}

bool tetris_high_scores_save(const TetrisHighScores *scores, const char *path) {
    FILE *file;
    if (!path || !path[0]) return false;
    file = fopen(path, "w");
    if (!file) return false;
    if (fprintf(file, "TETRIS_PC_SCORES_V1\n") < 0) {
        fclose(file);
        return false;
    }
    for (int mode = 0; mode < 2; ++mode) {
        for (int rank = 0; rank < TETRIS_HIGH_SCORE_COUNT; ++rank) {
            const TetrisHighScoreEntry *entry = &scores->entries[mode][rank];
            if (fprintf(file, "%c %s %d %d %d\n",
                        mode == 0 ? 'A' : 'B', entry->name,
                        entry->score, entry->level, entry->height) < 0) {
                fclose(file);
                return false;
            }
        }
    }
    return fclose(file) == 0;
}

bool tetris_high_scores_load(TetrisHighScores *scores, const char *path) {
    FILE *file;
    char header[32];
    TetrisHighScores loaded;
    int counts[2] = {0, 0};
    tetris_high_scores_init(&loaded);
    if (!path || !path[0]) return false;
    file = fopen(path, "r");
    if (!file) return false;
    if (!fgets(header, sizeof(header), file) ||
        strncmp(header, "TETRIS_PC_SCORES_V1", 19) != 0) {
        fclose(file);
        return false;
    }

    for (int item = 0; item < 2 * TETRIS_HIGH_SCORE_COUNT; ++item) {
        char mode_character = 0;
        char name[TETRIS_HIGH_SCORE_NAME_LENGTH + 1] = {0};
        int score = 0;
        int level = 0;
        int height = 0;
        int mode;
        int rank;
        TetrisHighScoreEntry *entry;
        if (fscanf(file, " %c %6s %d %d %d",
                   &mode_character, name, &score, &level, &height) != 5) {
            fclose(file);
            return false;
        }
        if (mode_character != 'A' && mode_character != 'B') {
            fclose(file);
            return false;
        }
        mode = mode_character == 'B' ? 1 : 0;
        rank = counts[mode]++;
        if (rank >= TETRIS_HIGH_SCORE_COUNT || score < 0 || score > 999999 ||
            level < 0 || level > 99 || height < 0 || height > 5) {
            fclose(file);
            return false;
        }
        entry = &loaded.entries[mode][rank];
        copy_name(entry->name, name);
        entry->score = score;
        entry->level = level;
        entry->height = height;
        if (rank > 0 && loaded.entries[mode][rank - 1].score < score) {
            fclose(file);
            return false;
        }
    }
    if (counts[0] != TETRIS_HIGH_SCORE_COUNT ||
        counts[1] != TETRIS_HIGH_SCORE_COUNT) {
        fclose(file);
        return false;
    }
    fclose(file);
    *scores = loaded;
    return true;
}

const TetrisHighScoreEntry *tetris_high_scores_top(const TetrisHighScores *scores,
                                                   TetrisMode mode) {
    return &scores->entries[mode_index(mode)][0];
}
