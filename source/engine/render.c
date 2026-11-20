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

static Vector normalizeVec(Vector v) {
    float lenSq = dot(v, v);
    if (lenSq <= 1e-12f) {
        return blankVector();
    }
    float invLen = 1.0f / sqrtf(lenSq);
    linearMult(&v, invLen);
    return v;
}

static Vector rotateByEulerXYZ(Vector v, Vector rot) {
    float cosX = cosf(rot.x);
    float sinX = sinf(rot.x);
    Vector r1 = {
        v.x,
        v.y * cosX - v.z * sinX,
        v.y * sinX + v.z * cosX
    };

    float cosY = cosf(rot.y);
    float sinY = sinf(rot.y);
    Vector r2 = {
        r1.x * cosY + r1.z * sinY,
        r1.y,
        -r1.x * sinY + r1.z * cosY
    };

    float cosZ = cosf(rot.z);
    float sinZ = sinf(rot.z);
    return (Vector){
        r2.x * cosZ - r2.y * sinZ,
        r2.x * sinZ + r2.y * cosZ,
        r2.z
    };
}

static Vector rotateByInverseEulerXYZ(Vector v, Vector rot) {
    float cosZ = cosf(-rot.z);
    float sinZ = sinf(-rot.z);
    Vector r1 = {
        v.x * cosZ - v.y * sinZ,
        v.x * sinZ + v.y * cosZ,
        v.z
    };

    float cosY = cosf(-rot.y);
    float sinY = sinf(-rot.y);
    Vector r2 = {
        r1.x * cosY + r1.z * sinY,
        r1.y,
        -r1.x * sinY + r1.z * cosY
    };

    float cosX = cosf(-rot.x);
    float sinX = sinf(-rot.x);
    return (Vector){
        r2.x,
        r2.y * cosX - r2.z * sinX,
        r2.y * sinX + r2.z * cosX
    };
}

static Vector getFaceNormalOrFallback(const Face* face, const Vector* storedNormal) {
    if (storedNormal != NULL) {
        float lenSq = dot(*storedNormal, *storedNormal);
        if (lenSq > 1e-12f) {
            return normalizeVec(*storedNormal);
        }
    }

    Vector u = *face->b;
    vectorSub(&u, face->a); // u = b - a

    Vector v = *face->c;
    vectorSub(&v, face->a); // v = c - a

    return normalizeVec(cross(u, v));
}

static bool isFaceBackfacingCameraSpace(
    const Face* face,
    const Vector* storedNormal,
    Vector modelRot,
    Vector cameraRot,
    Vector pointInCamSpace
) {
    Vector normal = getFaceNormalOrFallback(face, storedNormal);
    normal = normalizeVec(rotateByEulerXYZ(normal, modelRot));
    Vector camNormal = normalizeVec(rotateByInverseEulerXYZ(normal, cameraRot));

    // Camera is at (0, 0, 0) in camera space; vector from camera to surface is pointInCamSpace.
    // Normal pointing away from or perpendicular to camera has dot(camNormal, pointInCamSpace) >= 0.
    return dot(camNormal, pointInCamSpace) >= 0.0f;
}

static float computeFaceBrightness(
    const Face* face,
    const Vector* storedNormal,
    Vector modelRot,
    Vector lightDirTowardLight,
    float ambient
) {
    Vector normal = getFaceNormalOrFallback(face, storedNormal);
    normal = normalizeVec(rotateByEulerXYZ(normal, modelRot));

    Vector lightDir = normalizeVec(lightDirTowardLight);

    float ndotl = dot(normal, lightDir);
    if (ndotl < 0.0f) {
        ndotl = 0.0f;
    }

    float brightness = ambient + (1.0f - ambient) * ndotl;
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;
    return brightness;
}

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
    const Vector* faceNormal,
    float scale,
    SDL_FPoint* uvA,
    SDL_FPoint* uvB,
    SDL_FPoint* uvC
) {
    float nx, ny, nz;

    if (faceNormal != NULL && dot(*faceNormal, *faceNormal) > 1e-12f) {
        nx = faceNormal->x;
        ny = faceNormal->y;
        nz = faceNormal->z;
    } else {
        float ux = b->x - a->x;
        float uy = b->y - a->y;
        float uz = b->z - a->z;

        float vx = c->x - a->x;
        float vy = c->y - a->y;
        float vz = c->z - a->z;

        nx = uy * vz - uz * vy;
        ny = uz * vx - ux * vz;
        nz = ux * vy - uy * vx;
    }

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

        const Vector* faceNormal = (model->normals != NULL) ? &model->normals[i] : NULL;
        if (isFaceBackfacingCameraSpace(&model->faces[i], faceNormal, model->rotation, camera.rotation, cA)) {
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
    float uvScale = 1.0f;

    Vector lightDir = { -0.5f, -0.8f, -0.3f }; // Points toward the light source
    float ambient = 0.2f;

    int base = 0;
    for (int i = 0; i < validFaceCount; i++) {
        int faceIdx = sortedFaces[i].index;
        const Face* face = &model->faces[faceIdx];
        const Vector* faceNormal = (model->normals != NULL) ? &model->normals[faceIdx] : NULL;

        SDL_FPoint uvA, uvB, uvC;
        computeTriplanarUVs(face->a, face->b, face->c, faceNormal, uvScale, &uvA, &uvB, &uvC);

        float b = computeFaceBrightness(face, faceNormal, model->rotation, lightDir, ambient);
        SDL_FColor faceColor = { b, b, b, 1.0f };

        vert[base + 0] = (SDL_Vertex){ {sortedFaces[i].x1, sortedFaces[i].y1}, faceColor, uvA };
        vert[base + 1] = (SDL_Vertex){ {sortedFaces[i].x2, sortedFaces[i].y2}, faceColor, uvB };
        vert[base + 2] = (SDL_Vertex){ {sortedFaces[i].x3, sortedFaces[i].y3}, faceColor, uvC };

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