#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "face.h"
#include "vector.h"
#include "mesh.h"
#include "files.h"

#define RESIZE_MULT 0.0001f
#define NORMAL_INV_SCALE (1.0f / 32767.0f)

#define MDL_FLAG_NORMALS 0x01
#define MDL_FLAG_UVS     0x02

static float readFloatLE(const uint8_t* ptr) {
    uint32_t bits = (uint32_t)ptr[0]
                  | ((uint32_t)ptr[1] << 8)
                  | ((uint32_t)ptr[2] << 16)
                  | ((uint32_t)ptr[3] << 24);
    float val;
    memcpy(&val, &bits, sizeof(float));
    return val;
}

Vector computeFaceNormal(Face f) {
    // Edge 1 = B - A, Edge 2 = C - A
    float ux = f.b->x - f.a->x;
    float uy = f.b->y - f.a->y;
    float uz = f.b->z - f.a->z;

    float vx = f.c->x - f.a->x;
    float vy = f.c->y - f.a->y;
    float vz = f.c->z - f.a->z;

    // Cross product U x V
    float nx = uy * vz - uz * vy;
    float ny = uz * vx - ux * vz;
    float nz = ux * vy - uy * vx;

    float len = sqrtf(nx * nx + ny * ny + nz * nz);
    if (len > 1e-6f) {
        nx /= len;
        ny /= len;
        nz /= len;
    }

    return createVector(nx, ny, nz);
}

void resizeFaceArray(Mesh* m, int size) {
    if (size < 0) {
        size = m->faceAmount - size;
    }

    Face* tempFaces = realloc(m->faces, sizeof(Face) * size);
    if (tempFaces == NULL) {
        printf("Failed to allocate memory to resizing face array\n");
        return;
    }
    m->faces = tempFaces;

    Vector* tempNormals = realloc(m->normals, sizeof(Vector) * size);
    if (tempNormals == NULL) {
        printf("Failed to allocate memory to resizing normals array\n");
        return;
    }
    m->normals = tempNormals;

    m->faceAmount = size;
}

void resizeVectorArray(Mesh* m, int size) {
    if (size < 0) {
        size = m->vectorAmount - size;
    }

    Vector* temp = realloc(m->vectors, sizeof(Vector) * size);
    if (temp == NULL) {
        printf("Failed to allocate memory to resizing vector array\n");
        return;
    }
    m->vectors = temp;
    m->vectorAmount = size;
}

Mesh createMesh() {
    Mesh m;
    m.gpuVertices = NULL;
    m.gpuVertexCount = 0;
    m.gpuVertexBuffer = NULL;
    m.faces = (Face*)malloc(sizeof(Face));
    m.normals = (Vector*)malloc(sizeof(Vector));
    m.vectors = (Vector*)malloc(sizeof(Vector));
    m.vectorAmount = 0;
    m.faceAmount = 0;
    m.texture = "./source/res/images/default.lif";
    m.position = blankVector();
    m.rotation = blankVector();
    m.scale = 1.0f;
    m.collision = 0;
    return m;
}

/**
 * @brief Rescales by scale rel to current
 * 
 * @param mesh 
 * @param scale 
 */
void rescaleMesh(Mesh* mesh, float scale) {
    mesh->scale = scale;

    for (int i = 0; i < mesh->vectorAmount; i++) {
        linearMult(&mesh->vectors[i], scale);
    }
}

static UV computeTriplanarUVForPoint(Vector p, Vector n, float scale) {
    float ax = fabsf(n.x);
    float ay = fabsf(n.y);
    float az = fabsf(n.z);

    if (ax >= ay && ax >= az) {
        return (UV){ p.z * scale, p.y * scale };
    } else if (ay >= ax && ay >= az) {
        return (UV){ p.x * scale, p.z * scale };
    } else {
        return (UV){ p.x * scale, p.y * scale };
    }
}

