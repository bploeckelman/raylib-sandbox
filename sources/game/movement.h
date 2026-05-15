#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "shared/ecs_world.h"
#include "raylib.h"

typedef struct {
    bool     blocked_x;          // motion was stopped on X
    bool     blocked_y;          // motion was stopped on Y
    bool     stopped_velocity_x; // caller should zero velocity.x
    bool     stopped_velocity_y; // caller should zero velocity.y
    Vector2  applied;            // integer pixels actually move (signed)
    int      hits_count;         // number of entities touched during the step
    EntityId hits[8];            // entities touched during the step
} MoveResult;

typedef struct {
    int  max_slide_up; // X-axis only, 0 disables
    bool skip_handlers; // true == bypass handler chain, use default response (for computer controlled planning)
} MoveOptions;

// Lowest-level primitive: walk dx pixels on X, then dy on Y.
// Mutates *pos directly (always by integer offsets).
// Zeroes vel->value.x/.y on STOP responses if vel != null.
// Does not touch vel->remainder.
// Pass exclude_id == ENTITY_NONE for hypothetical movers (for computer controlled planning).
void move_step_pixels(
    World             *world,
    Position          *pos,
    Velocity          *vel,
    const Collider    *col,
    EntityId           exclude_id,
    int                dx,
    int                dy,
    const MoveOptions *opts,
    MoveResult        *out_result
);

// Velocity-aware step: standard per-tick entry point.
void move_step_dt(
    World             *world,
    Position          *pos,
    Velocity          *vel,
    const Collider    *col,
    EntityId           exclude_id,
    float              dt,
    const MoveOptions *opts,
    MoveResult        *out_result
);

// Entity wrapper: reads pos/vel/col from World, calls move_step_dt.
MoveResult move_with_collision(World *world, EntityId mover, float dt, const MoveOptions *opts);

//
// NOTE: example for speculative movement such as computer controlled player could use
//
// // Take a snapshot — no allocations, all stack-local.
// Position pos_snap   = *world_get_position(world, e);
// Velocity vel_snap   = *world_get_velocity(world, e);
// Collider col_snap   = *world_get_collider(world, e); // value copy
//
// MoveResult r;
// for (int i = 0; i < lookahead_steps; i++) {
//     // ... AI decides what to do, maybe mutates vel_snap ...
//     move_step_pos(world, &pos_snap, &col_snap, &vel_snap, e,
//                   (Vector2){ vel_snap.x * dt, vel_snap.y * dt },
//                   &opts, &r);
//     if (r.stopped_velocity_x) vel_snap.x = 0;
//     if (r.stopped_velocity_y) vel_snap.y = 0;
// }
// // pos_snap is the predicted final position; the real entity was never touched

#endif //MOVEMENT_H
