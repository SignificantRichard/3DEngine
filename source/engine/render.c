#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include "render.h"

#define NEAR_PLANE 0.1f
#define FAR_PLANE  1000.0f
#define FOV_DEG    70.0f
#define DEG2RAD    0.01745329251994329577f

static Mat4 mat4Identity(void) {
    Mat4 out = {0};
    out.m[0]  = 1.0f;
    out.m[5]  = 1.0f;
    out.m[10] = 1.0f;
    out.m[15] = 1.0f;
    return out;
}

static Mat4 mat4Multiply(Mat4 a, Mat4 b) {
    Mat4 out = {0};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            out.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return out;
}

static Mat4 mat4Translate(float x, float y, float z) {
    Mat4 out = mat4Identity();
    out.m[12] = x;
    out.m[13] = y;
    out.m[14] = z;
    return out;
}

static Mat4 mat4Scale(float s) {
    Mat4 out = {0};
    out.m[0]  = s;
    out.m[5]  = s;
    out.m[10] = s;
    out.m[15] = 1.0f;
    return out;
}

static Mat4 mat4RotateXYZ(Vector rot) {
    float cx = cosf(rot.x), sx = sinf(rot.x);
    float cy = cosf(rot.y), sy = sinf(rot.y);
    float cz = cosf(rot.z), sz = sinf(rot.z);

    Mat4 rx = mat4Identity();
    rx.m[5]  = cx;  rx.m[9]  = -sx;
    rx.m[6]  = sx;  rx.m[10] = cx;

    Mat4 ry = mat4Identity();
    ry.m[0]  = cy;  ry.m[8]  = sy;
    ry.m[2]  = -sy; ry.m[10] = cy;

    Mat4 rz = mat4Identity();
    rz.m[0]  = cz;  rz.m[4]  = -sz;
    rz.m[1]  = sz;  rz.m[5]  = cz;

    return mat4Multiply(rz, mat4Multiply(ry, rx));
}

static Mat4 mat4InverseRotateXYZ(Vector rot) {
    float cx = cosf(-rot.x), sx = sinf(-rot.x);
    float cy = cosf(-rot.y), sy = sinf(-rot.y);
    float cz = cosf(-rot.z), sz = sinf(-rot.z);

    Mat4 rx = mat4Identity();
    rx.m[5]  = cx;  rx.m[9]  = -sx;
    rx.m[6]  = sx;  rx.m[10] = cx;

    Mat4 ry = mat4Identity();
    ry.m[0]  = cy;  ry.m[8]  = sy;
    ry.m[2]  = -sy; ry.m[10] = cy;

    Mat4 rz = mat4Identity();
    rz.m[0]  = cz;  rz.m[4]  = -sz;
    rz.m[1]  = sz;  rz.m[5]  = cz;

    return mat4Multiply(rx, mat4Multiply(ry, rz));
}

static Mat4 buildModelMatrix(const Mesh* mesh) {
    Mat4 t = mat4Translate(mesh->position.x, mesh->position.y, mesh->position.z);
    Mat4 r = mat4RotateXYZ(mesh->rotation);
    Mat4 s = mat4Scale(mesh->scale != 0.0f ? mesh->scale : 1.0f);
    return mat4Multiply(t, mat4Multiply(r, s));
}

static Mat4 buildViewMatrix(struct Camera camera) {
    Mat4 r = mat4InverseRotateXYZ(camera.rotation);
    Mat4 t = mat4Translate(-camera.position.x, -camera.position.y, -camera.position.z);
    return mat4Multiply(r, t);
}

static Mat4 buildPerspectiveMatrix(float fovDeg, float aspect, float nearZ, float farZ) {
    float f = 1.0f / tanf((fovDeg * DEG2RAD) * 0.5f);
    float rangeInv = 1.0f / (nearZ - farZ);

    Mat4 out = {0};
    out.m[0]  = f / aspect;
    out.m[5]  = f;
    out.m[10] = farZ * rangeInv;
    out.m[11] = -1.0f;
    out.m[14] = nearZ * farZ * rangeInv;
    return out;
}

