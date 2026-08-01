#ifndef TETRIS_NAME_ENTRY_H
#define TETRIS_NAME_ENTRY_H

#define TETRIS_NAME_ENTRY_LENGTH 6

typedef struct TetrisNameEntry {
    char text[TETRIS_NAME_ENTRY_LENGTH + 1];
    int cursor;
} TetrisNameEntry;

void tetris_name_entry_init(TetrisNameEntry *entry, const char *initial);
void tetris_name_entry_move(TetrisNameEntry *entry, int delta);
void tetris_name_entry_cycle(TetrisNameEntry *entry, int delta);
void tetris_name_entry_type(TetrisNameEntry *entry, char character);
void tetris_name_entry_backspace(TetrisNameEntry *entry);

#endif
