// game.c — compiled as a shared library (game.dll / libgame.so / libgame.dylib).
// All exported symbols below are looked up by name by the platform layer.
// This file knows nothing about the main loop, the window, or timing —
// only how to advance and draw the simulation.

#include "game/game.h"

#include "camera.h"
#include "game/systems.h"
#include "shared/common.h"
#include "shared/assets.h"
#include "shared/ecs_components.h"
#include "raylib.h"
#include "raymath.h"

#if defined(_WIN32)
  #define GAME_EXPORT __declspec(dllexport)
#else
  #define GAME_EXPORT __attribute__((visibility("default")))
#endif

GAME_EXPORT void game_load(GameMemory *m) {
    // Always wipe stale system entities (their handlers point into freed code),
    // and re-register, which also rebinds this DLL's component-id calls
    ecs_delete_with(m->ecs, EcsSystem);
    register_systems(m);

    if (!m->initialized) {
        m->world_curr = (WorldSnapshot){
            .camera_pos = (Vector2){ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
            .camera_zoom = 1.0f,
        };
        m->world_prev = m->world_curr;

        m->atlas_hero = assets_load_atlas(&m->assets, "atlas.rtpa", &m->arena);
        m->tex_test   = assets_load_texture(&m->assets, "test.png");
        m->tex_grid   = assets_load_texture(&m->assets, "grid.png");

        // First-time spawn - one moving sprite to verify the ECS setup, update/render pipeline
        const float width  = 100.0f;
        const float height = 100.0f;
        const ecs_entity_t e1 = ecs_new(m->ecs);
        ecs_set(m->ecs, e1, Position, { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f });
        ecs_set(m->ecs, e1, Velocity, { 200, 140 });
        ecs_set(m->ecs, e1, Renderable, { RENDERABLE_DEFAULTS, .size = (Vector2){ width, height }, .layer = 0 });
        ecs_set(m->ecs, e1, Collider,   { COLLIDER_DEFAULTS,   .size = (Vector2){ width, height } });
        const char      *anim_tag  = "hero-idle";
        const AtlasSlot *atlas      = assets_get_atlas(&m->assets, m->atlas_hero);
        const int        num_frames = atlas_num_regions_for_tag(atlas, anim_tag);
        if (num_frames > 0) {
            TexRegion *frames = ARENA_NEW_ARRAY(&m->arena, TexRegion, num_frames);
            const int frames_count = atlas_find_regions_by_tag(atlas, anim_tag, frames, num_frames);
            if (frames_count != num_frames) {
                TraceLog(LOG_WARNING, "animator(%s): num frames - expected(%d) != actual(%d)", anim_tag, num_frames, frames_count);
            }
            ecs_set(m->ecs, e1, Animator, {
                ANIMATOR_DEFAULTS,
                .mode          = ANIM_LOOP,
                .frame_seconds = 0.1f,
                .frames_count  = frames_count,
                .frames        = frames
            });
        }
        m->test_entity_1 = e1;

        const ecs_entity_t e2 = ecs_new(m->ecs);
        ecs_set(m->ecs, e2, Position, { SCREEN_WIDTH / 2.0f + 50.0f, SCREEN_HEIGHT / 2.0f + 50.0f });
        ecs_set(m->ecs, e2, Velocity, { 200, 140 });
        ecs_set(m->ecs, e2, Renderable, { RENDERABLE_DEFAULTS, .size = (Vector2){ width, height }, .layer = 1 });
        ecs_set(m->ecs, e2, Collider,   { COLLIDER_DEFAULTS,   .size = (Vector2){ width, height } });
        const char      *anim_tag_2   = "hero-run";
        const int        num_frames_2 = atlas_num_regions_for_tag(atlas, anim_tag_2);
        if (num_frames_2 > 0) {
            TexRegion *frames = ARENA_NEW_ARRAY(&m->arena, TexRegion, num_frames_2);
            const int frames_count = atlas_find_regions_by_tag(atlas, anim_tag_2, frames, num_frames_2);
            if (frames_count != num_frames_2) {
                TraceLog(LOG_WARNING, "animator(%s): num frames - expected(%d) != actual(%d)", anim_tag_2, num_frames_2, frames_count);
            }
            ecs_set(m->ecs, e2, Animator, {
                ANIMATOR_DEFAULTS,
                .mode          = ANIM_LOOP,
                .frame_seconds = 0.1f,
                .frames_count  = frames_count,
                .frames        = frames
            });
        }
        m->test_entity_2 = e2;

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
    m->world_prev = m->world_curr;
    WorldSnapshot *w = &m->world_curr;

    // TESTING: squash, stretch
    if (input->key_space) {
        Renderable *renderable = ecs_get_mut(m->ecs, m->test_entity_1, Renderable);
        renderable->scale = (Vector2){ 1.5f, 1.5f };
    }

    // TODO: camera update will go here, none yet though because it's static

    ecs_singleton_set(m->ecs, Bounds, { camera_world_bounds(w) });
    ecs_progress(m->ecs, dt);
    extract_render_snapshot(m->ecs, m->q_renderable_static, m->q_renderable_animated, &m->assets, &w->render);

    w->tick++;
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
    const Texture2D texture = assets_get_texture(&m->assets, m->tex_test);
    if (texture.id != 0) {
        const int texture_x = SCREEN_WIDTH  / 2 - texture.width  / 2;
        const int texture_y = SCREEN_HEIGHT / 2 - texture.height / 2;
        DrawTexture(texture, texture_x, texture_y, WHITE);
    }

    // Interpolate between ECS render snapshots
    const RenderSnapshot *prev = &m->world_prev.render;
    const RenderSnapshot *curr = &m->world_curr.render;
    for (uint32_t i = 0; i < curr->count; i++) {
        const RenderInstance *curr_inst = &curr->instances[i];
        const RenderInstance *prev_inst = NULL;

        // Skip if there's nothing to draw for this component renderer instance
        const Texture2D texture = assets_get_texture(&m->assets, curr_inst->tex_handle);
        if (texture.id == 0) continue;

        // Find matching instances between prev and curr based on stable id matches...
        // Linear scan; fine for now, change to hash lookup above ~200 entities
        for (uint32_t j = 0; j < prev->count; j++) {
            if (prev->stable_id[j] == curr->stable_id[i]) {
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
