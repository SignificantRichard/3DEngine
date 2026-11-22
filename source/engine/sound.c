#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <SDL3/SDL.h>
#include "sound.h"

#define M_PI 3.14159265358979323846

SDL_AudioDeviceID audio_device = 0;
SDL_AudioSpec audio_spec = {0};
static SDL_AudioStream* master_stream = NULL;

static void compute_gains(float lx, float ly, float fx, float fy,
                          float sx, float sy, float max_dist,
                          float* out_left, float* out_right) {
    float dx = sx - lx;
    float dy = sy - ly;
    float dist = hypotf(dx, dy);

    if (max_dist <= 1e-5f) {
        *out_left = 0.0f;
        *out_right = 0.0f;
        return;
    }

    // Distance attenuation (1.0 at source -> 0.0 at >= max_dist)
    float atten = 1.0f - (dist / max_dist);
    if (atten < 0.0f) atten = 0.0f;
    atten = atten * atten;

    if (dist < 1e-5f) {
        *out_left  = atten * 0.70710678f;
        *out_right = atten * 0.70710678f;
        return;
    }

    float inv_dist = 1.0f / dist;
    float dir_x = dx * inv_dist;
    float dir_y = dy * inv_dist;

    // Right vector in 2D from forward (fx, fy)
    float right_x = -fy;
    float right_y =  fx;

    // pan in [-1, +1]: -1 = left, 0 = center, +1 = right
    float pan = dir_x * right_x + dir_y * right_y;
    if (pan < -1.0f) pan = -1.0f;
    if (pan >  1.0f) pan =  1.0f;

    float p = (pan + 1.0f) * 0.5f;
    float angle = p * ((float)M_PI * 0.5f);
    *out_left  = atten * cosf(angle);
    *out_right = atten * sinf(angle);
}

int initAudio(void) {
    audio_spec.format = SDL_AUDIO_F32;
    audio_spec.channels = 2;
    audio_spec.freq = 44100;

    audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec);
    if (!audio_device) {
        SDL_Log("Failed to open audio device: %s", SDL_GetError());
        return 1;
    }

    master_stream = SDL_CreateAudioStream(&audio_spec, &audio_spec);
    if (!master_stream) {
        SDL_Log("Failed to create master stream: %s", SDL_GetError());
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
        return 1;
    }

    if (!SDL_BindAudioStream(audio_device, master_stream)) {
        SDL_Log("Failed to bind master stream: %s", SDL_GetError());
        SDL_DestroyAudioStream(master_stream);
        master_stream = NULL;
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
        return 1;
    }

    SDL_ResumeAudioDevice(audio_device);
    return 0;
}

int destroyAudio(void) {
    if (master_stream) {
        SDL_DestroyAudioStream(master_stream);
        master_stream = NULL;
    }
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

void playSoundDirectional(Sound* sound,
                          float lx, float ly, float fx, float fy,
                          float sx, float sy, float max_dist) {
    if (!sound || !sound->buffer || !audio_device || !master_stream) {
        return;
    }

    SDL_AudioStream* conv_stream = SDL_CreateAudioStream(&sound->spec, &audio_spec);
    if (!conv_stream) {
        SDL_Log("Failed to create conversion stream: %s", SDL_GetError());
        return;
    }

    SDL_PutAudioStreamData(conv_stream, sound->buffer, (int)sound->length);
    SDL_FlushAudioStream(conv_stream);

    int available_bytes = SDL_GetAudioStreamAvailable(conv_stream);
    if (available_bytes <= 0) {
        SDL_DestroyAudioStream(conv_stream);
        return;
    }

    float* samples = (float*)malloc((size_t)available_bytes);
    if (!samples) {
        SDL_DestroyAudioStream(conv_stream);
        return;
    }

    int got_bytes = SDL_GetAudioStreamData(conv_stream, samples, available_bytes);
    SDL_DestroyAudioStream(conv_stream);

    if (got_bytes <= 0) {
        free(samples);
        return;
    }

    float left_gain = 1.0f;
    float right_gain = 1.0f;
    compute_gains(lx, ly, fx, fy, sx, sy, max_dist, &left_gain, &right_gain);

    float vol_scale = (float)sound->volume / 128.0f;
    left_gain *= vol_scale;
    right_gain *= vol_scale;

    // audio_spec is 2-channel SDL_AUDIO_F32 -> 8 bytes per stereo frame
    int num_frames = got_bytes / (int)(sizeof(float) * 2);
    for (int i = 0; i < num_frames; i++) {
        samples[i * 2 + 0] *= left_gain;
        samples[i * 2 + 1] *= right_gain;
    }

    SDL_PutAudioStreamData(master_stream, samples, num_frames * (int)(sizeof(float) * 2));
    free(samples);
}

void playSound(Sound* sound) {
    // Centered at listener position with full gain
    playSoundDirectional(sound, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
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