void buildMeshGpuVertices(Mesh* mesh) {
    if (!mesh || mesh->faceAmount <= 0) {
        free(mesh->gpuVertices);
        mesh->gpuVertices = NULL;
        mesh->gpuVertexCount = 0;
        return;
    }

    int vertCount = mesh->faceAmount * 3;
    GpuVertex* verts = (GpuVertex*)malloc((size_t)vertCount * sizeof(GpuVertex));
    if (!verts) {
        fprintf(stderr, "Error: Failed to allocate GPU vertex buffer\n");
        return;
    }

    const float fallbackUVScale = 1.0f;

    for (int i = 0; i < mesh->faceAmount; i++) {
        const Face* f = &mesh->faces[i];
        Vector n = (mesh->normals != NULL) ? mesh->normals[i] : computeFaceNormal(*f);

        UV uv0 = computeTriplanarUVForPoint(*f->a, n, fallbackUVScale);
        UV uv1 = computeTriplanarUVForPoint(*f->b, n, fallbackUVScale);
        UV uv2 = computeTriplanarUVForPoint(*f->c, n, fallbackUVScale);

        int base = i * 3;
        verts[base + 0] = (GpuVertex){ .pos = *f->a, .normal = n, .uv = uv0 };
        verts[base + 1] = (GpuVertex){ .pos = *f->b, .normal = n, .uv = uv1 };
        verts[base + 2] = (GpuVertex){ .pos = *f->c, .normal = n, .uv = uv2 };
    }

    free(mesh->gpuVertices);
    mesh->gpuVertices = verts;
    mesh->gpuVertexCount = vertCount;
}

