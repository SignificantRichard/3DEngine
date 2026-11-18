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

typedef struct {
    Vector movementDirection;
    Vector position;
    Vector rotation;
    Vector rotationDirection;
} Body;

bool getVectorCameraSpace(Vector* v, Vector modelPos, Vector modelRot, struct Camera camera, Vector* outCamSpace) {
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
        return false; // Behind camera
    }

    *outCamSpace = nv;
    return true;
}

bool vectorToScreenPos(Vector* v, Vector modelPos, Vector modelRot, struct Camera camera, float* x, float* y) {
    Vector nv;
    if (!getVectorCameraSpace(v, modelPos, modelRot, camera, &nv)) {
        return false;
    }

    float fovDegrees = 70.0f;
    float fovRadians = fovDegrees * (3.14159265f / 180.0f);
    float fovScale = (INTERNAL_HEIGHT / 2.0f) / tanf(fovRadians / 2.0f);
    
    float centerX = INTERNAL_WIDTH / 2.0f;
    float centerY = INTERNAL_HEIGHT / 2.0f;

    *x = (nv.x / nv.z) * fovScale + centerX;
    *y = (nv.y / nv.z) * fovScale + centerY;

    return true;
}

struct SortedFace {
    int index;
    float depth;
    float x1, y1, x2, y2, x3, y3;
};

int compareFaces(const void* a, const void* b) {
    const struct SortedFace* fa = (const struct SortedFace*)a;
    const struct SortedFace* fb = (const struct SortedFace*)b;
    if (fa->depth < fb->depth) return 1;
    if (fa->depth > fb->depth) return -1;
    return 0;
}

void renderModel(SDL_Renderer* renderer, Mesh* model, struct Camera camera, AssetManager* assetManager) {
    if (!model || model->faceAmount <= 0) return;

    struct SortedFace* sortedFaces = (struct SortedFace*)malloc(model->faceAmount * sizeof(struct SortedFace));
    if (!sortedFaces) return;

    int validFaceCount = 0;

    float fovDegrees = 70.0f;
    float fovRadians = fovDegrees * (3.14159265f / 180.0f);
    float fovScale = (INTERNAL_HEIGHT / 2.0f) / tanf(fovRadians / 2.0f);
    float centerX = INTERNAL_WIDTH / 2.0f;
    float centerY = INTERNAL_HEIGHT / 2.0f;

    for (int i = 0; i < model->faceAmount; i++) {
        Vector cA, cB, cC;

        if (!getVectorCameraSpace(model->faces[i].a, model->position, model->rotation, camera, &cA) ||
            !getVectorCameraSpace(model->faces[i].b, model->position, model->rotation, camera, &cB) ||
            !getVectorCameraSpace(model->faces[i].c, model->position, model->rotation, camera, &cC)) {
            continue;
        }

        float avgDepth = (cA.z + cB.z + cC.z) / 3.0f;

        float x1 = (cA.x / cA.z) * fovScale + centerX;
        float y1 = (cA.y / cA.z) * fovScale + centerY;

        float x2 = (cB.x / cB.z) * fovScale + centerX;
        float y2 = (cB.y / cB.z) * fovScale + centerY;

        float x3 = (cC.x / cC.z) * fovScale + centerX;
        float y3 = (cC.y / cC.z) * fovScale + centerY;

        sortedFaces[validFaceCount] = (struct SortedFace){
            .index = i,
            .depth = avgDepth,
            .x1 = x1, .y1 = y1,
            .x2 = x2, .y2 = y2,
            .x3 = x3, .y3 = y3
        };
        validFaceCount++;
    }

    if (validFaceCount <= 0) {
        free(sortedFaces);
        return;
    }

    qsort(sortedFaces, validFaceCount, sizeof(struct SortedFace), compareFaces);

    int totalVerts = validFaceCount * 3;
    SDL_Vertex* vert = (SDL_Vertex*)malloc(totalVerts * sizeof(SDL_Vertex));
    int* indices = (int*)malloc(totalVerts * sizeof(int));
    
    if (!vert || !indices) {
        free(vert);
        free(indices);
        free(sortedFaces);
        return;
    }

    SDL_Texture* texture = assetManager->textures[0].texture;
    SDL_FColor white = { 1.0f, 1.0f, 1.0f, 1.0f };

    int base = 0;
    for (int i = 0; i < validFaceCount; i++) {
        vert[base + 0] = (SDL_Vertex){ {sortedFaces[i].x1, sortedFaces[i].y1}, white, {0.0f, 0.0f} };
        vert[base + 1] = (SDL_Vertex){ {sortedFaces[i].x2, sortedFaces[i].y2}, white, {0.0f, 1.0f} };
        vert[base + 2] = (SDL_Vertex){ {sortedFaces[i].x3, sortedFaces[i].y3}, white, {1.0f, 1.0f} };

        indices[base + 0] = base + 0;
        indices[base + 1] = base + 1;
        indices[base + 2] = base + 2;

        base += 3;
    }

    SDL_RenderGeometry(renderer, texture, vert, totalVerts, indices, totalVerts);

    free(vert);
    free(indices);
    free(sortedFaces);
}

