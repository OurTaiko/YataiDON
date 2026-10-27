#pragma once

#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"
#include "../enums.h"

class DrumHitEffect {
protected:
    DrumType type;
    Side side;
    FadeAnimation* fade = nullptr;
    TextureObject* t_effect = nullptr;

public:
    DrumHitEffect(DrumType type, Side side);
    virtual ~DrumHitEffect() = default;

    void update(double current_ms);

    virtual void draw(float y);

    bool is_finished() const;
};
