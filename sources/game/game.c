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
        m->world_curr = (GameWorld){
            .camera_pos = (Vector2){ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
            .camera_zoom = 1.0f,
        };
        m->world_prev = m->world_curr;
        m->tex_test = assets_load_texture(&m->assets, "test.png");
        m->tex_grid = assets_load_texture(&m->assets, "grid.png");

        // First-time spawn - one moving sprite to verify the ECS setup, update/render pipeline
        const Texture2D tex = assets_get_texture(&m->assets, m->tex_grid);
        const float cw = 100.0f;
        const float ch = 100.0f;
        const float sx = cw / (float)tex.width;
        const float sy = ch / (float)tex.height;
        const ecs_entity_t e = ecs_new(m->ecs);
        ecs_set(m->ecs, e, Position, { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f });
        ecs_set(m->ecs, e, Velocity, { 200, 140 });
        ecs_set(m->ecs, e, Sprite, {
            .tint = WHITE,
            .texture = m->tex_grid,
            .scale = (Vector2){ sx, sy },
        });
        ecs_set(m->ecs, e, Collider, {
            .origin = (Vector2){ 0, 0 },
            .size   = (Vector2){ cw, ch },
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

    // TODO: camera update will go here, none yet though because it's static

    ecs_singleton_set(m->ecs, Bounds, { camera_world_bounds(w) });
    ecs_progress(m->ecs, dt);
    extract_render_snapshot(m->ecs, m->q_renderable, &w->render);

    w->tick++;
}

GAME_EXPORT void game_render(const GameMemory *m, float alpha) {
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
        const RenderInstance *ci = &curr->instances[i];
        const RenderInstance *pi = NULL;

        // Skip if there's nothing to draw for this component renderr instance
        const Texture2D tex = assets_get_texture(&m->assets, ci->texture);
        if (tex.id == 0) continue;

        // Find matching instances between prev and curr based on stable id matches...
        // Linear scan; fine for now, change to hash lookup above ~200 entities
        for (uint32_t j = 0; j < prev->count; j++) {
            if (prev->stable_id[j] == curr->stable_id[i]) {
                pi = &prev->instances[j];
                break;
            }
        }

        const Vector2 pos = pi ? (Vector2) {
            Lerp(pi->position.x, ci->position.x, alpha),
            Lerp(pi->position.y, ci->position.y, alpha)
        } : ci->position; // newly spawned: snap, don't lerp from garbage

        const float dw = (float)tex.width  * ci->scale.x;
        const float dh = (float)tex.height * ci->scale.y;
        const Rectangle source = (Rectangle){ 0, 0, (float)tex.width, (float)tex.height };
        const Rectangle dest   = (Rectangle){ pos.x, pos.y, dw, dh };
        const Vector2   origin = (Vector2){ -ci->origin.x, -ci->origin.y };
        DrawTexturePro(tex, source, dest, origin, ci->rotation, ci->tint);
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
