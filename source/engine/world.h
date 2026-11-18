#ifndef WORLD_H
#define WORLD_H

#include "mesh.h"

typedef struct {
    Mesh* meshes;
    int worldSize;
} World;

World createWorld();
Mesh* addMeshToWorld(World* world, Mesh* mesh);

#endif