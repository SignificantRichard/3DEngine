#ifndef UI_RENDER_H
#define UI_RENDER_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include "render.h"
#include "ui.h"

typedef struct {
    float x;
    float y;
    float u;
    float v;
} UiVertex;

typedef struct {
    SDL_GPUGraphicsPipeline* pipeline;
    SDL_GPUSampler* sampler;
    SDL_GPUBuffer* quadBuffer;
    float screenWidth;
    float screenHeight;
} UiRenderer;

bool initUiRenderer(UiRenderer* ui, Renderer* gpuRenderer,
    const char* vertSpvPath, const char* fragSpvPath,
    float screenWidth, float screenHeight);

void renderLabelGPU(UiRenderer* ui, Renderer* gpuRenderer, const Label* label);

void destroyUiRenderer(UiRenderer* ui, Renderer* gpuRenderer);

#endif