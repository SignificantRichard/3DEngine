#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "files.h"

typedef struct {
    uint32_t* data;
    int width;
    int height;
    int size;
    float brightness;
} Sprite;

enum INTERPOLATION_METHOD {
    NEAREST_NEIGHBOR,
    BILINEAR,
    BICUBIC,
    REPEAT
};

Sprite constructSprite(int width, int height);
uint32_t getPixel(Sprite* sprite, int x, int y);
void setPixel(Sprite* sprite, int x, int y, uint32_t color);
uint32_t makeColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a);
int getColorFragment(uint32_t color, int place);
Sprite loadImageAsSprite(char* path);
Sprite resample(Sprite original, int w, int h, int method);
void destroySprite(Sprite* sprite);

#endif