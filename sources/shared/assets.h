#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"

#include <stdbool.h>
#include <stdint.h>

#define ASSETS_MAX_TEXTURES 256
#define ASSETS_MAX_SOUNDS   128
#define ASSETS_MAX_FONTS     16
#define ASSETS_PATH_LENGTH  128

typedef struct { uint16_t slot; uint16_t gen; } TextureHandle;
typedef struct { uint16_t slot; uint16_t gen; } SoundHandle;
typedef struct { uint16_t slot; uint16_t gen; } FontHandle;

#define ASSET_HANDLE_NULL ((TextureHandle){0,0})

typedef struct {
    char       path[ASSETS_PATH_LENGTH];
    Texture2D  tex;
    long       mtime;
    uint16_t   gen; // bumped each (re)load; 0 == empty slot
    bool       in_use;
} TextureSlot;

typedef struct {
    char       path[ASSETS_PATH_LENGTH];
    Sound      sound;
    long       mtime;
    uint16_t   gen; // bumped each (re)load; 0 == empty slot
    bool       in_use;
} SoundSlot;

typedef struct {
    char       path[ASSETS_PATH_LENGTH];
    Font       font;
    int        base_size;
    long       mtime;
    uint16_t   gen; // bumped each (re)load; 0 == empty slot
    bool       in_use;
} FontSlot;

typedef struct {
    TextureSlot  textures[ASSETS_MAX_TEXTURES];
    SoundSlot    sounds  [ASSETS_MAX_SOUNDS];
    FontSlot     fonts   [ASSETS_MAX_FONTS];

    // Hot-reload: scan a few slots per frame instead of stat()-ing everything
    uint16_t     scan_cursor;
} Assets;

// Lookup-or-load. Returns ASSET_HANDLE_NULL on table-full or load failure.
TextureHandle assets_load_texture(Assets *a, const char *path);
SoundHandle   assets_load_sound  (Assets *a, const char *path);
FontHandle    assets_load_font   (Assets *a, const char *path, int base_size);

// Resolve handle → resource. Returns a fallback (1x1 magenta etc.) if stale.
Texture2D     assets_get_texture(const Assets *a, TextureHandle h);
Sound         assets_get_sound  (const Assets *a, SoundHandle   h);
Font          assets_get_font   (const Assets *a, FontHandle    h);

// Called by platform once per frame. Walks a few slots, reloads if mtime changed.
void          assets_poll_reload(Assets *a);

// Drop everything (used at shutdown, or on a "reset assets" hotkey).
void          assets_unload_all(Assets *a);

#endif //ASSETS_H
