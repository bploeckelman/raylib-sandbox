#ifndef ECS_SETUP_H
#define ECS_SETUP_H

#include "flecs.h"

ecs_world_t *ecs_setup_create_world(void);
void         ecs_setup_register_components(ecs_world_t *world);
void         ecs_setup_destroy_world(ecs_world_t *world);

#endif //ECS_SETUP_H
