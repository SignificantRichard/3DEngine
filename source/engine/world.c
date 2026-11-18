#include <stdlib.h>
#include <stdio.h>
#include "world.h"

World createWorld() {
    World world;
    world.meshes = (Mesh*)malloc(sizeof(Mesh));
    world.worldSize = 0;
    return world;
}

Mesh* addMeshToWorld(World* world, Mesh* mesh) {
    Mesh* temp = (Mesh*)realloc(world->meshes, sizeof(Mesh) * ++world->worldSize);
    
    if (temp == NULL) {
        printf("Bad realloc\n");
        return mesh;
    }

    world->meshes = temp;
    world->meshes[world->worldSize - 1] = *mesh;
    return &world->meshes[world->worldSize - 1];
}