#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include "vector.h"

Vector createVector(float x, float y, float z) {
    Vector v = { .x = x, .y = y, .z = z } ;
    return v;
}

Vector blankVector() {
    return createVector(0, 0, 0);
}

void linearMult(Vector* v, float a) {
    v->x *= a;
    v->y *= a;
    v->z *= a;
}

void linearAdd(Vector* v, float a) {
    v->x *= a;
    v->y *= a;
    v->z *= a;
}

void linearDiv(Vector* v, float a) {
    linearMult(v, 1 / a);
}

void linearSub(Vector* v, float a) {
    linearAdd(v, -a);
}

int vectorEquals(Vector v1, Vector v2) {
    return !(v1.x - v2.x + v1.y - v2.y + v1.z - v2.z);
}