#ifndef INSTANCES_H
#define INSTANCES_H

#include "vector.h"

struct Camera {
    Vector position;
    Vector rotation;
};

typedef struct {
    Vector movementDirection;
    Vector position;
    Vector rotation;
    Vector rotationDirection;
} Body;

#endif