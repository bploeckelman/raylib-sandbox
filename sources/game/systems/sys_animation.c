#include "ecs_systems.h"

void sys_animation(World *world, const float dt) {
    for (int i = 0; i < world->num_entities; i++) {
        if (!world->alive[i])               continue;
        if (!world->animators  .present[i]) continue;

        const EntityId entity_id = (EntityId)i;
        Animator  *anim       = world_get_animator(world, entity_id);
        const int  num_frames = anim->frames.count;

        anim->state_time += dt;

        if (num_frames == 0 || anim->frame_seconds <= 0.0f) continue;

        if (num_frames == 1) {
            anim->current_frame = 0;
        } else {
            const uint16_t last_frame_index = num_frames - 1;

            uint16_t frame_index = (int)(anim->state_time / anim->frame_seconds);

            switch (anim->mode) {
                case ANIM_NORMAL:  frame_index = min(last_frame_index, frame_index);   break;
                case ANIM_REVERSE: frame_index = max(num_frames - frame_index - 1, 0); break;
                case ANIM_LOOP:    frame_index = frame_index % num_frames;             break;
                case ANIM_LOOP_REVERSE:
                    frame_index = frame_index % num_frames;
                    frame_index = num_frames - frame_index - 1;
                    break;
                case ANIM_LOOP_PINGPONG:
                    frame_index = frame_index % ((num_frames * 2) - 2);
                    if (frame_index >= num_frames) {
                        frame_index = num_frames - 2 - (frame_index - num_frames);
                    }
                    break;
            }

            anim->current_frame = frame_index;
        }
    }
}
