#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "image.h"
#include "files.h"

static inline int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static inline uint8_t clampColor(float val) {
    if (val < 0.0f) return 0;
    if (val > 255.0f) return 255;
    return (uint8_t)val;
}

static float cubicWeight(float x) {
    x = fabsf(x);
    if (x <= 1.0f) {
        return 1.5f * x * x * x - 2.5f * x * x + 1.0f;
    } else if (x < 2.0f) {
        return -0.5f * x * x * x + 2.5f * x * x - 4.0f * x + 2.0f;
    }
    return 0.0f;
}


Sprite constructSprite(int width, int height) {
    Sprite sprite;
    sprite.width = width;
    sprite.height = height;
    sprite.size = width * height;
    sprite.brightness = 1.0f;
    
    sprite.data = (uint32_t*)malloc(sprite.size * sizeof(uint32_t));

    for (int i = 0; i < sprite.size; i++) {
        sprite.data[i] = makeColor(255, 255, 255, 255);
    }

    return sprite;
}

uint32_t getPixel(Sprite* sprite, int x, int y) {
    return sprite->data[y * sprite->width + x];
}


void setPixel(Sprite* sprite, int x, int y, uint32_t color) {
    sprite->data[y * sprite->width + x] = color;
}

uint32_t makeColor(int r, int g, int b, int a) {
    uint32_t color = 0;
    
    r <<= 24;
    g <<= 16;
    b <<= 8;
    r |= g;
    r |= b;
    r |= a;

    color |= r;
    return color;
}

int getColorFragment(uint32_t color, int place) {
    // 0a 1g 2b 3a
    color >>= place * 8;
    // mask
    color &= 0x000000ff;
    return (int)color;
}

Sprite loadImageAsSprite(char* path) {
    BinaryFileContents imageData = readBinFromFile(path);
    if (imageData.content == NULL || imageData.size == 0) {
        fprintf(stderr, "Failed to read image file: %s\n", path);
        return constructSprite(0, 0);
    }

    uint16_t headerLength = (imageData.content[0] << 8) + imageData.content[1];
    uint8_t widthLength = imageData.content[2];
    uint8_t heightLength = imageData.content[3];

    uint16_t width = 0;
    uint16_t height = 0;

    int idx = 4;
    for (int i = 0; i < widthLength; i++) {
        width = (width << 8) + imageData.content[idx++];
    }
    for (int i = 0; i < heightLength; i++) {
        height = (height << 8) + imageData.content[idx++];
    }
    
    // Read alpha flag (0 means alpha is enabled in your encoder script)
    uint8_t alphaEnabled = imageData.content[idx++];

    Sprite sprite = constructSprite(width, height);

    // Each palette entry is 4 bytes (RGBA) if alpha is enabled, or 3 bytes (RGB) if disabled
    int bytesPerPaletteEntry = (alphaEnabled == 0) ? 4 : 3;
    uint8_t* palette = &imageData.content[idx];

    // Compute color table offset robustly using total file size and dimensions
    int colorTableOffset = imageData.size - (width * height);

    for (int i = 0; i < width * height; i++) {
        if (colorTableOffset + i >= imageData.size) break;

        uint8_t paletteIndex = imageData.content[colorTableOffset + i];
        int r = palette[paletteIndex * bytesPerPaletteEntry + 0];
        int g = palette[paletteIndex * bytesPerPaletteEntry + 1];
        int b = palette[paletteIndex * bytesPerPaletteEntry + 2];
        int a = (bytesPerPaletteEntry == 4) ? palette[paletteIndex * bytesPerPaletteEntry + 3] : 255;

        uint32_t color = makeColor(r, g, b, a);
        int x = i % width;
        int y = i / width;
        setPixel(&sprite, x, y, color);
    }

    // Free the raw file content buffer to prevent memory leaks
    free(imageData.content);

    return sprite;
}

