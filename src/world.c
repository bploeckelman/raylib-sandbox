#include <raymath.h>

#include "world.h"


//----------------------------------------------------------------------------------
// Utility
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

    Solid movingSolid = {
        .bounds = { solidMinX, solidMinY, 20, 20 },
        .remainder = Vector2Zero(),
        .collidable = true
    };
    stb_arr_push(world->solids, movingSolid);

    // TODO: add ability to save solids created in editing mode out to a file format and change this to load from file
    stb_arr_push(world->solids, ((Solid) { (Rectangle) {   0,    0, 500,  20 }, Vector2Zero(), true }));
    stb_arr_push(world->solids, ((Solid) { (Rectangle) {   0, -100,  20, 100 }, Vector2Zero(), true }));
    stb_arr_push(world->solids, ((Solid) { (Rectangle) { 480, -100,  20, 100 }, Vector2Zero(), true }));
    stb_arr_push(world->solids, ((Solid) { (Rectangle) {   0, -120, 500,  20 }, Vector2Zero(), true }));

    Rectangle playerInitialBounds = {200, -32, 32, 32 };
    Vector2 playerCenter = getCenter(playerInitialBounds);
    int hitWidth = playerInitialBounds.width - 20;
    int hitHeight = playerInitialBounds.height - 4;
    Rectangle playerHitbox = {
            playerCenter.x - hitWidth / 2,
            playerInitialBounds.y + 4,
            hitWidth, hitHeight
    };
    Actor player = {
            .bounds = playerInitialBounds,
            .hitbox = playerHitbox,
            .center = playerCenter,
            .remainder = Vector2Zero(),
            .speed = Vector2Zero(),
            .animation = getAnimation(character_idle_right),
            .facing = right,
            .stateTime = 0
    };
    stb_arr_push(world->actors, player);
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
        if (!solid.collidable) continue;
        if (collide(solid.bounds, collider)) {
            return true;
        }
    }
    return false;
}

bool isRiding(Actor *actor, Solid *solid) {
    // TODO ....
    return false;
}

void onCollide_Squish(Actor *actor) {
    // TODO: this kills the actor
    TraceLog(LOG_INFO, "squished actor");
}

//----------------------------------------------------------------------------------
// Actor functions
//----------------------------------------------------------------------------------

void moveActorX(Actor *actor, float amount, World *world, ON_COLLIDE onCollide) {
    actor->remainder.x += amount;
    int move = round(actor->remainder.x);
    if (move == 0) return;

    actor->remainder.x -= move;
    int moveSign = sign(move);
    while (move != 0) {
        Rectangle nextStepHitbox = {
                actor->hitbox.x + moveSign,
                actor->hitbox.y,
                actor->hitbox.width,
                actor->hitbox.height
        };
        if (collidesWithSolids(world, nextStepHitbox)) {
            // hit a solid, don't take the next step and trigger the collide callback
            if (onCollide != NULL) {
                onCollide(actor);
            }
            break;
        } else {
            // no solid in the way, take the next step
            actor->bounds.x += moveSign;
            actor->hitbox.x += moveSign;
            actor->center = getCenter(actor->bounds);
            move -= moveSign;
        }
    }
}

void moveActorY(Actor *actor, float amount, World *world, ON_COLLIDE onCollide) {
    actor->remainder.y += amount;
    int move = round(actor->remainder.y);
    if (move == 0) return;

    actor->remainder.y -= move;
    int moveSign = sign(move);
    while (move != 0) {
        Rectangle nextStepBounds = {
                actor->hitbox.x,
                actor->hitbox.y + moveSign,
                actor->hitbox.width,
                actor->hitbox.height
        };
        if (collidesWithSolids(world, nextStepBounds)) {
            // hit a solid, don't take the next step and trigger the collide callback
            if (onCollide != NULL) {
                onCollide(actor);
            }
            break;
        } else {
            // no solid in the way, take the next step
            actor->bounds.y += moveSign;
            actor->hitbox.y += moveSign;
            actor->center = getCenter(actor->bounds);
            move -= moveSign;
        }
    }
}