static SDL_GPUShader* loadSpirvShader(SDL_GPUDevice* device, const char* path,
    SDL_GPUShaderStage stage,
    Uint32 uniformBuffers, Uint32 samplers) {
    size_t size = 0;
    void* code = SDL_LoadFile(path, &size);
    if (!code || size == 0) {
        fprintf(stderr, "Failed to load shader file: %s\n", path);
        if (code) SDL_free(code);
        return NULL;
    }

    SDL_GPUShaderCreateInfo info = {0};
    info.code = (const Uint8*)code;
    info.code_size = size;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = stage;
    info.num_uniform_buffers = uniformBuffers;
    info.num_samplers = samplers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    SDL_free(code);

    if (!shader) {
        fprintf(stderr, "Failed to create GPU shader (%s): %s\n", path, SDL_GetError());
    }
    return shader;
}

static SDL_GPUTextureFormat chooseDepthFormat(SDL_GPUDevice* device) {
    if (SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            SDL_GPU_TEXTURETYPE_2D,
            SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    }
    return SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
}

bool ensureDepthTexture(Renderer* r, Uint32 width, Uint32 height) {
    if (!r || !r->device || width == 0 || height == 0) {
        return false;
    }
    if (r->depthTexture && r->depthWidth == width && r->depthHeight == height) {
        return true;
    }

    if (r->depthTexture) {
        SDL_ReleaseGPUTexture(r->device, r->depthTexture);
        r->depthTexture = NULL;
    }

    SDL_GPUTextureCreateInfo info = {0};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = r->depthFormat;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    r->depthTexture = SDL_CreateGPUTexture(r->device, &info);
    if (!r->depthTexture) {
        fprintf(stderr, "Failed to create depth texture: %s\n", SDL_GetError());
        return false;
    }

    r->depthWidth = width;
    r->depthHeight = height;
    return true;
}

