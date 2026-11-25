#ifndef UI_H
#define UI_H

TTF_TextEngine* createTextEngine(SDL_Renderer* renderer);

TTF_Font* createFont(char* fontPath, int fontSize);

void destroyFont(TTF_Font* font);

Label createLabel(TTF_TextEngine* engine, TTF_Font* font, const char* text, int x, int y);

void destroyLabel(Label* label);

#endif