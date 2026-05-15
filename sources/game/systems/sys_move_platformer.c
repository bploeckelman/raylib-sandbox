#include "game/movement.h"
#include "shared/ecs_world.h"

// Inner step, operates entirely on pointers, mutates supplied state.
// AI can copy components onto its stack, call this for N steps,
// and never touch the real entity.
static void platformer_step(
    World          *world,
    const EntityId  mover_or_none, // ENTITY_NONE for hypothetical movement
    const float     dt,
    Position       *pos,
    Velocity       *vel,
    const Collider *col,
    MovePlatformer *move
) {
    // TODO: refine, this is a minimal first pass
    const MoveOptions opts = (MoveOptions){
        .max_slide_up  = move->slide_up_when_grounded,
        .skip_handlers = false,
    };
    MoveResult result;
    move_step_dt(world, pos, vel, col, mover_or_none, dt, &opts, &result);
    (void)move; // TODO: ground/jump/grav bookkeeping
}

void sys_move_platformer(World *world, const float dt) {
    for (int i = 0; i < world->num_entities; i++) {
        if (!world->alive[i]) continue;
        const EntityId entity_id = (EntityId)i;

        Position       *pos  = world_get_position(world, entity_id);
        Velocity       *vel  = world_get_velocity(world, entity_id);
        const Collider *col  = world_get_collider(world, entity_id);
        MovePlatformer *move = world_get_move_platformer(world, entity_id);
        if (!pos || !vel || !col || !move) continue;

        platformer_step(world, entity_id, dt, pos, vel, col, move);
    }
}