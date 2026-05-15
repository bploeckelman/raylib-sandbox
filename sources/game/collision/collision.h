#ifndef COLLISION_H
#define COLLISION_H

#include "shared/ecs_components.h"
#include "shared/ecs_world.h"

#define COL_NONE        0u
#define COL_SOLID      (1u << 0)
#define COL_PLAYER     (1u << 1)
#define COL_SENSOR     (1u << 2)

typedef struct {
    EntityId  mover,      target;
    Vector2   mover_pos,  target_pos;
    Collider *mover_col, *target_col;
    int       dir_x,      dir_y; // -1, 0, +1; sign of displacement axis
} CollisionContext;

typedef enum {
    COL_RESP_STOP_BOTH = 0, // stop movement, zero velocity
    COL_RESP_KEEP_VELOCITY, // stop movement, keep velocity
    COL_RESP_KEEP_MOVEMENT, // keep movement, zero velocity (rare)
    COL_RESP_PASSTHROUGH,   // keep movement, keep velocity (no stop)
} CollisionResponse;

static bool collide_resp_stops_movement(const CollisionResponse response) {
    return response == COL_RESP_STOP_BOTH || response == COL_RESP_KEEP_VELOCITY;
}

static bool collide_resp_stops_velocity(const CollisionResponse response) {
    return response == COL_RESP_STOP_BOTH || response == COL_RESP_KEEP_MOVEMENT;
}

typedef bool              (*collide_applies_fn)(const World *, const CollisionContext *);
typedef CollisionResponse (*collide_handle_fn) (      World *,       CollisionContext *);

typedef struct {
    const char         *name;    // for logging
    collide_applies_fn  applies; // NULL == always applies
    collide_handle_fn   handle;
} CollisionHandler;

// Build a CollisionContext from a real entity pair. axis: -1, 0, +1 per axis.
CollisionContext  collide_build_context(World *world, EntityId mover, EntityId target, int axis_sign_x, int axis_sign_y);

// Default response derived only from masks (used in simulation mode and as the final fallback when no handler runs).
CollisionResponse collide_default_response(uint32_t target_mask);

// Dispatches collision event to appropriate handler function
CollisionResponse collide_handlers_dispatch(World *world, CollisionContext *context);

// Variant of shape-overlap that takes the second-shape's mask-resolved Position.
// Both 'pos_a + mover_offset' and 'pos_b' are world-space.
bool collide_shape_overlaps(
    const ColliderShape *shape_a, Vector2 pos_a, Vector2 mover_offset,
    const ColliderShape *shape_b, Vector2 pos_b
);

// Per-axis edges. Raylib coordinate convention is top-left = (0,0),
// Y values are inverted in world space (-Y is up, +Y is down).
float collide_shape_top         (const ColliderShape *shape, Vector2 pos);
float collide_shape_bottom      (const ColliderShape *shape, Vector2 pos);
float collide_shape_inner_top   (const ColliderShape *shape, Vector2 pos); // same as top for rect
float collide_shape_inner_bottom(const ColliderShape *shape, Vector2 pos); // same as bottom for rect

#endif //COLLISION_H
