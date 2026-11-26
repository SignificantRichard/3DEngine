#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include "vector.h"
#include "instances.h"
#include "mesh.h"
#include "asset_manager.h"
#include "world.h"

typedef struct {
    float m[16];
} Mat4;

typedef struct {
    Mat4 mvp;
    Mat4 model;
} VertexUniforms;

typedef struct {
    float lightDir[3];
    float ambient;
    float _pad[2]; // 16-byte / std140-friendly alignment
} FragmentUniforms;

typedef struct {
    SDL_GPUDevice* device;
    SDL_Window* window;
    SDL_GPUGraphicsPipeline* pipeline;
    SDL_GPUTexture* depthTexture;
    SDL_GPUTextureFormat depthFormat;
    SDL_GPUSampler* sampler;
    Uint32 depthWidth;
    Uint32 depthHeight;
} Renderer;

bool initRenderer(Renderer* r, SDL_Window* window, const char* vertSpvPath, const char* fragSpvPath);
bool ensureDepthTexture(Renderer* r, Uint32 width, Uint32 height);
void renderWorld(Renderer* r, World* world, struct Camera camera, AssetManager* assetManager);
void renderMesh(Renderer* r, SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                const Mesh* mesh, const Mat4* viewProj, AssetManager* assetManager);
void destroyRenderer(Renderer* r);

#endif