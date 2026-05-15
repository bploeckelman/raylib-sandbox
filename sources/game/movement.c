#include "game/movement.h"
#include "game/collision/collision.h"
#include "game/collision/collision_query.h"

#include "raymath.h"

typedef enum { AXIS_X, AXIS_Y } Axis;

static int sign_i(const int value) { return (value > 0) - (value < 0); }

static bool already_hit(const MoveResult *result, const EntityId entity_id) {
    for (int i = 0; i < result->hits_count; i++) {
        if (result->hits[i] == entity_id) return true;
    }
    return false;
}

static void record_hit(MoveResult *result, const EntityId entity_id) {
    const int capacity = (int)(sizeof result->hits / sizeof result->hits[0]);
    if (result->hits_count < capacity) {
        result->hits[result->hits_count] = entity_id;
        result->hits_count++;
    }
}

static void move_axis_pixels(
    World             *world,
    Position          *pos,
    Velocity          *vel,
    const Collider    *col,
    const EntityId     exclude_id,
    const Axis         axis,
    const int          delta_px,
    const MoveOptions *opts,
    MoveResult        *result
) {
    if (delta_px == 0) return;
    const int dir = sign_i(delta_px);
    int remaining = (delta_px < 0) ? -delta_px : delta_px;

    while (remaining > 0) {
        // One-pixel step in move direction
        const Vector2 offset = (axis == AXIS_X)
            ? (Vector2){ (float)dir, 0.0f }
            : (Vector2){ 0.0f, (float)dir };

        EntityId hit = ENTITY_NONE;
        if (collide_first_at_pos(world, *pos, col, exclude_id, offset, col->collides_with, &hit)) {
            // Slide-up: only on X, only if option is set (in raylib -Y is up)
            if (axis == AXIS_X && opts->max_slide_up > 0) {
                for (int step_up = 1; step_up <= opts->max_slide_up; step_up++) {
                    const Vector2 probe = (Vector2){ (float)dir, -(float)step_up };
                    if (!collide_would_collide_pos(world, *pos, col, exclude_id, probe, col->collides_with)) {
                        pos->y -= (float)step_up;
                        hit = ENTITY_NONE;
                        break;
                    }
                }
            }

            if (hit != ENTITY_NONE) {
                // Already-handled hits keep blocking/passing by the same rule.
                // In practice every non-STOP response means "keep moving and stop
                // bothering us", so it's safe to just consume the pixel and continue;
                if (already_hit(result, hit)) {
                    pos->x += offset.x;
                    pos->y += offset.y;
                    if (axis == AXIS_X) result->applied.x += dir;
                    else                result->applied.y += dir;
                    remaining--;
                    continue;
                }
            }

            CollisionResponse response;
            if (opts->skip_handlers) {
                const Collider *target_col = world_get_collider(world, hit);
                const uint32_t  mask = target_col ? target_col->mask : COL_NONE;
                response = collide_default_response(mask);
            } else {
                CollisionContext ctx = collide_build_context(
                    world, exclude_id, hit,
                    (axis == AXIS_X) ? dir : 0,
                    (axis == AXIS_Y) ? dir : 0
                );
                // For hypothetical movers, context's mover_pos/mover_col
                // came from world lookups against ENTITY_NONE, patch them.
                if (exclude_id == ENTITY_NONE) {
                    ctx.mover_pos = *pos;
                    ctx.mover_col = (Collider *)col;
                }
                response = collide_handlers_dispatch(world, &ctx);
            }

            record_hit(result, hit);

            if (collide_resp_stops_velocity(response)) {
                if (axis == AXIS_X) { result->stopped_velocity_x = true; if (vel) vel->value.x = 0; }
                else                { result->stopped_velocity_y = true; if (vel) vel->value.y = 0; }
            }
            if (collide_resp_stops_movement(response)) {
                if (axis == AXIS_X) result->blocked_x = true;
                else                result->blocked_y = true;
                // Also zero the remainder for this axis, partially-spent
                // fractional motion shouldn't bleed into next tick after
                // we've been stopped.
                if (vel) {
                    if (axis == AXIS_X) vel->remainder.x = 0.0f;
                    else                vel->remainder.y = 0.0f;
                }
                return;
            }
            // PASSTHROUGH / KEEP_MOVEMENT: fall through and apply the pixel.
        }
        pos->x += offset.x;
        pos->y += offset.y;
        if (axis == AXIS_X) result->applied.x += dir;
        else                result->applied.y += dir;
        remaining--;
    }
}

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
) {
    *out_result = (MoveResult){0};
    move_axis_pixels(world, pos, vel, col, exclude_id, AXIS_X, dx, opts, out_result);
    move_axis_pixels(world, pos, vel, col, exclude_id, AXIS_Y, dy, opts, out_result);
}

void move_step_dt(
    World             *world,
    Position          *pos,
    Velocity          *vel,
    const Collider    *col,
    const EntityId     exclude_id,
    const float        dt,
    const MoveOptions *opts,
    MoveResult        *out_result
) {
    // Accumulate intended motion (sub-pixel) into the remainder, extract
    // integer pixels, stash the fraction back. truncf() is symmetric around
    // zero, important so that decelerating-and-reversing motion doesn't
    // round biased.
    const float total_x = vel->remainder.x + vel->value.x * dt;
    const float total_y = vel->remainder.y + vel->value.y * dt;
    const int   dx      = (int)truncf(total_x);
    const int   dy      = (int)truncf(total_y);
    vel->remainder.x    = total_x - (float)dx;
    vel->remainder.y    = total_y - (float)dy;

    move_step_pixels(world, pos, vel, col, exclude_id, dx, dy, opts, out_result);
}

MoveResult move_with_collision(World *world, const EntityId mover, const float dt, const MoveOptions *opts) {
    MoveResult result = (MoveResult){0};

    Position *pos = world_get_position(world, mover);
    Velocity *vel = world_get_velocity(world, mover);
    Collider *col = world_get_collider(world, mover);
    if (!pos || !vel || !col) return result;

    move_step_dt(world, pos, vel, col, mover, dt, opts, &result);
    return result;
}
