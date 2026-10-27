#include "gogo_time.h"

#include <algorithm>

GogoTime::GogoTime() {
    int resize_anim = 24;
    if (!tex.has_animation(resize_anim)) {
        throw std::runtime_error("gogo time: animation " + std::to_string(resize_anim) + " is missing");
    }
    fire_resize = dynamic_cast<TextureResizeAnimation*>(tex.get_animation(resize_anim, true));
    if (!fire_resize) {
        throw std::runtime_error("gogo time: animation 24 is not a resize animation");
    }

    int change_anim = 25;
    if (const SkinInfo* a = tex.get_skin_config(SC::GAME_GOGO_FIRE_ANIM);
        a && a->x > 0 && tex.has_animation(static_cast<int>(a->x))) {
        change_anim = static_cast<int>(a->x);
    }
    if (!tex.has_animation(change_anim)) {
        throw std::runtime_error("gogo time: animation " + std::to_string(change_anim) + " is missing");
    }
    fire_change = dynamic_cast<TextureChangeAnimation*>(tex.get_animation(change_anim, true));
    if (!fire_change) {
        throw std::runtime_error("gogo time: animation " + std::to_string(change_anim) + " is not a change animation");
    }
    fire_fade = 0.5f;
    if (const SkinInfo* f = tex.get_skin_config(SC::GAME_GOGO_FIRE_FADE); f && f->x > 0) {
        fire_fade = std::clamp(f->x, 0.0f, 1.0f);
    }

    fire_resize->start();
    fire_change->start();

    t_fire = tex.get_texture("gogo_time/fire");

    // The lane effect (animation 76) is newer than many skins' game/animation.json: without it, or
    // with a skin's own 76 of another type, there is no lane effect
    if (tex.has_animation(76) && tex.has_texture("gogo_time/lane_effect")) {
        lane_resize = dynamic_cast<TextureResizeAnimation*>(tex.get_animation(76, true));
        if (lane_resize) {
            lane_resize->start();
            t_lane_effect = tex.get_texture("gogo_time/lane_effect");
        }
    }
}

void GogoTime::update(double current_ms) {
    fire_resize->update(current_ms);
    fire_change->update(current_ms);
    if (lane_resize) {
        lane_resize->update(current_ms);
    }
}

void GogoTime::draw(float judge_x, float judge_y) {
    if (lane_resize && t_lane_effect) {
        // grow vertically only: shrink height by (1-s), shift down half of that to stay centered
        float cut = t_lane_effect->y2[0] * (1.0f - static_cast<float>(lane_resize->attribute));
        tex.draw_texture(t_lane_effect, {.y = judge_y + cut / 2, .y2 = -cut, .fade = 0.5});
    }
    tex.draw_texture(t_fire, {.frame = static_cast<int>(fire_change->attribute),
                              .scale = static_cast<float>(fire_resize->attribute),
                              .center = true,
                              .x = judge_x,
                              .y = judge_y,
                              .fade = fire_fade});
}
