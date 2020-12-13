#include "assets.h"

#include "../lib/stb/stb.h"

//----------------------------------------------------------------------------------
// Global variable initializations
//----------------------------------------------------------------------------------

static Assets assets = {0};

//----------------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------------

Animation loadAnimation(AnimID animId, const char *baseFilename, int numFrames);

//----------------------------------------------------------------------------------
// Public function implementations
//----------------------------------------------------------------------------------

void loadAssets() {
    loadAnimation(character_idle,      "../assets/character/char-idle",     10);
//    loadAnimation(character_run_right, "../assets/character/char-run-right", 6);
//    loadAnimation(character_jump_up,   "../assets/character/char-jump-up",   1);
//    loadAnimation(character_jump_down, "../assets/character/char-jump-down", 1);
}

void unloadAssets() {
    if (assets.animations != NULL) {
        int numAnimations = stb_arr_len(assets.animations);
        for (int i = 0; i < numAnimations; ++i) {
            stb_arr_free(assets.animations[i].frames);
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

Animation loadAnimation(AnimID animId, const char *baseFilename, int numFrames) {
    Animation animation = {
            .id = animId,
            .mode = looped,
            .frameDuration = 0.1f,
            .frames = NULL
    };

    for (int i = 0; i < numFrames; ++i) {
        snprintf(filename, sizeof(filename), "%s_%d.png", baseFilename, i);
        // TODO: check whether texture is already loaded
        Texture2D texture = LoadTexture(filename);
        stb_arr_push(assets.textures, texture);
        stb_arr_push(animation.frames, texture);
    }

    stb_arr_push(assets.animations, animation);
}

Animation getAnimation(AnimID animId) {
    for (int i = 0; i < stb_arr_len(assets.animations); ++i) {
        Animation animation = assets.animations[i];
        if (animation.id == animId) {
            return animation;
        }
    }
    TraceLog(LOG_ERROR, "Failed to find animation with id %d", animId);
    return (Animation) {0};
}

Texture2D getAnimationFrame(const Animation *animation, float stateTime) {
    int numFrames = stb_arr_len(animation->frames);
    if (numFrames == 0) {
        TraceLog(LOG_ERROR, "Failed to find animation frame in animation: %d for stateTime: %.2f", animation->id, stateTime);
        return (Texture2D) {0};
    }
    if (numFrames == 1) return animation->frames[0];

    unsigned int frameIndex = (unsigned int) (stateTime / animation->frameDuration);
    switch (animation->mode) {
        case normal:          frameIndex = min(numFrames - 1, frameIndex);        break;
        case reversed:        frameIndex = max(numFrames - frameIndex - 1, 0); break;
        case looped:          frameIndex = frameIndex % numFrames;                   break;
        case looped_reversed: frameIndex = numFrames - (frameIndex % numFrames) - 1; break;
        case looped_pingpong: {
            frameIndex = frameIndex % ((numFrames * 2) - 2);
            if (frameIndex >= numFrames) {
                frameIndex = numFrames - 2 - (frameIndex - numFrames);
            }
        } break;
    }

//    animation->lastFrameIndex = frameIndex;
//    animation->lastStateTime = stateTime;

    return animation->frames[frameIndex];
}

