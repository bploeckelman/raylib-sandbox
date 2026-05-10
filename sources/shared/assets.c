#include "shared/assets.h"
#include "shared/arena.h"

#include <string.h>
#include <stdio.h>

// ----------------------------------------------------------------------------
// Static Asset path tables: canonical "assets this game knows about" list.
// Register new assets by editing enums in assets.h and these tables together.
// ----------------------------------------------------------------------------

static const char *ATLAS_PATHS[ATLAS_COUNT] = {
    [ATLAS_NONE] = NULL,
    [ATLAS_HERO] = "atlas.rtpa",
};

static const char *TEXTURE_PATHS[TEX_COUNT] = {
    [TEX_NONE]       = NULL,
    [TEX_TEST]       = "test.png",
    [TEX_GRID]       = "grid.png",
    [TEX_ATLAS_HERO] = "atlas.png",
};

static const char *SOUND_PATHS[SOUND_COUNT] = {
    [SOUND_NONE] = NULL,
};

static const char *FONT_PATHS[FONT_COUNT] = {
    [FONT_NONE] = NULL,
};

// Per-font base sizes (`LoadFontEx` param).
// Add a row alongside `FONT_PATHS` whenever a new font is added.
static const int FONT_SIZES[FONT_COUNT] = {
    [FONT_NONE] = 0,
};

// Which TextureId backs each AtlasId.
// The '.rtpa' file also references the image by filename,
// but this is ignored at parse time in lieu of this table.
static const TextureId ATLAS_IMAGES[ATLAS_COUNT] = {
    [ATLAS_NONE] = TEX_NONE,
    [ATLAS_HERO] = TEX_ATLAS_HERO,
};

// ----------------------------------------------------------------------------
// Atlas data parser ('.rtpa' files), texture binding uses `ATLAS_IMAGES`.
// ----------------------------------------------------------------------------

static bool parse_rtpa(Atlas *atlas, const char *text, Arena *arena) {
    int         expected = -1;
    const char *cursor   = text;

    while (*cursor) {
        const char *eol = strchr(cursor, '\n');
        size_t      len = eol ? (size_t)(eol - cursor) : strlen(cursor);
        char        line[512];
        if (len >= sizeof line) len = sizeof line - 1;
        memcpy(line, cursor, len);
        line[len] = 0;
        cursor = eol ? eol + 1 : cursor + len;

        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == 0 || *p == '#' || *p == '\r') continue;

        if (line[0] == 'a' && line[1] == ' ') {
            char img[128];
            int  w, h, count, is_font, fsize;
            if (sscanf(line, "a %127s %d %d %d %d %d",
                       img, &w, &h, &count, &is_font, &fsize) == 6) {
                atlas->sprites      = ARENA_NEW_ARRAY(arena, AtlasSprite, count);
                atlas->sprite_count = 0;
                expected            = count;
                if (!atlas->sprites) return false;
            }
        } else if (line[0] == 's' && line[1] == ' ' && atlas->sprites
                   && atlas->sprite_count < expected) {
            char name[ATLAS_NAME_LENGTH], tag_q[ATLAS_TAG_LENGTH + 2];
            int  ox, oy, px, py, sw, sh, pad, trimmed;
            int  tx, ty, tw, th, ct, cx, cy, cw, ch;
            const int matched = sscanf(line,
                "s %31s %33s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                name, tag_q,
                &ox, &oy, &px, &py, &sw, &sh,
                &pad, &trimmed,
                &tx, &ty, &tw, &th,
                &ct, &cx, &cy, &cw, &ch);

            if (matched == 19) {
                const char *tag_in = tag_q;
                size_t      tag_len = strlen(tag_in);
                if (tag_len >= 2 && tag_in[0] == '"' && tag_in[tag_len - 1] == '"') {
                    tag_in++;
                    tag_len -= 2;
                }
                AtlasSprite *sprite = &atlas->sprites[atlas->sprite_count++];
                snprintf(sprite->name, sizeof sprite->name, "%s", name);
                snprintf(sprite->tag,  sizeof sprite->tag,  "%.*s", (int)tag_len, tag_in);
                sprite->tex_source_rect = (Rectangle){
                    (float)px,
                    (float)py,
                    (float)sw,
                    (float)sh
                };
            }
        }
    }
    // Requires count match, truncated '.rtpa' files fail to load
    return expected > 0 && atlas->sprite_count == expected;
}

