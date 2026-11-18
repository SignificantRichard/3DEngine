#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "image.h"
#include "asset_manager.h"
#include "mesh.h"
#include "world.h"
#include "../world/entry.h"

#define INTERNAL_WIDTH 800
#define INTERNAL_HEIGHT 600

struct Camera {
    Vector position;
    Vector rotation;
};

bool vectorToScreenPos(Vector* v, Vector modelPos, Vector modelRot, struct Camera camera, float* x, float* y) {
    Vector nv = *v;

    {
        float cosX = cosf(modelRot.x);
        float sinX = sinf(modelRot.x);
        float y1 = nv.y * cosX - nv.z * sinX;
        float z1 = nv.y * sinX + nv.z * cosX;
        nv.y = y1; nv.z = z1;

        float cosY = cosf(modelRot.y);
        float sinY = sinf(modelRot.y);
        float x1 = nv.x * cosY + nv.z * sinY;
        float z2 = -nv.x * sinY + nv.z * cosY;
        nv.x = x1; nv.z = z2;

        float cosZ = cosf(modelRot.z);
        float sinZ = sinf(modelRot.z);
        float x2 = nv.x * cosZ - nv.y * sinZ;
        float y2 = nv.x * sinZ + nv.y * cosZ;
        nv.x = x2; nv.y = y2;
    }

    nv.x *= 0.0001f;
    nv.y *= 0.0001f;
    nv.z *= 0.0001f;

    vectorAdd(&nv, &modelPos);

    vectorSub(&nv, &camera.position);

    {
        float cosZ = cosf(-camera.rotation.z);
        float sinZ = sinf(-camera.rotation.z);
        float x1 = nv.x * cosZ - nv.y * sinZ;
        float y1 = nv.x * sinZ + nv.y * cosZ;
        nv.x = x1; nv.y = y1;

        float cosY = cosf(-camera.rotation.y);
        float sinY = sinf(-camera.rotation.y);
        float x2 = nv.x * cosY + nv.z * sinY;
        float z1 = -nv.x * sinY + nv.z * cosY;
        nv.x = x2; nv.z = z1;

        float cosX = cosf(-camera.rotation.x);
        float sinX = sinf(-camera.rotation.x);
        float y2 = nv.y * cosX - nv.z * sinX;
        float z2 = nv.y * sinX + nv.z * cosX;
        nv.y = y2; nv.z = z2;
    }

    if (nv.z <= 0.0f) {
        return false;
    }

    float fovDegrees = 70.0f;
    float fovRadians = fovDegrees * (3.14159265f / 180.0f);
    float fovScale = (INTERNAL_HEIGHT / 2.0f) / tanf(fovRadians / 2.0f);
    
    float centerX = INTERNAL_WIDTH / 2.0f;
    float centerY = INTERNAL_HEIGHT / 2.0f;

    *x = (nv.x / nv.z) * fovScale + centerX;
    *y = (nv.y / nv.z) * fovScale + centerY;

    return true; // Successfully projected
}

void renderModel(SDL_Renderer* renderer, Mesh* model, struct Camera camera, AssetManager* assetManager) {
    if (!model || model->faceAmount <= 0) return;
    
    int totalVerts = model->faceAmount * 3;
    
    SDL_Vertex* vert = (SDL_Vertex*)malloc(totalVerts * sizeof(SDL_Vertex));
    int* indices = (int*)malloc(totalVerts * sizeof(int));
    
    if (!vert || !indices) {
        free(vert);
        free(indices);
        return;
    }

    SDL_Texture* texture = assetManager->textures[0].texture;
    SDL_FColor white = { 1.0f, 1.0f, 1.0f, 1.0f };

    int base = 0;
    for (int i = 0; i < model->faceAmount; i++) {
        float x, y;

        // Vertex A
        vectorToScreenPos(model->faces[i].a, model->position, model->rotation, camera, &x, &y);
        vert[base + 0] = (SDL_Vertex){ {x, y}, white, {0.0f, 0.0f} };

        // Vertex B
        vectorToScreenPos(model->faces[i].b, model->position, model->rotation, camera, &x, &y);
        vert[base + 1] = (SDL_Vertex){ {x, y}, white, {0.0f, 1.0f} };

        // Vertex C
        vectorToScreenPos(model->faces[i].c, model->position, model->rotation, camera, &x, &y);
        vert[base + 2] = (SDL_Vertex){ {x, y}, white, {1.0f, 1.0f} };

        indices[base + 0] = base + 0;
        indices[base + 1] = base + 1;
        indices[base + 2] = base + 2;

        base += 3;
    }

    SDL_RenderGeometry(renderer, texture, vert, totalVerts, indices, totalVerts);

    free(vert);
    free(indices);
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if (!SDL_CreateWindowAndRenderer("Engine", INTERNAL_WIDTH, INTERNAL_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    AssetManager assetManager = createAssetManager(renderer);
    loadImageAsset(&assetManager, "./source/res/images/default.lif");
    SDL_Texture* texture = assetManager.textures[0].texture;

    World world = createWorld();
    
    Mesh* cube;
    {
        Mesh c = loadMeshFromBin("./source/res/models/cube.mdl");
        cube = addMeshToWorld(&world, &c);
    }
    

    cube->position = (Vector){ 0.0f, 0.0f, 20.0f };

    struct Camera camera = { .position = blankVector(), .rotation = blankVector() };

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        cube->rotation.x += 0.0001f;
        cube->rotation.y += 0.0001f;
        // camera.rotation.y += 0.0001f;

        renderModel(renderer, &world.meshes[0], camera, &assetManager);

        SDL_RenderPresent(renderer);
    }

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}