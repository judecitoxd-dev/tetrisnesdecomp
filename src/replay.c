#include "replay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char REPLAY_MAGIC[8] = {'T','T','R','P','L','Y','0','1'};

static void set_error(char *error, size_t error_size, const char *message) {
    if (error && error_size) snprintf(error, error_size, "%s", message);
}

static bool write_u16(FILE *file, uint16_t value) {
    unsigned char bytes[2] = {(unsigned char)value, (unsigned char)(value >> 8)};
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static bool write_u32(FILE *file, uint32_t value) {
    unsigned char bytes[4] = {
        (unsigned char)value, (unsigned char)(value >> 8),
        (unsigned char)(value >> 16), (unsigned char)(value >> 24)
    };
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static bool write_u64(FILE *file, uint64_t value) {
    unsigned char bytes[8];
    int index;
    for (index = 0; index < 8; ++index) bytes[index] = (unsigned char)(value >> (index * 8));
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static bool read_u16(FILE *file, uint16_t *value) {
    unsigned char bytes[2];
    if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) return false;
    *value = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    return true;
}

static bool read_u32(FILE *file, uint32_t *value) {
    unsigned char bytes[4];
    if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) return false;
    *value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
             ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
    return true;
}

static bool read_u64(FILE *file, uint64_t *value) {
    unsigned char bytes[8];
    uint64_t result = 0;
    int index;
    if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) return false;
    for (index = 0; index < 8; ++index) result |= (uint64_t)bytes[index] << (index * 8);
    *value = result;
    return true;
}

static uint16_t input_mask(const TetrisInput *input) {
    uint16_t mask = 0;
    if (!input) return 0;
    if (input->left) mask |= 1u << 0;
    if (input->right) mask |= 1u << 1;
    if (input->down) mask |= 1u << 2;
    if (input->rotate_cw_pressed) mask |= 1u << 3;
    if (input->rotate_ccw_pressed) mask |= 1u << 4;
    if (input->hard_drop_pressed) mask |= 1u << 5;
    if (input->pause_pressed) mask |= 1u << 6;
    if (input->restart_pressed) mask |= 1u << 7;
    if (input->toggle_next_pressed) mask |= 1u << 8;
    return mask;
}

static TetrisInput mask_input(uint16_t mask) {
    TetrisInput input;
    memset(&input, 0, sizeof(input));
    input.left = (mask & (1u << 0)) != 0;
    input.right = (mask & (1u << 1)) != 0;
    input.down = (mask & (1u << 2)) != 0;
    input.rotate_cw_pressed = (mask & (1u << 3)) != 0;
    input.rotate_ccw_pressed = (mask & (1u << 4)) != 0;
    input.hard_drop_pressed = (mask & (1u << 5)) != 0;
    input.pause_pressed = (mask & (1u << 6)) != 0;
    input.restart_pressed = (mask & (1u << 7)) != 0;
    input.toggle_next_pressed = (mask & (1u << 8)) != 0;
    return input;
}

void tetris_replay_init(TetrisReplay *replay) {
    if (!replay) return;
    memset(replay, 0, sizeof(*replay));
}

void tetris_replay_free(TetrisReplay *replay) {
    if (!replay) return;
    free(replay->frames);
    memset(replay, 0, sizeof(*replay));
}

bool tetris_replay_begin(TetrisReplay *replay, const TetrisGame *game) {
    if (!replay || !game) return false;
    replay->frame_count = 0;
    replay->final_hash = 0;
    replay->seed = game->initial_seed;
    replay->mode = game->mode;
    replay->start_level = game->start_level;
    replay->start_height = game->start_height;
    replay->initial_show_next = game->show_next;
    return true;
}

