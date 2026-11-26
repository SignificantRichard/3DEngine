#ifndef MESH_H
#define MESH_H

#include <stdint.h>
#include <SDL3/SDL.h>
#include "vector.h"
#include "face.h"

typedef struct {
    Vector pos;
    Vector normal;
    UV uv;
} GpuVertex;

typedef struct {
    char* texture; // texture names.
    Face* faces; // array of pointers to vector array
    int faceAmount;
    Vector* normals; // array of face normals (parallel to faces)
    int normalAmount;
    Vector* vectors; // vector array
    int vectorAmount;
    Vector position;
    Vector rotation;
    float scale;
    uint8_t collision;
    GpuVertex* gpuVertices;
    int gpuVertexCount;
    SDL_GPUBuffer* gpuVertexBuffer;
} Mesh;

Mesh createMesh();
Mesh loadMeshFromBin(const char* filename);
void addFaceToMesh(Mesh* m, Face* f);
void calculateMeshNormals(Mesh* m);
void rescaleMesh(Mesh* mesh, float scale);
Vector computeFaceNormal(Face f);
void buildMeshGpuVertices(Mesh* mesh);
bool uploadMeshToGPU(Mesh* mesh, SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass);
void destroyMesh(Mesh* mesh);

#endif