#include "ecs_systems.h"
#include "game/camera.h"

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
