#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <SDL3/SDL.h>
#include "image.h"

typedef struct {
    char* filePath;
    SDL_GPUTexture* texture;
} ImageAsset;

typedef struct {
    ImageAsset* textures;
    SDL_GPUDevice* device;
    int textureCount;
} AssetManager;

AssetManager createAssetManager(SDL_GPUDevice* device);
void addImageToAssetManager(AssetManager* assetManager, Sprite* sprite, char* filePath);
void removeImageFromAssetManager(AssetManager* assetManager, int index);
int loadImageAsset(AssetManager* assetManager, char* filePath);
int lookForImageAsset(AssetManager* assetManager, char* filePath);
void destroyAssetManager(AssetManager* assetManager);

#endif