bool tetris_replay_append(TetrisReplay *replay, const TetrisInput *input) {
    uint32_t next_capacity;
    uint16_t *next_frames;
    if (!replay || replay->frame_count >= TETRIS_REPLAY_MAX_FRAMES) return false;
    if (replay->frame_count >= replay->capacity) {
        next_capacity = replay->capacity ? replay->capacity * 2u : 4096u;
        if (next_capacity > TETRIS_REPLAY_MAX_FRAMES) next_capacity = TETRIS_REPLAY_MAX_FRAMES;
        next_frames = (uint16_t *)realloc(replay->frames,
                                          (size_t)next_capacity * sizeof(uint16_t));
        if (!next_frames) return false;
        replay->frames = next_frames;
        replay->capacity = next_capacity;
    }
    replay->frames[replay->frame_count++] = input_mask(input);
    return true;
}

TetrisInput tetris_replay_input(const TetrisReplay *replay, uint32_t frame) {
    if (!replay || frame >= replay->frame_count) {
        TetrisInput empty;
        memset(&empty, 0, sizeof(empty));
        return empty;
    }
    return mask_input(replay->frames[frame]);
}

static void hash_byte(uint64_t *hash, unsigned value) {
    *hash ^= (uint64_t)(value & 0xffu);
    *hash *= UINT64_C(1099511628211);
}

static void hash_u32(uint64_t *hash, uint32_t value) {
    int shift;
    for (shift = 0; shift < 32; shift += 8) hash_byte(hash, value >> shift);
}

static void hash_i32(uint64_t *hash, int value) {
    hash_u32(hash, (uint32_t)value);
}

uint64_t tetris_state_hash(const TetrisGame *game) {
    uint64_t hash = UINT64_C(1469598103934665603);
    int y;
    int x;
    int index;
    if (!game) return 0;
    for (y = 0; y < TETRIS_BOARD_H; ++y)
        for (x = 0; x < TETRIS_BOARD_W; ++x) hash_byte(&hash, game->board[y][x]);
    hash_i32(&hash, game->active);
    hash_i32(&hash, game->next);
    hash_i32(&hash, game->rotation);
    hash_i32(&hash, game->x);
    hash_i32(&hash, game->y);
    hash_u32(&hash, game->initial_seed);
    hash_i32(&hash, game->mode);
    hash_i32(&hash, game->start_height);
    hash_i32(&hash, game->score);
    hash_i32(&hash, game->lines);
    hash_i32(&hash, game->total_lines);
    hash_i32(&hash, game->level);
    hash_i32(&hash, game->start_level);
    hash_i32(&hash, game->transition_lines);
    for (index = 0; index < TETRIS_PIECE_COUNT; ++index)
        hash_i32(&hash, game->piece_count[index]);
    hash_i32(&hash, game->frame);
    hash_i32(&hash, game->fall_counter);
    hash_i32(&hash, game->das_counter);
    hash_i32(&hash, game->das_direction);
    hash_i32(&hash, game->soft_drop_counter);
    hash_i32(&hash, game->soft_drop_points);
    hash_u32(&hash, game->rng_seed);
    hash_byte(&hash, game->spawn_count);
    hash_i32(&hash, game->previous_piece);
    hash_i32(&hash, game->phase);
    hash_i32(&hash, game->phase_timer);
    for (index = 0; index < TETRIS_MAX_CLEAR_ROWS; ++index)
        hash_i32(&hash, game->clear_rows[index]);
    hash_i32(&hash, game->clear_count);
    hash_i32(&hash, game->clear_step);
    hash_i32(&hash, game->lock_bottom_row);
    hash_i32(&hash, game->curtain_rows);
    hash_byte(&hash, game->show_next ? 1u : 0u);
    hash_byte(&hash, game->paused ? 1u : 0u);
    hash_byte(&hash, game->game_over ? 1u : 0u);
    hash_byte(&hash, game->completed ? 1u : 0u);
    return hash;
}

void tetris_replay_finish(TetrisReplay *replay, const TetrisGame *game) {
    if (!replay || !game) return;
    replay->final_hash = tetris_state_hash(game);
}

