// platform.c — compiled as the main executable. Owns the window, owns GameMemory,
// runs the timing loop, gathers input, and dynamically loads the game module.

#include "game/game.h"
#include "shared/common.h"
#include "shared/assets.h"
#include "raylib.h"
#include "resource_dir.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #define NOGDI       // strips wingdi.h: Rectangle, etc.
  #define NOUSER      // strips winuser.h: CloseWindow, ShowCursor, DrawText macro, ...
  #include <windows.h>
  typedef HMODULE LibHandle;
  #define LIB_LOAD(p)     LoadLibraryA(p)
  #define LIB_SYM(h, n)   ((void*)GetProcAddress((h), (n)))
  #define LIB_FREE(h)     FreeLibrary(h)
  #define GAME_DLL_BUILT  "game.dll"
  #define GAME_LOADED_FMT "%sgame_loaded_%d.dll"
#else
  #include <dlfcn.h>
  typedef void *LibHandle;
  #define LIB_LOAD(p)     dlopen((p), RTLD_NOW | RTLD_LOCAL)
  #define LIB_SYM(h, n)   dlsym((h), (n))
  #define LIB_FREE(h)     dlclose(h)
  #define GAME_DLL_BUILT  "./libgame.so"
  #define GAME_LOADED_FMT "%slibgame_loaded_%d.so"
#endif

// Resolved at startup so hot-reload still works after SearchAndSetResourceDir changes the working dir
static char g_dll_built_path[1024];
static int  g_load_counter = 0;

// In BSS - zero-initialized, stable address across reloads. For a real
// project, VirtualAlloc/mmap at a fixed base for full pointer stability.
static GameMemory g_memory;

typedef struct {
    LibHandle handle;
    GameApi   api;
    long      built_mtime;       // mtime of game lib at last successful load
    char      loaded_path[1024]; // unique per game lib load (workaround for Win DLL write-lock)
} GameModule;

static void game_module_unload(GameModule *m) {
    if (m->handle) {
        LIB_FREE(m->handle);
        if (m->loaded_path[0]) FileRemove(m->loaded_path);
    }
    memset(m, 0, sizeof *m);
}

static bool game_module_load(GameModule *m) {
    if (!FileExists(g_dll_built_path)) return false;

    const char *app_dir = GetApplicationDirectory();
    snprintf(m->loaded_path, sizeof m->loaded_path,
        GAME_LOADED_FMT, app_dir, g_load_counter++);

    // Suppress raylib log output around this call, it happens
    // repeatedly while the new game module lib is still building
    SetTraceLogLevel(LOG_ERROR);
    bool copied = FileCopy(g_dll_built_path, m->loaded_path);
    SetTraceLogLevel(LOG_INFO);
    if (!copied) return false;

    m->handle = LIB_LOAD(m->loaded_path);
    if (!m->handle) return false;

    m->api.load     = (game_load_fn)     LIB_SYM(m->handle, "game_load");
    m->api.unload   = (game_unload_fn)   LIB_SYM(m->handle, "game_unload");
    m->api.update   = (game_update_fn)   LIB_SYM(m->handle, "game_update");
    m->api.render   = (game_render_fn)   LIB_SYM(m->handle, "game_render");
    m->api.shutdown = (game_shutdown_fn) LIB_SYM(m->handle, "game_shutdown");

    if (!m->api.load || !m->api.unload || !m->api.update || !m->api.render || !m->api.shutdown) {
        game_module_unload(m);
        return false;
    }

    m->built_mtime = GetFileModTime(g_dll_built_path);
    return true;
}

static bool game_module_should_reload(const GameModule *m) {
    if (!FileExists(g_dll_built_path)) return false;

    // Suppress raylib log output around this call, it happens
    // repeatedly while the new game module lib is still building
    SetTraceLogLevel(LOG_ERROR);
    long mtime = GetFileModTime(g_dll_built_path);
    SetTraceLogLevel(LOG_INFO);

    return mtime != 0 && mtime != m->built_mtime;
}

static void gather_input(GameInput *in) {
    *in = (GameInput){0};
    in->key_left  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    in->key_right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    in->key_up    = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    in->key_down  = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
    in->key_space = IsKeyDown(KEY_SPACE);
    in->mouse_left  = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    in->mouse_right = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    Vector2 mp  = GetMousePosition();
    in->mouse_x = mp.x;
    in->mouse_y = mp.y;
}

int main(void) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(0); // run as fast as vsync (or uncapped); state updates are decoupled from rendering

    // Persist game lib's absolute path so it can still be found after SearchAndSetResourceDir(...)
    {
        const char *app_dir = GetApplicationDirectory();
        snprintf(g_dll_built_path,  sizeof g_dll_built_path,
            "%s%s", app_dir, GAME_DLL_BUILT);
    }

    SearchAndSetResourceDir("resources");

    GameModule game = {0};
    if (!game_module_load(&game)) {
        TraceLog(LOG_FATAL, "could not load game module: %s", g_dll_built_path);
        CloseWindow();
        return 1;
    }
    game.api.load(&g_memory);

    // ----- Gaffer's "Fix Your Timestep" w/interpolation -----
    // https://gafferongames.com/post/fix_your_timestep/
    const double dt        = 1.0 / 60.0; // simulation step (Hz independent of display)
    const double max_frame = 0.25; // spiral of death clamp
    double accumulator     = 0.0;
    double last_time       = GetTime();

    while (!WindowShouldClose()) {
        double now = GetTime();
        double frame = now - last_time;
        if (frame > max_frame) frame = max_frame;
        last_time    = now;
        accumulator += frame;

        // Hot reload: load-new-first, then swap, then unload-old.
        // Old module stays valid until the new one is verified working.
        if (game_module_should_reload(&game)) {
            GameModule new_game = {0};
            if (game_module_load(&new_game)) {
                // Success: tear down the old module and swap in the new
                game.api.unload(&g_memory);
                game_module_unload(&game);
                game = new_game;
                game.api.load(&g_memory);
                TraceLog(LOG_INFO, "game module reloaded");
            }
            // Failure: built_mtime unchanged → retry next frame.
        }

        GameInput input;
        gather_input(&input);

        // Hot reload: live update of modified assets
        assets_poll_reload(&g_memory.assets);

        // Integrate at fixed dt as many times as the accumulator allows
        while (accumulator >= dt) {
            game.api.update(&g_memory, &input, (float)dt);
            accumulator -= dt;
        }

        // Remaining accumulator becomes the interpolation factor for rendering
        float alpha = (float)(accumulator / dt);
        game.api.render(&g_memory, alpha);
    }

    // Order matters: shutdown frees GPU/audio resources via raylib,
    // requires GL context to still be alive. CloseWindow destroys it.
    game.api.shutdown(&g_memory);
    game.api.unload(&g_memory);
    game_module_unload(&game);

    CloseWindow();
    return 0;
}
