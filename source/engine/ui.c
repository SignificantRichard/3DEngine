#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ui.h"

static char* dupString(const char* src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, src, len);
    }
    return copy;
}

static SDL_GPUTexture* uploadSurfaceToGPUTexture(SDL_GPUDevice* device, SDL_Surface* surface, int* outW, int* outH) {
    if (!device || !surface || surface->w <= 0 || surface->h <= 0) {
        return NULL;
    }

    SDL_Surface* rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
    if (!rgba) {
        SDL_Log("Failed to convert text surface to ABGR8888: %s", SDL_GetError());
        return NULL;
    }

    Uint32 width = (Uint32)rgba->w;
    Uint32 height = (Uint32)rgba->h;
    Uint32 rowBytes = width * 4u;
    Uint32 totalBytes = rowBytes * height;

    SDL_GPUTextureCreateInfo texInfo = {0};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = width;
    texInfo.height = height;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &texInfo);
    if (!texture) {
        SDL_Log("Failed to create UI GPU texture: %s", SDL_GetError());
        SDL_DestroySurface(rgba);
        return NULL;
    }

    SDL_GPUTransferBufferCreateInfo xferInfo = {0};
    xferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    xferInfo.size = totalBytes;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &xferInfo);
    if (!transfer) {
        SDL_Log("Failed to create UI transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTexture(device, texture);
        SDL_DestroySurface(rgba);
        return NULL;
    }

    uint8_t* mapped = (uint8_t*)SDL_MapGPUTransferBuffer(device, transfer, false);
    if (!mapped) {
        SDL_Log("Failed to map UI transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUTexture(device, texture);
        SDL_DestroySurface(rgba);
        return NULL;
    }

    const uint8_t* srcPixels = (const uint8_t*)rgba->pixels;
    for (Uint32 y = 0; y < height; y++) {
        memcpy(mapped + (y * rowBytes), srcPixels + (y * (Uint32)rgba->pitch), rowBytes);
    }

    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        SDL_Log("Failed to acquire command buffer for UI upload: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUTexture(device, texture);
        SDL_DestroySurface(rgba);
        return NULL;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
    if (!copyPass) {
        SDL_Log("Failed to begin copy pass for UI upload: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUTexture(device, texture);
        SDL_DestroySurface(rgba);
        return NULL;
    }

    SDL_GPUTextureTransferInfo src = {
        .transfer_buffer = transfer,
        .offset = 0,
        .pixels_per_row = width,
        .rows_per_layer = height
    };

    SDL_GPUTextureRegion dst = {
        .texture = texture,
        .mip_level = 0,
        .layer = 0,
        .x = 0,
        .y = 0,
        .z = 0,
        .w = width,
        .h = height,
        .d = 1
    };

    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    if (outW) *outW = rgba->w;
    if (outH) *outH = rgba->h;

    SDL_DestroySurface(rgba);
    return texture;
}

TTF_Font* createFont(const char* fontPath, int fontSize) {
    TTF_Font* font = TTF_OpenFont(fontPath, (float)fontSize);
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

Label createLabel(TTF_Font* font, const char* text, int x, int y) {
    Label label = {0};
    label.position = (Vector2D){ x, y };
    label.offset   = (Vector2F){ 0.0f, 0.0f };
    label.font     = font;
    label.text     = dupString(text ? text : "");
    label.color    = (SDL_Color){ 255, 255, 255, 255 };
    label.gpuTexture = NULL;
    label.dirty    = true;
    return label;
}

bool syncLabelTexture(Label* label, SDL_GPUDevice* device) {
    if (!label || !device) {
        return false;
    }
    if (!label->dirty && label->gpuTexture != NULL) {
        return true;
    }
    if (!label->font || !label->text || label->text[0] == '\0') {
        if (label->gpuTexture) {
            SDL_ReleaseGPUTexture(device, label->gpuTexture);
            label->gpuTexture = NULL;
        }
        label->size = (Vector2D){ 0, 0 };
        label->dirty = false;
        return true;
    }

    SDL_Surface* surf = TTF_RenderText_Blended(label->font, label->text, 0, label->color);
    if (!surf) {
        SDL_Log("Failed to render text surface: %s", SDL_GetError());
        return false;
    }

    int w = 0;
    int h = 0;
    SDL_GPUTexture* newTex = uploadSurfaceToGPUTexture(device, surf, &w, &h);
    SDL_DestroySurface(surf);

    if (!newTex) {
        return false;
    }

    if (label->gpuTexture) {
        SDL_ReleaseGPUTexture(device, label->gpuTexture);
    }

    label->gpuTexture = newTex;
    label->size = (Vector2D){ w, h };
    label->dirty = false;
    return true;
}

void destroyLabel(Label* label, SDL_GPUDevice* device) {
    if (!label) {
        return;
    }

    if (device && label->gpuTexture) {
        SDL_ReleaseGPUTexture(device, label->gpuTexture);
        label->gpuTexture = NULL;
    }

    free(label->text);
    label->text = NULL;
    label->size = (Vector2D){ 0, 0 };
    label->dirty = false;
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
    if (!label || !newText) {
        return 0;
    }

    if (label->text && strcmp(label->text, newText) == 0) {
        return 1;
    }

    char* copy = dupString(newText);
    if (!copy) {
        return 0;
    }

    free(label->text);
    label->text = copy;
    label->dirty = true;
    return 1;
}