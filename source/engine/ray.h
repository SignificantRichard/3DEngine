#ifndef RAY_H
#define RAY_H

#include "vector.h"
#include "world.h"

typedef struct {
    Vector position;
    Vector normal;
    double distance;
    int hit;
} RaycastResult;

RaycastResult raycast(Vector origin, Vector direction, World* world);

#endif