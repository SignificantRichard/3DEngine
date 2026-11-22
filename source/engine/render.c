#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL.h>
#include "vector.h"
#include "mesh.h"
#include "instances.h"
#include "asset_manager.h"
#include "settings.h"
#include "render.h"
#include "world.h"

#define NEAR_PLANE 0.1f

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

// Transforms a point into camera space without rejecting behind-camera points.
static void transformToCameraSpaceUnclipped(
    const Vector* v,
    Vector modelPos,
    Vector modelRot,
    struct Camera camera,
    Vector* outCamSpace
) {
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

    *outCamSpace = nv;
}

bool getVectorCameraSpace(Vector* v, Vector modelPos, Vector modelRot, struct Camera camera, Vector* outCamSpace) {
    Vector nv;
    transformToCameraSpaceUnclipped(v, modelPos, modelRot, camera, &nv);

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

static Vector lerpCamVec(Vector a, Vector b, float nearZ) {
    float denom = b.z - a.z;
    if (fabsf(denom) <= 1e-6f) {
        return a;
    }
    float t = (nearZ - a.z) / denom;
    return (Vector){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        nearZ
    };
}

static int clipTriangleNearPlane(Vector a, Vector b, Vector c, float nearZ, Vector outTris[2][3]) {
    Vector poly[4];
    int polyCount = 0;

    Vector src[3] = { a, b, c };
    Vector prev = src[2];
    bool prevInside = (prev.z >= nearZ);

    for (int i = 0; i < 3; i++) {
        Vector curr = src[i];
        bool currInside = (curr.z >= nearZ);

        if (currInside) {
            if (!prevInside) {
                poly[polyCount++] = lerpCamVec(prev, curr, nearZ);
            }
            poly[polyCount++] = curr;
        } else if (prevInside) {
            poly[polyCount++] = lerpCamVec(prev, curr, nearZ);
        }

        prev = curr;
        prevInside = currInside;
    }

    if (polyCount < 3) {
        return 0;
    }

    outTris[0][0] = poly[0];
    outTris[0][1] = poly[1];
    outTris[0][2] = poly[2];

    if (polyCount == 4) {
        outTris[1][0] = poly[0];
        outTris[1][1] = poly[2];
        outTris[1][2] = poly[3];
        return 2;
    }

    return 1;
}

static bool isTriangleOnScreen(float x1, float y1, float x2, float y2, float x3, float y3) {
    float minX = fminf(x1, fminf(x2, x3));
    float maxX = fmaxf(x1, fmaxf(x2, x3));
    float minY = fminf(y1, fminf(y2, y3));
    float maxY = fmaxf(y1, fmaxf(y2, y3));

    if (maxX < 0.0f || minX > (float)INTERNAL_WIDTH) return false;
    if (maxY < 0.0f || minY > (float)INTERNAL_HEIGHT) return false;
    return true;
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

    int maxCandidateTris = model->faceAmount * 2;
    struct SortedFace* sortedFaces = (struct SortedFace*)malloc((size_t)maxCandidateTris * sizeof(struct SortedFace));
    if (!sortedFaces) return;

    int validFaceCount = 0;

    float fovDegrees = 70.0f;
    float fovRadians = fovDegrees * (3.14159265f / 180.0f);
    float fovScale = (INTERNAL_HEIGHT / 2.0f) / tanf(fovRadians / 2.0f);
    float centerX = INTERNAL_WIDTH / 2.0f;
    float centerY = INTERNAL_HEIGHT / 2.0f;

    for (int i = 0; i < model->faceAmount; i++) {
        Vector cA, cB, cC;
        transformToCameraSpaceUnclipped(model->faces[i].a, model->position, model->rotation, camera, &cA);
        transformToCameraSpaceUnclipped(model->faces[i].b, model->position, model->rotation, camera, &cB);
        transformToCameraSpaceUnclipped(model->faces[i].c, model->position, model->rotation, camera, &cC);

        const Vector* faceNormal = (model->normals != NULL) ? &model->normals[i] : NULL;
        if (isFaceBackfacingCameraSpace(&model->faces[i], faceNormal, model->rotation, camera.rotation, cA)) {
            continue;
        }

        Vector clipped[2][3];
        int triCount = clipTriangleNearPlane(cA, cB, cC, NEAR_PLANE, clipped);

        for (int t = 0; t < triCount; t++) {
            Vector p1 = clipped[t][0];
            Vector p2 = clipped[t][1];
            Vector p3 = clipped[t][2];

            float avgDepth = (p1.z + p2.z + p3.z) / 3.0f;

            float x1 = (p1.x / p1.z) * fovScale + centerX;
            float y1 = (p1.y / p1.z) * fovScale + centerY;

            float x2 = (p2.x / p2.z) * fovScale + centerX;
            float y2 = (p2.y / p2.z) * fovScale + centerY;

            float x3 = (p3.x / p3.z) * fovScale + centerX;
            float y3 = (p3.y / p3.z) * fovScale + centerY;

            if (!isTriangleOnScreen(x1, y1, x2, y2, x3, y3)) {
                continue;
            }

            sortedFaces[validFaceCount++] = (struct SortedFace){
                .index = i,
                .depth = avgDepth,
                .x1 = x1, .y1 = y1,
                .x2 = x2, .y2 = y2,
                .x3 = x3, .y3 = y3
            };
        }
    }

    if (validFaceCount <= 0) {
        free(sortedFaces);
        return;
    }

    qsort(sortedFaces, (size_t)validFaceCount, sizeof(struct SortedFace), compareFaces);

    int totalVerts = validFaceCount * 3;
    SDL_Vertex* vert = (SDL_Vertex*)malloc((size_t)totalVerts * sizeof(SDL_Vertex));
    int* indices = (int*)malloc((size_t)totalVerts * sizeof(int));

    if (!vert || !indices) {
        free(vert);
        free(indices);
        free(sortedFaces);
        return;
    }

    SDL_Texture* texture = assetManager->textures[lookForImageAsset(assetManager, model->texture)].texture;
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