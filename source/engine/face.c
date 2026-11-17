#include "vector.h"
#include "face.h"

Face createFace(Vector* a, Vector* b, Vector* c) {
    Face f = { .a = a, .b = b, .c = c };
    return f;
}