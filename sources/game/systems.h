#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "game/game.h"

void register_systems(GameMemory *m);
void extract_render_snapshot(const ecs_world_t *world, ecs_query_t *query, const Assets *assets, RenderSnapshot *out);

#endif //SYSTEMS_H
