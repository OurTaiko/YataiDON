#include "dan_transition.h"

#include <algorithm>

DanTransition::DanTransition(int base_id) : base_id(base_id) {
    started = false;
}

bool DanTransition::start() {
    // a skin's own animation.json may predate these ids, or use them for other animations
    auto anim = [](int id) { return tex.has_animation(id) ? tex.get_animation(id) : nullptr; };
    slide_in = dynamic_cast<MoveAnimation*>(anim(base_id + 0));
    if (!slide_in) {
        return false;
    }
    don_rise = dynamic_cast<MoveAnimation*>(anim(base_id + 1));
    emblem_fade = dynamic_cast<FadeAnimation*>(anim(base_id + 2));
    emblem_spin = dynamic_cast<MoveAnimation*>(anim(base_id + 3));
    flash = dynamic_cast<FadeAnimation*>(anim(base_id + 4));
    // the Don, emblem and flash are optional: skins with only a curtain still work
    if (!tex.has_texture("dan_transition/flash") || !don_rise || !emblem_fade || !emblem_spin || !flash) {
        don_rise = emblem_spin = nullptr, emblem_fade = flash = nullptr;
    }
    BaseAnimation* animations[] = {slide_in, don_rise, emblem_fade, emblem_spin, flash};
    for (BaseAnimation* a : animations) {
        if (a) {
            a->start();
        }
    }
    started = true;
    start_ms = -1.0;
    last_ms = 0;
    t_background = tex.get_texture("dan_transition/background");
    if (flash) {
        t_don = tex.get_texture("dan_transition/don");
        t_emblem = tex.get_texture("dan_transition/emblem");
        t_flash = tex.get_texture("dan_transition/flash");
    }
    return true;
}

void DanTransition::update(double current_ms) {
    if (!started || !slide_in) {
        return;
    }
    if (start_ms < 0) {
        start_ms = current_ms;
    }
    last_ms = current_ms;
    BaseAnimation* animations[] = {slide_in, don_rise, emblem_fade, emblem_spin, flash};
    for (BaseAnimation* a : animations) {
        if (a) {
            a->update(current_ms);
        }
    }
}

bool DanTransition::is_started() const {
    return started;
}

bool DanTransition::is_finished() const {
    return started && (flash ? flash->is_finished : slide_in->is_finished);
}

double DanTransition::duration() const {
    return (slide_in && slide_in->duration > 0) ? slide_in->duration : 0.0;
}

double DanTransition::progress() const {
    if (!started || start_ms < 0) {
        return 0.0;
    }
    double d = duration();
    if (d <= 0.0) {
        return 1.0;
    }
    return std::clamp((last_ms - start_ms) / d, 0.0, 1.0);
}

void DanTransition::draw() {
    if (!started || !slide_in) {
        return;
    }
    tex.draw_texture(t_background, {.x2 = static_cast<float>(slide_in->attribute)});
    if (!flash) {
        return;
    }
    tex.draw_texture(t_don, {.y = static_cast<float>(don_rise->attribute)});
    // texture.json x, y is the emblem's centre, which is also the pivot
    tex.draw_texture(t_emblem, {.origin = {t_emblem->width / 2.0f, t_emblem->height / 2.0f},
                                .rotation = static_cast<float>(emblem_spin->attribute),
                                .fade = emblem_fade->attribute});
    tex.draw_texture(t_flash, {.fade = flash->attribute});
}
