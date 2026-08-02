#include "audio_ring.h"

bool tetris_audio_ring_capacity_valid(size_t capacity) {
    return capacity >= 2u && (capacity & (capacity - 1u)) == 0u;
}

size_t tetris_audio_ring_used(size_t read_index, size_t write_index,
                              size_t capacity) {
    if (!tetris_audio_ring_capacity_valid(capacity)) return 0u;
    return (write_index - read_index) & (capacity - 1u);
}

size_t tetris_audio_ring_free(size_t read_index, size_t write_index,
                              size_t capacity) {
    if (!tetris_audio_ring_capacity_valid(capacity)) return 0u;
    return capacity - 1u -
           tetris_audio_ring_used(read_index, write_index, capacity);
}

size_t tetris_audio_ring_advance(size_t index, size_t count,
                                 size_t capacity) {
    if (!tetris_audio_ring_capacity_valid(capacity)) return 0u;
    return (index + count) & (capacity - 1u);
}
