#ifndef RAYLIB_SANDBOX_WORLD_H
#define RAYLIB_SANDBOX_WORLD_H

#include <raylib.h>
#include "../lib/stb/stb.h"
#include "../lib/cute_tiled/cute_tiled.h"

#include "assets.h"

//----------------------------------------------------------------------------------
// Data structures
//----------------------------------------------------------------------------------

typedef struct Solid {
    Rectangle bounds;
    Vector2 remainder;
    bool collidable;
} Solid;

typedef enum Facing { left, right } Facing;

typedef struct Actor {
    Rectangle bounds;
    Rectangle hitbox;
    Vector2 center;
    Vector2 remainder;
    Vector2 speed;
    Animation animation;
    Facing facing;
    float stateTime;
    bool grounded;
} Actor, *ActorPtr;

typedef struct World {
    Solid *solids;
    Actor *actors;
    cute_tiled_map_t *map;
    Texture2D mapTexture;
} World;

//----------------------------------------------------------------------------------
// Utility
//----------------------------------------------------------------------------------

typedef void (*ON_COLLIDE)(Actor *actor);

//----------------------------------------------------------------------------------
// World functions
//----------------------------------------------------------------------------------

static int solidMinX = 20;
static int solidMaxX = 480;
static int solidMinY = -50;
static int solidMaxY = 0;

void initializeWorld(World *world);
void unloadWorld(World *world);

//----------------------------------------------------------------------------------
// Actor functions
//----------------------------------------------------------------------------------

void moveActorX(Actor *actor, float amount, World *world, ON_COLLIDE onCollide);
void moveActorY(Actor *actor, float amount, World *world, ON_COLLIDE onCollide);
void updatePlayer(Actor *player, World *world, float dt);

//----------------------------------------------------------------------------------
// Solid functions
//----------------------------------------------------------------------------------

void moveSolid(Solid *solid, float x, float y, World *world);

#endif
