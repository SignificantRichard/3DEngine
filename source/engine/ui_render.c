#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "ui_render.h"

static SDL_GPUShader* loadUiShader(SDL_GPUDevice* device, const char* path,
    SDL_GPUShaderStage stage,
    Uint32 uniformBuffers, Uint32 samplers) {
    size_t size = 0;
    void* code = SDL_LoadFile(path, &size);
    if (!code) {
        SDL_Log("Failed to load UI shader '%s': %s", path, SDL_GetError());
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
        SDL_Log("Failed to create UI shader from '%s': %s", path, SDL_GetError());
    }
    return shader;
}

bool initUiRenderer(UiRenderer* ui, Renderer* gpuRenderer,
                    const char* vertSpvPath, const char* fragSpvPath,
                    float screenWidth, float screenHeight) {
    if (!ui || !gpuRenderer || !gpuRenderer->device || !gpuRenderer->window) {
        return false;
    }

    memset(ui, 0, sizeof(*ui));
    ui->screenWidth = screenWidth;
    ui->screenHeight = screenHeight;

    SDL_GPUDevice* device = gpuRenderer->device;

    SDL_GPUShader* vertShader = loadUiShader(device, vertSpvPath, SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
    SDL_GPUShader* fragShader = loadUiShader(device, fragSpvPath, SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
    if (!vertShader || !fragShader) {
        if (vertShader) SDL_ReleaseGPUShader(device, vertShader);
        if (fragShader) SDL_ReleaseGPUShader(device, fragShader);
        return false;
    }

    SDL_GPUVertexBufferDescription vbd = {0};
    vbd.slot = 0;
    vbd.pitch = sizeof(UiVertex);
    vbd.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[2] = {0};
    attrs[0].location = 0;
    attrs[0].buffer_slot = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[0].offset = offsetof(UiVertex, x);

    attrs[1].location = 1;
    attrs[1].buffer_slot = 0;
    attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attrs[1].offset = offsetof(UiVertex, u);

    SDL_GPUColorTargetDescription colorTarget = {0};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device, gpuRenderer->window);
    colorTarget.blend_state.enable_blend = true;
    colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {0};
    pipeInfo.vertex_shader = vertShader;
    pipeInfo.fragment_shader = fragShader;
    pipeInfo.vertex_input_state.vertex_buffer_descriptions = &vbd;
    pipeInfo.vertex_input_state.num_vertex_buffers = 1;
    pipeInfo.vertex_input_state.vertex_attributes = attrs;
    pipeInfo.vertex_input_state.num_vertex_attributes = 2;
    pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipeInfo.depth_stencil_state.enable_depth_test = false;
    pipeInfo.depth_stencil_state.enable_depth_write = false;
    pipeInfo.target_info.color_target_descriptions = &colorTarget;
    pipeInfo.target_info.num_color_targets = 1;

    ui->pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
    SDL_ReleaseGPUShader(device, vertShader);
    SDL_ReleaseGPUShader(device, fragShader);

    if (!ui->pipeline) {
        SDL_Log("Failed to create UI graphics pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_GPUSamplerCreateInfo sampInfo = {0};
    sampInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    sampInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    ui->sampler = SDL_CreateGPUSampler(device, &sampInfo);
    if (!ui->sampler) {
        SDL_Log("Failed to create UI sampler: %s", SDL_GetError());
        destroyUiRenderer(ui, gpuRenderer);
        return false;
    }

    SDL_GPUBufferCreateInfo bufInfo = {0};
    bufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufInfo.size = (Uint32)(sizeof(UiVertex) * 6);

    ui->quadBuffer = SDL_CreateGPUBuffer(device, &bufInfo);
    if (!ui->quadBuffer) {
        SDL_Log("Failed to create UI quad buffer: %s", SDL_GetError());
        destroyUiRenderer(ui, gpuRenderer);
        return false;
    }

    return true;
}

void renderLabelGPU(UiRenderer* ui, Renderer* gpuRenderer, const Label* label) {
    if (!ui || !ui->pipeline || !ui->sampler || !ui->quadBuffer ||
        !gpuRenderer || !gpuRenderer->device || !gpuRenderer->window ||
        !label || !label->gpuTexture || label->size.x <= 0 || label->size.y <= 0) {
        return;
    }

    SDL_GPUDevice* device = gpuRenderer->device;

    float x0 = (float)label->position.x + label->offset.x;
    float y0 = (float)label->position.y + label->offset.y;
    float x1 = x0 + (float)label->size.x;
    float y1 = y0 + (float)label->size.y;

    UiVertex quad[6] = {
        { x0, y0, 0.0f, 0.0f },
        { x1, y0, 1.0f, 0.0f },
        { x1, y1, 1.0f, 1.0f },

        { x0, y0, 0.0f, 0.0f },
        { x1, y1, 1.0f, 1.0f },
        { x0, y1, 0.0f, 1.0f },
    };

    SDL_GPUTransferBufferCreateInfo xferInfo = {0};
    xferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    xferInfo.size = (Uint32)sizeof(quad);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &xferInfo);
    if (!transfer) {
        return;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return;
    }
    memcpy(mapped, quad, sizeof(quad));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return;
    }

    SDL_GPUTexture* swapchain = NULL;
    if (!SDL_AcquireGPUSwapchainTexture(cmd, gpuRenderer->window, &swapchain, NULL, NULL) || !swapchain) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
    if (copyPass) {
        SDL_GPUTransferBufferLocation src = { .transfer_buffer = transfer, .offset = 0 };
        SDL_GPUBufferRegion dst = { .buffer = ui->quadBuffer, .offset = 0, .size = (Uint32)sizeof(quad) };
        SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
        SDL_EndGPUCopyPass(copyPass);
    }
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    SDL_GPUColorTargetInfo colorTarget = {0};
    colorTarget.texture = swapchain;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD; // preserve 3D scene drawn by renderWorld
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, NULL);
    if (pass) {
        struct {
            float screenWidth;
            float screenHeight;
            float pad0;
            float pad1;
        } uniforms = { ui->screenWidth, ui->screenHeight, 0.0f, 0.0f };

        SDL_BindGPUGraphicsPipeline(pass, ui->pipeline);
        SDL_PushGPUVertexUniformData(cmd, 0, &uniforms, sizeof(uniforms));

        SDL_GPUBufferBinding vtxBind = { .buffer = ui->quadBuffer, .offset = 0 };
        SDL_BindGPUVertexBuffers(pass, 0, &vtxBind, 1);

        SDL_GPUTextureSamplerBinding texBind = { .texture = label->gpuTexture, .sampler = ui->sampler };
        SDL_BindGPUFragmentSamplers(pass, 0, &texBind, 1);

        SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
        SDL_EndGPURenderPass(pass);
    }

    SDL_SubmitGPUCommandBuffer(cmd);
}

void destroyUiRenderer(UiRenderer* ui, Renderer* gpuRenderer) {
    if (!ui || !gpuRenderer || !gpuRenderer->device) {
        return;
    }
    SDL_GPUDevice* device = gpuRenderer->device;

    if (ui->quadBuffer) {
        SDL_ReleaseGPUBuffer(device, ui->quadBuffer);
        ui->quadBuffer = NULL;
    }
    if (ui->sampler) {
        SDL_ReleaseGPUSampler(device, ui->sampler);
        ui->sampler = NULL;
    }
    if (ui->pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, ui->pipeline);
        ui->pipeline = NULL;
    }
}