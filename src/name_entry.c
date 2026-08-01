#include "name_entry.h"

#include <string.h>

static const char NAME_CHARACTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-";

static int character_index(char character) {
    const char *found;
    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    found = strchr(NAME_CHARACTERS, character);
    return found ? (int)(found - NAME_CHARACTERS) : -1;
}

void tetris_name_entry_init(TetrisNameEntry *entry, const char *initial) {
    int index;
    const size_t initial_length = initial ? strlen(initial) : 0u;
    if (!entry) return;
    for (index = 0; index < TETRIS_NAME_ENTRY_LENGTH; ++index) {
        char character = (size_t)index < initial_length ? initial[index] : '-';
        if (character_index(character) < 0) character = '-';
        if (character >= 'a' && character <= 'z') {
            character = (char)(character - 'a' + 'A');
        }
        entry->text[index] = character;
    }
    entry->text[TETRIS_NAME_ENTRY_LENGTH] = '\0';
    entry->cursor = 0;
}

void tetris_name_entry_move(TetrisNameEntry *entry, int delta) {
    if (!entry || delta == 0) return;
    entry->cursor += delta;
    while (entry->cursor < 0) entry->cursor += TETRIS_NAME_ENTRY_LENGTH;
    while (entry->cursor >= TETRIS_NAME_ENTRY_LENGTH) {
        entry->cursor -= TETRIS_NAME_ENTRY_LENGTH;
    }
}

void tetris_name_entry_cycle(TetrisNameEntry *entry, int delta) {
    const int count = (int)sizeof(NAME_CHARACTERS) - 1;
    int index;
    if (!entry || delta == 0) return;
    index = character_index(entry->text[entry->cursor]);
    if (index < 0) index = count - 1;
    index += delta;
    while (index < 0) index += count;
    while (index >= count) index -= count;
    entry->text[entry->cursor] = NAME_CHARACTERS[index];
}

void tetris_name_entry_type(TetrisNameEntry *entry, char character) {
    int index;
    if (!entry) return;
    index = character_index(character);
    if (index < 0) return;
    entry->text[entry->cursor] = NAME_CHARACTERS[index];
    if (entry->cursor < TETRIS_NAME_ENTRY_LENGTH - 1) ++entry->cursor;
}

void tetris_name_entry_backspace(TetrisNameEntry *entry) {
    if (!entry) return;
    entry->text[entry->cursor] = '-';
    if (entry->cursor > 0) --entry->cursor;
}
