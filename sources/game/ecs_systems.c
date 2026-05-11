#include "game/ecs_systems.h"
#include "game/camera.h"

#include <math.h>
#include <stdlib.h>

void sys_bounce_in_bounds(World *world, const Bounds bounds) {
    const float world_left   = bounds.x;
    const float world_top    = bounds.y;
    const float world_right  = bounds.x + bounds.width;
    const float world_bottom = bounds.y + bounds.height;

    for (int i = 0; i < world->num_entities; i++) {
        if (!world->alive[i])              continue;
        if (!world->positions .present[i]) continue;
        if (!world->velocities.present[i]) continue;
        if (!world->colliders .present[i]) continue;

        Position       *pos = &world->positions .data[i];
        Velocity       *vel = &world->velocities.data[i];
        const Collider  col =  world->colliders .data[i];

        const float collider_left   = pos->x + col.origin.x;
        const float collider_top    = pos->y + col.origin.y;
        const float collider_right  = collider_left + col.size.x;
        const float collider_bottom = collider_top  + col.size.y;

        // Keep positions in bounds, invert velocities if at bounds
        if (collider_left   < world_left)   { pos->x += (world_left      - collider_left); if (vel->x < 0) vel->x = -vel->x; }
        if (collider_right  > world_right)  { pos->x -= (collider_right  - world_right);   if (vel->x > 0) vel->x = -vel->x; }
        if (collider_top    < world_top)    { pos->y += (world_top       - collider_top);  if (vel->y < 0) vel->y = -vel->y; }
        if (collider_bottom > world_bottom) { pos->y -= (collider_bottom - world_bottom);  if (vel->y > 0) vel->y = -vel->y; }
    }
}

void sys_integrate_velocity(World *world, const float dt) {
    for (int i = 0; i < world->num_entities; i++) {
        if (!world->alive[i])              continue;
        if (!world->positions .present[i]) continue;
        if (!world->velocities.present[i]) continue;

        Position       *pos = &world->positions .data[i];
        const Velocity  vel =  world->velocities.data[i];

        pos->x += vel.x * dt;
        pos->y += vel.y * dt;
    }
}

void sys_scale_return(World *world, const float dt) {
    for (int i = 0; i < world->num_entities; i++) {
        if (!world->alive[i])               continue;
        if (!world->renderables.present[i]) continue;

        Renderable *render = &world->renderables.data[i];
        if (render->scale_settle_secs <= 0.0f) continue;

        const float ease = 1.0f - exp2f(-dt / render->scale_settle_secs);
        render->scale.x += (render->scale_default.x - render->scale.x) * ease;
        render->scale.y += (render->scale_default.y - render->scale.y) * ease;
    }
}

void sys_animation(World *world, const float dt) {
    for (int i = 0; i < world->num_entities; i++) {
        if (!world->alive[i])               continue;
        if (!world->animators  .present[i]) continue;

        Animator  *anim   = &world->animators.data[i];
        const int  num_frames = anim->frames.count;

        anim->state_time += dt;

        if (num_frames == 0 || anim->frame_seconds <= 0.0f) continue;

        if (num_frames == 1) {
            anim->current_frame = 0;
        } else {
            const uint16_t last_frame_index = num_frames - 1;

            uint16_t frame_index = (int)(anim->state_time / anim->frame_seconds);

            switch (anim->mode) {
                case ANIM_NORMAL:  frame_index = min(last_frame_index, frame_index);   break;
                case ANIM_REVERSE: frame_index = max(num_frames - frame_index - 1, 0); break;
                case ANIM_LOOP:    frame_index = frame_index % num_frames;             break;
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

            anim->current_frame = frame_index;
        }
    }
}

static void emit_render_instance(
    RenderSnapshot      *out,
    const Assets        *assets,
    const EntityId       entity,
    const Position      *pos,
    const Renderable    *render,
    const TextureId      tex_id,
    Rectangle            tex_source
) {
    if (out->count >= MAX_RENDER_INSTANCES) return;

    // Zero-size texture source rect -> use the full texture
    const Texture2D texture = assets_get_texture(assets, tex_id);
    if (tex_source.width == 0.0f && tex_source.height == 0.0f) {
        tex_source = (Rectangle){
            0, 0,
            (float)texture.width,
            (float)texture.height
        };
    }

    // Set renderable to texture size if no explicit size is provided
    Vector2 base_size = render->size;
    if (base_size.x == 0.0f && base_size.y == 0.0f) {
        base_size.x = tex_source.width;
        base_size.y = tex_source.height;
    }
    const Vector2 scaled_size = (Vector2){
        base_size.x * render->scale.x,
        base_size.y * render->scale.y,
    };
    const Vector2 pivot_shift = (Vector2){
        base_size.x * render->scale_pivot.x * (1.0f - render->scale.x),
        base_size.y * render->scale_pivot.y * (1.0f - render->scale.y),
    };
    const Vector2 effective_origin = (Vector2){
        render->origin.x + pivot_shift.x,
        render->origin.y + pivot_shift.y
    };

    RenderInstance *inst = &out->instances[out->count];
    inst->entity_id  = entity;
    inst->position   = (Vector2){ pos->x, pos->y };
    inst->size       = scaled_size;
    inst->origin     = effective_origin;
    inst->rotation   = render->rotation;
    inst->tint       = render->tint;
    inst->layer      = render->layer;
    inst->texture    = texture;
    inst->tex_source = tex_source;
    out->count++;
}

void extract_render_snapshot(World *world, const Assets *assets, RenderSnapshot *out) {
    out->count = 0;

    for (int i = 0; i < world->num_entities && out->count < MAX_RENDER_INSTANCES; i++) {
        if (!world->alive[i])               continue;
        if (!world->positions  .present[i]) continue;
        if (!world->renderables.present[i]) continue;

        const EntityId    entity = (EntityId)i;
        const Position   *pos    = world_get_position(world, entity);
        const Renderable *render = world_get_renderable(world, entity);

        TextureId texture_id      = TEX_NONE;
        Rectangle tex_source_rect = (Rectangle){0};
        if (world->tex_regions.present[i]) {
            const TexRegion *region = &world->tex_regions.data[i];
            texture_id      = region->texture_id;
            tex_source_rect = region->tex_source_rect;
        } else if (world->animators.present[i]) {
            const Animator *anim = &world->animators.data[i];
            if (anim->frames.count == 0 || !anim->frames.regions) continue;

            const TexRegion frame = anim->frames.regions[anim->current_frame];
            texture_id      = frame.texture_id;
            tex_source_rect = frame.tex_source_rect;
        }
        emit_render_instance(out, assets, (EntityId)i, pos, render, texture_id, tex_source_rect);
    }

    // Sort RenderInstance's by layer, ascending. Higher layer draws on top.
    for (uint32_t i = 1; i < out->count; i++) {
        const RenderInstance inst  = out->instances[i];
        const int            layer = inst.layer;

        uint32_t j = i;
        while (j > 0 && out->instances[j - 1].layer > layer) {
            out->instances[j] = out->instances[j - 1];
            j--;
        }
        out->instances[j] = inst;
    }
}
