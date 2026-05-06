// game.c — compiled as a shared library (game.dll / libgame.so / libgame.dylib).
// All exported symbols below are looked up by name by the platform layer.
// This file knows nothing about the main loop, the window, or timing —
// only how to advance and draw the simulation.

#include "game.h"
#include "shared/common.h"
#include "shared/assets.h"
#include "raylib.h"
#include "raymath.h"

#if defined(_WIN32)
  #define GAME_EXPORT __declspec(dllexport)
#else
  #define GAME_EXPORT __attribute__((visibility("default")))
#endif

GAME_EXPORT void game_load(GameMemory *m) {
    if (!m->initialized) {
        m->world_curr = (GameWorld){ .player_x = 640, .player_y = 360 };
        m->world_prev = m->world_curr;
        m->tex_test = assets_load_texture(&m->assets, "test.png");
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

    const float accel = 2400.0f;
    const float drag = 6.0f;

    float ax = ((float) in->key_right - (float) in->key_left) * accel;
    float ay = ((float) in->key_down - (float) in->key_up) * accel;

    w->player_vx += (ax - w->player_vx * drag) * dt;
    w->player_vy += (ay - w->player_vy * drag) * dt;
    w->player_x += w->player_vx * dt;
    w->player_y += w->player_vy * dt;
    w->tick++;
}

GAME_EXPORT void game_render(const GameMemory *m, float alpha) {
    // Lerp between the two most recent fixed-step snapshots
    // so motion is smooth regardless of renderer's frame rate.
    float px = Lerp(m->world_prev.player_x, m->world_curr.player_x, alpha);
    float py = Lerp(m->world_prev.player_y, m->world_curr.player_y, alpha);

    BeginDrawing();

    ClearBackground(RAYWHITE);

    const Texture2D texture = assets_get_texture(&m->assets, m->tex_test);
    if (texture.id != 0) {
        const int texture_x = SCREEN_WIDTH  / 2 - texture.width  / 2;
        const int texture_y = SCREEN_HEIGHT / 2 - texture.height / 2;
        DrawTexture(texture, texture_x, texture_y, WHITE);
    }

    DrawCircle((int) px, (int) py, 24, MAGENTA);
    DrawText(TextFormat("fps %d  tick %llu", GetFPS(), (unsigned long long) m->world_curr.tick), 10, 10, 20, DARKGRAY);

    EndDrawing();
}

GAME_EXPORT void game_shutdown(GameMemory *m) {
    // Final teardown — release every GPU/audio handle. Called once at exit,
    // before raylib's GL context is destroyed by CloseWindow().
    assets_unload_all(&m->assets);
}
