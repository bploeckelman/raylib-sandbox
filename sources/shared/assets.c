#include "shared/assets.h"
#include "shared/arena.h"

#include <string.h>
#include <stdio.h>

// ---- per-type slot lookup --------------------------------------------------

static int find_or_alloc_atlas(const Assets *assets, const char *path) {
    int free_slot = -1;
    for (int i = 0; i < ASSETS_MAX_ATLASES; i++) {
        const bool in_use       = assets->atlases[i].in_use;
        const bool matches_path = strcmp(assets->atlases[i].path, path) == 0;
        if (in_use && matches_path) return i;
        if (!in_use && free_slot < 0) free_slot = i;
    }
    return free_slot;
}

static int find_or_alloc_texture(const Assets *assets, const char *path) {
    int free_slot = -1;
    for (int i = 0; i < ASSETS_MAX_TEXTURES; i++) {
        const bool in_use       = assets->textures[i].in_use;
        const bool matches_path = strcmp(assets->textures[i].path, path) == 0;
        if (in_use && matches_path) return i;
        if (!in_use && free_slot < 0) free_slot = i;
    }
    return free_slot;
}

static int find_or_alloc_sound(const Assets *assets, const char *path) {
    int free_slot = -1;
    for (int i = 0; i < ASSETS_MAX_SOUNDS; i++) {
        const bool in_use       = assets->sounds[i].in_use;
        const bool matches_path = strcmp(assets->sounds[i].path, path) == 0;
        if (in_use && matches_path) return i;
        if (!in_use && free_slot < 0) free_slot = i;
    }
    return free_slot;
}

static int find_or_alloc_font(const Assets *assets, const char *path) {
    int free_slot = -1;
    for (int i = 0; i < ASSETS_MAX_FONTS; i++) {
        const bool in_use       = assets->fonts[i].in_use;
        const bool matches_path = strcmp(assets->fonts[i].path, path) == 0;
        if (in_use && matches_path) return i;
        if (!in_use && free_slot < 0) free_slot = i;
    }
    return free_slot;
}

static bool parse_rtpa(AtlasSlot *slot, Assets *assets, const char *text, Arena *arena) {
    int expected = -1;
    const char *cursor = text;

    while (*cursor) {
        const char *eol = strchr(cursor, '\n');
        size_t len = eol ? (size_t)(eol - cursor) : strlen(cursor);
        char line[512];
        if (len >= sizeof line) len = sizeof line - 1;
        memcpy(line, cursor, len); line[len] = 0;
        cursor = eol ? eol + 1 : cursor + len;

        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == 0 || *p == '#' || *p == '\r') continue;

        if (line[0] == 'a' && line[1] == ' ') {
            char img[ASSETS_PATH_LENGTH];
            int w, h, count, is_font, fsize;
            const int matched = sscanf(line, "a %127s %d %d %d %d %d", img, &w, &h, &count, &is_font, &fsize);
            if (matched == 6) {
                const TextureHandle tex_handle = assets_load_texture(assets, img);
                if (tex_handle.slot == 0 && tex_handle.gen == 0) return false;

                slot->tex_handle = tex_handle;
                slot->sprites = ARENA_NEW_ARRAY(arena, AtlasSprite, count);
                slot->count   = 0;
                expected      = count;
            }
        } else if (line[0] == 's' && line[1] == ' ' && slot->sprites && slot->count < expected) {
            char name[ATLAS_NAME_LENGTH], tag_q[ATLAS_TAG_LENGTH + 2];
            int ox, oy, px, py, sw, sh, pad, trimmed;
            int tx, ty, tw, th;
            int ct, cx, cy, cw, ch;

            const int matched = sscanf(line,
                "s %31s %33s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                name, tag_q,
                &ox, &oy, &px, &py, &sw, &sh,
                &pad, &trimmed,
                &tx, &ty, &tw, &th,
                &ct, &cx, &cy, &cw, &ch);

            if (matched == 19) {
                const char *tag_in = tag_q;
                size_t tag_len = strlen(tag_in);
                if (tag_len >= 2 && tag_in[0] == '"' && tag_in[tag_len - 1] == '"') {
                    tag_in++; tag_len -= 2;
                }
                AtlasSprite *sp = &slot->sprites[slot->count++];
                snprintf(sp->name, sizeof sp->name, "%s", name);
                snprintf(sp->tag,  sizeof sp->tag,  "%.*s", (int)tag_len, tag_in);
                sp->tex_source = (Rectangle){
                    .x = (float)px,
                    .y = (float)py,
                    .width = (float)sw,
                    .height = (float)sh
                };
            }
        }
    }

    if (slot->count != expected) {
        TraceLog(LOG_WARNING, "parse_rtpa(%s): count(%d) != expected(%d)", slot->path, slot->count, expected);
    }
    return slot->count > 0;
}

// ---- load ------------------------------------------------------------------

