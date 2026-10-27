#pragma once

#include <array>
#include <optional>
#include <string>

#include "../../libs/file/script.h"

class RetryGameScene : public LuaScript {
    sol::protected_function fn_update;
    sol::protected_function fn_draw_back;
    sol::protected_function fn_draw_front;
    sol::protected_function fn_chara_pos;
    sol::protected_function fn_phase;
    sol::protected_function fn_hit;
    sol::protected_function fn_is_finished;
    sol::protected_function fn_is_success;

public:
    RetryGameScene(int player_num, int norma);
    void update(double current_ms);

    void draw_back();
    void draw_front();

    std::optional<std::array<float, 3>> chara_pos();

    std::string phase();
    void hit();
    bool is_finished();
    bool is_success();
};
