#include "box_base.h"

#include <string_view>
#include <utility>

#include "../../../libs/gfx/text.h"
#include "color_utils.h"

BaseBox::BaseBox(fs::path path, const BoxDef& box_def)
    : path(std::move(path)), texture_index(box_def.texture_index), genre_index(box_def.genre_index),
      genre_label(box_def.genre_label), collection(box_def.collection), explanation(box_def.explanation) {
    BoxColors colors = resolve_box_colors(box_def.box_color, box_def.back_color, box_def.fore_color);
    this->back_color = colors.box;
    this->fore_color = colors.outline;
    this->text_color = colors.text;

    position = std::numeric_limits<float>::infinity();
    target_position = std::numeric_limits<float>::infinity();

    open_anim = std::make_unique<MoveAnimation>(233, 150.0f * tex.screen_scale, false, false, 0, 133);
    open_fade = std::make_unique<FadeAnimation>(200, 0.0f, false, false, 1.0f, 133);
    move = std::make_unique<MoveAnimation>(133, 0, false, false, 0, 0.0, std::nullopt, std::nullopt, EaseType::Cubic);
    move->start();

    yellow_right_width = tex.get_texture("yellow_box/yellow_box_right")->width;
    folder_texture_left_width = tex.get_texture("box/folder_texture_left")->width;
    folder_texture_right_width = tex.get_texture("box/folder_texture_right")->width;

    fade_in(100);
}

BaseBox::~BaseBox() {
    if (shader_loaded) {
        unload_shader(shader);
    }
}

void BaseBox::preregister_text() {
    float font_size = tex.get_skin_config(SC::SONG_SELECT_BOX_NAME)->font_size;
    if (utf8_char_count(text_name) >= 30) {
        font_size -= static_cast<int>(10 * tex.screen_scale);
    }
    font_manager.register_text(text_name, static_cast<int>(font_size));
}

void BaseBox::load_text() {
    float font_size = tex.get_skin_config(SC::SONG_SELECT_BOX_NAME)->font_size;
    if (utf8_char_count(text_name) >= 30) {
        font_size -= static_cast<int>(10 * tex.screen_scale);
    }
    name = make_unique<OutlinedText>(text_name, font_size, text_color, fore_color, true);

    if (back_color.has_value()) {
        shader = load_shader("shader/dummy.vs", "shader/colortransform.fs");
        shader_loaded = true;
        const auto& target_rgb = back_color.value();
        float src[3] = {142 / 255.0f, 212 / 255.0f, 30 / 255.0f};
        float tgt[3] = {target_rgb.r / 255.0f, target_rgb.g / 255.0f, target_rgb.b / 255.0f};
        int source_loc = get_shader_location(shader, "sourceColor");
        int target_loc = get_shader_location(shader, "targetColor");
        set_shader_value(shader, source_loc, src, shader_uniform::VEC3);
        set_shader_value(shader, target_loc, tgt, shader_uniform::VEC3);
    }

    text_loaded = true;
}

void BaseBox::reset() {
    yellow_box_active = false;
    yellow_box_opened = false;
    is_diff_select = false;
    open_anim->reset();
    open_fade->reset();
}

void BaseBox::expand_box() {
    yellow_box_active = true;
    yellow_box_opened = false;
    is_diff_select = false;
    open_anim->start();
    bar_open_started_at = get_current_ms();
}

void BaseBox::close_box() {
    yellow_box_active = false;
    yellow_box_opened = false;
    is_diff_select = false;
}

void BaseBox::enter_box() {
    if (!yellow_box_active) {
        return;
    }
    is_diff_select = true;
}

void BaseBox::exit_box() {
    yellow_box_active = true;
    yellow_box_opened = false;
    is_diff_select = false;
    open_fade->start();
}

void BaseBox::set_position(float target_position) {
    position = target_position;
    this->target_position = position;
}

void BaseBox::move_box(float target_position, float duration, float delay, std::optional<EaseType> ease) {
    this->target_position = target_position;
    float delta = target_position - position;
    move_delta = delta;
    move = std::make_unique<MoveAnimation>(duration, delta, false, false, 0, delay, std::nullopt, std::nullopt, ease);
    move->start();
}

void BaseBox::fade_in(float delay) {
    fade = std::make_unique<FadeAnimation>(266, 0.0f, false, false, 1.0f, delay);
    fade->start();
}

void BaseBox::fade_out() {
    fade = std::make_unique<FadeAnimation>(166);
    fade->start();
}

void BaseBox::update(double current_time) {
    open_anim->update(current_time);
    open_fade->update(current_time);
    fade->update(current_time);
    float prev_position = move->attribute;
    move->update(current_time);
    if (!move->is_finished) {
        position += move->attribute - prev_position;
        if (move_delta != 0.0f && cross_lead != 0.0f) {
            float p = move->attribute / move_delta;
            if (p < 0.0f) {
                p = 0.0f;
            } else if (p > 1.0f) {
                p = 1.0f;
            }
            cross_pos = cross_target + cross_lead * (1.0f - p);
        }
    } else {
        position = target_position;
        cross_pos = cross_target;
        cross_lead = 0.0f;
        if (yellow_box_active && !yellow_box_opened) {
            yellow_box_opened = true;
            open_fade->start();
        }
    }
    if (yellow_box_active) {
        // The Lua skin advances the shared yellow-box animations; read their current
        // attributes to derive the box bounds the navigator uses for the inline-list bg.
        auto yb = [](int id) { return dynamic_cast<MoveAnimation*>(tex.get_animation(id)); };
        if (is_diff_select) {
            left_bound = position + (yb(13)->attribute - yb(13)->start_position);
            right_bound = yb(14)->attribute - yb(14)->start_position;
        } else {
            left_bound = position + (yb(9)->attribute - yb(9)->start_position);
            right_bound = yb(10)->attribute + yellow_right_width;
        }
    } else {
        left_bound = position;
        right_bound = position + folder_texture_left_width + folder_texture_right_width +
                      (tex.get_skin_config(SC::SONG_SELECT_BOX_BG)->width);
    }
}
