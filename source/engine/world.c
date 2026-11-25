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

int addMeshToWorld(World* world, Mesh* mesh) {

    {
        Mesh* temp = (Mesh*)realloc(world->meshes, sizeof(Mesh) * (world->worldSize + 1));
        
        if (temp == NULL) {
            printf("Bad realloc\n");
            return -1;
        }
    
        world->meshes = temp;
        world->meshes[world->worldSize] = *mesh;
    }
    uint32_t* temp = (uint32_t*)realloc(world->drawOrder, sizeof(uint32_t) * (world->worldSize + 1));
    
    if (temp == NULL) {
        printf("Bad realloc\n");
        return -1;
    }
    
    world->drawOrder = temp;
    world->drawOrder[world->worldSize] = world->worldSize;

    return world->worldSize++;
}

void removeMeshFromWorld(World* world, int index) {
    // freeing needs a seperate call
    Mesh holdMesh = world->meshes[world->worldSize - 1];

    {
        Mesh* temp = (Mesh*)realloc(world->meshes, sizeof(Mesh) * (world->worldSize - 1));
        
        if (temp == NULL) {
            printf("Bad realloc\n");
            return;
        }
    
        world->meshes = temp;
    }

    uint32_t* temp = (uint32_t*)realloc(world->drawOrder, sizeof(uint32_t) * (world->worldSize - 1));
    
    if (temp == NULL) {
        printf("Bad realloc\n");
        return;
    }

    world->meshes[index] = holdMesh;
    world->drawOrder[world->worldSize - 1] = index;
    world->worldSize--;
}

void destroyWorld(World* world) {
    for (int i = 0; i < world->worldSize; i++) {
        destroyMesh(&world->meshes[i]);
    }
    free(world->meshes);
    free(world->drawOrder);
}