#include "collision.h"

CollisionContext collide_build_context(
    World          *world,
    const EntityId  mover,
    const EntityId  target,
    const int       axis_sign_x,
    const int       axis_sign_y
) {
    return (CollisionContext){
        .mover      = mover,
        .target     = target,
        .mover_pos  = *world_get_position(world, mover),
        .target_pos = *world_get_position(world, target),
        .mover_col  =  world_get_collider(world, mover),
        .target_col =  world_get_collider(world, target),
        .dir_x      = axis_sign_x,
        .dir_y      = axis_sign_y,
    };
}

CollisionResponse collide_default_response(uint32_t target_mask) {
    // if (target_mask & COL_SENSOR)   return COL_RESP_PASSTHROUGH;
    // if (target_mask & COL_JUMPTHRU) return COL_RESP_PASSTHROUGH; // default; the handler overrides
    return COL_RESP_STOP_BOTH;
}

// TODO: implement remaining overlap_*_* stubs

static bool overlap_rect_rect(const ShapeRect *shape_a, const Vector2 pos_a, const ShapeRect *shape_b, const Vector2 pos_b) {
    const Rectangle rect_a = { pos_a.x + shape_a->offset.x, pos_a.y + shape_a->offset.y, shape_a->size.x, shape_a->size.y };
    const Rectangle rect_b = { pos_b.x + shape_b->offset.x, pos_b.y + shape_b->offset.y, shape_b->size.x, shape_b->size.y };
    return CheckCollisionRecs(rect_a, rect_b);
}

static bool overlap_rect_circ(const ShapeRect *shape_a, const Vector2 pos_a, const ShapeCirc *shape_b, const Vector2 pos_b) { (void)shape_a;(void)pos_a;(void)shape_b,(void)pos_b; return false; }
static bool overlap_rect_pill(const ShapeRect *shape_a, const Vector2 pos_a, const ShapePill *shape_b, const Vector2 pos_b) { (void)shape_a;(void)pos_a;(void)shape_b,(void)pos_b; return false; }
static bool overlap_rect_grid(const ShapeRect *shape_a, const Vector2 pos_a, const ShapeGrid *shape_b, const Vector2 pos_b) {
    // Mover rect in world space.
    const float rect_left   = pos_a.x + shape_a->offset.x;
    const float rect_top    = pos_a.y + shape_a->offset.y;
    const float rect_right  = rect_left + shape_a->size.x;
    const float rect_bottom = rect_top  + shape_a->size.y;

    // Grid origin in world space.
    const float grid_origin_x = pos_b.x + shape_b->offset.x;
    const float grid_origin_y = pos_b.y + shape_b->offset.y;
    const int   cell          = shape_b->cell_size;

    // Cell range (inclusive on both ends) that the rect can touch.
    int col_min = (int)floorf((rect_left   - grid_origin_x) / (float)cell);
    int col_max = (int)floorf((rect_right  - grid_origin_x) / (float)cell);
    int row_min = (int)floorf((rect_top    - grid_origin_y) / (float)cell);
    int row_max = (int)floorf((rect_bottom - grid_origin_y) / (float)cell);

    if (col_min < 0)              col_min = 0;
    if (row_min < 0)              row_min = 0;
    if (col_max >= shape_b->cols) col_max = shape_b->cols - 1;
    if (row_max >= shape_b->rows) row_max = shape_b->rows - 1;
    if (col_min > col_max || row_min > row_max) return false;

    for (int row = row_min; row <= row_max; row++) {
        for (int col = col_min; col <= col_max; col++) {
            if (!shape_b->solid[row * shape_b->cols + col]) continue;
            const Rectangle cell_rect = (Rectangle){
                grid_origin_x + (float)(col * cell),
                grid_origin_y + (float)(row * cell),
                (float)cell, (float)cell,
            };
            const Rectangle mover = { rect_left, rect_top, shape_a->size.x, shape_a->size.y };
            if (CheckCollisionRecs(mover, cell_rect)) return true;
        }
    }
    return false;
}

static bool overlap_circ_circ(const ShapeCirc *shape_a, const Vector2 pos_a, const ShapeCirc *shape_b, const Vector2 pos_b) { (void)shape_a;(void)pos_a;(void)shape_b,(void)pos_b; return false; }
static bool overlap_circ_pill(const ShapeCirc *shape_a, const Vector2 pos_a, const ShapePill *shape_b, const Vector2 pos_b) { (void)shape_a;(void)pos_a;(void)shape_b,(void)pos_b; return false; }
static bool overlap_circ_grid(const ShapeCirc *shape_a, const Vector2 pos_a, const ShapeGrid *shape_b, const Vector2 pos_b) { (void)shape_a;(void)pos_a;(void)shape_b,(void)pos_b; return false; }

