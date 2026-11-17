#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "sound.h"

SDL_AudioDeviceID audio_device = 0;
SDL_AudioSpec audio_spec = {0};

int initAudio(void) {
    audio_spec.format = SDL_AUDIO_F32;
    audio_spec.channels = 2;
    audio_spec.freq = 44100;

    audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec);
    if (!audio_device) {
        SDL_Log("Failed to open audio device: %s", SDL_GetError());
        return 1;
    }

    return 0;
}

int destroyAudio(void) {
    if (audio_device) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
    return 0;
}

Sound loadSound(const char* filePath) {
    Sound sound = {0};
    sound.filePath = strdup(filePath);
    sound.volume = 128;

    if (!SDL_LoadWAV(filePath, &sound.spec, &sound.buffer, &sound.length)) {
        SDL_Log("Failed to load WAV file '%s': %s", filePath, SDL_GetError());
        free(sound.filePath);
        sound.filePath = NULL;
        return sound;
    }

    return sound;
}

void playSound(Sound* sound) {
    if (!sound || !sound->buffer || !audio_device) {
        return;
    }

    SDL_AudioStream* voice_stream = SDL_CreateAudioStream(&sound->spec, &audio_spec);
    if (!voice_stream) {
        SDL_Log("Failed to create voice stream: %s", SDL_GetError());
        return;
    }

    if (!SDL_BindAudioStream(audio_device, voice_stream)) {
        SDL_Log("Failed to bind audio stream: %s", SDL_GetError());
        SDL_DestroyAudioStream(voice_stream);
        return;
    }

    SDL_PutAudioStreamData(voice_stream, sound->buffer, sound->length);

    SDL_FlushAudioStream(voice_stream);
}

void destroySound(Sound* sound) {
    if (sound) {
        if (sound->buffer) {
            SDL_free(sound->buffer);
            sound->buffer = NULL;
        }
        if (sound->filePath) {
            free(sound->filePath);
            sound->filePath = NULL;
        }
    }
}