bool initRenderer(Renderer* r, SDL_Window* window, const char* vertSpvPath, const char* fragSpvPath) {
    if (!r || !window) return false;
    memset(r, 0, sizeof(*r));
    r->window = window;

    r->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!r->device) {
        fprintf(stderr, "SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(r->device, window)) {
        fprintf(stderr, "SDL_ClaimWindowForGPUDevice failed: %s\n", SDL_GetError());
        SDL_DestroyGPUDevice(r->device);
        r->device = NULL;
        return false;
    }

    r->depthFormat = chooseDepthFormat(r->device);

    SDL_GPUSamplerCreateInfo samplerInfo = {0};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

    r->sampler = SDL_CreateGPUSampler(r->device, &samplerInfo);
    if (!r->sampler) {
        fprintf(stderr, "SDL_CreateGPUSampler failed: %s\n", SDL_GetError());
        destroyRenderer(r);
        return false;
    }

    SDL_GPUShader* vertShader = loadSpirvShader(r->device, vertSpvPath, SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
    SDL_GPUShader* fragShader = loadSpirvShader(r->device, fragSpvPath, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
    if (!vertShader || !fragShader) {
        if (vertShader) SDL_ReleaseGPUShader(r->device, vertShader);
        if (fragShader) SDL_ReleaseGPUShader(r->device, fragShader);
        destroyRenderer(r);
        return false;
    }

    SDL_GPUVertexBufferDescription vbDesc = {0};
    vbDesc.slot = 0;
    vbDesc.pitch = sizeof(GpuVertex);
    vbDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[3] = {
        { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = offsetof(GpuVertex, pos) },
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = offsetof(GpuVertex, normal) },
        { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(GpuVertex, uv) }
    };

    SDL_GPUColorTargetDescription colorDesc = {0};
    colorDesc.format = SDL_GetGPUSwapchainTextureFormat(r->device, window);

    SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {0};
    pipeInfo.vertex_shader = vertShader;
    pipeInfo.fragment_shader = fragShader;
    pipeInfo.vertex_input_state.vertex_buffer_descriptions = &vbDesc;
    pipeInfo.vertex_input_state.num_vertex_buffers = 1;
    pipeInfo.vertex_input_state.vertex_attributes = attrs;
    pipeInfo.vertex_input_state.num_vertex_attributes = 3;
    pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeInfo.depth_stencil_state.enable_depth_test = true;
    pipeInfo.depth_stencil_state.enable_depth_write = true;
    pipeInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    pipeInfo.target_info.color_target_descriptions = &colorDesc;
    pipeInfo.target_info.num_color_targets = 1;
    pipeInfo.target_info.has_depth_stencil_target = true;
    pipeInfo.target_info.depth_stencil_format = r->depthFormat;

    r->pipeline = SDL_CreateGPUGraphicsPipeline(r->device, &pipeInfo);

    SDL_ReleaseGPUShader(r->device, vertShader);
    SDL_ReleaseGPUShader(r->device, fragShader);

    if (!r->pipeline) {
        fprintf(stderr, "SDL_CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
        destroyRenderer(r);
        return false;
    }

    return true;
}

void renderMesh(Renderer* r, SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                const Mesh* mesh, const Mat4* viewProj, AssetManager* assetManager) {
    if (!r || !cmd || !pass || !mesh || !viewProj || !assetManager) return;
    if (!mesh->gpuVertexBuffer || mesh->gpuVertexCount <= 0) return;

    int texIdx = lookForImageAsset(assetManager, mesh->texture);
    if (texIdx < 0) return;

    SDL_GPUTexture* gpuTex = assetManager->textures[texIdx].texture;
    if (!gpuTex) return;

    Mat4 model = buildModelMatrix(mesh);
    VertexUniforms vUniforms = {
        .mvp = mat4Multiply(*viewProj, model),
        .model = model
    };
    SDL_PushGPUVertexUniformData(cmd, 0, &vUniforms, sizeof(vUniforms));

    FragmentUniforms fUniforms = {
        .lightDir = { -0.5f, 0.8f, -0.3f },
        .ambient = 0.2f,
        ._pad = { 0.0f, 0.0f }
    };
    SDL_PushGPUFragmentUniformData(cmd, 0, &fUniforms, sizeof(fUniforms));

    SDL_GPUBufferBinding vbBinding = {
        .buffer = mesh->gpuVertexBuffer,
        .offset = 0
    };
    SDL_BindGPUVertexBuffers(pass, 0, &vbBinding, 1);

    SDL_GPUTextureSamplerBinding texBinding = {
        .texture = gpuTex,
        .sampler = r->sampler
    };
    SDL_BindGPUFragmentSamplers(pass, 0, &texBinding, 1);

    SDL_DrawGPUPrimitives(pass, (Uint32)mesh->gpuVertexCount, 1, 0, 0);
}

void renderWorld(Renderer* r, World* world, struct Camera camera, AssetManager* assetManager) {
    if (!r || !r->device || !r->pipeline || !world || !assetManager) return;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(r->device);
    if (!cmd) {
        fprintf(stderr, "SDL_AcquireGPUCommandBuffer failed: %s\n", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchainTex = NULL;
    Uint32 w = 0, h = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, r->window, &swapchainTex, &w, &h)) {
        fprintf(stderr, "SDL_WaitAndAcquireGPUSwapchainTexture failed: %s\n", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    if (!swapchainTex || w == 0 || h == 0) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return;
    }

    if (!ensureDepthTexture(r, w, h)) {
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    SDL_GPUColorTargetInfo colorTarget = {0};
    colorTarget.texture = swapchainTex;
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.clear_color = (SDL_FColor){ 0.08f, 0.09f, 0.12f, 1.0f };

    SDL_GPUDepthStencilTargetInfo depthTarget = {0};
    depthTarget.texture = r->depthTexture;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.clear_depth = 1.0f;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &depthTarget);
    if (!pass) {
        fprintf(stderr, "SDL_BeginGPURenderPass failed: %s\n", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    SDL_BindGPUGraphicsPipeline(pass, r->pipeline);

    float aspect = (float)w / (float)h;
    Mat4 view = buildViewMatrix(camera);
    Mat4 proj = buildPerspectiveMatrix(FOV_DEG, aspect, NEAR_PLANE, FAR_PLANE);
    Mat4 viewProj = mat4Multiply(proj, view);

    for (int i = 0; i < world->worldSize; i++) {
        renderMesh(r, cmd, pass, &world->meshes[i], &viewProj, assetManager);
    }

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}

void destroyRenderer(Renderer* r) {
    if (!r) return;

    if (r->device) {
        SDL_WaitForGPUIdle(r->device);
        if (r->pipeline) SDL_ReleaseGPUGraphicsPipeline(r->device, r->pipeline);
        if (r->depthTexture) SDL_ReleaseGPUTexture(r->device, r->depthTexture);
        if (r->sampler) SDL_ReleaseGPUSampler(r->device, r->sampler);
        if (r->window) SDL_ReleaseWindowFromGPUDevice(r->device, r->window);
        SDL_DestroyGPUDevice(r->device);
    }

    memset(r, 0, sizeof(*r));
}