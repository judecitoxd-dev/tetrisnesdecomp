#include "songs.h"

#include <SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#endif

#ifdef TETRIS_HAVE_SDL_MIXER
#include <SDL_mixer.h>
#endif

#define SONG_PATH_LENGTH 1023

typedef struct TetrisSongEntry {
    char filename[256];
    char normal_path[SONG_PATH_LENGTH + 1];
    char fast_path[SONG_PATH_LENGTH + 1];
    char label[TETRIS_CUSTOM_SONG_LABEL_LENGTH + 1];
} TetrisSongEntry;

typedef struct TetrisSongsState {
    TetrisSongEntry entries[TETRIS_CUSTOM_SONG_MAX];
    int count;
    int active_index;
    bool active;
    bool owns_mixer;
    bool using_existing_mixer;
    bool playing_fast;
    const NesRom *rom;
    char directory[SONG_PATH_LENGTH + 1];
#ifdef TETRIS_HAVE_SDL_MIXER
    Mix_Music *normal_music;
    Mix_Music *fast_music;
#endif
} TetrisSongsState;

static TetrisSongsState g_songs;

static int text_compare(const char *left, const char *right) {
#ifdef _WIN32
    return _stricmp(left, right);
#else
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;
    while (*a && *b) {
        const int ca = tolower(*a);
        const int cb = tolower(*b);
        if (ca != cb) return ca - cb;
        ++a;
        ++b;
    }
    return (int)*a - (int)*b;
#endif
}

static bool text_ends_with(const char *text, const char *suffix) {
    const size_t text_length = text ? strlen(text) : 0u;
    const size_t suffix_length = suffix ? strlen(suffix) : 0u;
    if (suffix_length > text_length) return false;
    return text_compare(text + text_length - suffix_length, suffix) == 0;
}

static bool file_exists(const char *path) {
    struct stat info;
    return path && stat(path, &info) == 0 && (info.st_mode & S_IFREG) != 0;
}

static void join_path(char *destination, size_t destination_size,
                      const char *directory, const char *filename) {
    const size_t length = directory ? strlen(directory) : 0u;
    const bool separator = length > 0u &&
        (directory[length - 1u] == '/' || directory[length - 1u] == '\\');
    snprintf(destination, destination_size, "%s%s%s",
             directory ? directory : "", separator ? "" : "/",
             filename ? filename : "");
}

static bool ensure_directory(const char *path) {
    struct stat info;
    if (!path || !*path) return false;
    if (stat(path, &info) == 0) return (info.st_mode & S_IFDIR) != 0;
#ifdef _WIN32
    return CreateDirectoryA(path, NULL) != 0 ||
           GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#endif
}

static void make_song_label(const char *filename, char *label,
                            size_t label_size) {
    char stem[256];
    size_t source = 0u;
    size_t output = 0u;
    size_t length;
    bool previous_space = true;

    snprintf(stem, sizeof(stem), "%s", filename ? filename : "SONG");
    length = strlen(stem);
    if (length >= 4u && text_ends_with(stem, ".ogg"))
        stem[length - 4u] = '\0';

    while (stem[source] &&
           (isdigit((unsigned char)stem[source]) || stem[source] == ' ' ||
            stem[source] == '-' || stem[source] == '_')) {
        ++source;
    }

    while (stem[source] && output + 1u < label_size) {
        unsigned char character = (unsigned char)stem[source++];
        if (character == '_' || character == '.' || character == '-')
            character = ' ';
        character = (unsigned char)toupper(character);
        if ((character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9')) {
            label[output++] = (char)character;
            previous_space = false;
        } else if (!previous_space && output + 1u < label_size) {
            label[output++] = ' ';
            previous_space = true;
        }
    }
    while (output > 0u && label[output - 1u] == ' ') --output;
    if (output == 0u) {
        snprintf(label, label_size, "SONG");
        return;
    }
    label[output] = '\0';
}

static int compare_entries(const void *left, const void *right) {
    const TetrisSongEntry *a = (const TetrisSongEntry *)left;
    const TetrisSongEntry *b = (const TetrisSongEntry *)right;
    return text_compare(a->filename, b->filename);
}

static void add_song_file(const char *filename) {
    TetrisSongEntry *entry;
    char stem[256];
    size_t length;
    if (!filename || g_songs.count >= TETRIS_CUSTOM_SONG_MAX) return;
    if (!text_ends_with(filename, ".ogg") ||
        text_ends_with(filename, ".fast.ogg")) return;

    entry = &g_songs.entries[g_songs.count];
    memset(entry, 0, sizeof(*entry));
    snprintf(entry->filename, sizeof(entry->filename), "%s", filename);
    join_path(entry->normal_path, sizeof(entry->normal_path),
              g_songs.directory, filename);
    if (!file_exists(entry->normal_path)) return;

    snprintf(stem, sizeof(stem), "%s", filename);
    length = strlen(stem);
    if (length >= 4u) stem[length - 4u] = '\0';
    if (snprintf(entry->fast_path, sizeof(entry->fast_path),
                 "%s/%s.fast.ogg", g_songs.directory, stem) >=
        (int)sizeof(entry->fast_path) || !file_exists(entry->fast_path)) {
        entry->fast_path[0] = '\0';
    }
    make_song_label(filename, entry->label, sizeof(entry->label));
    ++g_songs.count;
}

static void scan_directory(void) {
#ifdef _WIN32
    WIN32_FIND_DATAA data;
    HANDLE handle;
    char pattern[SONG_PATH_LENGTH + 8];
    snprintf(pattern, sizeof(pattern), "%s\\*.ogg", g_songs.directory);
    handle = FindFirstFileA(pattern, &data);
    if (handle != INVALID_HANDLE_VALUE) {
        do {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                add_song_file(data.cFileName);
        } while (FindNextFileA(handle, &data));
        FindClose(handle);
    }
#else
    DIR *directory = opendir(g_songs.directory);
    if (directory) {
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL)
            add_song_file(entry->d_name);
        closedir(directory);
    }
#endif
    qsort(g_songs.entries, (size_t)g_songs.count,
          sizeof(g_songs.entries[0]), compare_entries);
}

