#ifndef ECS_COMPONENTS_H
#define ECS_COMPONENTS_H

#include "shared/assets.h"
#include "raylib.h"
#include "flecs.h"

typedef struct { Rectangle rect; } Bounds;

typedef struct { float x, y; } Position;

typedef struct { float x, y; } Velocity;

#define COLLIDER_DEFAULTS .origin = (Vector2){ 0, 0 }

typedef struct {
    Vector2 origin; // offset from Position to Collider rect (top-left)
    Vector2 size;   // Collider rect width, height
} Collider;

#define RENDERABLE_DEFAULTS                       \
    .tint              = WHITE,                   \
    .size              = (Vector2){ 0, 0 },       \
    .scale             = (Vector2){ 1, 1 },       \
    .scale_default     = (Vector2){ 1, 1 },       \
    .scale_pivot       = (Vector2){ 0.5f, 0.5f }, \
    .scale_settle_secs = 0.1f // 100 millis

typedef struct {
    Color    tint;
    Vector2  origin;           // offset from entity's Position component (top-left)
    Vector2  size;
    Vector2  scale;
    Vector2  scale_default;
    Vector2  scale_pivot;      // pivot point for scale changes [0..1] -> [top-left..bottom-right]
    float    scale_settle_secs; // seconds to return to scale_default
    float    rotation;
    int      layer;
} Renderable;

typedef struct { TextureHandle texture; } TexImage;
// TODO: Animator

extern ECS_COMPONENT_DECLARE(Bounds);
extern ECS_COMPONENT_DECLARE(Position);
extern ECS_COMPONENT_DECLARE(Velocity);
extern ECS_COMPONENT_DECLARE(Collider);
extern ECS_COMPONENT_DECLARE(Renderable);
extern ECS_COMPONENT_DECLARE(TexImage);

#endif //ECS_COMPONENTS_H
