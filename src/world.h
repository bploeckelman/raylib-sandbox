#pragma once

#include "raylib.h"
#include "../lib/stb/stb.h"

#include "assets.h"

//----------------------------------------------------------------------------------
// Data structures
//----------------------------------------------------------------------------------

typedef void (*ON_COLLIDE)();

typedef struct Solid {
    Rectangle bounds;
} Solid;

typedef enum Facing { left, right } Facing;

typedef struct Actor {
    Rectangle bounds;
    Vector2 center;
    Vector2 remainder;
    Animation animation;
    Facing facing;
    float stateTime;
} Actor;

typedef struct World {
    Solid *solids;
    Actor *actors;
} World;

//----------------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------------

int sign(float value) {
    if      (value > 0) return 1;
    else if (value < 0) return -1;
    else                return 0;
}

Vector2 getCenter(Rectangle rect) {
    return (Vector2) { rect.x + rect.width / 2, rect.y + rect.height / 2 };
}

//----------------------------------------------------------------------------------
// World functions
//----------------------------------------------------------------------------------

void initializeWorld(World *world) {
    (*world) = (World) {0};
    stb_arr_push(world->solids, ((Solid) {
            (Rectangle) { 0, 0, 500, 20 }
    }));
}

void unloadWorld(World *world) {
    if (world == NULL) return;
    if (world->solids != NULL) {
        stb_arr_free(world->solids);
    }
    if (world->actors != NULL) {
        stb_arr_free(world->actors);
    }
    (*world) = (World) {0};
}

bool collide(Rectangle collider1, Rectangle collider2) {
    return CheckCollisionRecs(collider1, collider2);
}

bool collidesWithSolids(World *world, Rectangle collider) {
    for (int i = 0; i < stb_arr_len(world->solids); ++i) {
        Solid solid = world->solids[i];
        if (collide(solid.bounds, collider)) {
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------
// Actor functions
//----------------------------------------------------------------------------------

void moveX(Actor *actor, float amount, World *world, ON_COLLIDE onCollide) {
    actor->remainder.x += amount;
    int move = round(actor->remainder.x);
    if (move == 0) return;

    actor->remainder.x -= move;
    int moveSign = sign(move);
    while (move != 0) {
        Rectangle nextStepBounds = {actor->bounds.x + moveSign, actor->bounds.y, actor->bounds.width, actor->bounds.height };
        if (collidesWithSolids(world, nextStepBounds)) {
            // hit a solid, don't take the next step and trigger the collide callback
            if (onCollide != NULL) {
                onCollide();
            }
            break;
        } else {
            // no solid in the way, take the next step
            actor->bounds.x += moveSign;
            actor->center = getCenter(actor->bounds);
            move -= moveSign;
        }
    }
}

void moveY(Actor *actor, float amount, World *world, ON_COLLIDE onCollide) {
    actor->remainder.y += amount;
    int move = round(actor->remainder.y);
    if (move == 0) return;

    actor->remainder.y -= move;
    int moveSign = sign(move);
    while (move != 0) {
        Rectangle nextStepBounds = {
                actor->bounds.x,
                actor->bounds.y + moveSign,
                actor->bounds.width,
                actor->bounds.height
        };
        if (collidesWithSolids(world, nextStepBounds)) {
            // hit a solid, don't take the next step and trigger the collide callback
            if (onCollide != NULL) {
                onCollide();
            }
            break;
        } else {
            // no solid in the way, take the next step
            actor->bounds.y += moveSign;
            actor->center = getCenter(actor->bounds);
            move -= moveSign;
        }
    }
}
