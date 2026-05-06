#include "shared/assets.h"

#include <string.h>
#include <stdio.h>

// ---- per-type slot lookup --------------------------------------------------

static int find_or_alloc_texture(Assets *a, const char *path) {
    int free_slot = -1;
    for (int i = 0; i < ASSETS_MAX_TEXTURES; i++) {
        if (a->textures[i].in_use && strcmp(a->textures[i].path, path) == 0) return i;
        if (!a->textures[i].in_use && free_slot < 0) free_slot = i;
    }
    return free_slot;
}

static int find_or_alloc_sound(Assets *a, const char *path) {
    int free_slot = -1;
    for (int i = 0; i < ASSETS_MAX_SOUNDS; i++) {
        if (a->sounds[i].in_use && strcmp(a->sounds[i].path, path) == 0) return i;
        if (!a->sounds[i].in_use && free_slot < 0) free_slot = i;
    }
    return free_slot;
}

static int find_or_alloc_font(Assets *a, const char *path) {
    int free_slot = -1;
    for (int i = 0; i < ASSETS_MAX_FONTS; i++) {
        if (a->fonts[i].in_use && strcmp(a->fonts[i].path, path) == 0) return i;
        if (!a->fonts[i].in_use && free_slot < 0) free_slot = i;
    }
    return free_slot;
}

// ---- load ------------------------------------------------------------------

TextureHandle assets_load_texture(Assets *a, const char *path) {
    int i = find_or_alloc_texture(a, path);
    if (i < 0) return ASSET_HANDLE_NULL;

    TextureSlot *s = &a->textures[i];
    if (s->in_use) return (TextureHandle){ (uint16_t)i, s->gen };

    Texture2D t = LoadTexture(path);
    if (t.id == 0) return ASSET_HANDLE_NULL;

    snprintf(s->path, sizeof s->path, "%s", path);
    s->tex    = t;
    s->mtime  = GetFileModTime(path);
    s->gen    = (s->gen == 0) ? 1 : s->gen + 1;  // skip 0 (reserved for null)
    s->in_use = true;
    return (TextureHandle){ (uint16_t)i, s->gen };
}

SoundHandle assets_load_sound(Assets *a, const char *path) {
    int i = find_or_alloc_sound(a, path);
    if (i < 0) return (SoundHandle){0, 0};

    SoundSlot *s = &a->sounds[i];
    if (s->in_use) return (SoundHandle){ (uint16_t)i, s->gen };

    Sound snd = LoadSound(path);
    if (snd.frameCount == 0) return (SoundHandle){0, 0};   // Sound has no .id

    snprintf(s->path, sizeof s->path, "%s", path);
    s->sound  = snd;
    s->mtime  = GetFileModTime(path);
    s->gen    = (s->gen == 0) ? 1 : s->gen + 1;
    s->in_use = true;
    return (SoundHandle){ (uint16_t)i, s->gen };
}

FontHandle assets_load_font(Assets *a, const char *path, int base_size) {
    int i = find_or_alloc_font(a, path);
    if (i < 0) return (FontHandle){0, 0};

    FontSlot *s = &a->fonts[i];
    if (s->in_use) return (FontHandle){ (uint16_t)i, s->gen };

    Font f = LoadFontEx(path, base_size, NULL, 0);  // NULL,0 = default ASCII set
    if (f.texture.id == 0) return (FontHandle){0, 0};

    snprintf(s->path, sizeof s->path, "%s", path);
    s->font      = f;
    s->base_size = base_size;
    s->mtime     = GetFileModTime(path);
    s->gen       = (s->gen == 0) ? 1 : s->gen + 1;
    s->in_use    = true;
    return (FontHandle){ (uint16_t)i, s->gen };
}

// ---- resolve ---------------------------------------------------------------

Texture2D assets_get_texture(const Assets *a, TextureHandle h) {
    if (h.slot < ASSETS_MAX_TEXTURES) {
        const TextureSlot *s = &a->textures[h.slot];
        if (s->in_use && s->gen == h.gen) return s->tex;
    }
    return (Texture2D){0};   // caller can check tex.id == 0
}

Sound assets_get_sound(const Assets *a, SoundHandle h) {
    if (h.slot < ASSETS_MAX_SOUNDS) {
        const SoundSlot *s = &a->sounds[h.slot];
        if (s->in_use && s->gen == h.gen) return s->sound;
    }
    return (Sound){0};
}

Font assets_get_font(const Assets *a, FontHandle h) {
    if (h.slot < ASSETS_MAX_FONTS) {
        const FontSlot *s = &a->fonts[h.slot];
        if (s->in_use && s->gen == h.gen) return s->font;
    }
    return GetFontDefault();   // raylib's built-in 10px font is a fine fallback
}

// ---- hot reload ------------------------------------------------------------

void assets_poll_reload(Assets *a) {
    // Spread the cost: ~8 slots per type per frame, round-robin.
    // Note: gen NOT bumped on reload — same logical asset, handles stay valid.

    for (int n = 0; n < 8; n++) {
        uint16_t i = a->scan_cursor++ % ASSETS_MAX_TEXTURES;
        TextureSlot *s = &a->textures[i];
        if (!s->in_use) continue;

        long now_mtime = GetFileModTime(s->path);
        if (now_mtime == 0 || now_mtime == s->mtime) continue;

        Texture2D fresh = LoadTexture(s->path);
        if (fresh.id == 0) continue;

        UnloadTexture(s->tex);
        s->tex   = fresh;
        s->mtime = now_mtime;
        TraceLog(LOG_INFO, "reloaded texture %s", s->path);
    }

    for (int n = 0; n < 8; n++) {
        uint16_t i = a->scan_cursor++ % ASSETS_MAX_SOUNDS;
        SoundSlot *s = &a->sounds[i];
        if (!s->in_use) continue;

        long now_mtime = GetFileModTime(s->path);
        if (now_mtime == 0 || now_mtime == s->mtime) continue;

        Sound fresh = LoadSound(s->path);
        if (fresh.frameCount == 0) continue;

        UnloadSound(s->sound);
        s->sound = fresh;
        s->mtime = now_mtime;
        TraceLog(LOG_INFO, "reloaded sound %s", s->path);
    }

    for (int n = 0; n < 8; n++) {
        uint16_t i = a->scan_cursor++ % ASSETS_MAX_FONTS;
        FontSlot *s = &a->fonts[i];
        if (!s->in_use) continue;

        long now_mtime = GetFileModTime(s->path);
        if (now_mtime == 0 || now_mtime == s->mtime) continue;

        Font fresh = LoadFontEx(s->path, s->base_size, NULL, 0);
        if (fresh.texture.id == 0) continue;

        UnloadFont(s->font);
        s->font  = fresh;
        s->mtime = now_mtime;
        TraceLog(LOG_INFO, "reloaded font %s", s->path);
    }
}

void assets_unload_all(Assets *a) {
    for (int i = 0; i < ASSETS_MAX_TEXTURES; i++) {
        if (a->textures[i].in_use) UnloadTexture(a->textures[i].tex);
    }
    for (int i = 0; i < ASSETS_MAX_SOUNDS; i++) {
        if (a->sounds[i].in_use) UnloadSound(a->sounds[i].sound);
    }
    for (int i = 0; i < ASSETS_MAX_FONTS; i++) {
        if (a->fonts[i].in_use) UnloadFont(a->fonts[i].font);
    }
    memset(a, 0, sizeof *a);
}