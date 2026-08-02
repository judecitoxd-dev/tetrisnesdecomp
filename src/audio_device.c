#define TETRIS_AUDIO_DEVICE_IMPLEMENTATION 1
#include "audio.h"

#include <stdlib.h>

static int is_power_of_two(int value) {
    return value > 0 && (value & (value - 1)) == 0;
}

static int configured_buffer_samples(void) {
#ifdef __ANDROID__
    int samples = 4096;
#else
    int samples = 2048;
#endif
    const char *value = SDL_getenv("TETRIS_AUDIO_BUFFER_SAMPLES");
    if (value && *value) {
        char *end = NULL;
        const long parsed = strtol(value, &end, 10);
        if (end && *end == '\0' && parsed >= 256 && parsed <= 8192 &&
            is_power_of_two((int)parsed)) {
            samples = (int)parsed;
        }
    }
    return samples;
}

SDL_AudioDeviceID tetris_open_audio_device_buffered(
    const char *device, int iscapture, const SDL_AudioSpec *desired,
    SDL_AudioSpec *obtained, int allowed_changes) {
    SDL_AudioSpec tuned;
    if (!desired || iscapture) {
        return SDL_OpenAudioDevice(device, iscapture, desired, obtained,
                                   allowed_changes);
    }
    tuned = *desired;
    if ((int)tuned.samples < configured_buffer_samples())
        tuned.samples = (Uint16)configured_buffer_samples();
    return SDL_OpenAudioDevice(device, iscapture, &tuned, obtained,
                               allowed_changes);
}
