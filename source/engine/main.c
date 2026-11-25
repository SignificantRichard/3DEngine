/**
 * @file main.c
 * @author Wen Xin Gao (gaowenxin2007@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-11-18
 * 
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
#include "image.h"
#include "asset_manager.h"
#include "mesh.h"
#include "mesh_extended.h"
#include "world.h"
#include "instances.h"
#include "render.h"
#include "ray.h"
#include "ui.h"
#include "sound.h"
#include "controller.h"
#include "../world/entry.h"

#include "settings.h"

void gameLoop(World* world, AssetManager* assetManager, SDL_Renderer* renderer) {
    SDL_Gamepad* gamepad = NULL;

    // create test ui
    TTF_TextEngine* textEngine = createRendererEngine(renderer);
    TTF_Font* font = createFirstAvailableFont(12);
    Label label = createLabel(textEngine, font, "TEST BUILD: TEST BUILD", 0, 0);

    Body player = { .position = blankVector(), .rotation = blankVector(), .movementDirection = blankVector() };
    struct Camera camera = { .position = blankVector(), .rotation = blankVector() };

    bool running = true;

    const uint64_t freq = SDL_GetPerformanceFrequency();
    const double targetFrameSec = (frameRate > 0.0) ? (1.0 / (double)frameRate) : 0.0;

    uint64_t frameStart = SDL_GetPerformanceCounter();
    double deltaTimeMs = (targetFrameSec > 0.0) ? (targetFrameSec * 1000.0) : 16.667;
    uint64_t tick = 0;

    while (running) {
        double fps = (deltaTimeMs > 0.0) ? (1000.0 / deltaTimeMs) : 0.0;
        char strFPS[64];
        snprintf(strFPS, sizeof(strFPS), "FPS: %.1f", fps);
        setLabelText(&label, strFPS);

        // events and input
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_GAMEPAD_ADDED:
                    printf("Gamepad added (id=%u)\n", (unsigned)event.gdevice.which);
                    fflush(stdout);
                    if (!gamepad) {
                        gamepad = SDL_OpenGamepad(event.gdevice.which);
                    }
                    break;
                case SDL_EVENT_GAMEPAD_REMOVED:
                    if (gamepad && event.gdevice.which == SDL_GetGamepadID(gamepad)) {
                        SDL_CloseGamepad(gamepad);
                        gamepad = getGamepad();
                    }
                    break;
            }
        }

        sortDrawOrder(world, camera.position, 0, world->worldSize - 1);

        { // gravity logic
            RaycastResult ray = raycast(camera.position, createVector(0.0f, -10.0f, 0.0f), world);
            if (!ray.hit) {
                if (player.velocity.y > -TERMINAL_VELOCITY) {
                    player.velocity.y -= GRAVITY * deltaTimeMs;
                }
                player.touchingGround = 0;
            } else {
                if (player.velocity.y <= 0.0f) {
                    if (ray.distance > 0.8f) { // for stairs
                        player.position.y = ray.position.y + player.hipHeight - 0.01f;
                    }
                    player.velocity.y = 0;
                    player.touchingGround = 1;
                }
            }
        }

        { // movement direction

            pollInput(&player, deltaTimeMs, gamepad);
            vectorAdd(&player.rotation, &player.rotationDirection);

            if (player.rotation.x > 1.57f) {
                player.rotation.x = 1.57f;
            } else if (player.rotation.x < -1.57f) {
                player.rotation.x = -1.57f;
            }

            const float PLAYER_RADIUS = 1.5f;
            const float EYE_HEIGHT = 2.0f;

            Vector wishDir = blankVector();
            wishDir.x = cosf(player.rotation.y) * player.movementDirection.x + sinf(player.rotation.y) * player.movementDirection.z;
            wishDir.z = -sinf(player.rotation.y) * player.movementDirection.x + cosf(player.rotation.y) * player.movementDirection.z;

            Vector moveXZ = createVector(wishDir.x, 0.0f, wishDir.z);
            float totalDist = vectorMag(moveXZ, blankVector());

            if (totalDist > 0.0001f) {
                float maxSubStep = PLAYER_RADIUS * 0.5f;
                int subSteps = (int)ceilf(totalDist / maxSubStep);
                if (subSteps < 1) subSteps = 1;
                if (subSteps > 8) subSteps = 8;

                Vector stepVec = createVector(moveXZ.x / subSteps, 0.0f, moveXZ.z / subSteps);
                const float fanAngles[] = { 0.0f, -0.5236f, 0.5236f, -1.0472f, 1.0472f }; // 0, +-30 deg, +-60 deg
                const int numAngles = 5;

                for (int s = 0; s < subSteps; s++) {
                    Vector rem = stepVec;

                    for (int iter = 0; iter < 3; iter++) {
                        float remLen = vectorMag(rem, blankVector());
                        if (remLen <= 0.0001f) break;

                        Vector dir = rem;
                        normalizeVector(&dir);

                        float probeReach = PLAYER_RADIUS + remLen;
                        bool foundHit = false;
                        double closestDist = DBL_MAX;
                        Vector bestNormal = blankVector();

                        Vector heights[2] = {
                            createVector(player.position.x, player.position.y - EYE_HEIGHT + 0.3f, player.position.z),
                            createVector(player.position.x, player.position.y - 0.3f, player.position.z)
                        };

                        for (int h = 0; h < 2; h++) {
                            for (int a = 0; a < numAngles; a++) {
                                Vector rayDir = rotateXZ(dir, fanAngles[a]);
                                Vector raySeg = createVector(rayDir.x * probeReach, 0.0f, rayDir.z * probeReach);
                                RaycastResult hit = raycast(heights[h], raySeg, world);

                                if (hit.hit && hit.distance <= probeReach && hit.distance < closestDist) {
                                    closestDist = hit.distance;
                                    bestNormal = hit.normal;
                                    foundHit = true;
                                }
                            }
                        }

                        if (!foundHit) {
                            player.position.x += rem.x;
                            player.position.z += rem.z;
                            rem = blankVector();
                            break;
                        }

                        normalizeVector(&bestNormal);
                        bestNormal.y = 0.0f;
                        normalizeVector(&bestNormal);

                        if (closestDist < (double)PLAYER_RADIUS) {
                            float pushOut = (float)((double)PLAYER_RADIUS - closestDist) + 0.01f;
                            player.position.x += bestNormal.x * pushOut;
                            player.position.z += bestNormal.z * pushOut;
                        }

                        // Remove inward motion along wall normal and advance tangent portion
                        float vn = rem.x * bestNormal.x + rem.z * bestNormal.z;
                        if (vn < 0.0f) {
                            rem.x -= vn * bestNormal.x;
                            rem.z -= vn * bestNormal.z;
                        }

                        // Advance along remaining slide vector up to remaining reach
                        float slideLen = vectorMag(rem, blankVector());
                        if (slideLen > 0.0001f) {
                            player.position.x += rem.x;
                            player.position.z += rem.z;
                            rem = blankVector();
                        }
                    }
                }
            }

            player.position.y += player.velocity.y * deltaTimeMs;

            Vector downRayOrigin = createVector(player.position.x, player.position.y, player.position.z);
            float groundProbeLen = EYE_HEIGHT + 1.0f;
            RaycastResult groundRay = raycast(downRayOrigin, createVector(0.0f, -groundProbeLen, 0.0f), world);

            if (groundRay.hit && groundRay.distance <= EYE_HEIGHT && player.velocity.y <= 0.0f) {
                player.position.y = groundRay.position.y + EYE_HEIGHT;
                player.velocity.y = 0.0f;
                player.touchingGround = 1;
            } else {
                player.touchingGround = 0;
            }

            camera.position = player.position;
            camera.rotation = player.rotation;
        }

        if (player.rotation.x > 1.57) {
            player.rotation.x = 1.57;
        } else if (player.rotation.x < -1.57) {
            player.rotation.x = -1.57;
        }
        camera.rotation = player.rotation;

        { // player element
            Vector lVector = lookVector(camera.rotation);
            linearMult(&lVector, 3.0f);
            world->meshes[1].position = lVector;
            Vector rVector = rightVector(camera.rotation);
            linearMult(&rVector, 2.5f);
            vectorAdd(&world->meshes[1].position, &rVector);
            vectorAdd(&world->meshes[1].position, &camera.position);
            world->meshes[1].rotation = camera.rotation;
        }

        SDL_SetRenderDrawColor(renderer, SKY_COLOR, 255);
        SDL_RenderClear(renderer);

        for (int i = 0; i < world->worldSize; i++) {
            renderModel(renderer, &world->meshes[world->drawOrder[i]], camera, assetManager);
        }

        renderLabel(&label);

        SDL_RenderPresent(renderer);

        if (targetFrameSec > 0.0) {
            uint64_t beforeSleep = SDL_GetPerformanceCounter();
            double elapsedSec = (double)(beforeSleep - frameStart) / (double)freq;
            if (targetFrameSec > elapsedSec) {
                double sleepSec = targetFrameSec - elapsedSec;
                if (sleepSec > 0.001) {
                    SDL_Delay((uint32_t)((sleepSec - 0.0005) * 1000.0));
                }
            }
        }

        uint64_t frameEnd = SDL_GetPerformanceCounter();
        deltaTimeMs = ((double)(frameEnd - frameStart) * 1000.0) / (double)freq;
        frameStart = frameEnd;

        tick++;
    }
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!TTF_Init()) {
        printf("SDL TTF FAILED\f");
        return 1;
    }

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if (!SDL_CreateWindowAndRenderer("Engine", INTERNAL_WIDTH, INTERNAL_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // set mouse lock
    SDL_SetWindowRelativeMouseMode(window, 1);

    AssetManager assetManager = createAssetManager(renderer);

    World world = createWorld();
    
    loadAndAddModel(&world, "./source/res/models/e1m1.mdl");
    loadImageAsset(&assetManager, "./source/res/images/alternate.lif");
    loadAndAddModel(&world, "./source/res/models/sword.mdl");
    loadAndAddModel(&world, "./source/res/models/cube.mdl");
    world.meshes[0].texture = "./source/res/images/alternate.lif";
    world.meshes[2].texture = "./source/res/images/alternate.lif";
    world.meshes[0].collision = 1;
    world.meshes[2].collision = 1;

    world.meshes[0].position = createVector(0.0f, -20.0f, 0.0f);
    world.meshes[2].position = createVector(0.0f, -10.0f, 0.0f);
    rescaleMesh(&world.meshes[0], 100.0f);

    gameLoop(&world, &assetManager, renderer);

    destroyWorld(&world);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}