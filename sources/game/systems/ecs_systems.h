#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "game/game.h"

void sys_animation         (World *world, float dt);
void sys_bounce_in_bounds  (World *world, Bounds bounds);
void sys_integrate_velocity(World *world, float dt);
void sys_move_platformer   (World *world, float dt);
void sys_scale_return      (World *world, float dt);

void extract_render_snapshot(World *world, const Assets *assets, RenderSnapshot *out);

#endif //SYSTEMS_H
