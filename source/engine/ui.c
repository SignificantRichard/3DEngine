#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <math.h>

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

TTF_TextEngine* createTextEngine(SDL_Renderer* renderer) {
    return TTF_CreateRendererTextEngine(renderer);
}

TTF_Font* createFont(char* fontPath, int fontSize) {
    TTF_Font* font = TTF_OpenFont(fontPath, fontSize);
    if (!font) {
        SDL_Log("Failed to load font '%s': %s", fontPath, SDL_GetError());
        return label;
    }
}

void destroyFont(TTF_Font* font) {
    TTF_CloseFont(font);
}

Label createLabel(TTF_TextEngine* engine, TTF_Font* font, const char* text, int x, int y) {
    Label label = {0};
    label.position = (Vector2D){ x, y };
    label.offset   = (Vector2F){ 0.0f, 0.0f };
    label.textbox  = TTF_CreateText(engine, font, text, 0);

    if (label.textbox) {
        TTF_GetTextSize(label.textbox, &label.size.x, &label.size.y);
    }

    return label;
}

void destroyLabel(Label* label) {
    if (!label) {
        return;
    }

    if (label->textbox) {
        TTF_DestroyText(label->textbox);
        label->textbox = NULL;
    }

    label->size = (Vector2D){ 0, 0 };
}