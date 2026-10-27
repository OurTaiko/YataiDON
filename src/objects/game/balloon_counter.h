#pragma once

#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"

class BalloonCounter {
private:
    int balloon_count{0};
    int balloon_total;
    bool is_popped{false};
    bool is_2p;
    int stage = 0;
    double now_ms = 0;
    double stage_ms = 0;
    TextStretchAnimation* stretch = nullptr;
    FadeAnimation* fade = nullptr;
    TextureObject* t_pop = nullptr;
    TextureObject* t_bubble = nullptr;
    TextureObject* t_counter = nullptr;

public:
    BalloonCounter(int count, bool is_2p);

    void update_count(int count);

    void update(double current_ms, int count);

    void draw(float y);

    bool is_finished() const;

    bool has_popped() const;
};
