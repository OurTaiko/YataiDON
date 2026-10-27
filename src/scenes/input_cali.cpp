#include "input_cali.h"

#include <algorithm>

#include "../libs/input/input.h"

static const fs::path CALI_TJA_PATH = "Songs/Calibration.tja";
static constexpr double NO_AUTOPLAY_HIT_YET = -1e18;

Modifiers InputCaliScreen::get_player_modifiers(PlayerNum pn) {
    Modifiers mods = GameScreen::get_player_modifiers(pn);
    mods.auto_play = true;
    return mods;
}

void InputCaliScreen::on_screen_start() {
    SessionData& session_data = global_data.session_data[static_cast<int>(global_data.player_num)];
    session_data.selected_song = CALI_TJA_PATH;
    session_data.selected_difficulty = 2;
    GameScreen::on_screen_start();

    background.reset();
    background.emplace(global_data.player_num, 150, "TUTORIAL");
    latencies.clear();
    average_latency = 0.0;
    if (!players.empty() && players[0]) {
        players[0]->last_note_hit = NO_AUTOPLAY_HIT_YET;
    }
}

Screens InputCaliScreen::on_screen_end(Screens next_screen) {
    if (!latencies.empty()) {
        global_data.config->general.audio_offset = average_latency * -1;
    }
    return GameScreen::on_screen_end(next_screen);
}

std::optional<Screens> InputCaliScreen::update() {
    if (check_key_pressed(global_data.config->keys.back_key)) {
        return on_screen_end(Screens::SETTINGS);
    }

    auto result = GameScreen::update();
    if (result == Screens::SONG_SELECT) {
        return std::nullopt;
    }

    if (result == Screens::RESULT) {
        return Screens::SETTINGS;
    }

    if ((is_l_don_pressed() || is_r_don_pressed()) && !players.empty() && players[0] &&
        players[0]->last_note_hit != NO_AUTOPLAY_HIT_YET) {
        double current_ms = get_current_ms();
        double latency = players[0]->last_note_hit - current_ms;
        latencies.push_back(latency);
        size_t n = latencies.size();
        std::ranges::sort(latencies);

        if (n % 2 == 0) {
            average_latency = (latencies[n / 2 - 1] + latencies[n / 2]) / 2.0;
        } else {
            average_latency = latencies[n / 2];
        }
        average_latency_text.reset();
        average_latency_text.emplace(text_format("Average Latency: %.2f ms", average_latency * -1), 40, Color::WHITE,
                                     Color::BLUE, false, 4.0);
    }

    return result;
}

void InputCaliScreen::draw() {
    clear_background(Color::BLACK);
    if (background.has_value()) {
        background->draw_back();
    }
    players[0]->draw(clock.position(), 0, 232 * tex.screen_scale);
    if (average_latency_text.has_value()) {
        average_latency_text->draw(
            {.x = (tex.screen_width / 2) - average_latency_text->width / 2,
             .y = static_cast<float>(tex.screen_height - 40 - static_cast<int>(average_latency_text->height * 1.5))});
    }
    if (background.has_value()) {
        background->draw_fore();
    }
    draw_overlay();
}
