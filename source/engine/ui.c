#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <math.h>
#include "ui.h"

TTF_TextEngine* createRendererEngine(SDL_Renderer* renderer) {
    return TTF_CreateRendererTextEngine(renderer);
}

void destroySurfaceEngine(TTF_TextEngine* engine) {
    if (engine) {
        TTF_DestroySurfaceTextEngine(engine);
    }
}

TTF_Font* createFont(const char* fontPath, int fontSize) {
    TTF_Font* font = TTF_OpenFont(fontPath, fontSize);
    if (!font) {
        SDL_Log("Failed to load font '%s': %s", fontPath, SDL_GetError());
        return NULL;
    }
    return font;
}

void destroyFont(TTF_Font* font) {
    if (font) {
        TTF_CloseFont(font);
    }
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

int renderLabel(const Label* label) {
    if (!label || !label->textbox) {
        return false;
    }

    float drawX = (float)label->position.x + label->offset.x;
    float drawY = (float)label->position.y + label->offset.y;

    if (!TTF_DrawRendererText(label->textbox, drawX, drawY)) {
        SDL_Log("Failed to draw text to renderer: %s", SDL_GetError());
        return false;
    }

    return true;
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

TTF_Font* createFirstAvailableFont(float fontSize) {
    #if defined(_WIN32)
        const char* candidates[] = {
            "C:\\Windows\\Fonts\\segoeui.ttf",
            "C:\\Windows\\Fonts\\arial.ttf",
            "C:\\Windows\\Fonts\\tahoma.ttf"
        };
    #elif defined(__APPLE__)
        const char* candidates[] = {
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "/Library/Fonts/Arial.ttf"
        };
    #else
        const char* candidates[] = {
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/liberation2/LiberationSans-Regular.ttf",
            "/usr/share/fonts/noto/NotoSans-Regular.ttf"
        };
    #endif

    size_t count = sizeof(candidates) / sizeof(candidates[0]);
    for (size_t i = 0; i < count; ++i) {
        TTF_Font* font = TTF_OpenFont(candidates[i], fontSize);
        if (font) {
            return font;
        }
    }

    SDL_Log("No fallback font found; last error: %s", SDL_GetError());
    return NULL;
}

int setLabelText(Label* label, const char* newText) {
    if (!label || !label->textbox || !newText) {
        return 0;
    }

    // Passing 0 for length reads until the null terminator '\0'
    if (!TTF_SetTextString(label->textbox, newText, 0)) {
        SDL_Log("Failed to update label text: %s", SDL_GetError());
        return 0;
    }

    TTF_GetTextSize(label->textbox, &label->size.x, &label->size.y);
    return 1;
}