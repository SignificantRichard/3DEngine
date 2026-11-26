#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    int x;
    int y;
} Vector2D;

typedef struct {
    float x;
    float y;
} Vector2F;

typedef struct {
    Vector2D position;
    Vector2D size;
    Vector2F offset;
    TTF_Font* font;
    char* text;
    SDL_Color color;
    SDL_GPUTexture* gpuTexture;
    bool dirty;
} Label;

TTF_Font* createFont(const char* fontPath, int fontSize);

void destroyFont(TTF_Font* font);

Label createLabel(TTF_Font* font, const char* text, int x, int y);

bool syncLabelTexture(Label* label, SDL_GPUDevice* device);

void destroyLabel(Label* label, SDL_GPUDevice* device);

TTF_Font* createFirstAvailableFont(float fontSize);

int setLabelText(Label* label, const char* newText);

#endif