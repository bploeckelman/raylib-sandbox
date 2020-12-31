#ifndef RAYLIB_SANDBOX_TILEMAP_H
#define RAYLIB_SANDBOX_TILEMAP_H

#include <raylib.h>
#include <cute_tiled.h>

#include "world.h"

typedef struct Solid Solid;
typedef struct Tile Tile;
typedef struct Tilemap Tilemap;

struct Tile {
    int gid;
    Rectangle src;
    Rectangle dst;
    Solid *solid;
};

struct Tilemap {
    char name[200];
    cute_tiled_map_t *map;
    Texture2D tilesetTexture;
    Tile *tiles;
};

Tilemap *loadTilemap(const char *path);
void unloadTilemap(Tilemap *tilemap);

#endif //RAYLIB_SANDBOX_TILEMAP_H
