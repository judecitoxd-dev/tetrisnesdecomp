#include "app.h"
#include "demo.h"
#include "replay.h"
#include "rom_picker.h"
#include "settings.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEMO_IDLE_MS 12000u
#define DEMO_MAX_MS 45000u
#define DEMO_ROM_MAX_MS 90000u
#define REPLAY_RESULT_MS 2500u
#define OPTION_COUNT 7

static void make_pref_file_path(const char *name, char *path, size_t path_size) {
    char *preference_path = SDL_GetPrefPath("YlPorts", "TetrisNESPC");
    if (preference_path) {
        snprintf(path, path_size, "%s%s", preference_path, name);
        SDL_free(preference_path);
    } else {
        snprintf(path, path_size, "%s", name);
    }
}

static bool try_audio_pack(TetrisAudio *audio, const char *directory,
                           bool report_failure) {
    char error[512];
    if (!directory || !*directory) return false;
    if (tetris_audio_load_ogg_pack(audio, directory, error, sizeof(error))) {
        fprintf(stdout, "Loaded OGG audio pack: %s\n", directory);
        return true;
    }
    if (report_failure) {
        fprintf(stderr, "Could not load OGG audio pack %s: %s\n",
                directory, error);
    }
    return false;
}

static void load_optional_audio_pack(TetrisAudio *audio,
                                     const char *command_line_path) {
    char preference_candidate[1024];
    char base_candidate[2048];
    char *base_path;
    const char *environment_path = getenv("TETRIS_AUDIO_PACK");

    if (command_line_path && *command_line_path) {
        (void)try_audio_pack(audio, command_line_path, true);
        return;
    }
    if (environment_path && *environment_path &&
        try_audio_pack(audio, environment_path, true)) return;

    make_pref_file_path("audio", preference_candidate,
                        sizeof(preference_candidate));
    if (try_audio_pack(audio, preference_candidate, false)) return;

    base_path = SDL_GetBasePath();
    if (base_path) {
        snprintf(base_candidate, sizeof(base_candidate), "%saudio", base_path);
        SDL_free(base_path);
        if (try_audio_pack(audio, base_candidate, false)) return;
    }
    (void)try_audio_pack(audio, "audio", false);
}

static void attach_original_audio(TetrisAudio *audio, const NesRom *rom) {
    char error[256];
    if (!audio || !audio->device || !rom) return;
    tetris_audio_detach_rom(audio);
    if (!tetris_audio_attach_rom(audio, rom, error, sizeof(error))) {
        fprintf(stderr, "Original ROM APU unavailable: %s\n", error);
    }
}

static void save_settings_quiet(const TetrisSettings *settings,
                                const char *settings_path) {
    if (!tetris_settings_save(settings, settings_path)) {
        fprintf(stderr, "Could not save settings to %s\n", settings_path);
    }
}

static bool load_candidate(SDL_Renderer *renderer, const char *path,
                           NesRom *rom, SDL_Texture **font,
                           TetrisSettings *settings, const char *settings_path,
                           bool show_error) {
    char error[256];
    if (!path || !*path) return false;
    if (!load_rom_and_font_ex(renderer, path, rom, font, show_error,
                              error, sizeof(error))) {
        if (!show_error) fprintf(stderr, "ROM candidate failed: %s (%s)\n", path, error);
        return false;
    }
    tetris_settings_set_rom_path(settings, path);
    save_settings_quiet(settings, settings_path);
    return true;
}

static bool load_initial_rom(SDL_Renderer *renderer, const char *cli_path,
                             NesRom *rom, SDL_Texture **font,
                             TetrisSettings *settings,
                             const char *settings_path) {
    char base_candidate[2048];
    char selected[2048];
    char *base_path;
    if (cli_path && *cli_path) {
        return load_candidate(renderer, cli_path, rom, font, settings,
                              settings_path, true);
    }
    if (settings->rom_path[0] &&
        load_candidate(renderer, settings->rom_path, rom, font, settings,
                       settings_path, false)) return true;
    base_path = SDL_GetBasePath();
    if (base_path) {
        snprintf(base_candidate, sizeof(base_candidate),
                 "%sTetris (USA).nes", base_path);
        SDL_free(base_path);
        if (load_candidate(renderer, base_candidate, rom, font, settings,
                           settings_path, false)) return true;
    }
    if (load_candidate(renderer, "Tetris (USA).nes", rom, font, settings,
                       settings_path, false)) return true;
    if (tetris_pick_rom(selected, sizeof(selected))) {
        return load_candidate(renderer, selected, rom, font, settings,
                              settings_path, true);
    }
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR, "Tetris NES PC Port",
        "No compatible ROM was found. Place Tetris (USA).nes beside the executable, pass it on the command line, or choose it from the file picker.",
        NULL);
    return false;
}

static bool choose_and_load_rom(SDL_Renderer *renderer, NesRom *rom,
                                SDL_Texture **font, TetrisSettings *settings,
                                const char *settings_path) {
    char selected[2048];
    if (!tetris_pick_rom(selected, sizeof(selected))) return false;
    return load_candidate(renderer, selected, rom, font, settings,
                          settings_path, true);
}

static void finish_replay_recording(TetrisReplay *recording,
                                    const TetrisGame *game,
                                    const char *replay_path,
                                    bool *recording_active) {
    if (!*recording_active || recording->frame_count == 0) return;
    tetris_replay_finish(recording, game);
    if (!tetris_replay_save(recording, replay_path)) {
        fprintf(stderr, "Could not save replay to %s\n", replay_path);
    }
    *recording_active = false;
}

static void begin_replay_recording(TetrisReplay *recording,
                                   const TetrisGame *game,
                                   bool *recording_active) {
    if (!tetris_replay_begin(recording, game)) {
        fprintf(stderr, "Could not initialize replay recording.\n");
        *recording_active = false;
        return;
    }
    *recording_active = true;
}

static void start_selected_game(TetrisGame *game, const AppMenuState *menu,
                                AppScreen *screen, bool *left, bool *right,
                                bool *down, bool *result_handled,
                                TetrisSettings *settings,
                                const char *settings_path,
                                TetrisReplay *recording,
                                bool *recording_active) {
