#pragma once

#include "raylib.h"

typedef enum AssetID {
      anim_character_idle = 0
    , anim_character_run_right
    , anim_character_jump_up
    , anim_character_jump_down
} AssetID;

typedef struct Animation {
    float frameDuration;
    Texture2D **frames;
} Animation;

typedef struct Assets {
    Texture2D *textures;
    Animation *animations;
} Assets;

void loadAssets();
void unloadAssets();
