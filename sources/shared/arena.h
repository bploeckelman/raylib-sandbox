#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

#define ARENA_BYTES (64 * 1024 * 1024)

typedef struct {
    uint8_t  bytes[ARENA_BYTES];
    size_t   used;
} Arena;

void *arena_alloc(Arena *arena, size_t size, size_t align);

#define ARENA_NEW_ARRAY(arena, type, count) \
    ((type*)arena_alloc((arena), sizeof(type) * (count), _Alignof(type)))

#endif //ARENA_H
