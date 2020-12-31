#define CUTE_TILED_IMPLEMENTATION
#include <cute_tiled.h>

#include "tilemap.h"

Tilemap *loadTilemap(const char *path) {
    char tilesetImagePath[200];

    Tilemap *tilemap = malloc(1 * sizeof(Tilemap));
    sprintf(tilemap->name, "%s", path);
    tilemap->map  = cute_tiled_load_map_from_file(path, NULL);
    // TODO: split path and filename from 'path' param and use path to replace this hardcoded path
    sprintf(tilesetImagePath, "../assets/maps/%s", tilemap->map->tilesets->image.ptr);
    tilemap->tilesetTexture = LoadTexture(tilesetImagePath);

    TraceLog(LOG_INFO, "TILED: Loaded map '%s'", path);
    return tilemap;
}

void unloadTilemap(Tilemap *tilemap) {
    if (tilemap == NULL) return;

    char name[200];
    if (tilemap->map != NULL) {
        sprintf(name, "%s", tilemap->name);
        cute_tiled_free_map(tilemap->map);
        UnloadTexture(tilemap->tilesetTexture);
    }

    free(tilemap);
    TraceLog(LOG_INFO, "TILED: Unloaded map '%s'", name);
}
