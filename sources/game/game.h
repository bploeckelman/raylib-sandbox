#ifndef GAME_H
#define GAME_H

#include "raylib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    Texture2D texTest;
    // Sound, Font, etc...
} Assets;

// Snapshot-able simulation state. Two of these live in GameMemory so the
// renderer can interpolate between the previous and current fixed step.
// Keep this plain data; no pointers into game.dll, no allocations, no handles.
// Anything that can't be serialized as bytes belongs elsewhere.
typedef struct {
    float     player_x, player_y;
    float     player_vx, player_vy;
    int       score;
    uint64_t  tick;
} GameWorld;

// Persistent state owned by the platform. Survives hot reloads because the
// platform never frees it, only the .dll/.so is unloaded and reloaded.
// Pointers *into* this block remain valid across reloads
// Pointers from the game module *out into* the game module's code
// (e.g. function pointers), must be re-bound in game_load()
typedef struct {
    bool       initialized;
    bool       assets_loaded;
    Assets     assets;
    GameWorld  world_prev; // state at start of last fixed step
    GameWorld  world_curr; // state at end   of last fixed step

    // Optional: bump arena the game can carve up however it wants
    size_t     arena_used;
    uint8_t    arena[64 * 1024 * 1024];
} GameMemory;

// Per-frame input snapshot gathered by the platform
typedef struct {
    bool   key_left, key_right, key_up, key_down, key_space;
    bool   mouse_left, mouse_right;
    float  mouse_x, mouse_y;
} GameInput;

// Entry points exported from the game module. Platform looks these up by name after dlopen / LoadLibrary
typedef void (*game_load_fn)    (GameMemory *m);                                // first load and every reload
typedef void (*game_unload_fn)  (GameMemory *m);                                // shutdown and before reload
typedef void (*game_update_fn)  (GameMemory *m, const GameInput *in, float dt); // fixed delta time
typedef void (*game_render_fn)  (const GameMemory *m, float alpha);             // alpha in [0,1]
typedef void (*game_shutdown_fn)(const GameMemory *m);                        // run before closing window to release assets

typedef struct {
    game_load_fn      load;
    game_unload_fn    unload;
    game_update_fn    update;
    game_render_fn    render;
    game_shutdown_fn  shutdown;
} GameApi;

#endif //GAME_H
