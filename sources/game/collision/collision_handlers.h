#ifndef COLLISION_HANDLERS_H
#define COLLISION_HANDLERS_H

#include "collision.h"

// Walks file-static handler array in collision_handlers.c.
// First-applicable wins, with PASSTHROUGH cascading to the next handler.
// Final fallback when no handler ran: collide_default_response(target.mask).
CollisionResponse collide_handlers_dispatch(World *world, CollisionContext *ctx);

#endif //COLLISION_HANDLERS_H
