#include "balloon_counter.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>

BalloonCounter::BalloonCounter(int count, bool is_2p) : balloon_total(count), is_2p(is_2p) {
    fade = dynamic_cast<FadeAnimation*>(tex.get_animation(7, true));
    stretch = dynamic_cast<TextStretchAnimation*>(tex.get_animation(6, true));
    if (!fade || !stretch) {
        throw std::runtime_error("balloon counter animation missing or of unexpected type");
    }
    fade->reset();
    stretch->reset();
    t_pop = tex.get_texture("balloon/pop");
    t_bubble = tex.get_texture("balloon/bubble");
    t_counter = tex.get_texture("balloon/counter");
}

void BalloonCounter::update_count(int count) {
    count = std::clamp(count, 0, std::max(balloon_total, 0));
    if (balloon_count != count) {
        balloon_count = count;
        const int new_stage = (balloon_total > 0) ? std::min(5, balloon_count * 6 / balloon_total) : 0;
        if (new_stage != stage || balloon_count == 1) {
            stage_ms = now_ms;
        }
        stage = new_stage;
        fade->start();
        stretch->start();
        if (balloon_total > 0 && balloon_count >= balloon_total) {
            is_popped = true;
        }
    }
}

void BalloonCounter::update(double current_ms, int count) {
    now_ms = current_ms;
    stretch->update(current_ms);
    if (is_popped) {
        fade->update(current_ms);
    }

    update_count(count);
}

void BalloonCounter::draw(float y) {
    const SkinInfo* p2 = is_2p ? tex.get_skin_config(SC::GAME_BALLOON_COUNTER_2P_OFFSET) : nullptr;
    const bool have_p2_key = p2 != nullptr;

    const float legacy_y = is_2p ? 230 * tex.screen_scale : 0.0f;
    const float rig_x = have_p2_key ? p2->x : 0.0f;
    const float rig_y = have_p2_key ? p2->y : 0.0f;

    const float x_offset = rig_x;
    const float y_offset = have_p2_key ? rig_y : legacy_y;
    const float digit_y_offset = have_p2_key ? rig_y : (legacy_y * 1.1f);
    const float body_x = rig_x;
    const float body_y = have_p2_key ? rig_y : 0.0f;
    const Mirror bubble_mirror = (is_2p && !have_p2_key) ? Mirror::VERTICAL : Mirror::NONE;

    if (is_popped) {
        tex.draw_texture(t_pop, {.frame = 7, .x = body_x, .y = y + body_y, .fade = fade->attribute});
    } else if (balloon_count >= 1) {
        // Lumen: stage 0 plays art 0, 1 (83 ms each) then 2; later stages hold the previous stage's art for 167 ms.
        static const int STAGE_ART[6] = {2, 3, 4, 5, 6, 6};
        const double t = now_ms - stage_ms;
        int balloon_index = STAGE_ART[stage];
        if (stage == 0) {
            balloon_index = t < 83 ? 0 : (t < 167 ? 1 : 2);
        } else if (t < 167) {
            balloon_index = STAGE_ART[stage - 1];
        }
        float shake = 0.0f;
        if (tex.get_skin_config(SC::GAME_BALLOON_POP_SHAKE)) {
            shake = (std::fmod(now_ms, 133.0) < 67 ? -2.0f : 1.0f) * tex.screen_scale;
        }
        tex.draw_texture(t_pop,
                         {.frame = balloon_index, .x = body_x + shake, .y = y + body_y, .fade = fade->attribute});
    }
    if (balloon_count > 0) {
        tex.draw_texture(t_bubble,
                         {.mirror = bubble_mirror, .x = x_offset, .y = y + y_offset, .fade = fade->attribute});
        std::string counter = std::to_string(std::max(0, balloon_total - balloon_count));
        float margin = tex.get_skin_config(SC::GAME_DRUMROLL_COUNTER_MARGIN)->x;
        if (const SkinInfo* m = tex.get_skin_config(SC::GAME_BALLOON_COUNTER_MARGIN); m && m->x > 0) {
            margin = m->x;
        }
        float total_width = counter.length() * margin;
        for (int i = 0; i < std::ssize(counter); i++) {
            char digit = counter[i];
            tex.draw_texture(t_counter, {.frame = digit - '0',
                                         .x = x_offset - (total_width / 2.0f) + (i * margin),
                                         .y = y - static_cast<float>(stretch->attribute) + digit_y_offset,
                                         .y2 = static_cast<float>(stretch->attribute),
                                         .fade = fade->attribute});
        }
    }
}

bool BalloonCounter::is_finished() const {
    return fade->is_finished;
}

bool BalloonCounter::has_popped() const {
    return is_popped;
}
