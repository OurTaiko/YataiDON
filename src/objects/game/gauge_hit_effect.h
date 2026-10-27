#pragma once

#include "../../libs/gfx.h"
#include "../../libs/gfx/color_utils.h"
#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"
#include "../../libs/parsers/tja.h"

class GaugeHitEffect {
private:
    NoteType note_type;
    bool is_big;
    bool is_2p;
    TextureChangeAnimation* texture_change;
    FadeAnimation* circle_fadein;
    TextureResizeAnimation* resize;
    FadeAnimation* fade_out;
    FadeAnimation* rotation;
    Color color;
    TextureObject* t_hit_effect = nullptr;
    TextureObject* t_note = nullptr;
    TextureObject* t_circle = nullptr;

    float width;

    float dest_width;
    float dest_height;
    float x2_pos;
    float y2_pos;
    Vec2 origin;
    float rotation_angle;

public:
    GaugeHitEffect(NoteType note_type, bool big, bool is_2p);

    void update(double current_ms);

    void draw(float y);

    bool is_finished() const;
};
