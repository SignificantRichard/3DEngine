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

static Vector computeFaceNormal(Face f) {
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

void calculateMeshNormals(Mesh* m) {
    for (int i = 0; i < m->faceAmount; i++) {
        m->normals[i] = computeFaceNormal(m->faces[i]);
    }
}

void addFaceToMesh(Mesh* m, Face* f) {
    // squash uses existing vector
    Face newFace;
    newFace.uvA = f->uvA;
    newFace.uvB = f->uvB;
    newFace.uvC = f->uvC;
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

    // Each face is 3 * uint16_t indices (6 bytes total)
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
        f.uvA = (UV){ 0.0f, 0.0f };
        f.uvB = (UV){ 0.0f, 0.0f };
        f.uvC = (UV){ 0.0f, 0.0f };
        f.hasUVs = false;
        
        mesh.faces[i] = f;
    }

    // Support both flagged files and legacy unflagged files that included normals
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

    // Read per-face corner UVs if present
    bool loadedUVs = false;
    if ((flags & MDL_FLAG_UVS) && fileData.size >= currentOffset + uvBytes) {
        for (int i = 0; i < faceAmount; i++) {
            mesh.faces[i].uvA.u = readFloatLE(ptr + 0);
            mesh.faces[i].uvA.v = readFloatLE(ptr + 4);
            mesh.faces[i].uvB.u = readFloatLE(ptr + 8);
            mesh.faces[i].uvB.v = readFloatLE(ptr + 12);
            mesh.faces[i].uvC.u = readFloatLE(ptr + 16);
            mesh.faces[i].uvC.v = readFloatLE(ptr + 20);
            mesh.faces[i].hasUVs = true;
            ptr += 24;
        }
        loadedUVs = true;
    }

    free(fileData.content);

    printf("Successfully loaded mesh: %d vertices, %d faces (UVs: %s)\n",
           vertexAmount, faceAmount, loadedUVs ? "yes" : "triplanar fallback");
    return mesh;
}