bool uploadMeshToGPU(Mesh* mesh, SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass) {
    if (!mesh || !device || !copyPass || !mesh->gpuVertices || mesh->gpuVertexCount <= 0) {
        return false;
    }

    Uint32 byteSize = (Uint32)((size_t)mesh->gpuVertexCount * sizeof(GpuVertex));

    if (mesh->gpuVertexBuffer) {
        SDL_ReleaseGPUBuffer(device, mesh->gpuVertexBuffer);
        mesh->gpuVertexBuffer = NULL;
    }

    SDL_GPUBufferCreateInfo bufInfo = {0};
    bufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufInfo.size = byteSize;

    mesh->gpuVertexBuffer = SDL_CreateGPUBuffer(device, &bufInfo);
    if (!mesh->gpuVertexBuffer) {
        fprintf(stderr, "Failed to create GPU vertex buffer: %s\n", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo xferInfo = {0};
    xferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    xferInfo.size = byteSize;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &xferInfo);
    if (!transfer) {
        fprintf(stderr, "Failed to create GPU transfer buffer: %s\n", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, mesh->gpuVertexBuffer);
        mesh->gpuVertexBuffer = NULL;
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (!mapped) {
        fprintf(stderr, "Failed to map GPU transfer buffer: %s\n", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUBuffer(device, mesh->gpuVertexBuffer);
        mesh->gpuVertexBuffer = NULL;
        return false;
    }

    memcpy(mapped, mesh->gpuVertices, byteSize);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUTransferBufferLocation src = {
        .transfer_buffer = transfer,
        .offset = 0
    };
    SDL_GPUBufferRegion dst = {
        .buffer = mesh->gpuVertexBuffer,
        .offset = 0,
        .size = byteSize
    };
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);

    // Release transfer buffer after the command buffer using copyPass is submitted
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return true;
}

void calculateMeshNormals(Mesh* m) {
    for (int i = 0; i < m->faceAmount; i++) {
        m->normals[i] = computeFaceNormal(m->faces[i]);
    }
}

void addFaceToMesh(Mesh* m, Face* f) {
    Face newFace;
    newFace.hasUVs = f->hasUVs;

    int facesSelected = 0;

    for (int i = 0; i < m->vectorAmount; i++) {
        Vector v = m->vectors[i];

        if (!(facesSelected & 0b100) && vectorEquals(*f->a, v)) {
            facesSelected |= 0b100;
            newFace.a = &m->vectors[i];
        } else if (!(facesSelected & 0b010) && vectorEquals(*f->b, v)) {
            facesSelected |= 0b010;
            newFace.b = &m->vectors[i];
        } else if (!(facesSelected & 0b001) && vectorEquals(*f->c, v)) {
            facesSelected |= 0b001;
            newFace.c = &m->vectors[i];
        }

        if (facesSelected == 7) {
            break;
        }
    }
    
    // Note: Keep track of indices rather than pointers across reallocs
    int idxA = (facesSelected & 0b100) ? (int)(newFace.a - m->vectors) : -1;
    int idxB = (facesSelected & 0b010) ? (int)(newFace.b - m->vectors) : -1;
    int idxC = (facesSelected & 0b001) ? (int)(newFace.c - m->vectors) : -1;

    if (idxA == -1) {
        Vector v = createVector(f->a->x, f->a->y, f->a->z);
        resizeVectorArray(m, -1);
        idxA = m->vectorAmount - 1;
        m->vectors[idxA] = v;
    }

    if (idxB == -1) {
        Vector v = createVector(f->b->x, f->b->y, f->b->z);
        resizeVectorArray(m, -1);
        idxB = m->vectorAmount - 1;
        m->vectors[idxB] = v;
    }

    if (idxC == -1) {
        Vector v = createVector(f->c->x, f->c->y, f->c->z);
        resizeVectorArray(m, -1);
        idxC = m->vectorAmount - 1;
        m->vectors[idxC] = v;
    }

    newFace.a = &m->vectors[idxA];
    newFace.b = &m->vectors[idxB];
    newFace.c = &m->vectors[idxC];

    resizeFaceArray(m, -1);
    int newIdx = m->faceAmount - 1;
    m->faces[newIdx] = newFace;
    m->normals[newIdx] = computeFaceNormal(newFace);
}

Mesh loadMeshFromBin(const char* filename) {
    Mesh mesh = createMesh();
    
    BinaryFileContents fileData = readBinFromFile(filename);
    if (fileData.content == NULL || fileData.size < 8) {
        fprintf(stderr, "Error: Failed to read or file too small: %s\n", filename);
        return mesh;
    }

    uint8_t* ptr = fileData.content;

    uint16_t vertexAmount = ptr[0] | (ptr[1] << 8);
    uint16_t faceAmount = ptr[2] | (ptr[3] << 8);
    uint8_t flags = ptr[4];
    ptr += 8;

    size_t baseSize = 8 + (size_t)vertexAmount * 6 + (size_t)faceAmount * 6;
    size_t normalsBytes = (size_t)faceAmount * 6;
    size_t uvBytes = (size_t)faceAmount * 24; // 3 corners * 2 floats (4 bytes each)

    if (fileData.size < baseSize) {
        fprintf(stderr, "Error: Truncated mesh file: %s\n", filename);
        free(fileData.content);
        return mesh;
    }

    resizeVectorArray(&mesh, vertexAmount);
    for (int i = 0; i < vertexAmount; i++) {
        int16_t x = (int16_t)(ptr[0] | (ptr[1] << 8));
        int16_t y = (int16_t)(ptr[2] | (ptr[3] << 8));
        int16_t z = (int16_t)(ptr[4] | (ptr[5] << 8));
        ptr += 6;

        mesh.vectors[i] = createVector((float)x * RESIZE_MULT, (float)y * RESIZE_MULT, (float)z * RESIZE_MULT);
    }

    
    resizeFaceArray(&mesh, faceAmount);
    for (int i = 0; i < faceAmount; i++) {
        uint16_t idxA = ptr[0] | (ptr[1] << 8);
        uint16_t idxB = ptr[2] | (ptr[3] << 8);
        uint16_t idxC = ptr[4] | (ptr[5] << 8);
        ptr += 6;

        Face f;
        f.a = &mesh.vectors[idxA];
        f.b = &mesh.vectors[idxB];
        f.c = &mesh.vectors[idxC];
        f.hasUVs = false;
        
        mesh.faces[i] = f;
    }

    
    bool hasNormals = (flags & MDL_FLAG_NORMALS) || (flags == 0 && fileData.size >= baseSize + normalsBytes);
    size_t currentOffset = baseSize;

    if (hasNormals && fileData.size >= currentOffset + normalsBytes) {
        for (int i = 0; i < faceAmount; i++) {
            int16_t nx = (int16_t)(ptr[0] | (ptr[1] << 8));
            int16_t ny = (int16_t)(ptr[2] | (ptr[3] << 8));
            int16_t nz = (int16_t)(ptr[4] | (ptr[5] << 8));
            ptr += 6;

            mesh.normals[i] = createVector(
                (float)nx * NORMAL_INV_SCALE,
                (float)ny * NORMAL_INV_SCALE,
                (float)nz * NORMAL_INV_SCALE
            );
        }
        currentOffset += normalsBytes;
    } else {
        calculateMeshNormals(&mesh);
    }

    free(fileData.content);

    buildMeshGpuVertices(&mesh);

    return mesh;
}

void destroyMesh(Mesh* mesh) {
    // WILL DESTROY ALL SHALLOW CLONED MESHES
    free(mesh->vectors);
    free(mesh->faces);
    free(mesh->normals);
    free(mesh->gpuVertices);
    mesh->gpuVertices = NULL;
    mesh->gpuVertexCount = 0;
}