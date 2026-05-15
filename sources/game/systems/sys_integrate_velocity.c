#include "ecs_systems.h"

void sys_integrate_velocity(World *world, const float dt) {
    for (int i = 0; i < world->num_entities; i++) {
        if (!world->alive[i])              continue;
        if (!world->positions .present[i]) continue;
        if (!world->velocities.present[i]) continue;

        const EntityId entity_id = (EntityId)i;
        Position       *pos =  world_get_position(world, entity_id);
        const Velocity  vel = *world_get_velocity(world, entity_id);

        pos->x += vel.value.x * dt;
        pos->y += vel.value.y * dt;
    }
}

