// 2026 Wen Xin, lord I feel strange

#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_events.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define ViewFalloffParam 0.2
#define ViewFalloffFlatRateParam 10
#define WIN_width 640
#define WIN_height 480
#define isNull == NULL

typedef struct Vector3D {
    int32_t x;
    int32_t y; // Y is up
    int32_t z;

} Vector3D;

typedef struct Vector2D {
    int16_t x;
    int16_t y;
} Vector2D;

typedef struct VertexGroup {
    uint16_t p1;
    uint16_t p2;
    uint16_t p3;
} VertexGroup;

typedef struct PointBuffer { // A buffer in which Vector3D reside
    Vector3D position;
    Vector3D rotation;
    Vector3D* address;
    int32_t size;
    VertexGroup* faces;
    uint16_t faceAmount;
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

void addFaceToBuffer(PointBuffer* buffer, VertexGroup face) {
    if ((*buffer).faceAmount == 0) {
        (*buffer).faces = (VertexGroup*)calloc((*buffer).faceAmount, sizeof(VertexGroup));
    } else {
        (*buffer).faces = (VertexGroup*)realloc((*buffer).faces, (1 + (*buffer).faceAmount) * sizeof(VertexGroup));
    }
    (*buffer).faces[(*buffer).faceAmount] = face;
    (*buffer).faceAmount++;
}

Vector2D worldspaceToScreen(Vector3D point) {
    Vector2D screenPoint;

    // case rendered point is in camera
    if (!point.z) {
        screenPoint.x = -0;
        screenPoint.y = -0;
        return screenPoint;
    }

    // x' = x/z
    // y' = y/z

    screenPoint.x = (int32_t)(ViewFalloffFlatRateParam * point.x / point.z) + WIN_width / 2;
    screenPoint.y = (int32_t)(WIN_height / 2 - (uint32_t)(ViewFalloffFlatRateParam * point.y / point.z));
    return screenPoint;
}

void applyRotation(Vector3D* point, double rX) {
    // Morbing time!
    // x' = xcosT-ysinT
    // z' = xsinT+ycosT
    // y = z

    const double c = cos(rX);
    const double s = sin(rX);

    const int32_t x = (*point).x;
    const int32_t y = (*point).y;
    const int32_t z = (*point).z;

    (*point).x = x*c-z*s;
    (*point).z = x*s+z*c;
}

void renderModel(SDL_Renderer* renderer, PointBuffer* buffer) {

    for (int i = 0; i < (*buffer).faceAmount; i++) {
        // get points
        struct Vector3D p1 = (*buffer).address[(*buffer).faces[i].p1];
        struct Vector3D p2 = (*buffer).address[(*buffer).faces[i].p2];
        struct Vector3D p3 = (*buffer).address[(*buffer).faces[i].p3];


        // Draw edges, probably temp
        Vector2D origin = worldspaceToScreen(p1);
        Vector2D destination = worldspaceToScreen(p2);
        SDL_RenderDrawLine(renderer, origin.x, origin.y, destination.x, destination.y);
        origin = worldspaceToScreen(p3);
        SDL_RenderDrawLine(renderer, origin.x, origin.y, destination.x, destination.y);
        destination = worldspaceToScreen(p1);
        SDL_RenderDrawLine(renderer, origin.x, origin.y, destination.x, destination.y);
    }
    return;

    POINT_ACCESS_FAIL:
    printf("failed to read point buffer");
}

uint8_t extractData(uint8_t* mode, char* data, Vector3D* point) {
    // change save modes
    if (!strcmp(data, "vert\n")) {
        *mode = 0;
        return 0;
    } else if (!strcmp(data, "face\n")){
        *mode = 1;
        return 0;
    } else if (!strcmp(data, "end\n")) {
        *mode = 9;
        return 0;
    }

    int16_t coordinates[3];
    char* value = strtok(data, ",");
    for (uint16_t i = 0; i < 3; i++) {
        int16_t conv = atoi(value);
        if (conv)
            coordinates[i] = conv;
        else
            coordinates[i] = 0;
        value = strtok(NULL, ",");
    }
    if (*mode == 1) {
        printf("%i, %i, %i\n", coordinates[0], coordinates[1], coordinates[2]);
    }
    (*point).x = coordinates[0];
    (*point).y = coordinates[1];
    (*point).z = coordinates[2];
    return 1;
}

void parseModelData(PointBuffer* exportBuffer) {
    const uint16_t fileReadSize = UINT16_MAX;
    FILE *fptr;
    fptr = fopen("./models/cube.mdl", "r");
    char data[fileReadSize];

    uint8_t mode = 0;

    if (strcmp(fgets(data, fileReadSize, fptr), "classic")) {

        while (fgets(data, fileReadSize, fptr)) {
            struct Vector3D point = {0, 0, 0};
            struct VertexGroup face = {0, 0, 0};

            // point detected
            if (!extractData(&mode, data, &point))
                continue;

            switch (mode) {
                case 0: // add point
                    addToBuffer(exportBuffer, point);
                    // printf("%i, %i, %i\n", point.x, point.y, point.z);
                    break;
                case 1: // add face
                    face.p1 = point.x;
                    face.p2 = point.y;
                    face.p3 = point.z;
                    addFaceToBuffer(exportBuffer, face);
                    break;
                case 9:
                    return;
            }
        }

        fclose(fptr);
        return;
    }
    // unused
    // while (fgets(data, (fileReadSize), fptr)) {
    //
    // }
    fclose(fptr);
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
    buffer.size = 0; buffer.faceAmount = 0;

    parseModelData(&buffer);

    struct Vector3D zero = {0, 0, 0};
    buffer.position = zero;
    buffer.rotation = zero;

    long deltaTime = 0;

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

        renderModel(renderer, &buffer);

        SDL_RenderPresent(renderer);

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
