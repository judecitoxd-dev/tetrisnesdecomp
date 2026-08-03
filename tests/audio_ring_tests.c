#include "audio_ring.h"

#include <assert.h>
#include <stdio.h>

static void test_capacity(void) {
    assert(!tetris_audio_ring_capacity_valid(0u));
    assert(!tetris_audio_ring_capacity_valid(3u));
    assert(tetris_audio_ring_capacity_valid(2u));
    assert(tetris_audio_ring_capacity_valid(32768u));
}

static void test_empty_and_full_geometry(void) {
    const size_t capacity = 8u;
    assert(tetris_audio_ring_used(0u, 0u, capacity) == 0u);
    assert(tetris_audio_ring_free(0u, 0u, capacity) == 7u);
    assert(tetris_audio_ring_used(2u, 5u, capacity) == 3u);
    assert(tetris_audio_ring_free(2u, 5u, capacity) == 4u);
}

static void test_wrap(void) {
    const size_t capacity = 8u;
    assert(tetris_audio_ring_advance(7u, 1u, capacity) == 0u);
    assert(tetris_audio_ring_advance(6u, 4u, capacity) == 2u);
    assert(tetris_audio_ring_used(6u, 2u, capacity) == 4u);
    assert(tetris_audio_ring_free(6u, 2u, capacity) == 3u);
}

int main(void) {
    test_capacity();
    test_empty_and_full_geometry();
    test_wrap();
    puts("audio ring tests: OK");
    return 0;
}
