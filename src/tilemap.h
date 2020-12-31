#ifndef RAYLIB_SANDBOX_TILEMAP_H
#define RAYLIB_SANDBOX_TILEMAP_H

#include <raylib.h>
#include <cute_tiled.h>

typedef struct Tile {
    int gid;
    Rectangle src;
    Rectangle dst;
} Tile;

typedef struct Tilemap {
    char name[200];
    cute_tiled_map_t *map;
    Texture2D tilesetTexture;
    Tile *tiles;
} Tilemap;

Tilemap *loadTilemap(const char *path);
void unloadTilemap(Tilemap *tilemap);

#endif //RAYLIB_SANDBOX_TILEMAP_H
