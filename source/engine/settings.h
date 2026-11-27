#ifndef SETTINGS_H
#define SETTINGS_H

// Render Data
#define FRAME_RATE 1.0 / 120.0
#define POLL_RATE 1.0 / 120.0
#define INTERNAL_WIDTH 1400
#define INTERNAL_HEIGHT 800

#define SKY_COLOR 43, 54, 230
#define SHADOW_COLOR 0x000000
#define PRIMARY_LIGHT_COLOR 0xffffff

#define IGNORE_MESH_REORDER_INDEX_START 3 // Ignores reordering. ALways drawn first

#define DEFAULT_MESH "default"
#define DEFAULT_TEXTURE "default"

// Player Data
#define WALK_SPEED 0.1f
#define RUN_SPEED 0.2f

#define JUMP_POWER 0.15f
#define CROUCH_HEIGHT 0.5f

// Game Data

#define GRAVITY 9.8f
#define TERMINAL_VELOCITY 98.0f

// Inventory Data

#endif