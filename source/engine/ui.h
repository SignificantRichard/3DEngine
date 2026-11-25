#ifndef UI_H
#define UI_H

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
    TTF_Text* textbox;
} Label;

TTF_TextEngine* createRendererEngine(SDL_Renderer* renderer);

TTF_Font* createFont(const char* fontPath, int fontSize);

void destroyFont(TTF_Font* font);

Label createLabel(TTF_TextEngine* engine, TTF_Font* font, const char* text, int x, int y);

int renderLabel(const Label* label);

void destroyLabel(Label* label);

TTF_Font* createFirstAvailableFont(float fontSize);

#endif