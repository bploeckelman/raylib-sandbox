#ifndef GAME_H
#define GAME_H

#include "shared/assets.h"
#include "shared/arena.h"
#include "raylib.h"
#include "flecs.h"

#include <stdbool.h>
#include <stdint.h>

// Fixed timestep interpolation is performed on 'render snapshots' from ECS, extracted each fixed step
typedef struct {
    Vector2        position;
    Vector2        size;
    Vector2        origin;
    float          rotation;
    Color          tint;
    int            layer;
    TextureHandle  tex_handle;
    Rectangle      tex_source; // (Rectangle){0,0,0,0} -> full texture, otherwise region
} RenderInstance;

#define MAX_RENDER_INSTANCES 4096

typedef struct {
    RenderInstance  instances[MAX_RENDER_INSTANCES];
    // Stable id per instance, so prev[i] and curr[i] are the same entity.
    uint64_t        stable_id[MAX_RENDER_INSTANCES];
    uint32_t        count;
} RenderSnapshot;

// Snapshot-able simulation state. Two of these live in GameMemory so the
// renderer can interpolate between the previous and current fixed step.
// Keep this plain data; no pointers into game.dll, no allocations.
// Anything that doesn't need per-tick interpolation belongs elsewhere.
typedef struct {
    // Non-ECS interpolatable state
    uint64_t  tick;
    Vector2   camera_pos;
    float     camera_zoom;

    // Non-interpolated, but snapshot-able data included in save state
    int       score;

    // ECS-derived data, extracted at end of each fixed step
    RenderSnapshot render;
} GameWorld;

// Persistent state owned by the platform. Survives hot reloads because the
// platform never frees it; only the .dll/.so is unloaded and reloaded.
typedef struct {
    bool         initialized;
    Assets       assets;
    Arena        arena;

    ecs_world_t *ecs;          // not included in snapshot, flecs owns its own state
    ecs_query_t *q_renderable_static;   // build once on first load
    ecs_query_t *q_renderable_animated; // build once on first load
    ecs_entity_t test_entity_1;
    ecs_entity_t test_entity_2;

    // game world state at start, end of last fixed step
    GameWorld  world_prev;
    GameWorld  world_curr;

    // game-wide asset handles set once in game_load
    AtlasHandle   atlas_hero;
    TextureHandle tex_test;
    TextureHandle tex_grid;
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
typedef void (*game_shutdown_fn)(GameMemory *m);                                // run before closing window to release assets

typedef struct {
    game_load_fn      load;
    game_unload_fn    unload;
    game_update_fn    update;
    game_render_fn    render;
    game_shutdown_fn  shutdown;
} GameApi;

#endif //GAME_H
