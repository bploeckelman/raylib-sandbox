#ifndef CAMERA_H
#define CAMERA_H

#include "game/game.h"
#include "shared/common.h"
#include "raylib.h"

// Visible world rect for the given camera state.
// Center-anchored; camera_pos is world point that appears at screen center
static Rectangle camera_world_bounds(const GameWorld *w) {
    const float zoom   = w->camera_zoom > 0 ? w->camera_zoom : 1.0f;
    const float half_w = (SCREEN_WIDTH  * 0.5f) / zoom;
    const float half_h = (SCREEN_HEIGHT * 0.5f) / zoom;
    return (Rectangle) {
        .x      = w->camera_pos.x - half_w,
        .y      = w->camera_pos.y - half_h,
        .width  = 2.0f * half_w,
        .height = 2.0f * half_h,
    };
}

static Camera2D camera_to_raylib(const Vector2 pos, const float zoom) {
    return (Camera2D) {
        .target   = pos,
        .offset   = (Vector2){ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
        .zoom     = zoom,
        .rotation = 0.0f,
    };
}

#endif //CAMERA_H