static void locate_songs_directory(void) {
    char *base = SDL_GetBasePath();
    if (base) {
        snprintf(g_songs.directory, sizeof(g_songs.directory),
                 "%ssongs", base);
        SDL_free(base);
    } else {
        snprintf(g_songs.directory, sizeof(g_songs.directory), "songs");
    }
}

#ifdef TETRIS_HAVE_SDL_MIXER
static void free_current_music(void) {
    Mix_HaltMusic();
    if (g_songs.normal_music) Mix_FreeMusic(g_songs.normal_music);
    if (g_songs.fast_music) Mix_FreeMusic(g_songs.fast_music);
    g_songs.normal_music = NULL;
    g_songs.fast_music = NULL;
    g_songs.active_index = -1;
    g_songs.playing_fast = false;
}

static bool open_private_mixer(void) {
    if ((Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG) == 0) {
        fprintf(stderr, "Custom songs: OGG decoder unavailable: %s\n",
                Mix_GetError());
        Mix_Quit();
        return false;
    }
    if (Mix_OpenAudio(TETRIS_ROM_AUDIO_SAMPLE_RATE,
                      AUDIO_S16SYS, 2, 1024) != 0) {
        fprintf(stderr, "Custom songs: could not open mixer: %s\n",
                Mix_GetError());
        Mix_Quit();
        return false;
    }
    Mix_AllocateChannels(16);
    g_songs.owns_mixer = true;
    return true;
}

static bool prepare_custom_backend(TetrisAudio *audio) {
    if (g_songs.active) return true;
    if (audio && audio->pack_active) {
        g_songs.using_existing_mixer = true;
        return true;
    }
    if (audio) tetris_audio_shutdown(audio);
    if (!open_private_mixer()) {
        if (audio && tetris_audio_init(audio) && g_songs.rom) {
            char error[256];
            (void)tetris_audio_attach_rom(audio, g_songs.rom,
                                          error, sizeof(error));
        }
        return false;
    }
    if (audio) {
        memset(audio, 0, sizeof(*audio));
        audio->music_track = -1;
        audio->rom_selected_track = -2;
        audio->pack_selected_track = -2;
    }
    return true;
}

static bool load_custom_index(int index) {
    const TetrisSongEntry *entry;
    if (index < 0 || index >= g_songs.count) return false;
    free_current_music();
    entry = &g_songs.entries[index];
    g_songs.normal_music = Mix_LoadMUS(entry->normal_path);
    if (!g_songs.normal_music) {
        fprintf(stderr, "Custom songs: could not load %s: %s\n",
                entry->normal_path, Mix_GetError());
        return false;
    }
    if (entry->fast_path[0]) {
        g_songs.fast_music = Mix_LoadMUS(entry->fast_path);
        if (!g_songs.fast_music) {
            fprintf(stderr, "Custom songs: ignoring invalid fast variant %s: %s\n",
                    entry->fast_path, Mix_GetError());
        }
    }
    g_songs.active_index = index;
    return true;
}

static void play_current(bool enabled, bool allegro) {
    Mix_Music *music;
    const bool use_fast = allegro && g_songs.fast_music != NULL;
    if (!enabled) {
        Mix_HaltMusic();
        return;
    }
    music = use_fast ? g_songs.fast_music : g_songs.normal_music;
    if (!music) return;
    if (Mix_PlayingMusic() && g_songs.playing_fast == use_fast) return;
    Mix_HaltMusic();
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
    if (Mix_PlayMusic(music, -1) != 0) {
        fprintf(stderr, "Custom songs: playback failed: %s\n", Mix_GetError());
        return;
    }
    g_songs.playing_fast = use_fast;
}