Sprite resample(Sprite original, int w, int h, int method) {
    Sprite new = constructSprite(w, h);
    
    // Scaling ratios from destination grid to source grid
    float scaleX = (float)original.width / w;
    float scaleY = (float)original.height / h;

    switch (method) {
        case NEAREST_NEIGHBOR: {
            for (int y = 0; y < h; y++) {
                int srcY = clamp((int)(y * scaleY), 0, original.height - 1);
                for (int x = 0; x < w; x++) {
                    int srcX = clamp((int)(x * scaleX), 0, original.width - 1);
                    setPixel(&new, x, y, getPixel(&original, srcX, srcY));
                }
            }
            break;
        }

        case REPEAT: {
            for (int y = 0; y < h; y++) {
                int srcY = (int)(y * scaleY) % original.height;
                if (srcY < 0) srcY += original.height;

                for (int x = 0; x < w; x++) {
                    int srcX = (int)(x * scaleX) % original.width;
                    if (srcX < 0) srcX += original.width;

                    setPixel(&new, x, y, getPixel(&original, srcX, srcY));
                }
            }
            break;
        }

        case BILINEAR: {
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    float u = x * scaleX;
                    float v = y * scaleY;

                    int x0 = (int)floorf(u);
                    int y0 = (int)floorf(v);
                    int x1 = x0 + 1;
                    int y1 = y0 + 1;

                    float fx = u - x0;
                    float fy = v - y0;

                    // Clamp sample coordinates within boundary
                    x0 = clamp(x0, 0, original.width - 1);
                    x1 = clamp(x1, 0, original.width - 1);
                    y0 = clamp(y0, 0, original.height - 1);
                    y1 = clamp(y1, 0, original.height - 1);

                    // Fetch 2x2 neighborhood
                    uint32_t c00 = getPixel(&original, x0, y0);
                    uint32_t c10 = getPixel(&original, x1, y0);
                    uint32_t c01 = getPixel(&original, x0, y1);
                    uint32_t c11 = getPixel(&original, x1, y1);

                    // Interpolate each RGBA channel separately
                    float r = (1 - fx) * (1 - fy) * getColorFragment(c00, 3) +
                              fx * (1 - fy)       * getColorFragment(c10, 3) +
                              (1 - fx) * fy       * getColorFragment(c01, 3) +
                              fx * fy             * getColorFragment(c11, 3);

                    float g = (1 - fx) * (1 - fy) * getColorFragment(c00, 2) +
                              fx * (1 - fy)       * getColorFragment(c10, 2) +
                              (1 - fx) * fy       * getColorFragment(c01, 2) +
                              fx * fy             * getColorFragment(c11, 2);

                    float b = (1 - fx) * (1 - fy) * getColorFragment(c00, 1) +
                              fx * (1 - fy)       * getColorFragment(c10, 1) +
                              (1 - fx) * fy       * getColorFragment(c01, 1) +
                              fx * fy             * getColorFragment(c11, 1);

                    float a = (1 - fx) * (1 - fy) * getColorFragment(c00, 0) +
                              fx * (1 - fy)       * getColorFragment(c10, 0) +
                              (1 - fx) * fy       * getColorFragment(c01, 0) +
                              fx * fy             * getColorFragment(c11, 0);

                    setPixel(&new, x, y, makeColor(clampColor(r), clampColor(g), clampColor(b), clampColor(a)));
                }
            }
            break;
        }

        case BICUBIC: {
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    float u = x * scaleX;
                    float v = y * scaleY;

                    int srcX = (int)floorf(u);
                    int srcY = (int)floorf(v);

                    float fx = u - srcX;
                    float fy = v - srcY;

                    float rSum = 0.0f, gSum = 0.0f, bSum = 0.0f, aSum = 0.0f;
                    float totalWeight = 0.0f;

                    // 4x4 grid interpolation loop (-1 to +2)
                    for (int m = -1; m <= 2; m++) {
                        float wy = cubicWeight(m - fy);
                        int py = clamp(srcY + m, 0, original.height - 1);

                        for (int n = -1; n <= 2; n++) {
                            float wx = cubicWeight(n - fx);
                            int px = clamp(srcX + n, 0, original.width - 1);

                            float weight = wx * wy;
                            uint32_t sample = getPixel(&original, px, py);

                            rSum += getColorFragment(sample, 3) * weight;
                            gSum += getColorFragment(sample, 2) * weight;
                            bSum += getColorFragment(sample, 1) * weight;
                            aSum += getColorFragment(sample, 0) * weight;
                            totalWeight += weight;
                        }
                    }

                    // Normalize weights to maintain proper brightness/luminance
                    if (totalWeight > 0.0f) {
                        rSum /= totalWeight;
                        gSum /= totalWeight;
                        bSum /= totalWeight;
                        aSum /= totalWeight;
                    }

                    setPixel(&new, x, y, makeColor(clampColor(rSum), clampColor(gSum), clampColor(bSum), clampColor(aSum)));
                }
            }
            break;
        }

        default:
            printf("Invalid resample method used\n");
            break;
    }

    return new;
}

void destroySprite(Sprite* sprite) {
    free(sprite->data);
}