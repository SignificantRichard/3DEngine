/**
 * @file asset_manager.c
 * @author Gao Wen Xin (gaowenxin2007@gmail.com)
 * @brief Manages GPU Texture assets
 * @version 0.2
 * @date 2026-11-16
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "asset_manager.h"
#include "image.h"

static SDL_GPUTexture* convertToGPUTexture(AssetManager* assetManager, Sprite* sprite) {
    if (!assetManager || !assetManager->device || !sprite || !sprite->data) {
        return NULL;
    }
    if (sprite->width <= 0 || sprite->height <= 0) {
        return NULL;
    }

    Uint32 width = (Uint32)sprite->width;
    Uint32 height = (Uint32)sprite->height;
    Uint32 byteSize = width * height * (Uint32)sizeof(uint32_t);

    SDL_GPUTextureCreateInfo texInfo = {0};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = width;
    texInfo.height = height;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(assetManager->device, &texInfo);
    if (!texture) {
        fprintf(stderr, "Failed to create GPU texture: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_GPUTransferBufferCreateInfo xferInfo = {0};
    xferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    xferInfo.size = byteSize;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(assetManager->device, &xferInfo);
    if (!transfer) {
        fprintf(stderr, "Failed to create texture transfer buffer: %s\n", SDL_GetError());
        SDL_ReleaseGPUTexture(assetManager->device, texture);
        return NULL;
    }

    void* mapped = SDL_MapGPUTransferBuffer(assetManager->device, transfer, false);
    if (!mapped) {
        fprintf(stderr, "Failed to map texture transfer buffer: %s\n", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(assetManager->device, transfer);
        SDL_ReleaseGPUTexture(assetManager->device, texture);
        return NULL;
    }

    memcpy(mapped, sprite->data, byteSize);
    SDL_UnmapGPUTransferBuffer(assetManager->device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(assetManager->device);
    if (!cmd) {
        fprintf(stderr, "Failed to acquire command buffer for texture upload: %s\n", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(assetManager->device, transfer);
        SDL_ReleaseGPUTexture(assetManager->device, texture);
        return NULL;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
    if (!copyPass) {
        fprintf(stderr, "Failed to begin copy pass for texture upload: %s\n", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(assetManager->device, transfer);
        SDL_ReleaseGPUTexture(assetManager->device, texture);
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
    SDL_ReleaseGPUTransferBuffer(assetManager->device, transfer);

    return texture;
}

int lookForImageAsset(AssetManager* assetManager, char* filePath) {
    if (!assetManager || !filePath) {
        return 0;
    }
    for (int i = 0; i < assetManager->textureCount; i++) {
        if (assetManager->textures[i].filePath &&
            strcmp(assetManager->textures[i].filePath, filePath) == 0) {
            return i;
        }
    }
    return 0; // loads the default texture if there is no asset
}

AssetManager createAssetManager(SDL_GPUDevice* device) {
    AssetManager a = {0};
    a.device = device;
    a.textures = (ImageAsset*)malloc(sizeof(ImageAsset));
    a.textureCount = 0;

    if (a.textures) {
        loadImageAsset(&a, "./source/res/images/default.lif"); // sets default texture in case of error
    }
    return a;
}

void addImageToAssetManager(AssetManager* assetManager, Sprite* sprite, char* filePath) {
    if (!assetManager || !sprite) return;

    SDL_GPUTexture* texture = convertToGPUTexture(assetManager, sprite);
    if (!texture) return;

    int nextCount = assetManager->textureCount + 1;
    ImageAsset* temp = (ImageAsset*)realloc(assetManager->textures, (size_t)nextCount * sizeof(ImageAsset));
    if (!temp) {
        fprintf(stderr, "Failed to reallocate texture assets\n");
        SDL_ReleaseGPUTexture(assetManager->device, texture);
        return;
    }

    assetManager->textures = temp;
    assetManager->textureCount = nextCount;
    assetManager->textures[nextCount - 1] = (ImageAsset){
        .filePath = filePath,
        .texture = texture
    };
}

void removeImageFromAssetManager(AssetManager* assetManager, int index) {
    if (!assetManager || index < 0 || index >= assetManager->textureCount) return;

    if (assetManager->device && assetManager->textures[index].texture) {
        SDL_ReleaseGPUTexture(assetManager->device, assetManager->textures[index].texture);
    }

    assetManager->textureCount--;
    if (index < assetManager->textureCount) {
        assetManager->textures[index] = assetManager->textures[assetManager->textureCount];
    }

    if (assetManager->textureCount > 0) {
        ImageAsset* temp = (ImageAsset*)realloc(assetManager->textures, (size_t)assetManager->textureCount * sizeof(ImageAsset));
        if (temp) {
            assetManager->textures = temp;
        }
    } else {
        free(assetManager->textures);
        assetManager->textures = NULL;
    }
}

int loadImageAsset(AssetManager* assetManager, char* filePath) {
    Sprite sprite = loadImageAsSprite(filePath);
    int w = sprite.width;
    addImageToAssetManager(assetManager, &sprite, filePath);
    destroySprite(&sprite);
    printf("Added %s to assets\n", filePath);
    return w;
}

void destroyAssetManager(AssetManager* assetManager) {
    if (!assetManager) return;

    if (assetManager->device && assetManager->textures) {
        for (int i = 0; i < assetManager->textureCount; i++) {
            if (assetManager->textures[i].texture) {
                SDL_ReleaseGPUTexture(assetManager->device, assetManager->textures[i].texture);
            }
        }
    }

    free(assetManager->textures);
    assetManager->textures = NULL;
    assetManager->textureCount = 0;
}