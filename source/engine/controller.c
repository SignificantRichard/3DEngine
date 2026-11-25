#include <SDL3/SDL_gamepad.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "controller.h"

SDL_Gamepad* getGamepad() {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    SDL_Gamepad* gamepad = count > 0 ? SDL_OpenGamepad(ids[0]) : NULL;
    SDL_free(ids);
    return gamepad;
}

char* getButtonName(SDL_Gamepad* gamepad, SDL_GamepadButton button) {
    switch (SDL_GetGamepadButtonLabel(gamepad, button)) {
    case SDL_GAMEPAD_BUTTON_LABEL_A: return "A";
    case SDL_GAMEPAD_BUTTON_LABEL_B: return "B";
    case SDL_GAMEPAD_BUTTON_LABEL_X: return "X";
    case SDL_GAMEPAD_BUTTON_LABEL_Y: return "Y";
    case SDL_GAMEPAD_BUTTON_LABEL_CROSS: return "Cross";
    case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE: return "Circle";
    case SDL_GAMEPAD_BUTTON_LABEL_SQUARE: return "Square";
    case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE: return "Triangle";
    default: return "Unknown";
    }
}

void flipOnBit(int* x, int bit) {
    *x |= (0b1 << bit);
}

int16_t getAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis) {
    int16_t x = SDL_GetGamepadAxis(gamepad, axis);
    return x;
}

void pollSticks(ControllerOutput* output, SDL_Gamepad* gamepad) {
    int16_t lx = getAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
    int16_t ly = getAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);

    lx = (abs(lx) < 9000) ? 0 : lx;
    ly = (abs(ly) < 9000) ? 0 : ly;

    Vector stick = createVector(lx, ly, 0.0f);
    output->left = stick;

    int16_t rx = getAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
    int16_t ry = getAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);

    rx = (abs(rx) < 9000) ? 0 : rx;
    ry = (abs(ry) < 9000) ? 0 : ry;
    
    stick = createVector(rx, ry, 0.0f);
    output->right = stick;
}

void pollTriggers(ControllerOutput* output, SDL_Gamepad* gamepad) {
    uint16_t l = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    uint16_t r = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    output->lt = l;
    output->rt = r;
}

void getGamepadInputs(ControllerOutput* output, SDL_Gamepad* gamepad) {
    // UDLRXYAB
    // LRLRSFBX
    // bpstshrbox
    int buttons = 0;
    int* bp = &buttons;

    if (!output) {
    return;
    }
    if (!gamepad) {
        output->buttons = 0;
        return;
    }

    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST)) {
        flipOnBit(bp, 0);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH)) {
        flipOnBit(bp, 1);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH)) {
        flipOnBit(bp, 2);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST)) {
        flipOnBit(bp, 3);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) {
        flipOnBit(bp, 4);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) {
        flipOnBit(bp, 5);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) {
        flipOnBit(bp, 6);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)) {
        flipOnBit(bp, 7);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_GUIDE)) {
        flipOnBit(bp, 8);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)) {
        flipOnBit(bp, 9);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) {
        flipOnBit(bp, 10);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK)) {
        flipOnBit(bp, 11);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK)) {
        flipOnBit(bp, 12);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_MISC1)) { // Share button. Not sure what someone's gonna do with this
        flipOnBit(bp, 13);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START)) {
        flipOnBit(bp, 14);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK)) {
        flipOnBit(bp, 15);
    }
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_GUIDE)) { // Xbox button, avoid this
        flipOnBit(bp, 16);
    }

    pollSticks(output, gamepad);
    pollTriggers(output, gamepad);

    output->buttons = buttons;

}

int decodeButtonState(ControllerOutput* output, SDL_GamepadButton button) {
    if (!output) {
        return 0;
    }

    int bit = -1;

    switch (button) {
        case SDL_GAMEPAD_BUTTON_EAST:           bit = 0;  break;
        case SDL_GAMEPAD_BUTTON_SOUTH:          bit = 1;  break;
        case SDL_GAMEPAD_BUTTON_NORTH:          bit = 2;  break;
        case SDL_GAMEPAD_BUTTON_WEST:           bit = 3;  break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     bit = 4;  break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      bit = 5;  break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      bit = 6;  break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:        bit = 7;  break;
        case SDL_GAMEPAD_BUTTON_GUIDE:          bit = 8;  break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  bit = 9;  break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: bit = 10; break;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:     bit = 11; break;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    bit = 12; break;
        case SDL_GAMEPAD_BUTTON_MISC1:          bit = 13; break;
        case SDL_GAMEPAD_BUTTON_START:          bit = 14; break;
        case SDL_GAMEPAD_BUTTON_BACK:           bit = 15; break;
        default:
            return 0;
    }

    return (((unsigned int)output->buttons & (1u << bit)) != 0) ? 1 : 0;
}