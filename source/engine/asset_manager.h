#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "image.h"

typedef struct {
    char* filePath;
    SDL_Texture* texture;
} ImageAsset;

typedef struct {
    ImageAsset* textures;
    SDL_Renderer* renderer;
    int textureCount;
} AssetManager;

AssetManager createAssetManager(SDL_Renderer* renderer);
void addImageToAssetManager(AssetManager* assetManager, Sprite sprite, char* filePath);
void removeImageFromAssetManager(AssetManager* assetManager, int index);
int loadImageAsset(AssetManager* assetManager, char* filePath);
int lookForImageAsset(AssetManager* assetManager, char* filePath);

#endif