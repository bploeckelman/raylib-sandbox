#include "ecs_systems.h"

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

        const EntityId  entity_id = (EntityId)i;
        Position       *pos =  world_get_position(world, entity_id);
        Velocity       *vel =  world_get_velocity(world, entity_id);
        const Collider  col = *world_get_collider(world, entity_id);

        const ShapeRect collider_rect = col.shape.as.rect;
        const float collider_left     = pos->x + collider_rect.offset.x;
        const float collider_top      = pos->y + collider_rect.offset.y;
        const float collider_right    = collider_left + collider_rect.size.x;
        const float collider_bottom   = collider_top  + collider_rect.size.y;

        // Keep positions in bounds, invert velocities if at bounds
        if (collider_left   < world_left)   { pos->x += (world_left      - collider_left); if (vel->value.x < 0) vel->value.x = -vel->value.x; }
        if (collider_right  > world_right)  { pos->x -= (collider_right  - world_right);   if (vel->value.x > 0) vel->value.x = -vel->value.x; }
        if (collider_top    < world_top)    { pos->y += (world_top       - collider_top);  if (vel->value.y < 0) vel->value.y = -vel->value.y; }
        if (collider_bottom > world_bottom) { pos->y -= (collider_bottom - world_bottom);  if (vel->value.y > 0) vel->value.y = -vel->value.y; }
    }
}
