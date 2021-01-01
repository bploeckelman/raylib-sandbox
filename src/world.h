#ifndef RAYLIB_SANDBOX_WORLD_H
#define RAYLIB_SANDBOX_WORLD_H

#include <raylib.h>
#include <stb.h>

#include "assets.h"
#include "tilemap.h"
#include "gui_utils.h"

//----------------------------------------------------------------------------------
// Data structures
//----------------------------------------------------------------------------------

typedef struct Solid Solid;
typedef struct Actor Actor, *ActorPtr;
typedef struct World World;
typedef struct Tilemap Tilemap;

struct Solid {
    Rectangle bounds;
    Vector2 remainder;
    bool collidable;
};

typedef enum Facing { left, right } Facing;

struct Actor {
    Rectangle bounds;
    Rectangle hitbox;
    Vector2 center;
    Vector2 remainder;
    Vector2 speed;
    Animation animation;
    Facing facing;
    float stateTime;
    bool grounded;
};

struct World {
    Solid *solids;
    Actor *actors;
    Tilemap *tilemap;
};

ValueSlider gravitySlider;
ValueSlider jumpSpeedSlider;

//----------------------------------------------------------------------------------
// Utility
//--struct --------------------------------------------------------------------------------

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
