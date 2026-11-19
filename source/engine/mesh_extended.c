#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "mesh.h"
#include "world.h"

int loadAndAddModel(World* world, const char* filename) {
    Mesh mesh = loadMeshFromBin(filename);
    return addMeshToWorld(world, &mesh);
}

/**
 * @brief Creates a clone with shared vectors
 * 
 * @param world 
 * @param mesh 
 * @return int 
 */
int addClonedModelShallow(World* world, Mesh mesh) {
    return addMeshToWorld(world, &mesh); // no need to free memory. would cause issues
}

/**
 * @brief Creates a completely new mesh
 * 
 * @param src 
 * @return Mesh 
 */
Mesh cloneMeshDeep(const Mesh* src) {
    Mesh copy = *src;
    copy.vectors = NULL;
    copy.faces = NULL;
    copy.textures = NULL;

    if (src == NULL) {
        copy.faceAmount = 0;
        copy.vectorAmount = 0;
        return copy;
    }

    if (src->vectorAmount > 0 && src->vectors != NULL) {
        copy.vectors = malloc(sizeof(Vector) * (size_t)src->vectorAmount);
        if (copy.vectors == NULL) {
            copy.vectorAmount = 0;
            copy.faceAmount = 0;
            return copy;
        }
        memcpy(copy.vectors, src->vectors, sizeof(Vector) * (size_t)src->vectorAmount);
    }

    if (src->faceAmount > 0 && src->faces != NULL) {
        copy.faces = malloc(sizeof(Face) * (size_t)src->faceAmount);
        if (copy.faces == NULL) {
            free(copy.vectors);
            copy.vectors = NULL;
            copy.vectorAmount = 0;
            copy.faceAmount = 0;
            return copy;
        }
        memcpy(copy.faces, src->faces, sizeof(Face) * (size_t)src->faceAmount);

        for (int i = 0; i < src->faceAmount; i++) {
            Vector* oldPts[3] = { src->faces[i].a, src->faces[i].b, src->faces[i].c };
            Vector** newPts[3] = { &copy.faces[i].a, &copy.faces[i].b, &copy.faces[i].c };

            for (int j = 0; j < 3; j++) {
                if (oldPts[j] != NULL && src->vectors != NULL) {
                    ptrdiff_t offset = oldPts[j] - src->vectors;
                    if (offset >= 0 && offset < src->vectorAmount) {
                        *newPts[j] = &copy.vectors[offset];
                    }
                }
            }
        }
    }

    return copy;
}

int addClonedModelDeep(World* world, const Mesh* mesh) {
    if (world == NULL || mesh == NULL) {
        return -1;
    }

    Mesh deepCopy = cloneMeshDeep(mesh);
    int index = addMeshToWorld(world, &deepCopy);

    if (index < 0) {
        free(deepCopy.vectors);
        free(deepCopy.faces);
        free(deepCopy.textures);
    }

    return index;
}