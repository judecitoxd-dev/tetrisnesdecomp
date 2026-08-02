#ifndef TETRIS_AUDIO_RING_H
#define TETRIS_AUDIO_RING_H

#include <stdbool.h>
#include <stddef.h>

bool tetris_audio_ring_capacity_valid(size_t capacity);
size_t tetris_audio_ring_used(size_t read_index, size_t write_index,
                              size_t capacity);
size_t tetris_audio_ring_free(size_t read_index, size_t write_index,
                              size_t capacity);
size_t tetris_audio_ring_advance(size_t index, size_t count,
                                 size_t capacity);

#endif
