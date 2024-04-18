#include "../include/utils.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * Plays a sound file in the WAV format.
 */
void play_sound(const char *sound_file) {
#ifdef _WIN32
        PlaySound(sound_file);
#else 
#ifdef __APPLE__
        const char *util = "afplay ";
        const uint8_t util_len = 7;
#else 
        const char *util = "aplay ";
        const uint8_t util_len = 6;
#endif
        size_t len = strlen(sound_file);
        char command[util_len + 1 + len];
        memcpy(command, util, util_len * sizeof(char));
        memcpy(command + util_len, sound_file, len);
        command[len] = '\0';
        system(command);
#endif
}
