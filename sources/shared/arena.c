#include "shared/arena.h"

// Round `used` up to the next multiple of `align`.
// Assumes align is a power of 2, which _Alignof guarantees for all standard C types.
//
//   adding (align - 1) pushes any not-yet-aligned `used` past the next boundary,
//   while leaving an already-aligned value alone;
//   ANDing with ~(align - 1) clears the low bits, snapping back down to the boundary.
//
// Example, align = 8:
//   used = 13:  (13 + 7) & ~7  =  20 & ~0b111  =  10100 & 11000  =  16   (rounded up)
//   used = 16:  (16 + 7) & ~7  =  23 & ~0b111  =  10111 & 11000  =  16   (already aligned, unchanged)
void *arena_alloc(Arena *arena, const size_t size, const size_t align) {
    const size_t aligned = (arena->used + align - 1) & ~(align - 1);
    if (aligned + size > ARENA_BYTES) return NULL;

    void *p = arena->bytes + aligned;
    arena->used = aligned + size;
    return p;
}
