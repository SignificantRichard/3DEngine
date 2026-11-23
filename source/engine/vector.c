#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include "vector.h"

#define PI 3.14159265359

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
    v->x += a;
    v->y += a;
    v->z += a;
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

void vectorAdd(Vector* v, Vector* a) {
    v->x += a->x;
    v->y += a->y;
    v->z += a->z;
}

void vectorSub(Vector* v, Vector* a) {
    v->x -= a->x;
    v->y -= a->y;
    v->z -= a->z;
}

float vectorMag(Vector v, Vector a) {
    v.x -= a.x;
    v.y -= a.y;
    v.z -= a.z;

    return cbrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float dot(Vector a, Vector b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector cross(Vector a, Vector b) {
    Vector out = blankVector();

    out.x = a.y * b.z - a.z * b.y;
    out.y = a.z * b.x - a.x * b.z;
    out.z = a.x * b.y - a.y * b.x;
    
    return out;
}

Vector lookVector(Vector rot) {
    float cosPitch = cosf(rot.x);
    float sinPitch = sinf(rot.x);
    float cosYaw   = cosf(rot.y);
    float sinYaw   = sinf(rot.y);

    float x = cosPitch * sinYaw;
    float y = -sinPitch;
    float z = cosPitch * cosYaw;

    return createVector(x, y, z);
}

Vector rightVector(Vector rot) {
    Vector rightRot = rot;
    rightRot.x = 0.0f;
    rightRot.y += PI * 0.5f;
    return lookVector(rightRot);
}

Vector upVector(Vector rot) {
    float upPitch = rot.x - (PI * 0.5f);
    float cosUpPitch = cosf(upPitch);
    float sinUpPitch = sinf(upPitch);
    float cosYaw     = cosf(rot.y);
    float sinYaw     = sinf(rot.y);

    float x = cosUpPitch * sinYaw;
    float y = -sinUpPitch;
    float z = cosUpPitch * cosYaw;

    return createVector(x, y, z);
}