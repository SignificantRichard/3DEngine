#include <stdlib.h>
#include <stdio.h>
#include "face.h"
#include "vector.h"
#include "mesh.h"
#include "files.h"

#define RESIZE_MULT 0.0001f

void resizeFaceArray(Mesh* m, int size) {
    if (size < 0) {
        size = m->faceAmount - size;
    }

    Face* temp = realloc(m->faces, sizeof(Face) * size);
    if (temp == NULL) {
        printf("Failed to allocate memory to resizing face array\n");
        return;
    }
    m->faces = temp;
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
    m.vectors = (Vector*)malloc(sizeof(Vector));
    m.vectorAmount = 0;
    m.faceAmount = 0;
    m.position = blankVector();
    m.rotation = blankVector();
    return m;
}

void addFaceToMesh(Mesh* m, Face* f) {
    // squash uses existing vector

    Face newFace;

    int facesSelected = 0;

    for (int i = 0; i < m->vectorAmount; i++) {
        Vector v = m->vectors[i];

        if (vectorEquals(*f->a, v)) {
            
            facesSelected += 4;
            newFace.a = &m->vectors[i];

        } else if (vectorEquals(*f->b, v)) {
            
            facesSelected += 2;
            newFace.b = &m->vectors[i];

        } else if (vectorEquals(*f->c, v)) {
            
            facesSelected += 1;
            newFace.c = &m->vectors[i];

        }

        if (facesSelected == 7) {
            break;
        }
    }
    
    if (facesSelected & 0b00000100) {
        Vector v = createVector(f->a->x, f->a->y, f->a->z);
        resizeVectorArray(m, -1);
        m->vectors[m->vectorAmount - 1] = v;
    }

    if (facesSelected & 0b00000010) {
        Vector v = createVector(f->b->x, f->b->y, f->b->z);
        resizeVectorArray(m, -1);
        m->vectors[m->vectorAmount - 1] = v;
    }

    if (facesSelected & 0b00000001) {
        Vector v = createVector(f->c->x, f->c->y, f->c->z);
        resizeVectorArray(m, -1);
        m->vectors[m->vectorAmount - 1] = v;
    }

    resizeFaceArray(m, -1);
    m->faces[m->faceAmount - 1] = newFace;

    f = &m->faces[m->faceAmount - 1];
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
    ptr += 8;

    resizeVectorArray(&mesh, vertexAmount);
    for (int i = 0; i < vertexAmount; i++) {
        int16_t x = ptr[0] | (ptr[1] << 8);
        int16_t y = ptr[2] | (ptr[3] << 8);
        int16_t z = ptr[4] | (ptr[5] << 8);
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
        
        mesh.faces[i] = f;
    }

    free(fileData.content);

    printf("Successfully loaded mesh: %d vertices, %d faces\n", vertexAmount, faceAmount);
    return mesh;
}