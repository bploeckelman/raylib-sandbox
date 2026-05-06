#include "game/systems.h"
#include "shared/ecs_components.h"
#include "shared/ecs_setup.h"

static void sys_apply_velocity(ecs_iter_t *it) {
    Position *p = ecs_field(it, Position, 0);
    Velocity *v = ecs_field(it, Velocity, 1);
    for (int i = 0; i < it->count; i++) {
        p[i].x += v[i].x * it->delta_time;
        p[i].y += v[i].y * it->delta_time;
    }
}

void register_systems(GameMemory *m) {
    // Re-bind THIS DLL's component-id cells to the world's ids
    ecs_setup_register_components(m->ecs);

    ECS_SYSTEM(m->ecs, sys_apply_velocity, EcsOnUpdate, Position, Velocity);

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
        Position *p = ecs_field(&it, Position, 0);
        Sprite   *s = ecs_field(&it, Sprite,   1);
        for (int i = 0; i < it.count && out->count < MAX_RENDER_INSTANCES; i++) {
            RenderInstance *ri = &out->instances[out->count];
            ri->position = (Vector2){ p[i].x, p[i].y };
            ri->scale = s[i].scale;
            ri->rotation = s[i].rotation;
            ri->texture = s[i].texture;
            ri->tint = s[i].tint;
            ri->layer = s[i].layer;
            out->stable_id[out->count] = it.entities[i];
            out->count++;
        }
    }
}
