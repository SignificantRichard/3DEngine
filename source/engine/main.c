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
#include "../world/entry.h"

#include "settings.h"

int pollInput(Body* body, float delta) {
    const bool *keyStates = SDL_GetKeyboardState(NULL);
    Vector localMovementDirection = blankVector();

    if (keyStates[SDL_SCANCODE_W]) {
        localMovementDirection.z += 1;
    }
    if (keyStates[SDL_SCANCODE_S]) {
        localMovementDirection.z -= 1;
    }
    if (keyStates[SDL_SCANCODE_A]) {
        localMovementDirection.x -= 1;
    }
    if (keyStates[SDL_SCANCODE_D]) {
        localMovementDirection.x += 1;
    }

    if (keyStates[SDL_SCANCODE_SPACE]) {
        localMovementDirection.y -= 1;
    }
    if (keyStates[SDL_SCANCODE_LCTRL]) {
        localMovementDirection.y += 1;
    }

    if (keyStates[SDL_SCANCODE_0]) {
        body->position = blankVector();
        body->rotation = blankVector();
    }

    if (keyStates[SDL_SCANCODE_9]) {
        body->position = createVector(99999999,99999999,99999999);
        body->rotation = blankVector();
    }


    float speed = 0.02f;
    if (keyStates[SDL_SCANCODE_LSHIFT]) {
        speed = 0.5f;
    }

    linearMult(&localMovementDirection, delta * speed);
    body->movementDirection = localMovementDirection;

    localMovementDirection = blankVector();

    if (keyStates[SDL_SCANCODE_UP]) {
        localMovementDirection.x += 1;
    }
    if (keyStates[SDL_SCANCODE_DOWN]) {
        localMovementDirection.x -= 1;
    }
    if (keyStates[SDL_SCANCODE_LEFT]) {
        localMovementDirection.y -= 1;
    }
    if (keyStates[SDL_SCANCODE_RIGHT]) {
        localMovementDirection.y += 1;
    }
    
    linearMult(&localMovementDirection, delta * 0.01f);
    body->rotationDirection = localMovementDirection;
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

    bool running = true;
    SDL_Event event;

    AssetManager assetManager = createAssetManager(renderer);

    World world = createWorld();
    
    loadAndAddModel(&world, "./source/res/models/cube.mdl");
    loadImageAsset(&assetManager, "./source/res/images/alternate.lif");
    world.meshes[0].texture = "./source/res/images/alternate.lif";

    Body player = { .position = blankVector(), .rotation = blankVector(), .movementDirection = blankVector() };

    world.meshes[0].position = (Vector){ 0.0f, 0.0f, 20.0f };

    struct Camera camera = { .position = blankVector(), .rotation = blankVector() };

    uint64_t NOW = SDL_GetPerformanceCounter();
    uint64_t LAST = 0;
    double deltaTime = 0.0;

    double waitTime = frameRate;
    double waitedTime = 0.0;

    uint64_t tick = 0;

    while (running) {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST)*1000 / (double)SDL_GetPerformanceFrequency());
        // events and input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        waitedTime += deltaTime;
        pollInput(&player, deltaTime);
        if (waitedTime > waitedTime) {
            waitedTime = 0.0;
            continue;
        }
        vectorAdd(&player.rotation, &player.rotationDirection);
        
        {
            Vector localMovementDirection = blankVector();
            localMovementDirection.z = cosf(player.rotation.y) * player.movementDirection.z + sinf(player.rotation.y) * -player.movementDirection.x;
            localMovementDirection.x = cosf(player.rotation.y) * player.movementDirection.x + sinf(player.rotation.y) * player.movementDirection.z;
            localMovementDirection.y = player.movementDirection.y;
            vectorAdd(&player.position, &localMovementDirection);
            camera.position = player.position;
        }

        if (player.rotation.x > 1.57) {
            player.rotation.x = 1.57;
        } else if (player.rotation.x < -1.57) {
            player.rotation.x = -1.57;
        }
        camera.rotation = player.rotation;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // sort the models
        sortDrawOrder(&world, camera.position, 0, world.worldSize - 1);

        for (int i = 0; i < world.worldSize; i++) {
            renderModel(renderer, &world.meshes[world.drawOrder[i]], camera, &assetManager);
        }

        SDL_RenderPresent(renderer);

        tick++;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}