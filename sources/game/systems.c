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

    Position       *pos = ecs_field(iter, Position, 0);
    Velocity       *vel = ecs_field(iter, Velocity, 1);
    const Collider *col = ecs_field(iter, Collider, 2);

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
    Position       *positions  = ecs_field(iter, Position, 0);
    const Velocity *velocities = ecs_field(iter, Velocity, 1);
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

static void sys_animation(ecs_iter_t *iter) {
    Animator *animators = ecs_field(iter, Animator, 0);
    const float dt = iter->delta_time;

    for (int i = 0; i < iter->count; i++) {
        Animator  *animator   = &animators[i];
        const int  num_frames = animator->frames_count;

        animator->state_time += dt;

        if (num_frames == 0 || animator->frame_seconds <= 0.0f) continue;

        if (animator->frames_count == 1) {
            animator->current_frame = 0;
        } else {
            const int last_frame_index = num_frames - 1;

            int frame_index = (int)(animator->state_time / animator->frame_seconds);

            switch (animator->mode) {
                case ANIM_NORMAL:
                    frame_index = min(last_frame_index, frame_index);
                    break;
                case ANIM_REVERSE:
                    frame_index = max(num_frames - frame_index - 1, 0);
                    break;
                case ANIM_LOOP:
                    frame_index = frame_index % num_frames;
                    break;
                case ANIM_LOOP_REVERSE:
                    frame_index = frame_index % num_frames;
                    frame_index = num_frames - frame_index - 1;
                    break;
                case ANIM_LOOP_PINGPONG:
                    frame_index = frame_index % ((num_frames * 2) - 2);
                    if (frame_index >= num_frames) {
                        frame_index = num_frames - 2 - (frame_index - num_frames);
                    }
                    break;
            }

            animator->current_frame = frame_index;
        }
    }
}

void register_systems(GameMemory *m) {
    // Re-bind THIS DLL's component-id cells to the world's ids
    ecs_setup_register_components(m->ecs);

    ECS_SYSTEM(m->ecs, sys_apply_velocity,   EcsOnUpdate,   Position, Velocity);
    ECS_SYSTEM(m->ecs, sys_scale_return,     EcsOnUpdate,   Renderable);
    ECS_SYSTEM(m->ecs, sys_animation,        EcsOnUpdate,   Animator);
    ECS_SYSTEM(m->ecs, sys_bounce_in_bounds, EcsPostUpdate, Position, Velocity, Collider);

    // Build the renderable queries once; it's plain data, survives reloads.
    if (!m->q_renderable_static) {
        m->q_renderable_static = ecs_query(m->ecs, {
            .terms = {
                { .id = ecs_id(Position)   },
                { .id = ecs_id(Renderable) },
                { .id = ecs_id(TexRegion)  },
                { .id = ecs_id(Animator), .oper = EcsNot },
            },
        });
    }
    if (!m->q_renderable_animated) {
        m->q_renderable_animated = ecs_query(m->ecs, {
            .terms = {
                { .id = ecs_id(Position)   },
                { .id = ecs_id(Renderable) },
                { .id = ecs_id(Animator) },
                { .id = ecs_id(TexRegion), .oper = EcsNot  },
            },
        });
    }
}

static void emit_render_instance(
    RenderSnapshot      *out,
    const Assets        *assets,
    const ecs_entity_t   entity,
    const Position      *position,
    const Renderable    *renderable,
    const TextureHandle  tex_handle,
    Rectangle            tex_source
) {
    if (out->count >= MAX_RENDER_INSTANCES) return;

    // // Zero-size texture source rect -> use the full texture
    if (tex_source.width == 0.0f && tex_source.height == 0.0f) {
        const Texture2D texture = assets_get_texture(assets, tex_handle);
        tex_source = (Rectangle){
            0, 0,
            (float)texture.width,
            (float)texture.height
        };
    }

    // Set renderable to texture size if no explicit size is provided
    Vector2 base_size = renderable->size;
    if (base_size.x == 0.0f && base_size.y == 0.0f) {
        base_size.x = tex_source.width;
        base_size.y = tex_source.height;
    }
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
        renderable->origin.y + pivot_shift.y
    };

    RenderInstance *inst = &out->instances[out->count];
    inst->position   = (Vector2){ position->x, position->y };
    inst->size       = scaled_size;
    inst->origin     = effective_origin;
    inst->rotation   = renderable->rotation;
    inst->tint       = renderable->tint;
    inst->layer      = renderable->layer;
    inst->tex_handle = tex_handle;
    inst->tex_source = tex_source;
    out->stable_id[out->count] = entity;
    out->count++;
}

void extract_render_snapshot(
    const ecs_world_t *world,
    ecs_query_t       *query_static,
    ecs_query_t       *query_animated,
    const Assets      *assets,
    RenderSnapshot    *out
) {
    out->count = 0;

    if (query_static) {
        ecs_iter_t iter = ecs_query_iter(world, query_static);
        while (ecs_query_next(&iter)) {
            Position   *positions   = ecs_field(&iter, Position,   0);
            Renderable *renderables = ecs_field(&iter, Renderable, 1);
            TexRegion  *regions     = ecs_field(&iter, TexRegion,  2);
            for (int i = 0; i < iter.count; i++) {
                emit_render_instance(out, assets,
                    iter.entities[i], &positions[i], &renderables[i],
                    regions[i].tex_handle, regions[i].tex_source);
            }
        }
    }

    if (query_animated) {
        ecs_iter_t iter = ecs_query_iter(world, query_animated);
        while (ecs_query_next(&iter)) {
            Position   *positions   = ecs_field(&iter, Position,   0);
            Renderable *renderables = ecs_field(&iter, Renderable, 1);
            Animator   *animators   = ecs_field(&iter, Animator,   2);
            for (int i = 0; i < iter.count; i++) {
                const Animator  animator = animators[i];
                if (animator.frames_count == 0 || !animator.frames) continue;

                const TexRegion frame    = animator.frames[animator.current_frame];
                emit_render_instance(out, assets,
                    iter.entities[i], &positions[i], &renderables[i],
                    frame.tex_handle, frame.tex_source);
            }
        }
    }

    // Sort RenderInstance's by layer, ascending. Higher layer draws on top.
    // Uses stable insertion sort so equal-layer entities preserve insertion
    // order across frames.
    for (uint32_t i = 1; i < out->count; i++) {
        const RenderInstance inst  = out->instances[i];
        const uint64_t       sid   = out->stable_id[i];
        const int            layer = inst.layer;

        uint32_t j = i;
        while (j > 0 && out->instances[j - 1].layer > layer) {
            out->instances[j] = out->instances[j - 1];
            out->stable_id[j] = out->stable_id[j - 1];
            j--;
        }
        out->instances[j] = inst;
        out->stable_id[j] = sid;
    }
}
