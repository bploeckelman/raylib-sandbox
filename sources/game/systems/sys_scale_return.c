#include "ecs_systems.h"

void sys_scale_return(World *world, const float dt) {
    for (int i = 0; i < world->num_entities; i++) {
        if (!world->alive[i])               continue;
        if (!world->renderables.present[i]) continue;

        const EntityId entity_id = (EntityId)i;
        Renderable *render = world_get_renderable(world, i);
        if (render->scale_settle_secs <= 0.0f) continue;

        const float ease = 1.0f - exp2f(-dt / render->scale_settle_secs);
        render->scale.x += (render->scale_default.x - render->scale.x) * ease;
        render->scale.y += (render->scale_default.y - render->scale.y) * ease;
    }
}
