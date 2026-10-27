#pragma once

#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"

class GogoTime {
private:
    TextureResizeAnimation* fire_resize = nullptr;
    TextureChangeAnimation* fire_change = nullptr;
    float fire_fade;
    TextureObject* t_fire = nullptr;
    TextureResizeAnimation* lane_resize = nullptr;
    TextureObject* t_lane_effect = nullptr;

public:
    GogoTime();

    void update(double current_ms);
    void draw(float judge_x, float judge_y);
};
