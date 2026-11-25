#include <float.h>
#include <math.h>
#include <stdio.h>
#include "ray.h"
#include "vector.h"
#include "mesh.h"
#include "world.h"
#include "face.h"

static Vector rotatePointXYZ(Vector v, Vector rot) {
    float cx = cosf(rot.x), sx = sinf(rot.x);
    float cy = cosf(rot.y), sy = sinf(rot.y);
    float cz = cosf(rot.z), sz = sinf(rot.z);

    // Rotate around X axis
    float y1 = v.y * cx - v.z * sx;
    float z1 = v.y * sx + v.z * cx;
    float x1 = v.x;

    // Rotate around Y axis
    float x2 = x1 * cy + z1 * sy;
    float z2 = -x1 * sy + z1 * cy;
    float y2 = y1;

    // Rotate around Z axis
    Vector out;
    out.x = x2 * cz - y2 * sz;
    out.y = x2 * sz + y2 * cz;
    out.z = z2;
    return out;
}

static Vector normalizeVec(Vector v) {
    float lenSq = dot(v, v);
    if (lenSq <= 1e-12f) {
        return blankVector();
    }
    float invLen = 1.0f / sqrtf(lenSq);
    Vector out = v;
    linearMult(&out, invLen);
    return out;
}

RaycastResult raycast(Vector origin, Vector direction, World* world) {
    RaycastResult r = {
        .position = blankVector(),
        .normal = blankVector(),
        .distance = FLT_MAX,
        .hit = 0
    };

    const float EPSILON = 1e-6f;

    float maxDist = sqrtf(dot(direction, direction));
    if (maxDist <= EPSILON) {
        return r;
    }
    Vector rayDir = normalizeVec(direction);

    for (int i = world->worldSize - 1; i >= 0; i--) {
        Mesh mesh = world->meshes[world->drawOrder[i]]; // use draw order to shortcut

        if (!mesh.collision) {
            continue;
        }
        
        Vector meshPos = mesh.position;
        Vector meshRot = mesh.rotation;

        for (int j = mesh.faceAmount - 1; j >= 0; j--) {
            Face face = mesh.faces[j];

            Vector a = rotatePointXYZ(*face.a, meshRot);
            vectorAdd(&a, &meshPos);

            Vector b = rotatePointXYZ(*face.b, meshRot);
            vectorAdd(&b, &meshPos);

            Vector c = rotatePointXYZ(*face.c, meshRot);
            vectorAdd(&c, &meshPos);

            Vector edge1 = b;
            vectorSub(&edge1, &a);

            Vector edge2 = c;
            vectorSub(&edge2, &a);

            Vector p = cross(rayDir, edge2);
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
            float v = dot(rayDir, q) * invDet;
            if (v < 0.0f || u + v > 1.0f) {
                continue;
            }

            float t = dot(edge2, q) * invDet;
            if (t <= EPSILON || t > maxDist) {
                continue;
            }

            if ((double)t < r.distance) {
                r.distance = (double)t;
                r.hit = 1;
                r.normal = computeFaceNormal(face);

                Vector offset = rayDir;
                linearMult(&offset, t);
                r.position = origin;
                vectorAdd(&r.position, &offset);
            }
        }
    }

    return r;
}