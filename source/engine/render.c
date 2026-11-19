#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "vector.h"
#include "mesh.h"
#include "instances.h"
#include "asset_manager.h"
#include "settings.h"
#include "render.h"
#include "world.h"

struct SortedFace {
    int index;
    float depth;
    float x1, y1, x2, y2, x3, y3;
};

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

    // nv.x *= 0.0001f;
    // nv.y *= 0.0001f;
    // nv.z *= 0.0001f;

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

int compareFaces(const void* a, const void* b) {
    const struct SortedFace* fa = (const struct SortedFace*)a;
    const struct SortedFace* fb = (const struct SortedFace*)b;
    if (fa->depth < fb->depth) return 1;
    if (fa->depth > fb->depth) return -1;
    return 0;
}

static void computeTriplanarUVs(
    const Vector* a,
    const Vector* b,
    const Vector* c,
    float scale,
    SDL_FPoint* uvA,
    SDL_FPoint* uvB,
    SDL_FPoint* uvC
) {
    float ux = b->x - a->x;
    float uy = b->y - a->y;
    float uz = b->z - a->z;

    float vx = c->x - a->x;
    float vy = c->y - a->y;
    float vz = c->z - a->z;

    float nx = uy * vz - uz * vy;
    float ny = uz * vx - ux * vz;
    float nz = ux * vy - uy * vx;

    float ax = fabsf(nx);
    float ay = fabsf(ny);
    float az = fabsf(nz);

    if (ax >= ay && ax >= az) {
        *uvA = (SDL_FPoint){ a->z * scale, a->y * scale };
        *uvB = (SDL_FPoint){ b->z * scale, b->y * scale };
        *uvC = (SDL_FPoint){ c->z * scale, c->y * scale };
    } else if (ay >= ax && ay >= az) {
        *uvA = (SDL_FPoint){ a->x * scale, a->z * scale };
        *uvB = (SDL_FPoint){ b->x * scale, b->z * scale };
        *uvC = (SDL_FPoint){ c->x * scale, c->z * scale };
    } else {
        *uvA = (SDL_FPoint){ a->x * scale, a->y * scale };
        *uvB = (SDL_FPoint){ b->x * scale, b->y * scale };
        *uvC = (SDL_FPoint){ c->x * scale, c->y * scale };
    }
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
    float uvScale = 1.0f;

    int base = 0;
    for (int i = 0; i < validFaceCount; i++) {
        int faceIdx = sortedFaces[i].index;
        SDL_FPoint uvA, uvB, uvC;

        computeTriplanarUVs(
            model->faces[faceIdx].a,
            model->faces[faceIdx].b,
            model->faces[faceIdx].c,
            uvScale,
            &uvA, &uvB, &uvC
        );

        vert[base + 0] = (SDL_Vertex){ {sortedFaces[i].x1, sortedFaces[i].y1}, white, uvA };
        vert[base + 1] = (SDL_Vertex){ {sortedFaces[i].x2, sortedFaces[i].y2}, white, uvB };
        vert[base + 2] = (SDL_Vertex){ {sortedFaces[i].x3, sortedFaces[i].y3}, white, uvC };

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



void swapDrawOrder(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int sortOrderPartition(World* world, Vector position, int low, int high) {
    int pivotIndex = low + (high - low) / 2;
    float pivotDist = vectorMag(world->meshes[world->drawOrder[pivotIndex]].position, position);

    swapDrawOrder(&world->drawOrder[pivotIndex], &world->drawOrder[high]);

    int i = low - 1;
    for (int j = low; j < high; j++) {
        float distJ = vectorMag(world->meshes[world->drawOrder[j]].position, position);
        if (distJ > pivotDist) {
            i++;
            swapDrawOrder(&world->drawOrder[i], &world->drawOrder[j]);
        }
    }

    i++;
    swapDrawOrder(&world->drawOrder[i], &world->drawOrder[high]);
    return i;
}

void sortDrawOrder(World* world, Vector position, int low, int high) {
    if (low < high) {
        int pi = sortOrderPartition(world, position, low, high);
        sortDrawOrder(world, position, low, pi - 1);
        sortDrawOrder(world, position, pi + 1, high);
    }
}