bool tetris_replay_save(const TetrisReplay *replay, const char *path) {
    FILE *file;
    char temporary[1400];
    uint32_t index;
    bool ok = true;
    if (!replay || !path || !*path || replay->frame_count > TETRIS_REPLAY_MAX_FRAMES)
        return false;
    if (snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= (int)sizeof(temporary))
        return false;
    file = fopen(temporary, "wb");
    if (!file) return false;
    ok = fwrite(REPLAY_MAGIC, 1, sizeof(REPLAY_MAGIC), file) == sizeof(REPLAY_MAGIC);
    ok = ok && write_u32(file, 1u);
    ok = ok && write_u32(file, replay->seed);
    ok = ok && write_u32(file, replay->mode == TETRIS_MODE_B ? 1u : 0u);
    ok = ok && write_u32(file, (uint32_t)replay->start_level);
    ok = ok && write_u32(file, (uint32_t)replay->start_height);
    ok = ok && write_u32(file, replay->initial_show_next ? 1u : 0u);
    ok = ok && write_u32(file, replay->frame_count);
    ok = ok && write_u64(file, replay->final_hash);
    for (index = 0; ok && index < replay->frame_count; ++index)
        ok = write_u16(file, replay->frames[index]);
    if (fclose(file) != 0) ok = false;
    if (!ok) {
        remove(temporary);
        return false;
    }
    if (rename(temporary, path) != 0) {
        remove(path);
        if (rename(temporary, path) != 0) {
            remove(temporary);
            return false;
        }
    }
    return true;
}

bool tetris_replay_load(TetrisReplay *replay, const char *path,
                        char *error, size_t error_size) {
    FILE *file;
    unsigned char magic[8];
    uint32_t version;
    uint32_t seed;
    uint32_t mode;
    uint32_t level;
    uint32_t height;
    uint32_t show_next;
    uint32_t count;
    uint64_t final_hash;
    uint16_t *frames = NULL;
    uint32_t index;
    if (!replay || !path) return false;
    file = fopen(path, "rb");
    if (!file) {
        set_error(error, error_size, "Could not open replay file.");
        return false;
    }
    if (fread(magic, 1, sizeof(magic), file) != sizeof(magic) ||
        memcmp(magic, REPLAY_MAGIC, sizeof(magic)) != 0) {
        fclose(file);
        set_error(error, error_size, "Invalid replay signature.");
        return false;
    }
    if (!read_u32(file, &version) || version != 1u ||
        !read_u32(file, &seed) || !read_u32(file, &mode) ||
        !read_u32(file, &level) || !read_u32(file, &height) ||
        !read_u32(file, &show_next) || !read_u32(file, &count) ||
        !read_u64(file, &final_hash)) {
        fclose(file);
        set_error(error, error_size, "Incomplete or unsupported replay header.");
        return false;
    }
    if (mode > 1u || level > 19u || height > 5u || show_next > 1u ||
        count == 0u || count > TETRIS_REPLAY_MAX_FRAMES) {
        fclose(file);
        set_error(error, error_size, "Replay values are outside supported limits.");
        return false;
    }
    if (count > 0) {
        frames = (uint16_t *)malloc((size_t)count * sizeof(uint16_t));
        if (!frames) {
            fclose(file);
            set_error(error, error_size, "Out of memory while loading replay.");
            return false;
        }
        for (index = 0; index < count; ++index) {
            if (!read_u16(file, &frames[index]) || (frames[index] & ~0x01ffu) != 0u) {
                free(frames);
                fclose(file);
                set_error(error, error_size, "Replay input stream is corrupt.");
                return false;
            }
        }
    }
    if (fgetc(file) != EOF) {
        free(frames);
        fclose(file);
        set_error(error, error_size, "Replay has unexpected trailing data.");
        return false;
    }
    fclose(file);
    free(replay->frames);
    replay->frames = frames;
    replay->seed = seed;
    replay->capacity = count;
    replay->frame_count = count;
    replay->mode = mode == 1u ? TETRIS_MODE_B : TETRIS_MODE_A;
    replay->start_level = (int)level;
    replay->start_height = (int)height;
    replay->initial_show_next = show_next != 0u;
    replay->final_hash = final_hash;
    set_error(error, error_size, "");
    return true;
}
