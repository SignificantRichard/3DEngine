#ifndef SOUND_H
#define SOUND_H

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char* filePath;
    Uint8* buffer;
    Uint32 length;
    SDL_AudioSpec spec;
    uint8_t volume;
} Sound;

extern SDL_AudioDeviceID audio_device;
extern SDL_AudioSpec audio_spec;

int initAudio(void);
int destroyAudio(void);
Sound loadSound(const char* filePath);
void playSound(Sound* sound);
void playSoundDirectional(Sound* sound,
    float lx, float ly, float fx, float fy,
    float sx, float sy, float max_dist);
void destroySound(Sound* sound);

#endif