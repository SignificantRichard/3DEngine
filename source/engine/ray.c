#include <float.h>
#include <math.h>
#include "ray.h"
#include "vector.h"
#include "mesh.h"
#include "world.h"
#include "face.h"

RaycastResult raycast(Vector origin, Vector direction, World* world) {
    RaycastResult r = {
        .position = blankVector(),
        .distance = FLT_MAX,
        .hit = 0
    };

    const float EPSILON = 1e-6f;

    for (int i = 0; i < world->worldSize; i++) {
        Mesh mesh = world->meshes[i];
        Vector meshPos = mesh.position;

        for (int j = 0; j < mesh.faceAmount; j++) {
            Face face = mesh.faces[j];

            Vector a = *face.a;
            vectorAdd(&a, &meshPos);

            Vector b = *face.b;
            vectorAdd(&b, &meshPos);

            Vector c = *face.c;
            vectorAdd(&c, &meshPos);

            Vector edge1 = b;
            vectorSub(&edge1, &a);

            Vector edge2 = c;
            vectorSub(&edge2, &a);

            Vector p = cross(direction, edge2);
            float det = dot(edge1, p);

            if (fabsf(det) < EPSILON) {
                continue;
            }

            float invDet = 1.0f / det;

            Vector tVec = origin;
            vectorSub(&tVec, &a);

            float u = dot(tVec, p) * invDet;
            if (u < 0.0f || u > 1.0f) {
                continue;
            }

            Vector q = cross(tVec, edge1);
            float v = dot(direction, q) * invDet;
            if (v < 0.0f || u + v > 1.0f) {
                continue;
            }

            // t is normalized to the length of `direction` (0 = origin, 1 = end)
            float t = dot(edge2, q) * invDet;
            if (t <= EPSILON || t > 1.0f) {
                continue;
            }

            Vector offset = direction;
            linearMult(&offset, t);
            double worldDist = (double)sqrtf(dot(offset, offset));

            if (worldDist < r.distance) {
                r.distance = worldDist;
                r.hit = 1;

                r.position = origin;
                vectorAdd(&r.position, &offset);
            }
        }
    }

    return r;
}