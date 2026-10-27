#pragma once

#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"
#include "../enums.h"

class LaneHitEffect {
private:
    DrumType type;
    Judgments judgment;
    FadeAnimation* fade = nullptr;
    TextureObject* t_effect = nullptr;

public:
    LaneHitEffect(DrumType type, Judgments judgment);

    void update(double current_ms);

    void draw(float y);

    bool is_finished() const;
};
