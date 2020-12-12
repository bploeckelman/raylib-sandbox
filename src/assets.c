#include "assets.h"

#include "../lib/stb/stb.h"

//----------------------------------------------------------------------------------
// Global variable initializations
//----------------------------------------------------------------------------------

static Assets assets = {0};

//----------------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------------

Animation *loadAnimation(AssetID assetID, const char *baseFilename, int numFrames);

//----------------------------------------------------------------------------------
// Public function implementations
//----------------------------------------------------------------------------------

void loadAssets() {
    loadAnimation(anim_character_idle,      "../assets/character/char-idle",     10);
    loadAnimation(anim_character_run_right, "../assets/character/char-run-right", 6);
    loadAnimation(anim_character_jump_up,   "../assets/character/char-jump-up",   1);
    loadAnimation(anim_character_jump_down, "../assets/character/char-jump-down", 1);
}

void unloadAssets() {
    if (assets.animations != NULL) {
        int numAnimations = stb_arr_len(assets.animations);
        for (int i = 0; i < numAnimations; ++i) {
            free(assets.animations[i].frames);
        }
        stb_arr_free(assets.animations);
    }

    if (assets.textures != NULL) {
        int numTextures = stb_arr_len(assets.textures);
        for (int i = 0; i < numTextures; ++i) {
            UnloadTexture(assets.textures[i]);
        }
        stb_arr_free(assets.textures);
    }

    assets = (Assets) {0};
}

//----------------------------------------------------------------------------------
// Private function implementations
//----------------------------------------------------------------------------------

char filename[1000];

Animation *loadAnimation(AssetID assetID, const char *baseFilename, int numFrames) {
    Animation animation = {
            .frameDuration = 0.1f,
            .frames = calloc(numFrames, sizeof(Texture2D *))
    };

    for (int i = 0; i < numFrames; ++i) {
        snprintf(filename, sizeof(filename), "%s_%d.png", baseFilename, i);
        Texture2D texture = LoadTexture(filename);
        animation.frames[i] = &texture;
        stb_arr_push(assets.textures, texture);
    }

    stb_arr_insert(assets.animations, assetID, animation);
}
