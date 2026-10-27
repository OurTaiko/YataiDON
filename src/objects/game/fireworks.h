#pragma once

#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"

class Fireworks {
private:
    TextureChangeAnimation* explosion_anim = nullptr;
    TextureObject* t_explosion = nullptr;

public:
    Fireworks();

    void update(double current_ms);
    void draw();

    bool is_finished() const;
};
