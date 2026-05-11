#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "shared/assets.h"
#include "shared/raytmx.h"
#include "raylib.h"

#include <stdbool.h>
#include <stdint.h>

// Some component types are just aliases for simple raylib structs
typedef Rectangle Bounds;
typedef Vector2   Position;
typedef Vector2   Velocity;

#define COLLIDER_DEFAULTS .origin = (Vector2){ 0, 0 }

typedef struct { Vector2 origin; Vector2 size; } Collider;

#define RENDERABLE_DEFAULTS                       \
    .tint              = WHITE,                   \
    .size              = (Vector2){ 0, 0 },       \
    .scale             = (Vector2){ 1, 1 },       \
    .scale_default     = (Vector2){ 1, 1 },       \
    .scale_pivot       = (Vector2){ 0.5f, 0.5f }, \
    .scale_settle_secs = 0.1f // 100 millis

typedef struct {
    Color    tint;
    Vector2  origin;            // offset from entity's Position component (top-left)
    Vector2  size;
    Vector2  scale;
    Vector2  scale_default;
    Vector2  scale_pivot;       // pivot point for scale changes [0..1] -> [top-left..bottom-right]
    float    scale_settle_secs; // seconds to return to scale_default
    float    rotation;
    int      layer;
} Renderable;

typedef enum { ANIM_NORMAL, ANIM_REVERSE, ANIM_LOOP, ANIM_LOOP_REVERSE, ANIM_LOOP_PINGPONG } AnimMode;

#define ANIMATOR_DEFAULTS .mode = ANIM_NORMAL

typedef struct {
    AnimMode     mode;
    AtlasRegions frames; // regions array is arena-owned, lifetime tied to GameMemory
    uint16_t     current_frame;
    float        frame_seconds;
    float        state_time;
} Animator;

#define TILEMAP_LAYER_OBJECTS   "objects"
#define TILEMAP_LAYER_COLLISION "solid"

typedef struct {
    TmxMap   *map;
    uint32_t  cols;
    uint32_t  rows;
    uint32_t  tile_size;
} Tilemap;

#endif //COMPONENTS_H
