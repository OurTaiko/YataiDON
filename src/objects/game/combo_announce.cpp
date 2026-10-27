#include "combo_announce.h"

#include <stdexcept>

#include "../../libs/audio/audio.h"

ComboAnnounce::ComboAnnounce(int combo, double current_ms, PlayerNum player_num)
    : combo(combo), wait(current_ms), player_num(player_num) {

    fade = dynamic_cast<FadeAnimation*>(tex.get_animation(65, true));
    if (fade == nullptr) {
        throw std::runtime_error("combo announce fade animation missing or of unexpected type");
    }
    fade->start();

    if (load("ComboAnnounce", "combo_announce", combo, static_cast<int>(player_num))) {
        fn_draw = lua_object["draw"];
    }
}

void ComboAnnounce::update(double current_ms) {
    if (is_finished && fade->is_finished) {
        return;
    }

    if (current_ms >= wait + 1666.67f && !is_finished) {
        fade->start();
        is_finished = true;
    }

    fade->update(current_ms);

    if (!audio_played && combo >= 100) {
        audio_played = true;
        std::string sound_name =
            "combo_" + std::to_string(combo) + "_" + std::to_string(static_cast<int>(player_num)) + "p";
        if (audio.has_sound(sound_name)) {
            audio.play_sound(sound_name, VolumePreset::VOICE);
        }
    }
}

void ComboAnnounce::draw(float y) {
    if (combo == 0) {
        return;
    }

    if (is_finished && fade->is_finished) {
        return;
    }

    float fade_value = is_finished ? fade->attribute : 1 - fade->attribute;

    if (fn_draw.valid()) {
        call(fn_draw, "ComboAnnounce:draw", y, fade_value);
        return;
    }
}