void updatePlayer(Actor *player, World *world, float dt) {
    const float speed = 200;
    player->stateTime += dt;

    int gamepad = GAMEPAD_PLAYER1;
    bool gamepadActive = IsGamepadAvailable(gamepad);

    bool keyUpLeft  = IsKeyUp(KEY_A) && (gamepadActive && IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
    bool keyUpRight = IsKeyUp(KEY_D) && (gamepadActive && IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT));

    bool keyDownLeft  = IsKeyDown(KEY_A) || (gamepadActive && IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
    bool keyDownRight = IsKeyDown(KEY_D) || (gamepadActive && IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
    bool keyDownUp    = IsKeyDown(KEY_W) || (gamepadActive && IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP));
    bool keyDownDown  = IsKeyDown(KEY_S) || (gamepadActive && IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN));

    bool keyDownJump = IsKeyDown(KEY_SPACE) || (gamepadActive && IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
    if (keyDownJump) {
        TraceLog(LOG_INFO, "Jomped");
    }

    // horizontal movement and facing
    if (keyUpLeft && keyUpRight) {
        player->animation = getAnimation(character_idle_right);
    }
    else if (keyDownLeft) {
        moveActorX(player, -speed * dt, world, NULL);
        if (player->animation.id != character_run_right) {
            player->animation = getAnimation(character_run_right);
            player->stateTime = 0;
        }
        player->facing = left;
    }
    else if (keyDownRight) {
        moveActorX(player, speed * dt, world, NULL);
        if (player->animation.id != character_run_right) {
            player->animation = getAnimation(character_run_right);
            player->stateTime = 0;
        }
        player->facing = right;
    }

    // vertical movement
    if (keyDownUp) {
        moveActorY(player, -speed * dt, world, NULL);
    }
    else if (keyDownDown) {
        moveActorY(player, speed * dt, world, NULL);
    }
}

//----------------------------------------------------------------------------------
// Solid functions
//----------------------------------------------------------------------------------

void moveSolid(Solid *solid, float x, float y, World *world) {
    solid->remainder.x += x;
    solid->remainder.y += y;

    int moveX = round(solid->remainder.x);
    int moveY = round(solid->remainder.y);
    if (moveX == 0 && moveY == 0) {
        return;
    }

    // loop through every actor to determine which actors are riding the solid
    ActorPtr *ridingActors = NULL;
    int numActors = stb_arr_len(world->actors);
    for (int i = 0; i < numActors; ++i) {
        Actor *actor = &world->actors[i];
        if (isRiding(actor, solid)) {
            stb_arr_push(ridingActors, actor);
        }
    }

    // make this solid non-collidable for actors
    // so actors moved by it do not get stuck on it
    solid->collidable = false;

    // do x-axis movement
    if (moveX != 0) {
        solid->remainder.x -= moveX;
        solid->bounds.x    += moveX;

        if (moveX > 0) {
            numActors = stb_arr_len(world->actors);
            for (int i = 0; i < numActors; ++i) {
                Actor *actor = &world->actors[i];
                if (collide(actor->hitbox, solid->bounds)) {
                    // push right
                    float amount = (solid->bounds.x + solid->bounds.width) - actor->hitbox.x;
                    TraceLog(LOG_DEBUG, "Pushing right by x: %.1f", amount);
                    moveActorX(actor, amount, world, onCollide_Squish);
                } else {
                    // carry right (if riding actor)
                    int numRidingActors = stb_arr_len(ridingActors);
                    for (int j = 0; j < numRidingActors; ++j) {
                        Actor *ridingActor = ridingActors[j];
                        if (ridingActor == actor) {
                            moveActorX(actor, moveX, world, NULL);
                            break;
                        }
                    }
                }
            }
        } else {
            numActors = stb_arr_len(world->actors);
            for (int i = 0; i < numActors; ++i) {
                Actor *actor = &world->actors[i];
                if (collide(actor->hitbox, solid->bounds)) {
                    // push left
                    float amount = solid->bounds.x - (actor->hitbox.x + actor->hitbox.width);
                    moveActorX(actor, amount, world, onCollide_Squish);
                } else {
                    // carry left (if riding actor)
                    int numRidingActors = stb_arr_len(ridingActors);
                    for (int j = 0; j < numRidingActors; ++j) {
                        Actor *ridingActor = ridingActors[j];
                        if (ridingActor == actor) {
                            moveActorX(actor, moveX, world, NULL);
                            break;
                        }
                    }
                }
            }
        }
    }

    // do y-axis movement
    if (moveY != 0) {
        solid->remainder.y -= moveY;
        solid->bounds.y    += moveY;

        if (moveY > 0) {
            numActors = stb_arr_len(world->actors);
            for (int i = 0; i < numActors; ++i) {
                Actor *actor = &world->actors[i];
                if (collide(actor->hitbox, solid->bounds)) {
                    // push down
                    float amount = (solid->bounds.y + solid->bounds.height) - actor->hitbox.y;
                    moveActorY(actor, amount, world, onCollide_Squish);
                } else {
                    // carry down (if riding actor)
                    int numRidingActors = stb_arr_len(ridingActors);
                    for (int j = 0; j < numRidingActors; ++j) {
                        Actor *ridingActor = ridingActors[j];
                        if (ridingActor == actor) {
                            moveActorY(actor, moveY, world, NULL);
                            break;
                        }
                    }
                }
            }
        } else {
            numActors = stb_arr_len(world->actors);
            for (int i = 0; i < numActors; ++i) {
                Actor *actor = &world->actors[i];
                if (collide(actor->hitbox, solid->bounds)) {
                    // push up
                    float amount = solid->bounds.y - (actor->hitbox.y + actor->hitbox.height);
                    moveActorY(actor, amount, world, onCollide_Squish);
                } else {
                    // carry up (if riding actor)
                    int numRidingActors = stb_arr_len(ridingActors);
                    for (int j = 0; j < numRidingActors; ++j) {
                        Actor *ridingActor = ridingActors[j];
                        if (ridingActor == actor) {
                            moveActorY(actor, moveY, world, NULL);
                            break;
                        }
                    }
                }
            }
        }
    }

    // re-enable collisions for this solid
    solid->collidable = true;

    // clear out temp riding actors array
    if (ridingActors != NULL) {
        stb_arr_free(ridingActors);
    }
}
