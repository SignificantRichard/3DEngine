#ifndef MESH_EXTENDED_H
#define MESH_EXTENDED_H

#include "world.h"
#include "mesh.h"

int loadAndAddModel(World* world, const char* filename);
int addClonedModelShallow(World* world, Mesh mesh);
int addClonedModelDeep(World* world, const Mesh* mesh);

#endif