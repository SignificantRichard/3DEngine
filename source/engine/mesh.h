#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "face.h"

typedef struct {
    char* texture; // texture names. faces use offset to determine texture
    Face* faces; // array of pointers to vector array
    Vector* normals; // array of face normals (parallel to faces)
    int faceAmount;
    Vector* vectors; // vector array
    int vectorAmount;
    Vector position;
    Vector rotation;
    float scale;
} Mesh;

Mesh createMesh();
Mesh loadMeshFromBin(const char* filename);
void addFaceToMesh(Mesh* m, Face* f);
void calculateMeshNormals(Mesh* m);
void rescaleMesh(Mesh* mesh, float scale);

#endif