AtlasHandle assets_load_atlas(Assets *assets, const char *path, Arena *arena) {
    const int i = find_or_alloc_atlas(assets, path);
    if (i < 0) return ATLAS_HANDLE_NULL;

    AtlasSlot *slot = &assets->atlases[i];
    if (slot->in_use) {
        return (AtlasHandle){ (uint16_t)i, slot->gen };
    }

    char *text = LoadFileText(path);
    if (!text) return ATLAS_HANDLE_NULL;

    snprintf(slot->path, sizeof slot->path, "%s", path);
    slot->mtime  = GetFileModTime(path);
    slot->gen    = (slot->gen == 0) ? 1 : slot->gen + 1;
    slot->in_use = true;

    const bool ok = parse_rtpa(slot, assets, text, arena);
    UnloadFileText(text);

    if (!ok) {
        slot->in_use = false;
        return ATLAS_HANDLE_NULL;
    }
    return (AtlasHandle){ (uint16_t)i, slot->gen };
}

TextureHandle assets_load_texture(Assets *assets, const char *path) {
    const int i = find_or_alloc_texture(assets, path);
    if (i < 0) return TEXTURE_HANDLE_NULL;

    TextureSlot *slot = &assets->textures[i];
    if (slot->in_use) {
        return (TextureHandle){ (uint16_t)i, slot->gen };
    }

    const Texture2D texture = LoadTexture(path);
    if (texture.id == 0) return TEXTURE_HANDLE_NULL;

    snprintf(slot->path, sizeof slot->path, "%s", path);
    slot->texture = texture;
    slot->mtime   = GetFileModTime(path);
    slot->gen     = (slot->gen == 0) ? 1 : slot->gen + 1;  // skip 0 (reserved for null)
    slot->in_use  = true;
    return (TextureHandle){ (uint16_t)i, slot->gen };
}

SoundHandle assets_load_sound(Assets *assets, const char *path) {
    const int i = find_or_alloc_sound(assets, path);
    if (i < 0) return SOUND_HANDLE_NULL;

    SoundSlot *slot = &assets->sounds[i];
    if (slot->in_use) {
        return (SoundHandle){ (uint16_t)i, slot->gen };
    }

    const Sound sound = LoadSound(path);
    if (sound.frameCount == 0) return SOUND_HANDLE_NULL;

    snprintf(slot->path, sizeof slot->path, "%s", path);
    slot->sound  = sound;
    slot->mtime  = GetFileModTime(path);
    slot->gen    = (slot->gen == 0) ? 1 : slot->gen + 1;
    slot->in_use = true;
    return (SoundHandle){ (uint16_t)i, slot->gen };
}

FontHandle assets_load_font(Assets *assets, const char *path, const int base_size) {
    const int i = find_or_alloc_font(assets, path);
    if (i < 0) return FONT_HANDLE_NULL;

    FontSlot *slot = &assets->fonts[i];
    if (slot->in_use) {
        return (FontHandle){ (uint16_t)i, slot->gen };
    }

    const Font font = LoadFontEx(path, base_size, NULL, 0);  // NULL,0 = default ASCII set
    if (font.texture.id == 0) return (FontHandle){0, 0};

    snprintf(slot->path, sizeof slot->path, "%s", path);
    slot->font      = font;
    slot->base_size = base_size;
    slot->mtime     = GetFileModTime(path);
    slot->gen       = (slot->gen == 0) ? 1 : slot->gen + 1;
    slot->in_use    = true;
    return (FontHandle){ (uint16_t)i, slot->gen };
}

// ---- resolve ---------------------------------------------------------------

const AtlasSlot *assets_get_atlas(const Assets *assets, const AtlasHandle handle) {
    if (handle.slot < ASSETS_MAX_ATLASES) {
        const AtlasSlot *slot = &assets->atlases[handle.slot];
        if (slot->in_use && slot->gen == handle.gen) return slot;
    }
    return NULL;
}

Texture2D assets_get_texture(const Assets *assets, const TextureHandle handle) {
    if (handle.slot < ASSETS_MAX_TEXTURES) {
        const TextureSlot *slot = &assets->textures[handle.slot];
        if (slot->in_use && slot->gen == handle.gen) return slot->texture;
    }
    return (Texture2D){0};   // caller can check tex.id == 0
}

Sound assets_get_sound(const Assets *assets, const SoundHandle handle) {
    if (handle.slot < ASSETS_MAX_SOUNDS) {
        const SoundSlot *slot = &assets->sounds[handle.slot];
        if (slot->in_use && slot->gen == handle.gen) return slot->sound;
    }
    return (Sound){0};
}

Font assets_get_font(const Assets *assets, const FontHandle handle) {
    if (handle.slot < ASSETS_MAX_FONTS) {
        const FontSlot *slot = &assets->fonts[handle.slot];
        if (slot->in_use && slot->gen == handle.gen) return slot->font;
    }
    return GetFontDefault();   // raylib's built-in 10px font is a fine fallback
}

TexRegion atlas_find_region(const AtlasSlot *atlas, const char *region_name) {
    if (!atlas || !atlas->in_use) return (TexRegion){0};
    for (int i = 0; i < atlas->count; i++) {
        if (strcmp(atlas->sprites[i].name, region_name) == 0) {
            return (TexRegion){ atlas->tex_handle, atlas->sprites[i].tex_source };
        }
    }
    return (TexRegion){0};
}

