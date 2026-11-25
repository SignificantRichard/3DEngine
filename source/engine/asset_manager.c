/**
 * @file asset_manager.c
 * @author Gao Wen Xin (gaowenxin2007@gmail.com)
 * @brief Manages Texture assets
 * @version 0.1
 * @date 2026-11-16
 * 
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "asset_manager.h"
#include "image.h"

SDL_Texture* convertToTexture(AssetManager* assetManager, Sprite* sprite) {
    int width = sprite->width;
    int height = sprite->height;

    SDL_Texture* texture = SDL_CreateTexture(
        assetManager->renderer,
        SDL_PIXELFORMAT_ABGR32,
        SDL_TEXTUREACCESS_STATIC,
        width,
        height
    );

    int pitch = width * sizeof(uint32_t);

    SDL_UpdateTexture(texture, NULL, (const void *)sprite->data, pitch);

    return texture;
}

int lookForImageAsset(AssetManager* assetManager, char* filePath) {
    for (int i = 0; i < assetManager->textureCount; i++) {
        if (assetManager->textures[i].filePath &&
            strcmp(assetManager->textures[i].filePath, filePath) == 0) {
            return i;
        }
    }
    return 0; // loads the default texture if there is no asset
}

AssetManager createAssetManager(SDL_Renderer* renderer) {
    AssetManager a;
    a.renderer = renderer;
    a.textures = (ImageAsset*)malloc(sizeof(ImageAsset));
    a.textureCount = 0;

    loadImageAsset(&a, "./source/res/images/default.lif"); // sets default texture in case of error
    return a;
}

void addImageToAssetManager(AssetManager* assetManager, Sprite* sprite, char* filePath) {
    SDL_Texture* texture = convertToTexture(assetManager, sprite);
    ImageAsset asset;
    asset.texture = texture;
    asset.filePath = filePath;
    ImageAsset* temp = (ImageAsset*)realloc(assetManager->textures, ++assetManager->textureCount * sizeof(ImageAsset));
    assetManager->textures = temp;
    assetManager->textures[assetManager->textureCount - 1] = asset;
}

void removeImageFromAssetManager(AssetManager* assetManager, int index) {
    assetManager->textures[index] = assetManager->textures[--assetManager->textureCount];
    ImageAsset* temp = (ImageAsset*)realloc(assetManager->textures, assetManager->textureCount * sizeof(ImageAsset));
    assetManager->textures = temp;
}

int loadImageAsset(AssetManager* assetManager, char* filePath) {
    Sprite sprite = loadImageAsSprite(filePath);
    addImageToAssetManager(assetManager, &sprite, filePath);
    destroySprite(&sprite);
    printf("Added %s to assets\n", filePath);
    return sprite.width;
}

void destroyAssetManager(AssetManager* assetManager) {
    free(assetManager->textures);
}