int pollInput(Body* body, float delta) {
    const bool *keyStates = SDL_GetKeyboardState(NULL);
    Vector localMovementDirection = blankVector();

    if (keyStates[SDL_SCANCODE_W]) {
        localMovementDirection.z += 1;
    }
    if (keyStates[SDL_SCANCODE_S]) {
        localMovementDirection.z -= 1;
    }
    if (keyStates[SDL_SCANCODE_A]) {
        localMovementDirection.x -= 1;
    }
    if (keyStates[SDL_SCANCODE_D]) {
        localMovementDirection.x += 1;
    }

    if (keyStates[SDL_SCANCODE_SPACE]) {
        localMovementDirection.y -= 1;
    }
    if (keyStates[SDL_SCANCODE_LCTRL]) {
        localMovementDirection.y += 1;
    }

    linearMult(&localMovementDirection, delta * 0.01f);
    body->movementDirection = localMovementDirection;

    localMovementDirection = blankVector();

    if (keyStates[SDL_SCANCODE_UP]) {
        localMovementDirection.x += 1;
    }
    if (keyStates[SDL_SCANCODE_DOWN]) {
        localMovementDirection.x -= 1;
    }
    if (keyStates[SDL_SCANCODE_LEFT]) {
        localMovementDirection.y -= 1;
    }
    if (keyStates[SDL_SCANCODE_RIGHT]) {
        localMovementDirection.y += 1;
    }
    
    linearMult(&localMovementDirection, delta * 0.01f);
    body->rotationDirection = localMovementDirection;
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
    
    {
        Mesh c = loadMeshFromBin("./source/res/models/cube.mdl");
        addMeshToWorld(&world, &c);
    }
    {
        Mesh c = loadMeshFromBin("./source/res/models/sphere.mdl");
        addMeshToWorld(&world, &c);
    }
    
    Body player = { .position = blankVector(), .rotation = blankVector(), .movementDirection = blankVector() };

    world.meshes[0].position = (Vector){ 0.0f, 0.0f, 20.0f };
    world.meshes[1].position = (Vector){ 0.0f, 10.0f, 20.0f };

    struct Camera camera = { .position = blankVector(), .rotation = blankVector() };

    uint64_t NOW = SDL_GetPerformanceCounter();
    uint64_t LAST = 0;
    double deltaTime = 0;

    while (running) {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST)*1000 / (double)SDL_GetPerformanceFrequency());
        // events and input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        pollInput(&player, deltaTime);
        vectorAdd(&player.rotation, &player.rotationDirection);
        
        {
            Vector localMovementDirection = blankVector();
            localMovementDirection.z = cosf(player.rotation.y) * player.movementDirection.z + sinf(player.rotation.y) * -player.movementDirection.x;
            localMovementDirection.x = cosf(player.rotation.y) * player.movementDirection.x + sinf(player.rotation.y) * player.movementDirection.z;
            localMovementDirection.y = player.movementDirection.y;
            vectorAdd(&player.position, &localMovementDirection);
            camera.position = player.position;
        }

        if (player.rotation.x > 1.57) {
            player.rotation.x = 1.57;
        } else if (player.rotation.x < -1.57) {
            player.rotation.x = -1.57;
        }
        camera.rotation = player.rotation;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // camera.rotation.y += 0.0001f;

        for (int i = 0; i < world.worldSize; i++) {
            renderModel(renderer, &world.meshes[i], camera, &assetManager);
        }


        SDL_RenderPresent(renderer);
    }

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}