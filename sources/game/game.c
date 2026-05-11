// game.c — compiled as a shared library (game.dll / libgame.so / libgame.dylib).
// All exported symbols below are looked up by name by the platform layer.
// This file knows nothing about the main loop, the window, or timing —
// only how to advance and draw the simulation.

#include "game/game.h"
#include "game/camera.h"
#include "game/ecs_systems.h"
#include "shared/assets.h"
#include "shared/common.h"
#include "raylib.h"
#include "raymath.h"

#if defined(_WIN32)
  #define GAME_EXPORT __declspec(dllexport)
#else
  #define GAME_EXPORT __attribute__((visibility("default")))
#endif

static EntityId spawn_animated(
    GameMemory    *m,
    const Vector2  pos,
    const Vector2  vel,
    const Vector2  size,
    const int      layer,
    const char    *anim_tag
) {
    World        *world  = &m->world;
    Arena        *arena  = &m->arena;
    const Assets *assets = &m->assets;

    const EntityId entity = world_create_entity(world);
    if (entity == ENTITY_NONE) return ENTITY_NONE;

    world_set_position  (world, entity, pos);
    world_set_velocity  (world, entity, vel);
    world_set_renderable(world, entity, (Renderable){ RENDERABLE_DEFAULTS, .size = size, .layer = layer });
    world_set_collider  (world, entity, (Collider)  { COLLIDER_DEFAULTS,   .size = size });

    const Atlas        *atlas         = assets_get_atlas(assets, ATLAS_HERO);
    const AtlasRegions  atlas_regions = atlas_find_regions_by_tag(atlas, anim_tag, arena);
    if (atlas_regions.count > 0) {
        world_set_animator(world, entity, (Animator){
            ANIMATOR_DEFAULTS,
            .mode          = ANIM_LOOP,
            .frames        = atlas_regions,
            .frame_seconds = 0.1f,
        });
    } else {
        TraceLog(LOG_WARNING, "anim(%s): no atlas regions found", anim_tag);
    }

    return entity;
}

GAME_EXPORT void game_load(GameMemory *m) {
    if (!m->initialized) {
        const Vector2 screen_center = (Vector2){ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f };

        m->world_curr = (WorldSnapshot){
            .camera_pos  = screen_center,
            .camera_zoom = 1.0f,
        };
        m->world_prev = m->world_curr;

        assets_init(&m->assets, &m->arena);

        const Vector2 size = (Vector2){ 100, 100 };
        const Vector2 vel  = (Vector2){ 200, 140 };
        m->test_entity_1 = spawn_animated(m, screen_center, vel, size, 0, "hero-idle");
        m->test_entity_2 = spawn_animated(m, screen_center, vel, size, 1, "hero-run");

        m->initialized = true;
    }
    // NOTE: Re-bind anything tied to this module's code/.rodata here.
    // GameWorld values are already valid because GameMemory lives in the platform.
}

GAME_EXPORT void game_unload(GameMemory *m) {
    (void) m;
    // NOTE: Release transient module-local resources here, if any.
    // Do NOT touch GameMemory contents that should survive the reload.
}

GAME_EXPORT void game_update(GameMemory *m, const GameInput *input, const float dt) {
    // Snapshot the just-finished step before integrating the new one.
    // After this function returns: world_prev = "t", world_curr = "t+dt"
    m->world_prev           =  m->world_curr;
    WorldSnapshot *snapshot = &m->world_curr;
    World *world            = &m->world;

    // TESTING: squash, stretch
    if (input->key_space) {
        Renderable *renderable = world_get_renderable(world, m->test_entity_1);
        if (renderable) renderable->scale = (Vector2){ 1.5f, 1.5f };
    }

    m->world.world_bounds = camera_world_bounds(snapshot);

    // TODO: camera update will go here, none yet though because it's static

    // Run entity systems, ORDER MATTERS!
    sys_integrate_velocity(world, dt);
    sys_scale_return      (world, dt);
    sys_animation         (world, dt);
    sys_bounce_in_bounds  (world, world->world_bounds);

    extract_render_snapshot(world, &m->assets, &snapshot->render);
    snapshot->tick++;
}

GAME_EXPORT void game_render(const GameMemory *m, const float alpha) {
    const float   cam_zoom = Lerp(m->world_prev.camera_zoom, m->world_curr.camera_zoom, alpha);
    const Vector2 cam_pos  = (Vector2){
        Lerp(m->world_prev.camera_pos.x, m->world_curr.camera_pos.x, alpha),
        Lerp(m->world_prev.camera_pos.y, m->world_curr.camera_pos.y, alpha),
    };

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Apply 'world' camera transform
    BeginMode2D(camera_to_raylib(cam_pos, cam_zoom));

    // Draw background texture
    const Texture2D background = assets_get_texture(&m->assets, TEX_TEST);
    if (background.id != 0) {
        const int texture_x = (SCREEN_WIDTH  - background.width ) / 2;
        const int texture_y = (SCREEN_HEIGHT - background.height) / 2;
        DrawTexture(background, texture_x, texture_y, WHITE);
    }

    // Interpolate between ECS render snapshots
    const RenderSnapshot *prev = &m->world_prev.render;
    const RenderSnapshot *curr = &m->world_curr.render;
    for (uint32_t i = 0; i < curr->count; i++) {
        const RenderInstance *curr_inst = &curr->instances[i];
        const RenderInstance *prev_inst = NULL;

        // Skip if there's nothing to draw for this component renderer instance
        const Texture2D texture = curr_inst->texture;
        if (texture.id == 0) continue;

        // Find matching instances between prev and curr based on stable id matches...
        // Linear scan; fine for now, change to hash lookup above ~200 entities
        for (uint32_t j = 0; j < prev->count; j++) {
            if (prev->instances[j].entity_id == curr->instances[i].entity_id) {
                prev_inst = &prev->instances[j];
                break;
            }
        }

        const Vector2 pos = prev_inst ? (Vector2) {
            Lerp(prev_inst->position.x, curr_inst->position.x, alpha),
            Lerp(prev_inst->position.y, curr_inst->position.y, alpha)
        } : curr_inst->position; // newly spawned: snap, don't lerp from garbage

        const Rectangle dest   = (Rectangle){ pos.x, pos.y, curr_inst->size.x, curr_inst->size.y };
        const Vector2   origin = (Vector2){ -curr_inst->origin.x, -curr_inst->origin.y };
        DrawTexturePro(texture, curr_inst->tex_source, dest, origin, curr_inst->rotation, curr_inst->tint);
    }

    // End 'world' camera transform
    EndMode2D();

    // Draw info text in screen space overlay
    DrawText(TextFormat("fps %d | ents %d | tick %llu",
        GetFPS(), curr->count, (unsigned long long) m->world_curr.tick),
        10, 10, 20, DARKGRAY);

    EndDrawing();
}

GAME_EXPORT void game_shutdown(GameMemory *m) {
    // Final teardown — release every GPU/audio handle. Called once at exit,
    // before raylib's GL context is destroyed by CloseWindow().
    assets_unload_all(&m->assets);
}
