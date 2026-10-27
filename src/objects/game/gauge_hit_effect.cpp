#include "gauge_hit_effect.h"

#include <stdexcept>

GaugeHitEffect::GaugeHitEffect(NoteType note_type, bool is_big, bool is_2p)
    : note_type(note_type), is_big(is_big), is_2p(is_2p) {
    texture_change = dynamic_cast<TextureChangeAnimation*>(tex.get_animation(2, true));
    circle_fadein = dynamic_cast<FadeAnimation*>(tex.get_animation(31, true));
    resize = dynamic_cast<TextureResizeAnimation*>(tex.get_animation(32, true));
    fade_out = dynamic_cast<FadeAnimation*>(tex.get_animation(33, true));
    rotation = dynamic_cast<FadeAnimation*>(tex.get_animation(34, true));

    if (!texture_change || !circle_fadein || !resize || !fade_out || !rotation) {
        throw std::runtime_error("GaugeHitEffect: animation 2/31/32/33/34 missing or of unexpected type");
    }

    texture_change->start();
    circle_fadein->start();
    resize->start();
    fade_out->start();
    rotation->start();

    color = fade(Color::YELLOW, circle_fadein->attribute);

    t_hit_effect = tex.get_texture("gauge/hit_effect");
    t_note = tex.get_texture("notes/" + std::to_string(static_cast<int>(note_type)));
    t_circle = is_big ? tex.get_texture("gauge/hit_effect_circle_big") : tex.get_texture("gauge/hit_effect_circle");

    if (!t_hit_effect) {
        throw std::runtime_error("GaugeHitEffect: texture gauge/hit_effect missing");
    }

    width = t_hit_effect->width;

    dest_width = width * tex.screen_scale;
    dest_height = width * tex.screen_scale;
    origin = {width / 2, width / 2};
    rotation_angle = 0;
    x2_pos = -width;
    y2_pos = -width;
}

void GaugeHitEffect::update(double current_ms) {
    texture_change->update(current_ms);
    circle_fadein->update(current_ms);
    resize->update(current_ms);
    fade_out->update(current_ms);
    rotation->update(current_ms);

    if (texture_change->attribute < 2 || resize->attribute <= 0.70) {
        color = Color::WHITE;
    } else if (resize->attribute <= 0.80) {
        color = Color::YELLOW;
    } else if (resize->attribute <= 0.90) {
        if (is_2p) {
            color = Color::GREEN;
        } else {
            color = Color::ORANGE;
        }
    } else {
        if (is_2p) {
            color = Color{84, 250, 238, 255};
        } else {
            color = Color::RED;
        }
    }

    const bool ring = texture_change->attribute >= 2.0;
    const double scale = ring ? resize->attribute : 1.0;
    dest_width = width * scale;
    dest_height = width * scale;
    origin = {dest_width / 2, dest_height / 2};
    x2_pos = -width + (width * scale);
    y2_pos = -width + (width * scale);

    rotation_angle = ring ? rotation->attribute : 0;
}

void GaugeHitEffect::draw(float y) {
    // Main hit effect texture
    tex.draw_texture(t_hit_effect, {.color = fade(color, fade_out->attribute),
                                    .frame = static_cast<int>(texture_change->attribute),
                                    .center = true,
                                    .y = y,
                                    .x2 = x2_pos,
                                    .y2 = y2_pos,
                                    .origin = origin,
                                    .rotation = rotation_angle,
                                    .index = is_2p});

    // Note type texture
    const SkinInfo& pos_data = *tex.get_skin_config(SC::GAME_GAUGE_HIT_EFFECT_NOTE);
    tex.draw_texture(t_note,
                     {.x = pos_data.x, .y = y + pos_data.y + (pos_data.height * is_2p), .fade = fade_out->attribute});

    // Circle effect texture
    Color texture_color;
    if (circle_fadein->is_finished) {
        texture_color = fade(Color::WHITE, std::min(circle_fadein->attribute, fade_out->attribute));
    } else {
        texture_color = fade(Color::YELLOW, std::min(circle_fadein->attribute, fade_out->attribute));
    }
    tex.draw_texture(t_circle, {.color = texture_color, .y = y, .index = is_2p});
}

bool GaugeHitEffect::is_finished() const {
    return fade_out->is_finished;
}
