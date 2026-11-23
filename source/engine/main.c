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
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "image.h"
#include "asset_manager.h"
#include "mesh.h"
#include "mesh_extended.h"
#include "world.h"
#include "instances.h"
#include "render.h"
#include "ray.h"
#include "sound.h"
#include "../world/entry.h"

#include "settings.h"

void pollInput(Body* body, float delta) {
    const bool *keyStates = SDL_GetKeyboardState(NULL);
    Vector localMovementDirection = blankVector();

    if (keyStates[SDL_SCANCODE_W]) {
        localMovementDirection.z += 1.0f;
    }
    if (keyStates[SDL_SCANCODE_S]) {
        localMovementDirection.z -= 1.0f;
    }
    if (keyStates[SDL_SCANCODE_A]) {
        localMovementDirection.x -= 1.0f;
    }
    if (keyStates[SDL_SCANCODE_D]) {
        localMovementDirection.x += 1.0f;
    }

    if (keyStates[SDL_SCANCODE_SPACE] && body->touchingGround) {
        body->velocity.y = JUMP_POWER;
    }
    if (keyStates[SDL_SCANCODE_LCTRL]) {
        // localMovementDirection.y -= 1;
    }

    if (keyStates[SDL_SCANCODE_0]) {
        body->position = blankVector();
        body->rotation = blankVector();
    }

    if (keyStates[SDL_SCANCODE_9]) {
        body->position = createVector(99999.9f,0,99999.9f);
        body->rotation = blankVector();
    }

    float speed = WALK_SPEED;
    if (keyStates[SDL_SCANCODE_LSHIFT]) {
        speed = RUN_SPEED;
    }

    linearMult(&localMovementDirection, delta * speed);
    body->movementDirection = localMovementDirection;

    // backup
    localMovementDirection = blankVector();
    // mouse movement
    Vector mouseDelta = blankVector();

    SDL_GetRelativeMouseState(&mouseDelta.y, &mouseDelta.x);

    if (keyStates[SDL_SCANCODE_UP]) {
        localMovementDirection.x -= 1.0f;
    }
    if (keyStates[SDL_SCANCODE_DOWN]) {
        localMovementDirection.x += 1.0f;
    }
    if (keyStates[SDL_SCANCODE_LEFT]) {
        localMovementDirection.y -= 1.0f;
    }
    if (keyStates[SDL_SCANCODE_RIGHT]) {
        localMovementDirection.y += 1.0f;
    }
    
    linearMult(&localMovementDirection, delta * 0.0025f);
    linearMult(&mouseDelta, delta * 0.025f);
    body->rotationDirection = localMovementDirection;
    vectorAdd(&body->rotation, &mouseDelta);
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
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

    bool running = true;
    SDL_Event event;

    AssetManager assetManager = createAssetManager(renderer);

    World world = createWorld();
    
    loadAndAddModel(&world, "./source/res/models/plane.mdl");
    loadImageAsset(&assetManager, "./source/res/images/alternate.lif");
    loadAndAddModel(&world, "./source/res/models/sword.mdl");
    loadAndAddModel(&world, "./source/res/models/cube.mdl");
    world.meshes[0].texture = "./source/res/images/alternate.lif";
    world.meshes[2].texture = "./source/res/images/alternate.lif";
    world.meshes[0].collision = 1;
    world.meshes[2].collision = 1;

    Body player = { .position = blankVector(), .rotation = blankVector(), .movementDirection = blankVector() };

    world.meshes[0].position = createVector(0.0f, -20.0f, 0.0f);
    world.meshes[2].position = createVector(0.0f, -10.0f, 0.0f);
    rescaleMesh(&world.meshes[0], 50.0f);

    struct Camera camera = { .position = blankVector(), .rotation = blankVector() };

    uint64_t NOW = SDL_GetPerformanceCounter();
    uint64_t LAST = 0;
    double deltaTime = 0.0;
    double tDeltaTime = 0.0;

    double waitTime = frameRate;
    double waitedTime = 0.0;

    uint64_t tick = 0;

    while (running) {
        LAST = NOW;

        // do some stuff before the frame delay
        if (waitedTime == 0.0) { // makes sure this only runs once per frame
            sortDrawOrder(&world, camera.position, 0, world.worldSize - 1);
        }

        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST)*1000 / (double)SDL_GetPerformanceFrequency());
        tDeltaTime += deltaTime;
        // events and input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        waitedTime += deltaTime;

        if (waitedTime < waitTime) {
            SDL_Delay((uint32_t)((waitTime - waitedTime) * 1000));
            continue; // doesn't allow rendering until frame time is done
        }

        waitedTime = 0.0;

        pollInput(&player, tDeltaTime);
        vectorAdd(&player.rotation, &player.rotationDirection);

        { // gravity logic
            RaycastResult ray = raycast(camera.position, createVector(0.0f, -10.0f, 0.0f), &world);
            if (!ray.hit) {
                // player.position.y -= 0.005f;
                player.velocity.y -= 0.0001f * tDeltaTime;
                player.touchingGround = 0;
            } else {
                if (player.velocity.y <= 0.0f) {
                    if (ray.distance > 0.8f) { // for stairs
                        player.position.y = ray.position.y + 9.99f;
                    }
                    player.velocity.y = 0;
                    player.touchingGround = 1;
                }
            }
        }

        { // movement direction
            Vector localMovementDirection = blankVector();
            localMovementDirection.z = cosf(player.rotation.y) * player.movementDirection.z + sinf(player.rotation.y) * -player.movementDirection.x;
            localMovementDirection.x = cosf(player.rotation.y) * player.movementDirection.x + sinf(player.rotation.y) * player.movementDirection.z;
            localMovementDirection.y = player.movementDirection.y;
            
            // project collision rays
            Vector step = localMovementDirection;
            vectorAdd(&localMovementDirection, &player.velocity);
            RaycastResult ray = raycast(player.position, step, &world);
            
            if (!ray.hit) {
                vectorAdd(&player.position, &localMovementDirection);
                vectorAdd(&player.position, &player.velocity);
            } else { // touching wall. slide now
                // snap to x units away from wall
                for (int i = 0; i < 5; i++) {
                    vectorAdd(&player.position, &ray.normal);
                }
                linearMult(&ray.normal, vectorMag(step, blankVector()));
                vectorAdd(&localMovementDirection, &ray.normal);
                vectorAdd(&player.position, &localMovementDirection);
                vectorAdd(&player.position, &player.velocity);
            }
            
            camera.position = player.position;
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
            world.meshes[1].position = lVector;
            Vector rVector = rightVector(camera.rotation);
            linearMult(&rVector, 2.5f);
            vectorAdd(&world.meshes[1].position, &rVector);
            vectorAdd(&world.meshes[1].position, &camera.position);
            world.meshes[1].rotation = camera.rotation;
        }


        SDL_SetRenderDrawColor(renderer, SKY_COLOR, 255);
        SDL_RenderClear(renderer);

        for (int i = 0; i < world.worldSize; i++) {
            renderModel(renderer, &world.meshes[world.drawOrder[i]], camera, &assetManager);
        }

        SDL_RenderPresent(renderer);

        tick++;

        tDeltaTime = 0.0;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}