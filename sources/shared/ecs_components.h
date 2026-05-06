#ifndef ECS_COMPONENTS_H
#define ECS_COMPONENTS_H

#include "shared/assets.h"
#include "raylib.h"
#include "flecs.h"

typedef struct { float x, y; } Position;
typedef struct { float x, y; } Velocity;

typedef struct {
    TextureHandle  texture;
    Vector2        origin;
    Vector2        scale;
    Color          tint;
    float          rotation;
    int            layer;
} Sprite;

extern ECS_COMPONENT_DECLARE(Position);
extern ECS_COMPONENT_DECLARE(Velocity);
extern ECS_COMPONENT_DECLARE(Sprite);

#endif //ECS_COMPONENTS_H
