#ifndef VECTOR_H
#define VECTOR_H

typedef struct {
    float x;
    float y;
    float z;
} Vector;

Vector createVector(float x, float y, float z);
Vector blankVector();

void linearMult(Vector* v, float a);
void linearAdd(Vector* v, float a);
void linearDiv(Vector* v, float a);
void linearSub(Vector* v, float a);
int vectorEquals(Vector v1, Vector v2);

void vectorAdd(Vector* v, Vector* a);
void vectorSub(Vector* v, Vector* a);

float vectorMag(Vector v, Vector a);

#endif