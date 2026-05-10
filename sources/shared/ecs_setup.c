#include "shared/ecs_setup.h"
#include "shared/ecs_components.h"

ECS_COMPONENT_DECLARE(Bounds);
ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(Velocity);
ECS_COMPONENT_DECLARE(Collider);
ECS_COMPONENT_DECLARE(Renderable);
ECS_COMPONENT_DECLARE(TexRegion);
ECS_COMPONENT_DECLARE(Animator);

void ecs_setup_register_components(ecs_world_t *world) {
    ECS_COMPONENT_DEFINE(world, Bounds);
    ECS_COMPONENT_DEFINE(world, Position);
    ECS_COMPONENT_DEFINE(world, Velocity);
    ECS_COMPONENT_DEFINE(world, Collider);
    ECS_COMPONENT_DEFINE(world, Renderable);
    ECS_COMPONENT_DEFINE(world, TexRegion);
    ECS_COMPONENT_DEFINE(world, Animator);
}

ecs_world_t *ecs_setup_create_world(void) {
    ecs_world_t *world = ecs_init();
    ecs_setup_register_components(world);
    return world;
}

void ecs_setup_destroy_world(ecs_world_t *world) {
    if (world) ecs_fini(world);
}
