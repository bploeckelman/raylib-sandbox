#include "game/systems.h"
#include "game/camera.h"
#include "shared/ecs_components.h"
#include "shared/ecs_setup.h"

#include <math.h>

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

static void sys_apply_velocity(ecs_iter_t *iter) {
    Position *positions  = ecs_field(iter, Position, 0);
    Velocity *velocities = ecs_field(iter, Velocity, 1);
    const float dt = iter->delta_time;

    for (int i = 0; i < iter->count; i++) {
        positions[i].x += velocities[i].x * dt;
        positions[i].y += velocities[i].y * dt;
    }
}

static void sys_scale_return(ecs_iter_t *iter) {
    Renderable *renderables = ecs_field(iter, Renderable, 0);
    const float dt = iter->delta_time;

    for (int i = 0; i < iter->count; i++) {
        Renderable *renderable = &renderables[i];
        if (renderable->scale_settle_secs <= 0.0f) continue;

        const float ease = 1.0f - exp2f(-dt / renderable->scale_settle_secs);
        renderable->scale.x += (renderable->scale_default.x - renderable->scale.x) * ease;
        renderable->scale.y += (renderable->scale_default.y - renderable->scale.y) * ease;
    }
}

void register_systems(GameMemory *m) {
    // Re-bind THIS DLL's component-id cells to the world's ids
    ecs_setup_register_components(m->ecs);

    ECS_SYSTEM(m->ecs, sys_apply_velocity,   EcsOnUpdate,   Position, Velocity);
    ECS_SYSTEM(m->ecs, sys_scale_return,     EcsOnUpdate,   Renderable);
    ECS_SYSTEM(m->ecs, sys_bounce_in_bounds, EcsPostUpdate, Position, Velocity, Collider);

    // Build the renderable query once; it's plain data, survives reloads.
    if (!m->q_renderable) {
        m->q_renderable = ecs_query(m->ecs, {
            .terms = {
                { .id = ecs_id(Position) },
                { .id = ecs_id(Renderable) },
                { .id = ecs_id(TexImage) },
            },
        });
    }
}

void extract_render_snapshot(const ecs_world_t *world, ecs_query_t *query, const Assets *assets, RenderSnapshot *out) {
    out->count = 0;
    if (!query) return;

    ecs_iter_t it = ecs_query_iter(world, query);
    while (ecs_query_next(&it)) {
        Position   *positions   = ecs_field(&it, Position,   0);
        Renderable *renderables = ecs_field(&it, Renderable, 1);
        TexImage   *tex_images  = ecs_field(&it, TexImage,   2);

        for (int i = 0; i < it.count && out->count < MAX_RENDER_INSTANCES; i++) {
            const Renderable *renderable = &renderables[i];

            // Set renderable to texture size if no explicit size is provided
            Vector2 base_size = renderable->size;
            if (base_size.x == 0.0f && base_size.y == 0.0f) {
                const Texture2D source_texture = assets_get_texture(assets, tex_images[i].texture);
                base_size.x = (float)source_texture.width;
                base_size.y = (float)source_texture.height;
            }

            // Apply squash/stretch to renderable, scaling at 'pivot' point rather than default top left
            const Vector2 scaled_size = (Vector2){
                base_size.x * renderable->scale.x,
                base_size.y * renderable->scale.y,
            };
            const Vector2 pivot_shift = (Vector2){
                base_size.x * renderable->scale_pivot.x * (1.0f - renderable->scale.x),
                base_size.y * renderable->scale_pivot.y * (1.0f - renderable->scale.y),
            };
            const Vector2 effective_origin = (Vector2){
                renderable->origin.x + pivot_shift.x,
                renderable->origin.y + pivot_shift.y,
            };

            const Vector2 position = (Vector2){ positions[i].x, positions[i].y };

            // Create render instance for this entity to draw at fixed step
            RenderInstance *inst = &out->instances[out->count];
            inst->position = position;
            inst->size     = scaled_size;
            inst->origin   = effective_origin;
            inst->rotation = renderable->rotation;
            inst->tint     = renderable->tint;
            inst->layer    = renderable->layer;
            inst->texture  = tex_images[i].texture;
            out->stable_id[out->count] = it.entities[i];
            out->count++;
        }
    }
}
