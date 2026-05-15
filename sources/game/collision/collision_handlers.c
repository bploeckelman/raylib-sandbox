#include "collision_handlers.h"

// Example handler: every entity touching a sensor passes through.
// TODO: Add real handlers
static bool applies_sensor(const World *w, const CollisionContext *ctx) {
    (void)w;
    return ctx->target_col && (ctx->target_col->mask & COL_SENSOR);
}
static CollisionResponse handle_sensor(World *world, CollisionContext *ctx) {
    (void)world; (void)ctx;
    return COL_RESP_PASSTHROUGH;
}

static const CollisionHandler HANDLERS[] = {
    { "sensor_passthrough", applies_sensor, handle_sensor },
};
static const int HANDLERS_COUNT = (int)(sizeof HANDLERS / sizeof HANDLERS[0]);

CollisionResponse collide_handlers_dispatch(World *world, CollisionContext *ctx) {
    bool any_ran = false;

    for (int i = 0; i < HANDLERS_COUNT; i++) {
        const CollisionHandler *handler = &HANDLERS[i];
        if (handler->applies && !handler->applies(world, ctx)) {
            continue;
        }
        any_ran = true;

        const CollisionResponse response = handler->handle(world, ctx);
        if (response != COL_RESP_PASSTHROUGH) return response;
        // PASSTHROUGH cascades to next handler
    }

    // Default fallback: solid stops, sensor passes, etc.
    if (any_ran) return COL_RESP_PASSTHROUGH;
    return collide_default_response(ctx->target_col ? ctx->target_col->mask : COL_NONE);
}
