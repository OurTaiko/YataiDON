#include "fireworks.h"

#include <algorithm>
#include <cmath>

static constexpr int GOGO_EXPLOSION_ANIM_ID = 23;

Fireworks::Fireworks() {
    explosion_anim = dynamic_cast<TextureChangeAnimation*>(tex.get_animation(GOGO_EXPLOSION_ANIM_ID, true));
    if (!explosion_anim) {
        throw std::runtime_error("Animation " + std::to_string(GOGO_EXPLOSION_ANIM_ID) +
                                 " is not a TextureChangeAnimation");
    }

    explosion_anim->start();
    t_explosion = tex.get_texture("gogo_time/explosion");
}

void Fireworks::update(double current_ms) {
    if (!explosion_anim) {
        return;
    }
    explosion_anim->update(current_ms);
}

void Fireworks::draw() {
    if (!explosion_anim || !t_explosion) { // a skin without gogo_time/explosion art
        return;
    }
    if (!explosion_anim->is_finished) {
        if (const SkinInfo* t = tex.get_skin_config(SC::GAME_GOGO_EXPLOSION_TILT);
            t && std::isfinite(t->x) && t->x > 0) {
            static constexpr int ANGLE[6] = {2, 1, 0, 0, -1, -2};
            for (int i = 0; i < 6; i++) {
                float w = t_explosion->x2[i], h = t_explosion->y2[i];
                tex.draw_texture(t_explosion, {.frame = static_cast<int>(explosion_anim->attribute),
                                               .mirror = i >= 4 ? Mirror::HORIZONTAL : Mirror::NONE,
                                               .origin = {w / 2, h / 2},
                                               .rotation = ANGLE[i] * t->x,
                                               .index = i});
            }
            return;
        }
        int slots = 5;
        int mirror_from = -1;
        constexpr int MAX_EXPLOSION_SLOTS = 32;
        if (const SkinInfo* s = tex.get_skin_config(SC::GAME_GOGO_EXPLOSION_SLOTS);
            s && std::isfinite(s->x) && s->x > 0) {
            slots = static_cast<int>(std::clamp(s->x, 1.0f, static_cast<float>(MAX_EXPLOSION_SLOTS)));
            if (std::isfinite(s->y) && s->y > 0) {
                mirror_from = static_cast<int>(std::min(s->y, static_cast<float>(slots)));
            }
        }
        for (int i = 0; i < slots; i++) {
            tex.draw_texture(t_explosion,
                             {.frame = static_cast<int>(explosion_anim->attribute),
                              .mirror = (mirror_from >= 0 && i >= mirror_from) ? Mirror::HORIZONTAL : Mirror::NONE,
                              .index = i});
        }
    }
}

bool Fireworks::is_finished() const {
    return !explosion_anim || explosion_anim->is_finished;
}
