#ifndef FACE_H
#define FACE_H

#include <stdbool.h>
#include "vector.h"

typedef struct {
    float u;
    float v;
} UV;

typedef struct {
    Vector* a;
    Vector* b;
    Vector* c;
    UV uvA;
    UV uvB;
    UV uvC;
    bool hasUVs;
} Face;

#endif