static bool restore_original_backend(TetrisAudio *audio,
                                     bool enabled, int selection) {
    if (!g_songs.active) {
        if (audio) tetris_audio_apply_settings(audio, enabled, selection);
        return true;
    }
    free_current_music();
    if (g_songs.owns_mixer) {
        Mix_CloseAudio();
        Mix_Quit();
        g_songs.owns_mixer = false;
        if (!audio || !tetris_audio_init(audio)) return false;
        if (g_songs.rom) {
            char error[256];
            if (!tetris_audio_attach_rom(audio, g_songs.rom,
                                         error, sizeof(error))) {
                fprintf(stderr, "Custom songs: ROM audio restore failed: %s\n",
                        error);
            }
        }
    }
    g_songs.using_existing_mixer = false;
    g_songs.active = false;
    if (audio) tetris_audio_apply_settings(audio, enabled, selection);
    return true;
}
#endif

bool tetris_songs_init(TetrisAudio *audio, const NesRom *rom) {
    (void)audio;
    memset(&g_songs, 0, sizeof(g_songs));
    g_songs.active_index = -1;
    g_songs.rom = rom;
    locate_songs_directory();
    if (!ensure_directory(g_songs.directory)) {
        fprintf(stderr, "Custom songs: could not create %s\n",
                g_songs.directory);
        return false;
    }
#ifdef TETRIS_HAVE_SDL_MIXER
    scan_directory();
    fprintf(stdout, "Custom songs: %d found in %s\n",
            g_songs.count, g_songs.directory);
#else
    fprintf(stdout, "Custom songs disabled: build has no SDL2_mixer support.\n");
#endif
    return true;
}

void tetris_songs_set_rom(const NesRom *rom) {
    g_songs.rom = rom;
}

void tetris_songs_shutdown(TetrisAudio *audio) {
#ifdef TETRIS_HAVE_SDL_MIXER
    if (g_songs.active) {
        free_current_music();
        if (g_songs.owns_mixer) {
            Mix_CloseAudio();
            Mix_Quit();
            g_songs.owns_mixer = false;
            if (audio) memset(audio, 0, sizeof(*audio));
        }
    }
#else
    (void)audio;
#endif
    memset(&g_songs, 0, sizeof(g_songs));
    g_songs.active_index = -1;
}

int tetris_songs_count(void) {
    return g_songs.count;
}

int tetris_songs_total_music_count(void) {
    return 3 + g_songs.count;
}

int tetris_songs_step(int current, int direction) {
    const int last = tetris_songs_total_music_count() - 1;
    if (last < 2) return -1;
    if (current < -1 || current > last) current = 0;
    if (direction > 0) {
        if (current < 0) return -1;
        if (current < last) return current + 1;
        return -1;
    }
    if (direction < 0) {
        if (current < 0) return last;
        if (current > 0) return current - 1;
        return 0;
    }
    return current;
}

int tetris_songs_apply(TetrisAudio *audio, bool enabled, int selection) {
    const int total = tetris_songs_total_music_count();
    if (selection < -1 || selection >= total) selection = 0;
#ifdef TETRIS_HAVE_SDL_MIXER
    if (selection >= 3) {
        const int custom_index = selection - 3;
        if (!prepare_custom_backend(audio) ||
            (g_songs.active_index != custom_index &&
             !load_custom_index(custom_index))) {
            (void)restore_original_backend(audio, enabled, 0);
            if (audio) audio->music_track = 0;
            return 0;
        }
        g_songs.active = true;
        if (audio) {
            audio->enabled = enabled;
            audio->music_track = selection;
        }
        play_current(enabled, audio ? audio->rom_allegro : false);
        return selection;
    }
    if (!restore_original_backend(audio, enabled, selection)) {
        if (audio) {
            audio->enabled = false;
            audio->music_track = -1;
        }
        return -1;
    }
#else
    if (selection >= 3) selection = 0;
    if (audio) tetris_audio_apply_settings(audio, enabled, selection);
#endif
    if (audio) audio->music_track = selection;
    return selection;
}

void tetris_songs_update(TetrisAudio *audio) {
#ifdef TETRIS_HAVE_SDL_MIXER
    if (!g_songs.active || !audio) return;
    play_current(audio->enabled, audio->rom_allegro);
#else
    (void)audio;
#endif
}

const char *tetris_songs_label(int selection) {
    static const char *original[3] = {"MUSIC-1", "MUSIC-2", "MUSIC-3"};
    if (selection < 0) return "OFF";
    if (selection < 3) return original[selection];
    selection -= 3;
    if (selection >= 0 && selection < g_songs.count)
        return g_songs.entries[selection].label;
    return "MUSIC-1";
}

int tetris_songs_menu_window_start(int selection) {
    const int total = tetris_songs_total_music_count();
    int start;
    if (total <= 3) return 0;
    if (selection < 0) return total - 3;
    start = selection - 1;
    if (start < 0) start = 0;
    if (start > total - 3) start = total - 3;
    return start;
}

int tetris_songs_menu_row(int selection) {
    int row;
    if (selection < 0) return 3;
    row = selection - tetris_songs_menu_window_start(selection);
    if (row < 0) row = 0;
    if (row > 2) row = 2;
    return row;
}

const char *tetris_songs_directory(void) {
    return g_songs.directory;
}
