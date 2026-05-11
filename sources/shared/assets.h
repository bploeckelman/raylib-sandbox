#ifndef ASSETS_H
#define ASSETS_H

#include "shared/arena.h"
#include "raylib.h"

#define ATLAS_NAME_LENGTH    32
#define ATLAS_TAG_LENGTH     32

typedef enum {
    ATLAS_NONE = 0,
    ATLAS_HERO,
    ATLAS_COUNT,
} AtlasId;

typedef enum {
    TEX_NONE = 0,
    TEX_TEST,
    TEX_GRID,
    TEX_ATLAS_HERO,
    TEX_COUNT,
} TextureId;

typedef enum {
    SOUND_NONE = 0,
    // ...
    SOUND_COUNT,
} SoundId;

typedef enum {
    FONT_NONE = 0,
    // ...
    FONT_COUNT,
} FontId;

typedef struct {
    char       name[ATLAS_NAME_LENGTH];
    char       tag [ATLAS_TAG_LENGTH];
    Rectangle  tex_source_rect;
} AtlasSprite;

typedef struct {
    const char  *path;
    TextureId    tex_id;  // which TextureId is the atlas image
    AtlasSprite *sprites; // arena-owned
    int          sprite_count;
    long         mtime;
} Atlas;

#define TEX_REGION_DEFAULTS .tex_source_rect = (Rectangle){ 0, 0, 0, 0 }

typedef struct { TextureId texture_id; Rectangle tex_source_rect; } TexRegion;

typedef struct { TexRegion *regions; int count; } AtlasRegions;

typedef struct {
    Atlas       atlases[ATLAS_COUNT]; // atlases keep more metadata than the other resource types

    Texture2D   textures  [TEX_COUNT];
    long        tex_mtimes[TEX_COUNT];

    Sound       sounds      [SOUND_COUNT];
    long        sound_mtimes[SOUND_COUNT];

    Font        fonts      [FONT_COUNT];
    long        font_mtimes[FONT_COUNT];
} Assets;

void assets_init       (Assets *assets, Arena *arena);
void assets_poll_reload(Assets *assets, Arena *arena);
void assets_unload_all (Assets *assets);

const Atlas *assets_get_atlas  (const Assets *assets, AtlasId   id);
Texture2D    assets_get_texture(const Assets *assets, TextureId id);
Sound        assets_get_sound  (const Assets *assets, SoundId   id);
Font         assets_get_font   (const Assets *assets, FontId    id);

TexRegion    atlas_find_region         (const Atlas *atlas, const char *region_name);
AtlasRegions atlas_find_regions_by_tag (const Atlas *atlas, const char *tag, Arena *arena);
int          atlas_count_regions_by_tag(const Atlas *atlas, const char *tag);

#endif //ASSETS_H