// Commit-on-success wrapper around `parse_rtpa`
static bool atlas_reload(Atlas *atlas, const char *path, Arena *arena) {
    char *text = LoadFileText(path);
    if (!text) return false;

    Atlas staging = *atlas;
    staging.sprite_count = 0;
    staging.mtime        = GetFileModTime(path);

    const bool ok = parse_rtpa(&staging, text, arena);
    UnloadFileText(text);

    if (!ok) return false;
    *atlas = staging;
    return true;
}

// ----------------------------------------------------------------------------
// Assets API
// ----------------------------------------------------------------------------

void assets_init(Assets *assets, Arena * arena) {
    // Textures
    for (TextureId id = 1; id < TEX_COUNT; id++) {
        if (!TEXTURE_PATHS[id]) {
            TraceLog(LOG_WARNING, "assets_init: missing TEXTURE_PATH entry for id=%d", id);
            continue;
        }
        const Texture2D texture = LoadTexture(TEXTURE_PATHS[id]);
        if (texture.id == 0) {
            TraceLog(LOG_WARNING, "assets_init: missing texture %s (id=%d)", TEXTURE_PATHS[id], id);
            continue;
        }
        assets->textures  [id] = texture;
        assets->tex_mtimes[id] = GetFileModTime(TEXTURE_PATHS[id]);
    }

    // Sounds
    for (SoundId id = 1; id < SOUND_COUNT; id++) {
        if (!SOUND_PATHS[id]) {
            TraceLog(LOG_WARNING, "assets_init: missing SOUND_PATHS entry for id=%d", id);
            continue;
        }
        const Sound sound = LoadSound(SOUND_PATHS[id]);
        if (sound.frameCount == 0) {
            TraceLog(LOG_WARNING, "assets_init: missing sound %s (id=%d)", SOUND_PATHS[id], id);
            continue;
        }
        assets->sounds      [id] = sound;
        assets->sound_mtimes[id] = GetFileModTime(SOUND_PATHS[id]);
    }

    // Fonts
    for (FontId id = 1; id < FONT_COUNT; id++) {
        if (!FONT_PATHS[id]) {
            TraceLog(LOG_WARNING, "assets_init: missing FONT_PATH entry for id=%d", id);
            continue;
        }
        const Font font = LoadFont(FONT_PATHS[id]);
        if (font.texture.id == 0) {
            TraceLog(LOG_WARNING, "assets_init: missing font %s (id=%d)", FONT_PATHS[id], id);
            continue;
        }
        assets->fonts      [id] = font;
        assets->font_mtimes[id] = GetFileModTime(FONT_PATHS[id]);
    }

    // Atlases (after textures because they reference a TextureId)
    for (AtlasId id = 1; id < ATLAS_COUNT; id++) {
        if (!ATLAS_PATHS[id]) {
            TraceLog(LOG_WARNING, "assets_init: missing ATLAS_PATH entry for id=%d", id);
            continue;
        }
        assets->atlases[id].tex_id = ATLAS_IMAGES[id];
        if (!atlas_reload(&assets->atlases[id], ATLAS_PATHS[id], arena)) {
            TraceLog(LOG_WARNING, "assets_init: missing atlas %s", ATLAS_PATHS[id]);
        } else {
            TraceLog(LOG_INFO, "assets_init: loaded atlas %s (%d sprites)",
                ATLAS_PATHS[id], assets->atlases[id].sprite_count);
        }
    }
}

