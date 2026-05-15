#include "shared/ecs_world.h"
#include "shared/ecs_components.h"
#include "collision.h"

int  collide_overlaps_at_pos(
    const World   *world,
    const Vector2  position, const Collider *collider, const EntityId exclude_id,
    const Vector2  offset,   const uint32_t mask_filter,
    EntityId      *out_hits, const int max_hits
) {
    const uint32_t effective_mask = (mask_filter != 0) ? mask_filter : collider->collides_with;
    int count = 0;

    for (int i = 0; i < world->num_entities && count < max_hits; i++) {
        if ((EntityId)i == exclude_id)            continue;
        if (!world->alive[i])                     continue;
        if (!world->positions.present[i])         continue;
        if (!world->colliders.present[i])         continue;

        const Collider *other_col = &world->colliders.data[i];
        if ((effective_mask & other_col->mask) == 0) continue;

        const Vector2 other_pos = world->positions.data[i];
        if (collide_shape_overlaps(&collider->shape, position, offset, &other_col->shape, other_pos)) {
            out_hits[count++] = (EntityId)i;
        }
    }
    return count;
}

bool collide_first_at_pos(const World *world, const Vector2 pos, const Collider *col, const EntityId exclude_id, const Vector2 offset, const uint32_t mask_filter, EntityId *out_hit) {
    return collide_overlaps_at_pos(world, pos, col, exclude_id, offset, mask_filter, out_hit, 1) > 0;

}

bool collide_would_collide_pos(const World *world, const Vector2 pos, const Collider *col, const EntityId exclude_id, const Vector2 offset, const uint32_t mask_filter) {
    EntityId discard;
    return collide_first_at_pos(world, pos, col, exclude_id, offset, mask_filter, &discard);
}

bool collide_is_on_ground_pos(const World *world, const Vector2 pos, const Collider *col, const EntityId exclude_id) {
    // raylib: +Y is down, '1 pixel below' is +1 Y
    return collide_would_collide_pos(world, pos, col, exclude_id, (Vector2){0, 1}, COL_SOLID);
}

int  collide_overlaps_at(const World *world, const EntityId entity_id, const Vector2 offset, const uint32_t mask, EntityId *out, const int max) {
    const Position  pos = *world_get_position((World*)world, entity_id);
    const Collider *col =  world_get_collider((World*)world, entity_id);
    return collide_overlaps_at_pos(world, pos, col, entity_id, offset, mask, out, max);
}

bool collide_first_at(const World *world, const EntityId entity_id, const Vector2 offset, const uint32_t mask, EntityId *out) {
    const Position  pos = *world_get_position((World*)world, entity_id);
    const Collider *col =  world_get_collider((World*)world, entity_id);
    return collide_first_at_pos(world, pos, col, entity_id, offset, mask, out);
}

bool collide_would_collide(const World *world, const EntityId entity_id, const Vector2 offset, const uint32_t mask) {
    const Position  pos = *world_get_position((World*)world, entity_id);
    const Collider *col =  world_get_collider((World*)world, entity_id);
    return collide_would_collide_pos(world, pos, col, entity_id, offset, mask);
}

bool collide_is_on_ground (const World *world, const EntityId entity_id) {
    const Position  pos = *world_get_position((World*)world, entity_id);
    const Collider *col =  world_get_collider((World*)world, entity_id);
    return collide_is_on_ground_pos(world, pos, col, entity_id);
}
