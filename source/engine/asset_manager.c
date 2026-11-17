/**
 * @file asset_manager.c
 * @author Gao Wen Xin (gaowenxin2007@gmail.com)
 * @brief Manages Texture assets
 * @version 0.1
 * @date 2026-11-16
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "asset_manager.h"
#include "image.h"

SDL_Texture* convertToTexture(AssetManager* assetManager, Sprite sprite) {
    int width = sprite.width;
    int height = sprite.height;

    SDL_Texture* texture = SDL_CreateTexture(
        assetManager->renderer,
        SDL_PIXELFORMAT_ABGR32,
        SDL_TEXTUREACCESS_STATIC,
        width,
        height
    );

    int pitch = width * sizeof(uint32_t);

    SDL_UpdateTexture(texture, NULL, (const void *)sprite.data, pitch);

    return texture;
}

AssetManager createAssetManager(SDL_Renderer* renderer) {
    AssetManager a;
    a.renderer = renderer;
    a.textures = (ImageAsset*)malloc(sizeof(ImageAsset));
    a.textureSize = 0;
    return a;
}

void addImageToAssetManager(AssetManager* assetManager, Sprite sprite, char* filePath) {
    SDL_Texture* texture = convertToTexture(assetManager, sprite);
    ImageAsset asset;
    asset.texture = texture;
    asset.filePath = filePath;
    ImageAsset* temp = (ImageAsset*)realloc(assetManager->textures, ++assetManager->textureSize * sizeof(ImageAsset));
    assetManager->textures = temp;
    assetManager->textures[assetManager->textureSize - 1] = asset;
}

void removeImageFromAssetManager(AssetManager* assetManager, int index) {
    assetManager->textures[index] = assetManager->textures[--assetManager->textureSize];
    ImageAsset* temp = (ImageAsset*)realloc(assetManager->textures, assetManager->textureSize * sizeof(ImageAsset));
    assetManager->textures = temp;
}

void loadImageAsset(AssetManager* assetManager, char* filePath) {
    Sprite sprite = loadImageAsSprite(filePath);
    addImageToAssetManager(assetManager, sprite, filePath);
}