void assets_poll_reload(Assets *assets, Arena *arena) {
    // Scan all slots each frame. TEX_COUNT et al. are small.
    for (TextureId id = 1; id < TEX_COUNT; id++) {
        const char *path = TEXTURE_PATHS[id];
        if (!path) continue;

        const long now_mtime = GetFileModTime(path);
        if (now_mtime == 0 || now_mtime == assets->tex_mtimes[id]) continue;

        const Texture2D fresh = LoadTexture(path);
        if (fresh.id == 0) continue;

        UnloadTexture(assets->textures[id]);
        assets->textures   [id] = fresh;
        assets->tex_mtimes [id] = now_mtime;
        TraceLog(LOG_INFO, "reloaded texture %s", path);
    }

    for (SoundId id = 1; id < SOUND_COUNT; id++) {
        const char *path = SOUND_PATHS[id];
        if (!path) continue;

        const long now_mtime = GetFileModTime(path);
        if (now_mtime == 0 || now_mtime == assets->sound_mtimes[id]) continue;

        const Sound fresh = LoadSound(path);
        if (fresh.frameCount == 0) continue;

        UnloadSound(assets->sounds[id]);
        assets->sounds       [id] = fresh;
        assets->sound_mtimes [id] = now_mtime;
        TraceLog(LOG_INFO, "reloaded sound %s", path);
    }

    for (FontId id = 1; id < FONT_COUNT; id++) {
        const char *path = FONT_PATHS[id];
        if (!path) continue;

        const long now_mtime = GetFileModTime(path);
        if (now_mtime == 0 || now_mtime == assets->font_mtimes[id]) continue;

        const Font fresh = LoadFontEx(path, FONT_SIZES[id], NULL, 0);
        if (fresh.texture.id == 0) continue;

        UnloadFont(assets->fonts[id]);
        assets->fonts       [id] = fresh;
        assets->font_mtimes [id] = now_mtime;
        TraceLog(LOG_INFO, "reloaded font %s", path);
    }

    for (AtlasId id = 1; id < ATLAS_COUNT; id++) {
        const char *path = ATLAS_PATHS[id];
        if (!path) continue;

        const long now_mtime = GetFileModTime(path);
        if (now_mtime == 0 || now_mtime == assets->atlases[id].mtime) continue;

        if (atlas_reload(&assets->atlases[id], path, arena)) {
            TraceLog(LOG_INFO, "reloaded atlas %s", path);
        } else {
            TraceLog(LOG_WARNING, "failed to reload atlas %s", path);
            // mtime not bumped → next poll retries
        }
    }
}

void assets_unload_all(Assets *assets) {
    for (TextureId id = 1; id < TEX_COUNT; id++) {
        if (assets->textures[id].id != 0) {
            UnloadTexture(assets->textures[id]);
        }
    }
    for (SoundId id = 1; id < SOUND_COUNT; id++) {
        if (assets->sounds[id].frameCount != 0) {
            UnloadSound(assets->sounds[id]);
        }
    }
    for (FontId id = 1; id < FONT_COUNT; id++) {
        if (assets->fonts[id].texture.id != 0) {
            UnloadFont(assets->fonts[id]);
        }
    }
    // Atlas backing textures are in assets->textures and were freed above, this clears metadata too.
    memset(assets, 0, sizeof *assets);
}

const Atlas *assets_get_atlas(const Assets *assets, const AtlasId id) {
    if (id == ATLAS_NONE || id >= ATLAS_COUNT) return NULL;
    return &assets->atlases[id];
}

Texture2D assets_get_texture(const Assets *assets, const TextureId id) {
    if (id == TEX_NONE || id >= TEX_COUNT) return (Texture2D){0};
    return assets->textures[id];
}

Sound assets_get_sound(const Assets *assets, const SoundId id) {
    if (id == SOUND_NONE || id >= SOUND_COUNT) return (Sound){0};
    return assets->sounds[id];
}

Font assets_get_font(const Assets *assets, const FontId    id) {
    if (id == FONT_NONE || id >= FONT_COUNT) return (Font){0};
    return assets->fonts[id];
}

TexRegion atlas_find_region(const Atlas *atlas, const char *region_name) {
    if (!atlas) return (TexRegion){0};
    for (int i = 0; i < atlas->sprite_count; i++) {
        if (strcmp(atlas->sprites[i].name, region_name) == 0) {
            return (TexRegion){ atlas->tex_id, atlas->sprites[i].tex_source_rect };
        }
    }
    return (TexRegion){0};
}

int atlas_find_regions_by_tag(const Atlas *atlas, const char *tag, TexRegion *out, const int max) {
    if (!atlas) return 0;
    int num_regions = 0;
    for (int i = 0; i < atlas->sprite_count && num_regions < max; i++) {
        if (strcmp(atlas->sprites[i].tag, tag) == 0) {
            out[num_regions++] = (TexRegion){ atlas->tex_id, atlas->sprites[i].tex_source_rect };
        }
    }
    return num_regions;
}

int atlas_count_regions_by_tag(const Atlas *atlas, const char *tag) {
    if (!atlas) return 0;
    int num_regions = 0;
    for (int i = 0; i < atlas->sprite_count; i++) {
        if (strcmp(atlas->sprites[i].tag, tag) == 0) {
            num_regions++;
        }
    }
    return num_regions;
}
