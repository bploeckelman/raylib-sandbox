// game.c — compiled as a shared library (game.dll / libgame.so / libgame.dylib).
// All exported symbols below are looked up by name by the platform layer.
// This file knows nothing about the main loop, the window, or timing —
// only how to advance and draw the simulation.

#include "game/game.h"
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
        m->world_curr = (GameWorld){
            .camera_pos = (Vector2){ .x = 0, .y = 0 },
            .camera_zoom = 1.0f,
        };
        m->world_prev = m->world_curr;
        m->tex_test = assets_load_texture(&m->assets, "test.png");

        // First-time spawn - one moving sprite to verify the ECS setup, update/render pipeline
        const ecs_entity_t e = ecs_new(m->ecs);
        ecs_set(m->ecs, e, Position, { 100, 300 });
        ecs_set(m->ecs, e, Velocity, {  60,   0 });
        ecs_set(m->ecs, e, Sprite, {
            .texture = m->tex_test,
            .tint = WHITE,
            .scale = (Vector2){ 1, 1 },
        });

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

GAME_EXPORT void game_update(GameMemory *m, const GameInput *in, float dt) {
    // Snapshot the just-finished step before integrating the new one.
    // After this function returns: world_prev = "t", world_curr = "t+dt"
    m->world_prev = m->world_curr;
    GameWorld *w = &m->world_curr;

    ecs_progress(m->ecs, dt);

    // examples of non-ecs state that lives in GameWorld directly
    // update_camera(w, in, dt);
    // update_screen_shake(w, dt);
    // update_transitions(w, dt);

    extract_render_snapshot(m->ecs, m->q_renderable, &w->render);

    w->tick++;
}

GAME_EXPORT void game_render(const GameMemory *m, float alpha) {

    BeginDrawing();
    ClearBackground(RAYWHITE);

    const Texture2D texture = assets_get_texture(&m->assets, m->tex_test);
    if (texture.id != 0) {
        const int texture_x = SCREEN_WIDTH  / 2 - texture.width  / 2;
        const int texture_y = SCREEN_HEIGHT / 2 - texture.height / 2;
        DrawTexture(texture, texture_x, texture_y, WHITE);
    }

    // Interpolate between the two most recent fixed-step renderable
    // snapshots from the ECS so motion is smooth independent of frame rate.
    const RenderSnapshot *prev = &m->world_prev.render;
    const RenderSnapshot *curr = &m->world_curr.render;
    for (uint32_t i = 0; i < curr->count; i++) {
        const RenderInstance *ci = &curr->instances[i];
        const RenderInstance *pi = NULL;
        // Linear scan; fine for now, change to hash lookup above ~200 entities
        for (uint32_t j = 0; j < prev->count; j++) {
            if (prev->stable_id[j] == curr->stable_id[i]) {
                pi = &prev->instances[j];
                break;
            }
        }
        Vector2 pos = pi ? (Vector2) {
            Lerp(pi->position.x, ci->position.x, alpha),
            Lerp(pi->position.y, ci->position.y, alpha)
        } : ci->position; // newly spawned: snap, don't lerp from garbage

        const Texture2D tex = assets_get_texture(&m->assets, ci->texture);
        if (tex.id != 0) {
            DrawTextureEx(tex, pos, ci->rotation, ci->scale.x, ci->tint);
        }
    }

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
