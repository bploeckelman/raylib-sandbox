#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "shared/assets.h"
#include "shared/raytmx.h"
#include "raylib.h"

#include <stdbool.h>
#include <stdint.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#define VIRTUAL_UNIT_PIXELS 16.0f  // 1 tile = 16 px

#define MOVE_PLATFORMER_DEFAULTS   \
    .max_ground_speed_u     = 10.0f,   \
    .max_air_speed_u        = 14.0f,   \
    .move_accel_u           = 100.0f,  \
    .friction_u             = 80.0f,   \
    .gravity_u              = 60.0f,   \
    .gravity_mul            =  1.0f,   \
    .jump_accel_u           = 20.0f,   \
    .jump_buffer_secs       =  0.1f,   \
    .coyote_secs            =  0.12f,  \
    .slide_up_when_grounded = 0

typedef struct {
    // tuning (in virtual units, multiplied by VIRTUAL_UNIT_PIXELS at use)
    float max_ground_speed_u;
    float max_air_speed_u;
    float move_accel_u;
    float friction_u;
    float gravity_u;
    float gravity_mul;
    float jump_accel_u;
    float jump_buffer_secs;
    float coyote_secs;
    int   slide_up_when_grounded;  // pixels, e.g. pill radius

    // runtime state (set by sys_input_*, read+written by sys_movement_platformer)
    Vector2 input_accel;  // u/s² in virtual units
    bool    want_jump;
    bool    is_grounded;
    bool    was_grounded;
    bool    is_dropping;
    bool    is_bouncing;
    float   jump_buffer_t;
    float   coyote_t;
} MovePlatformer;

#define MOVE_TOPDOWN_DEFAULTS \
    .max_speed_u   = 8.0f,        \
    .move_accel_u  = 80.0f,       \
    .friction_u    = 40.0f

typedef struct {
    float   max_speed_u;
    float   move_accel_u;
    float   friction_u;

    Vector2 input_accel;  // 8-way or 4-way; set by sys_input_topdown
} MoveTopdown;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Some component types are just aliases for simple raylib structs
typedef Rectangle Bounds;
typedef Vector2   Position;
typedef struct {
    Vector2 value;
    Vector2 remainder;
} Velocity;

typedef enum { SHAPE_RECT = 0, SHAPE_CIRC, SHAPE_PILL, SHAPE_GRID, SHAPE_COUNT } ShapeKind;
typedef enum { PILL_VERTICAL, PILL_HORIZONTAL } PillAxis;

typedef struct { Vector2 offset; Vector2 size; } ShapeRect;
typedef struct { Vector2 center; float radius; } ShapeCirc;
typedef struct { Vector2 offset; Vector2 size; PillAxis axis;   } ShapePill;
typedef struct { Vector2 offset; int cell_size; int cols, rows; uint8_t *solid; } ShapeGrid;

typedef struct {
    ShapeKind kind;
    union {
        ShapeRect rect;
        ShapeCirc circ;
        ShapePill pill;
        ShapeGrid grid;
    } as;
} ColliderShape;

typedef struct {
    uint32_t      mask;
    uint32_t      collides_with;
    ColliderShape shape;
} Collider;

static Collider collider_rect(const Vector2 offset, const Vector2 size, const uint32_t mask, const uint32_t collides_with) {
    return (Collider){
        .shape = { .kind = SHAPE_RECT, .as.rect = { .offset = offset, .size = size }},
        .mask = mask, .collides_with = collides_with,
    };
}

static Collider collider_circ(const Vector2 center, const float radius, const uint32_t mask, const uint32_t collides_with) {
    return (Collider){
        .shape = { .kind = SHAPE_CIRC, .as.circ = { .center = center, .radius = radius }},
        .mask = mask, .collides_with = collides_with,
    };
}

static Collider collider_pill(const Vector2 offset, const Vector2 size, const PillAxis axis, const uint32_t mask, const uint32_t collides_with) {
    return (Collider){
        .shape = { .kind = SHAPE_PILL, .as.pill = { .offset = offset, .size = size, .axis = axis }},
        .mask = mask, .collides_with = collides_with,
    };
}

static Collider collider_grid(const int cell_size, const int cols, const int rows, uint8_t *solid, const uint32_t mask, const uint32_t collides_with) {
    return (Collider){
        .shape = { .kind = SHAPE_GRID, .as.grid = { .cell_size = cell_size, .cols = cols, .rows = rows, .solid = solid }},
        .mask = mask, .collides_with = collides_with,
    };
}

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
