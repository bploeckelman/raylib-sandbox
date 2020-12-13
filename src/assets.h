#pragma once

#include "raylib.h"

typedef enum AnimID {
      character_idle = 0
    , character_run_right
    , character_jump_up
    , character_jump_down
} AnimID;

typedef enum AnimMode {
      normal = 0
    , reversed
    , looped
    , looped_reversed
    , looped_pingpong
} AnimMode;

typedef struct Animation {
    AnimID id;
    AnimMode mode;
    float frameDuration;
    Texture2D *frames;
} Animation;

typedef struct Assets {
    Texture2D *textures;
    Animation *animations;
} Assets;

extern Assets assets;

void loadAssets();
void unloadAssets();

Animation getAnimation(AnimID animId);
Texture2D getAnimationFrame(const Animation *animation, float stateTime);
