#pragma once

#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"

class DanTransition {
private:
    MoveAnimation* slide_in = nullptr;
    MoveAnimation* don_rise = nullptr;
    MoveAnimation* emblem_spin = nullptr;
    FadeAnimation* emblem_fade = nullptr;
    FadeAnimation* flash = nullptr;
    int base_id;
    bool started;
    double start_ms = -1.0;
    double last_ms = 0;
    TextureObject* t_background = nullptr;
    TextureObject* t_don = nullptr;
    TextureObject* t_emblem = nullptr;
    TextureObject* t_flash = nullptr;

public:
    explicit DanTransition(int base_id = 38);
    bool start(); // false when the skin has no slide-in animation at base_id
    void update(double current_ms);
    void draw();
    bool is_started() const;
    bool is_finished() const;

    double progress() const;
    double duration() const;
};
