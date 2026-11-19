#include "ray.h"
#include "vector.h"
#include "mesh.h"
#include "world.h"
#include "face.h"

RaycastResult raycast(Vector origin, Vector direction, World* world) {
    RaycastResult r = (RaycastResult){ .position = blankVector(), .distance = 0xffffffff, .hit = 0 };
    
    linearMult(&direction, -1.0);
    // R(t) = O + tD
    // P(u, v) = (1 - u - v)V0 + uV1 + vV2

    // O + tD = (1 - u - v)V0 + uV1 + vV2
    // E1 = V1 - V0
    // E2 = V2 - V0
    // -tD + uE1 + vE2 = O + V0

    for (int i = 0; i < world->worldSize; i++) {
        Mesh mesh = world->meshes[i];
        for (int j = 0; j < mesh.faceAmount; j++) {
            Face face = mesh.faces[j];

            // a = 0
            // b = 1
            // c = 2
            Vector edge1 = *face.b;
            vectorSub(&edge1, face.b);

            Vector edge2 = *face.c;
            vectorSub(&edge2, face.c);


            // find u
            Vector p = cross(direction, edge2);
            float det = dot(edge1, p); // TODO: make sure det != 0
            Vector t = origin;
            vectorSub(&t, face.a);

            float u = dot(t, p) / det;

            if (u < 0.0 || u > 1.0) {
                continue;
            }

            Vector q = cross(t, edge1);

            float v = dot(direction, q) / det;

            if (v < 0.0 || u > 1.0) {
                continue;
            }

            float dist = dot(edge2, q) / det;

            if (dist < r.distance) {
                r.distance = dist;
                r.hit = 1;

                r.position = origin;
                Vector tDirection = direction;
                linearMult(&tDirection, -r.distance);
                vectorAdd(&r.position, &tDirection);
            }
        }
    }
    
    return r;
}