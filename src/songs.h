#ifndef TETRIS_SONGS_H
#define TETRIS_SONGS_H

#include "audio.h"
#include "rom.h"

#include <stdbool.h>

#define TETRIS_CUSTOM_SONG_MAX 64
#define TETRIS_CUSTOM_SONG_LABEL_LENGTH 31

/*
 * Custom OGG files live in a `songs` directory beside the executable.
 * Selection values keep the original menu mapping:
 *   -1 = OFF, 0..2 = the three original ROM songs, 3+ = custom songs.
 */
bool tetris_songs_init(TetrisAudio *audio, const NesRom *rom);
void tetris_songs_set_rom(const NesRom *rom);
void tetris_songs_shutdown(TetrisAudio *audio);

int tetris_songs_count(void);
int tetris_songs_total_music_count(void);
int tetris_songs_step(int current, int direction);
int tetris_songs_apply(TetrisAudio *audio, bool enabled, int selection);
void tetris_songs_update(TetrisAudio *audio);

const char *tetris_songs_label(int selection);
int tetris_songs_menu_window_start(int selection);
int tetris_songs_menu_row(int selection);
const char *tetris_songs_directory(void);

#endif
