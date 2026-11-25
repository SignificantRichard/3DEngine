#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#include "vector.h"

typedef struct {
    int buttons;
    Vector left;
    Vector right;
    uint16_t lt;
    uint16_t rt;
} ControllerOutput;

SDL_Gamepad* getGamepad();
void getGamepadInputs(ControllerOutput* output, SDL_Gamepad* gamepad);
int decodeButtonState(ControllerOutput* output, SDL_GamepadButton button);

#endif