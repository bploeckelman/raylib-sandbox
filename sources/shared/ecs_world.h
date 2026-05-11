#ifndef WORLD_H
#define WORLD_H

#include "shared/assets.h"
#include "shared/ecs_components.h"
#include "shared/raytmx.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_ENTITIES 1024

typedef uint32_t EntityId;
#define ENTITY_NONE ((EntityId)-1)

#define DECLARE_COMPONENT_STORE(name, component_type) \
    typedef struct {                                  \
        bool           present[MAX_ENTITIES];         \
        component_type data   [MAX_ENTITIES];         \
    } name;

DECLARE_COMPONENT_STORE(BoundsStore,     Bounds)
DECLARE_COMPONENT_STORE(PositionStore,   Position)
DECLARE_COMPONENT_STORE(VelocityStore,   Velocity)
DECLARE_COMPONENT_STORE(ColliderStore,   Collider)
DECLARE_COMPONENT_STORE(RenderableStore, Renderable)
DECLARE_COMPONENT_STORE(TexRegionStore,  TexRegion)
DECLARE_COMPONENT_STORE(AnimatorStore,   Animator)

#undef DECLARE_COMPONENT_STORE

typedef struct {
    bool     alive[MAX_ENTITIES];
    int      num_entities;
    EntityId next_entity_id;

    Bounds world_bounds;
    TmxMap *map;

    BoundsStore     bounds;
    PositionStore   positions;
    VelocityStore   velocities;
    ColliderStore   colliders;
    RenderableStore renderables;
    TexRegionStore  tex_regions;
    AnimatorStore   animators;
} World;

// Lifecycle
EntityId world_create_entity  (World *world);
void     world_destroy_entity (World *world, EntityId id);
bool     world_entity_is_alive(const World *world, EntityId id);

// Per-component setters
void world_set_bounds     (World *world, EntityId id, Bounds     value);
void world_set_position   (World *world, EntityId id, Position   value);
void world_set_velocity   (World *world, EntityId id, Velocity   value);
void world_set_collider   (World *world, EntityId id, Collider   value);
void world_set_renderable (World *world, EntityId id, Renderable value);
void world_set_tex_region (World *world, EntityId id, TexRegion  value);
void world_set_animator   (World *world, EntityId id, Animator   value);

// Per-component getters (returns NULL if not present)
Bounds     *world_get_bounds     (World *world, EntityId id);
Position   *world_get_position   (World *world, EntityId id);
Velocity   *world_get_velocity   (World *world, EntityId id);
Collider   *world_get_collider   (World *world, EntityId id);
Renderable *world_get_renderable (World *world, EntityId id);
TexRegion  *world_get_tex_region (World *world, EntityId id);
Animator   *world_get_animator   (World *world, EntityId id);

#endif //WORLD_H
