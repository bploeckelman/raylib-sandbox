// game.c - compiled as a shared library (game.dll / libgame.so... libgame.dylib for mac?)
// All exported symbols below are looked up by name by the platform layer.
// This file knows nothing about the main loop, the window, or timing,
// only how to advance and draw the simulation

#include "game.h"
#include "../common.h"
#include "raylib.h"
#include "raymath.h"

#if defined(_WIN32)
  #define GAME_EXPORT __declspec(dllexport)
#else
  #define GAME_EXPORT __attribute__((visibility("default")))
#endif

GAME_EXPORT void game_load(GameMemory *m) {
    // Load assets
    if (!m->assets_loaded) {
        m->assets.texTest = LoadTexture("test.png");
        m->assets_loaded = true;
    }

    // Initialize game world
    if (!m->initialized) {
        m->world_curr = (GameWorld){ .player_x = 640, .player_y = 360 };
        m->world_prev = m->world_curr;
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

    const Texture2D texture = m->assets.texTest;
    const int texture_x = SCREEN_WIDTH / 2 - texture.width / 2;
    const int texture_y = SCREEN_HEIGHT / 2 - texture.height / 2;
    DrawTexture(texture, texture_x, texture_y, WHITE);

    DrawCircle((int) px, (int) py, 24, BLACK);
    DrawText(TextFormat("tick %llu  fps %d", (unsigned long long) m->world_curr.tick, GetFPS()), 10, 10, 20, DARKGRAY);

    EndDrawing();
}

GAME_EXPORT void game_shutdown(const GameMemory *m) {
    UnloadTexture(m->assets.texTest);
}
