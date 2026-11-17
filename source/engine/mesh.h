#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "face.h"

typedef struct {
    char** textures; // texture names. faces use offset to determine texture
    Face* faces; // array of pointers to vector array
    int faceAmount;
    Vector* vectors; // vector array
    int vectorAmount;
} Mesh;

Mesh createMesh();
Mesh loadMeshFromBin(const char* filename);

#endif