#ifndef FACE_H
#define FACE_H

#include "vector.h"

typedef struct {
    Vector* a;
    Vector* b;
    Vector* c;
    int texture;
} Face;

Face createFace(Vector* a, Vector* b, Vector* c);

#endif