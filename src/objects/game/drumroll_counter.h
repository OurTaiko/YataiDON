#pragma once

#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"

class DrumrollCounter {
private:
    int drumroll_count;
    FadeAnimation* fade = nullptr;
    TextStretchAnimation* stretch = nullptr;
    TextureObject* t_bubble = nullptr;
    TextureObject* t_counter = nullptr;

public:
    DrumrollCounter();

    void update_count(int count);

    void update(double current_ms, int count);

    void update_animations(double current_ms);

    void draw(float y);

    bool is_finished() const;
};
