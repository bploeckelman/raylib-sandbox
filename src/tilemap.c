#define CUTE_TILED_IMPLEMENTATION
#include <cute_tiled.h>
#include <stb.h>

#include "tilemap.h"

Tilemap *loadTilemap(const char *path) {
    char tilesetImagePath[200];

    Tilemap *tilemap = malloc(1 * sizeof(Tilemap));
    (*tilemap) = (Tilemap) {0};

    sprintf(tilemap->name, "%s", path);

    tilemap->map  = cute_tiled_load_map_from_file(path, NULL);

    // TODO: split path and filename from 'path' param and use path to replace this hardcoded path
    sprintf(tilesetImagePath, "../assets/maps/%s", tilemap->map->tilesets->image.ptr);
    tilemap->tilesetTexture = LoadTexture(tilesetImagePath);

    const Vector2 mapOrigin = {0, -15 * 32};
    const cute_tiled_tileset_t *tileset = tilemap->map->tilesets;

    const cute_tiled_layer_t *layer = tilemap->map->layers;
    while (layer) {
        // build objects from object layer data
        if (0 == strcmp(layer->name.ptr, "object")) {
            cute_tiled_object_t *object = layer->objects;
            while (object != NULL) {
                // TODO: parse other object types
                if (0 == strcmp(object->type.ptr, "spawn")) {
                    tilemap->spawnPos = (Vector2) {
                            mapOrigin.x + object->x + object->width / 2,
                            mapOrigin.y + object->y - object->height / 2,
                    };
                }
                object = object->next;
            }
        }
        // build tiles from collision layer data
        else if (0 == strcmp(layer->name.ptr, "collision")) {
            int *tileGids = layer->data;
            int tileCount = layer->data_count;

            int x = 0, y = 0;
            for (int i = 0; i < tileCount; ++i) {
                Tile tile = {0};

                tile.gid = tileGids[i];

                int hflip, vflip, dflip;
                cute_tiled_get_flags(tile.gid, &hflip, &vflip, &dflip);
                tile.gid = cute_tiled_unset_flags(tile.gid);

                // TODO: handle special tileset properties: flip flags, spacing, margins, rotation, etc...
                // TODO: handle picking the correct tile texture when using multiple tilesets (based on tileset->firstgid)

                // gid 0 is empty so don't create Tiles for those
                if (tile.gid != 0) {
                    // calculate source texture pixel bounds for the tile type specified by tile.gid
                    // NOTE: tile gids are 1 indexed, but src x,y are 0 indexed pixel coords (0,0 is top left of tileset image)
                    int srcX = ((tile.gid - 1) % tileset->columns);
                    int srcY = ((tile.gid - 1) / tileset->columns);
                    tile.src = (Rectangle) {
                            srcX * tileset->tilewidth,
                            srcY * tileset->tileheight,
                            tileset->tilewidth,
                            tileset->tileheight
                    };

                    // calculate world bounds where this tile will be draw
                    tile.dst = (Rectangle) {
                            mapOrigin.x + x * tileset->tilewidth,
                            mapOrigin.y + y * tileset->tileheight,
                            tileset->tilewidth,
                            tileset->tileheight
                    };

                    // TODO: this is a quick hack to make Tiles collidable with the same interface as Solid
                    tile.solid = malloc(1 * sizeof(Solid));
                    (*tile.solid) = (Solid) { tile.dst, {0}, true };

                    // save the newly initialized tile
                    stb_arr_push(tilemap->tiles, tile);
                }

                // move to next tile coordinate
                x++;
                if (x >= layer->width) {
                    x = 0;
                    y++;
                }
            }
        }

        layer = layer->next;
    }

    TraceLog(LOG_INFO, "TILED: Loaded map '%s'", path);
    return tilemap;
}

void unloadTilemap(Tilemap *tilemap) {
    if (tilemap == NULL) return;

    char name[200];
    if (tilemap->map != NULL) {
        // make a copy of the name for logging after the tilemap is freed
        sprintf(name, "%s", tilemap->name);

        if (tilemap->tiles != NULL) {
            for (int i = 0; i < stb_arr_len(tilemap->tiles); ++i) {
                free(tilemap->tiles[i].solid);
            }
            stb_arr_free(tilemap->tiles);
        }

        cute_tiled_free_map(tilemap->map);
        UnloadTexture(tilemap->tilesetTexture);
    }

    free(tilemap);
    TraceLog(LOG_INFO, "TILED: Unloaded map '%s'", name);
}
