#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include "../engine/vector.h"
#include "../engine/controller.h"
#include "../engine/instances.h"
#include "../engine/settings.h"

void pollInput(Body* body, float delta, SDL_Gamepad* gamepad) {
    const bool *keyStates = SDL_GetKeyboardState(NULL);
    Vector localMovementDirection = blankVector();

    ControllerOutput output = {0};

    getGamepadInputs(&output, gamepad);
    localMovementDirection.x = output.left.x / 0xffff;
    localMovementDirection.z = output.left.y / -0xffff;

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

    if ((keyStates[SDL_SCANCODE_SPACE] || decodeButtonState(&output, SDL_GAMEPAD_BUTTON_SOUTH)) && body->touchingGround) {
        body->velocity.y = JUMP_POWER;
    }
    // if (keyStates[SDL_SCANCODE_LCTRL]) {
    //     // localMovementDirection.y -= 1;
    // }

    if (keyStates[SDL_SCANCODE_0]) {
        body->position = blankVector();
        body->rotation = blankVector();
    }

    if (keyStates[SDL_SCANCODE_9]) {
        body->position = createVector(999999.0f, 999999.0f, 999999.0f);
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
    localMovementDirection.y = output.right.x / 0xffff;
    localMovementDirection.x = output.right.y / 0xffff;

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