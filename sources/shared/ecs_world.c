#include "shared/ecs_world.h"

// ----------------------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------------------

EntityId world_create_entity(World *world) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        const EntityId id = (world->next_entity_id + (EntityId)i) % MAX_ENTITIES;
        if (!world->alive[id]) {
            world->alive[id] = true;
            if ((int)id >= world->num_entities) {
                world->num_entities = (int)id + 1;
            }
            world->next_entity_id = (id + 1) % MAX_ENTITIES;
            return id;
        }
    }
    // TODO: could we have auto-growing arrays for entities, component stores, etc... stored in the main game arena?
    return ENTITY_NONE; // pool exhausted
}

void world_destroy_entity(World *world, const EntityId id) {
    if (id >= MAX_ENTITIES) return;
    world->alive[id] = false;
    world->bounds     .present[id] = false;
    world->positions  .present[id] = false;
    world->velocities .present[id] = false;
    world->colliders  .present[id] = false;
    world->renderables.present[id] = false;
    world->tex_regions.present[id] = false;
    world->animators  .present[id] = false;
    // count not decremented, high-water mark stays
    // dead slots refill on next `entity_create()`
}

bool world_entity_is_alive(const World *world, const EntityId id) {
    return id < MAX_ENTITIES && world->alive[id];
}

// ----------------------------------------------------------------------------
// Per-component setters
// ----------------------------------------------------------------------------

void world_set_bounds     (World *world, const EntityId id, const Bounds     value) { world->bounds      .present[id] = true; world->bounds      .data[id] = value; }
void world_set_position   (World *world, const EntityId id, const Position   value) { world->positions   .present[id] = true; world->positions   .data[id] = value; }
void world_set_velocity   (World *world, const EntityId id, const Velocity   value) { world->velocities  .present[id] = true; world->velocities  .data[id] = value; }
void world_set_collider   (World *world, const EntityId id, const Collider   value) { world->colliders   .present[id] = true; world->colliders   .data[id] = value; }
void world_set_renderable (World *world, const EntityId id, const Renderable value) { world->renderables .present[id] = true; world->renderables .data[id] = value; }
void world_set_tex_region (World *world, const EntityId id, const TexRegion  value) { world->tex_regions .present[id] = true; world->tex_regions .data[id] = value; }
void world_set_animator   (World *world, const EntityId id, const Animator   value) { world->animators   .present[id] = true; world->animators   .data[id] = value; }

// ----------------------------------------------------------------------------
// Per-component getters (returns NULL if not present)
// ----------------------------------------------------------------------------

Bounds     *world_get_bounds     (World *world, const EntityId id) { return world->bounds      .present[id] ? &world->bounds      .data[id] : NULL; }
Position   *world_get_position   (World *world, const EntityId id) { return world->positions   .present[id] ? &world->positions   .data[id] : NULL; }
Velocity   *world_get_velocity   (World *world, const EntityId id) { return world->velocities  .present[id] ? &world->velocities  .data[id] : NULL; }
Collider   *world_get_collider   (World *world, const EntityId id) { return world->colliders   .present[id] ? &world->colliders   .data[id] : NULL; }
Renderable *world_get_renderable (World *world, const EntityId id) { return world->renderables .present[id] ? &world->renderables .data[id] : NULL; }
TexRegion  *world_get_tex_region (World *world, const EntityId id) { return world->tex_regions .present[id] ? &world->tex_regions .data[id] : NULL; }
Animator   *world_get_animator   (World *world, const EntityId id) { return world->animators   .present[id] ? &world->animators   .data[id] : NULL; }
