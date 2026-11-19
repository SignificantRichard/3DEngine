#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "vector.h"
#include "instances.h"
#include "mesh.h"
#include "asset_manager.h"
#include "world.h"

bool getVectorCameraSpace(Vector* v, Vector modelPos, Vector modelRot, struct Camera camera, Vector* outCamSpace);

bool vectorToScreenPos(Vector* v, Vector modelPos, Vector modelRot, struct Camera camera, float* x, float* y);

int compareFaces(const void* a, const void* b);

static void computeTriplanarUVs(
    const Vector* a,
    const Vector* b,
    const Vector* c,
    float scale,
    SDL_FPoint* uvA,
    SDL_FPoint* uvB,
    SDL_FPoint* uvC
);

void renderModel(SDL_Renderer* renderer, Mesh* model, struct Camera camera, AssetManager* assetManager);

void swapDrawOrder(int* a, int* b);

int sortOrderPartition(World* world, Vector position, int low, int high);

void sortDrawOrder(World* world, Vector position, int low, int high);