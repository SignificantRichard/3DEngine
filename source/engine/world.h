#ifndef WORLD_H
#define WORLD_H

#include "mesh.h"
#include <stdint.h>

typedef struct {
    Mesh* meshes;
    int worldSize;
    uint32_t* drawOrder;
} World;

World createWorld();
Mesh* addMeshToWorld(World* world, Mesh* mesh);

#endif