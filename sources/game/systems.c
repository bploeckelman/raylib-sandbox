#include "game/systems.h"
#include "game/camera.h"
#include "shared/ecs_components.h"
#include "shared/ecs_setup.h"

static void sys_bounce_in_bounds(ecs_iter_t *iter) {
    const Bounds *bounds = ecs_singleton_get(iter->world, Bounds);
    if (!bounds) return;

    const float world_left   = bounds->rect.x;
    const float world_top    = bounds->rect.y;
    const float world_right  = bounds->rect.x + bounds->rect.width;
    const float world_bottom = bounds->rect.y + bounds->rect.height;

    Position *pos = ecs_field(iter, Position, 0);
    Velocity *vel = ecs_field(iter, Velocity, 1);
    Collider *col = ecs_field(iter, Collider, 2);

    for (int i = 0; i < iter->count; i++) {
        const float collider_left   = pos[i].x + col[i].origin.x;
        const float collider_top    = pos[i].y + col[i].origin.y;
        const float collider_right  = collider_left + col[i].size.x;
        const float collider_bottom = collider_top  + col[i].size.y;

        // Keep positions in bounds, invert velocities if at bounds
        if (collider_left   < world_left)   { pos[i].x += (world_left      - collider_left); if (vel[i].x < 0) vel[i].x = -vel[i].x; }
        if (collider_right  > world_right)  { pos[i].x -= (collider_right  - world_right);   if (vel[i].x > 0) vel[i].x = -vel[i].x; }
        if (collider_top    < world_top)    { pos[i].y += (world_top       - collider_top);  if (vel[i].y < 0) vel[i].y = -vel[i].y; }
        if (collider_bottom > world_bottom) { pos[i].y -= (collider_bottom - world_bottom);  if (vel[i].y > 0) vel[i].y = -vel[i].y; }
    }
}

static void sys_apply_velocity(ecs_iter_t *it) {
    Position *pos = ecs_field(it, Position, 0);
    Velocity *vel = ecs_field(it, Velocity, 1);

    for (int i = 0; i < it->count; i++) {
        pos[i].x += vel[i].x * it->delta_time;
        pos[i].y += vel[i].y * it->delta_time;
    }
}

void register_systems(GameMemory *m) {
    // Re-bind THIS DLL's component-id cells to the world's ids
    ecs_setup_register_components(m->ecs);

    ECS_SYSTEM(m->ecs, sys_apply_velocity,   EcsOnUpdate,   Position, Velocity);
    ECS_SYSTEM(m->ecs, sys_bounce_in_bounds, EcsPostUpdate, Position, Velocity, Collider);

    // Build the renderable query once; it's plain data, survives reloads
    if (!m->q_renderable) {
        m->q_renderable = ecs_query(m->ecs, {
            .terms = {
                { .id = ecs_id(Position) },
                { .id = ecs_id(Sprite) },
            },
        });
    }
}

void extract_render_snapshot(ecs_world_t *world, ecs_query_t *q, RenderSnapshot *out) {
    out->count = 0;
    if (!q) return;

    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        Position *pos    = ecs_field(&it, Position, 0);
        Sprite   *sprite = ecs_field(&it, Sprite,   1);
        for (int i = 0; i < it.count && out->count < MAX_RENDER_INSTANCES; i++) {
            RenderInstance *inst = &out->instances[out->count];
            inst->position = (Vector2){ pos[i].x, pos[i].y };
            inst->origin   = sprite[i].origin;
            inst->scale    = sprite[i].scale;
            inst->rotation = sprite[i].rotation;
            inst->texture  = sprite[i].texture;
            inst->tint     = sprite[i].tint;
            inst->layer    = sprite[i].layer;
            out->stable_id[out->count] = it.entities[i];
            out->count++;
        }
    }
}
