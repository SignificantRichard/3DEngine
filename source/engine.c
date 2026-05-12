// 2026 Wen Xin, lord I feel strange

#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_events.h>
#include <stdlib.h>
#include <unistd.h>

#define WIN_width 640
#define WIN_height 480

typedef struct Vector3D {
    int32_t x;
    int16_t y; // Y is up
    int32_t z;

} Vector3D;

typedef struct Vector2D {
    int16_t x; // who's gonna have more than a 65535x65535p monitor anyways?
    int16_t y;
} Vector2D;

typedef struct PointBuffer { // A buffer in which Vector3D reside
    Vector3D* address;
    int32_t size;
} PointBuffer;

void addToBuffer(PointBuffer* buffer, Vector3D point) {
    if ((*buffer).size == 0) {
        (*buffer).address = (Vector3D*)calloc((*buffer).size, sizeof(Vector3D));
    } else {
        (*buffer).address = (Vector3D*)realloc((*buffer).address, (1 + (*buffer).size) * sizeof(Vector3D));
    }
    (*buffer).address[(*buffer).size] = point;
    (*buffer).size++;
}

void removeFromBuffer(PointBuffer* buffer, int32_t index) {
    if ((*buffer).size == 0) {
        printf("HOW THE FUCK DO I REMOVE NOTHING???");
        return;
    }

    (*buffer).address[index] = (*buffer).address[--(*buffer).size];
    (*buffer).address = (Vector3D*)realloc((*buffer).address, (1 + (*buffer).size) * sizeof(Vector3D));
}

Vector2D worldspaceToScreen(Vector3D point) {
    Vector2D screenPoint;

    // case rendered point is in camera
    if (!point.z) {
        screenPoint.x = -1;
        screenPoint.y = -1;
        return screenPoint;
    }

    // x' = x/z
    // y' = y/z

    screenPoint.x = (int16_t)(point.x / (int32_t)point.z + WIN_width / 2);
    screenPoint.y = (int16_t)(WIN_height / 2 - (point.y / (int32_t)point.z));
    return screenPoint;
}

int main(int argc, char * argv[]) {
    // Some setup stuff, probably bind some window output stuff
    printf("Hello, World\n");

    // Some SDL bullshit so I don't have to write X11/wayland/quartz and whatever the fuck winslop uses

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
            goto SDL_initiation_error;

    window = SDL_CreateWindow("Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_width, WIN_height, SDL_WINDOW_VULKAN);

    if (window == NULL)
        goto SDL_initiation_error;

    renderer = SDL_CreateRenderer(window, -1, 0);

    // setup code here

    struct PointBuffer buffer;
    buffer.size = 0;
    printf("size: %i\n",buffer.size);
    for (int i = 0; i < buffer.size; i++) {
        printf("%i,%i,%i\n", buffer.address[i].x, buffer.address[i].y, buffer.address[i].z);
    }
    // end of setup

    for (;;) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                goto DESTROY_WINDOW;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        for (int i = 0; i < 9; i++) {
            Vector2D b = worldspaceToScreen(buffer.address[i]);
            buffer.address[i].z++;
            printf("%i, %i\n", b.x, b.y);
            struct SDL_Rect rect = {b.x, b.y, 5, 5};
            SDL_RenderDrawRect(renderer, &rect);
        }


        SDL_RenderPresent(renderer);
        usleep(1000*16);

    }

    // free memory
    free(buffer.address);

    DESTROY_WINDOW:
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);

    EXIT_SIGNAL:
    printf("terminated\n");
    return 0;

    SDL_initiation_error:
    printf("sdl init error\n");
    return 1;
}