int atlas_find_regions_by_tag(const AtlasSlot *atlas, const char *tag, TexRegion *regions_out, const int max) {
    if (!atlas || !atlas->in_use) return 0;
    int num_found = 0;
    for (int i = 0; i < atlas->count && num_found < max; i++) {
        if (strcmp(atlas->sprites[i].tag, tag) == 0) {
            regions_out[num_found++] = (TexRegion){ atlas->tex_handle, atlas->sprites[i].tex_source };
        }
    }
    return num_found;
}

int atlas_num_regions_for_tag(const AtlasSlot *atlas, const char *tag) {
    if (!atlas || !atlas->in_use) return 0;
    int num_found = 0;
    for (int i = 0; i < atlas->count; i++) {
        if (strcmp(atlas->sprites[i].tag, tag) == 0) {
            num_found++;
        }
    }
    return num_found;
}

// ---- hot reload ------------------------------------------------------------

void assets_poll_reload(Assets *assets, Arena *arena) {
    // Spread the cost: ~8 slots per type per frame, round-robin.
    // Note: gen NOT bumped on reload — same logical asset, handles stay valid.

    for (int n = 0; n < 8; n++) {
        const uint16_t i = assets->scan_cursor++ % ASSETS_MAX_TEXTURES;
        TextureSlot *slot = &assets->textures[i];
        if (!slot->in_use) continue;

        const long now_mtime = GetFileModTime(slot->path);
        if (now_mtime == 0 || now_mtime == slot->mtime) continue;

        const Texture2D fresh = LoadTexture(slot->path);
        if (fresh.id == 0) continue;

        UnloadTexture(slot->texture);
        slot->texture   = fresh;
        slot->mtime = now_mtime;
        TraceLog(LOG_INFO, "reloaded texture %s", slot->path);
    }

    for (int n = 0; n < 8; n++) {
        const uint16_t i = assets->scan_cursor++ % ASSETS_MAX_SOUNDS;
        SoundSlot *slot = &assets->sounds[i];
        if (!slot->in_use) continue;

        const long now_mtime = GetFileModTime(slot->path);
        if (now_mtime == 0 || now_mtime == slot->mtime) continue;

        const Sound fresh = LoadSound(slot->path);
        if (fresh.frameCount == 0) continue;

        UnloadSound(slot->sound);
        slot->sound = fresh;
        slot->mtime = now_mtime;
        TraceLog(LOG_INFO, "reloaded sound %s", slot->path);
    }

    for (int n = 0; n < 8; n++) {
        const uint16_t i = assets->scan_cursor++ % ASSETS_MAX_FONTS;
        FontSlot *slot = &assets->fonts[i];
        if (!slot->in_use) continue;

        const long now_mtime = GetFileModTime(slot->path);
        if (now_mtime == 0 || now_mtime == slot->mtime) continue;

        const Font fresh = LoadFontEx(slot->path, slot->base_size, NULL, 0);
        if (fresh.texture.id == 0) continue;

        UnloadFont(slot->font);
        slot->font  = fresh;
        slot->mtime = now_mtime;
        TraceLog(LOG_INFO, "reloaded font %s", slot->path);
    }

    // ASSETS_MAX_ATLASES is small, ok to scan them all instead of a subset per frame
    for (int n = 0; n < ASSETS_MAX_ATLASES; n++) {
        AtlasSlot *slot = &assets->atlases[n];
        if (!slot->in_use) continue;

        const long prev_count = slot->count;
        const long prev_mtime = slot->mtime;
        const long now_mtime = GetFileModTime(slot->path);
        if (now_mtime == 0 || now_mtime == prev_mtime) continue;

        char *text = LoadFileText(slot->path);
        if (!text) continue;

        // gen NOT bumped; same logical atlas, handles stay valid
        // old sprite bytes leak into arena (dev-time only)
        slot->mtime = now_mtime;
        slot->count = 0;
        if (parse_rtpa(slot, assets, text, arena)) {
            TraceLog(LOG_INFO, "reloaded atlas %s", slot->path);
        } else {
            // Failed to parse, rollback to previous state
            TraceLog(LOG_WARNING, "failed to reload atlas %s", slot->path);
            slot->mtime = prev_mtime;
            slot->count = prev_count;
        }
        UnloadFileText(text);
    }
}

void assets_unload_all(Assets *assets) {
    // NOTE: atlas textures are automatically cleaned up because they're owned by `assets->textures[]`
    for (int i = 0; i < ASSETS_MAX_TEXTURES; i++) {
        if (assets->textures[i].in_use) UnloadTexture(assets->textures[i].texture);
    }
    for (int i = 0; i < ASSETS_MAX_SOUNDS; i++) {
        if (assets->sounds[i].in_use) UnloadSound(assets->sounds[i].sound);
    }
    for (int i = 0; i < ASSETS_MAX_FONTS; i++) {
        if (assets->fonts[i].in_use) UnloadFont(assets->fonts[i].font);
    }
    memset(assets, 0, sizeof *assets);
}