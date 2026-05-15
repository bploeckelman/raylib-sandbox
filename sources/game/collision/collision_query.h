#ifndef COLLISION_QUERY_H
#define COLLISION_QUERY_H

#include "shared/ecs_world.h"

int  collide_overlaps_at_pos  (const World *world, Vector2 position, const Collider *collider, EntityId exclude_id, Vector2 offset, uint32_t mask_filter, EntityId *out_hits, int max_hits);
bool collide_first_at_pos     (const World *world, Vector2 pos, const Collider *collider, EntityId exclude_id, Vector2 offset, uint32_t mask_filter, EntityId *out_hit);
bool collide_would_collide_pos(const World *world, Vector2 pos, const Collider *collider, EntityId exclude_id, Vector2 offset, uint32_t mask_filter);
bool collide_is_on_ground_pos (const World *world, Vector2 pos, const Collider *collider, EntityId exclude_id);

int  collide_overlaps_at  (const World *, EntityId, Vector2 offset, uint32_t mask, EntityId *out, int max);
bool collide_first_at     (const World *, EntityId, Vector2 offset, uint32_t mask, EntityId *out);
bool collide_would_collide(const World *, EntityId, Vector2 offset, uint32_t mask);
bool collide_is_on_ground (const World *, EntityId);

#endif //COLLISION_QUERY_H
