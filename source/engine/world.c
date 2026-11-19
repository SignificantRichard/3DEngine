#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "world.h"

World createWorld() {
    World world;
    world.meshes = (Mesh*)malloc(sizeof(Mesh));
    world.drawOrder = (uint32_t*)malloc(sizeof(uint32_t));
    world.worldSize = 0;
    return world;
}

Mesh* addMeshToWorld(World* world, Mesh* mesh) {

    {
        Mesh* temp = (Mesh*)realloc(world->meshes, sizeof(Mesh) * ++world->worldSize);
        
        if (temp == NULL) {
            printf("Bad realloc\n");
            return mesh;
        }
    
        world->meshes = temp;
        world->meshes[world->worldSize - 1] = *mesh;
    }
    uint32_t* temp = (uint32_t*)realloc(world->drawOrder, sizeof(uint32_t) * world->worldSize);
    
    if (temp == NULL) {
        printf("Bad realloc\n");
        return mesh;
    }
    
    world->drawOrder = temp;
    world->drawOrder[world->worldSize - 1] = world->worldSize - 1;

    return &world->meshes[world->worldSize - 1];
}