#ifndef ASSETS_H
#define ASSETS_H

#include "shared/arena.h"
#include "raylib.h"

#include <stdbool.h>
#include <stdint.h>

#define ASSETS_MAX_ATLASES    4
#define ASSETS_MAX_TEXTURES 256
#define ASSETS_MAX_SOUNDS   128
#define ASSETS_MAX_FONTS     16
#define ASSETS_PATH_LENGTH  128
#define ATLAS_NAME_LENGTH    32
#define ATLAS_TAG_LENGTH     32

typedef struct { uint16_t slot; uint16_t gen; } AtlasHandle;

typedef struct { uint16_t slot; uint16_t gen; } TextureHandle;

typedef struct { uint16_t slot; uint16_t gen; } SoundHandle;

typedef struct { uint16_t slot; uint16_t gen; } FontHandle;

#define ATLAS_HANDLE_NULL   ((AtlasHandle){0,0})
#define TEXTURE_HANDLE_NULL ((TextureHandle){0,0})
#define SOUND_HANDLE_NULL   ((SoundHandle){0,0})
#define FONT_HANDLE_NULL    ((FontHandle){0,0})

#define TEX_REGION_DEFAULTS .source = (Rectangle){0,0,0,0}

typedef struct {
    TextureHandle  tex_handle;
    Rectangle      tex_source;  // TEX_REGION_DEFAULTS.source = full texture
} TexRegion;

typedef struct {
    char       name[ATLAS_NAME_LENGTH];
    char       tag [ATLAS_TAG_LENGTH];
    Rectangle  tex_source;
} AtlasSprite;

typedef struct {
    char           path[ASSETS_PATH_LENGTH]; // .rtpa path
    TextureHandle  tex_handle;               // atlas image, from 'a' line
    AtlasSprite   *sprites;                  // owned by GameMemory Arena
    int            count;
    long           mtime;
    uint16_t       gen;
    bool           in_use;
} AtlasSlot;

typedef struct {
    char       path[ASSETS_PATH_LENGTH];
    Texture2D  texture;
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
    AtlasSlot    atlases [ASSETS_MAX_ATLASES];
    TextureSlot  textures[ASSETS_MAX_TEXTURES];
    SoundSlot    sounds  [ASSETS_MAX_SOUNDS];
    FontSlot     fonts   [ASSETS_MAX_FONTS];

    // Hot-reload: scan a few slots per frame instead of stat()-ing everything
    uint16_t     scan_cursor;
} Assets;

// Lookup-or-load. Returns ASSET_HANDLE_NULL on table-full or load failure.
AtlasHandle   assets_load_atlas  (Assets *assets, const char *path, Arena *arena);
TextureHandle assets_load_texture(Assets *assets, const char *path);
SoundHandle   assets_load_sound  (Assets *assets, const char *path);
FontHandle    assets_load_font   (Assets *assets, const char *path, int base_size);

// Resolve handle → resource.
const AtlasSlot *assets_get_atlas  (const Assets *assets, AtlasHandle   handle);
Texture2D        assets_get_texture(const Assets *assets, TextureHandle handle);
Sound            assets_get_sound  (const Assets *assets, SoundHandle   handle);
Font             assets_get_font   (const Assets *assets, FontHandle    handle);

TexRegion        atlas_find_region        (const AtlasSlot *atlas, const char *region_name);
int              atlas_find_regions_by_tag(const AtlasSlot *atlas, const char *tag, TexRegion *regions_out, int max);
int              atlas_num_regions_for_tag(const AtlasSlot *atlas, const char *tag);

// Called by platform once per frame. Walks a few slots, reloads if mtime changed.
void          assets_poll_reload(Assets *assets, Arena *arena);

// Drop everything (used at shutdown, or on a "reset assets" hotkey).
void          assets_unload_all(Assets *assets);

#endif //ASSETS_H