static bool overlap_pill_pill(const ShapePill *shape_a, const Vector2 pos_a, const ShapePill *shape_b, const Vector2 pos_b) { (void)shape_a;(void)pos_a;(void)shape_b,(void)pos_b; return false; }
static bool overlap_pill_grid(const ShapePill *shape_a, const Vector2 pos_a, const ShapeGrid *shape_b, const Vector2 pos_b) { (void)shape_a;(void)pos_a;(void)shape_b,(void)pos_b; return false; }

bool collide_shape_overlaps(
    const ColliderShape *shape_a, const Vector2 pos_a, const Vector2 mover_offset,
    const ColliderShape *shape_b, const Vector2 pos_b
) {
    const Vector2 pa = (Vector2){
        pos_a.x + mover_offset.x,
        pos_a.y + mover_offset.y
    };

    // Canonicalize: lower kind ordinal is the 'a' side. Grid is always 'b'.
    if (shape_b->kind < shape_a->kind) {
        return collide_shape_overlaps(shape_b, pos_b, (Vector2){0,0}, shape_a, pos_a);
    }

    switch (shape_a->kind) {
        case SHAPE_RECT:
            switch (shape_b->kind) {
            case SHAPE_RECT: return overlap_rect_rect(&shape_a->as.rect, pa, &shape_b->as.rect, pos_b);
            case SHAPE_CIRC: return overlap_rect_circ(&shape_a->as.rect, pa, &shape_b->as.circ, pos_b);
            case SHAPE_PILL: return overlap_rect_pill(&shape_a->as.rect, pa, &shape_b->as.pill, pos_b);
            case SHAPE_GRID: return overlap_rect_grid(&shape_a->as.rect, pa, &shape_b->as.grid, pos_b);
            default: return false;
            }
        case SHAPE_CIRC:
            switch (shape_b->kind) {
            case SHAPE_CIRC: return overlap_circ_circ(&shape_a->as.circ, pa, &shape_b->as.circ, pos_b);
            case SHAPE_PILL: return overlap_circ_pill(&shape_a->as.circ, pa, &shape_b->as.pill, pos_b);
            case SHAPE_GRID: return overlap_circ_grid(&shape_a->as.circ, pa, &shape_b->as.grid, pos_b);
            default: return false;
            }
        case SHAPE_PILL:
            switch (shape_b->kind) {
            case SHAPE_PILL: return overlap_pill_pill(&shape_a->as.pill, pa, &shape_b->as.pill, pos_b);
            case SHAPE_GRID: return overlap_pill_grid(&shape_a->as.pill, pa, &shape_b->as.grid, pos_b);
            default: return false;
            }
        case SHAPE_GRID:
            // Should never be the 'a' side after canonicalize. If it is, two grids
            // were passed — explicit assert is friendlier than silent false.
            return false;
        default: return false;
    }
}

float collide_shape_top(const ColliderShape *shape, Vector2 pos) {
    switch (shape->kind) {
        case SHAPE_RECT: return pos.y + shape->as.rect.offset.y;
        case SHAPE_CIRC: return pos.y + shape->as.circ.center.y - shape->as.circ.radius;
        case SHAPE_PILL: return pos.y + shape->as.pill.offset.y;
        case SHAPE_GRID: return pos.y + shape->as.grid.offset.y;
        default:         return pos.y;
    }
}

float collide_shape_bottom(const ColliderShape *shape, const Vector2 pos) {
    switch (shape->kind) {
        case SHAPE_RECT: return pos.y + shape->as.rect.offset.y + shape->as.rect.size.y;
        case SHAPE_CIRC: return pos.y + shape->as.circ.center.y + shape->as.circ.radius;
        case SHAPE_PILL: return pos.y + shape->as.pill.offset.y + shape->as.pill.size.y;
        case SHAPE_GRID: return pos.y + shape->as.grid.offset.y + (float)(shape->as.grid.rows * shape->as.grid.cell_size);
        default:         return pos.y;
    }
}

// Inner top/bottom — collapses to the rect-core extents for pill/circ.
// Used by jumpthru handlers etc. For rect/grid, inner == outer.

float collide_shape_inner_top(const ColliderShape *shape, const Vector2 pos) {
    if (shape->kind == SHAPE_PILL && shape->as.pill.axis == PILL_VERTICAL) {
        const float radius = shape->as.pill.size.x * 0.5f;
        return collide_shape_top(shape, pos) + radius;
    }
    if (shape->kind == SHAPE_CIRC) {
        // whole shape is a cap; collapses to center
        return pos.y + shape->as.circ.center.y;
    }
    return collide_shape_top(shape, pos);
}

float collide_shape_inner_bottom(const ColliderShape *shape, const Vector2 pos) {
    if (shape->kind == SHAPE_PILL && shape->as.pill.axis == PILL_VERTICAL) {
        const float radius = shape->as.pill.size.x * 0.5f;
        return collide_shape_bottom(shape, pos) - radius;
    }
    if (shape->kind == SHAPE_CIRC) {
        // whole shape is a cap; collapses to center
        return pos.y + shape->as.circ.center.y;
    }
    return collide_shape_bottom(shape, pos);
}
