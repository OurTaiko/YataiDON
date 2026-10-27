#pragma once

#include "../../libs/file/script.h"
#include "../../libs/global_data.h"

class FadeIn : public LuaScript {
    sol::protected_function fn_update, fn_is_finished, fn_draw, fn_start_reverse;

public:
    FadeIn() = default;
    FadeIn(PlayerNum player_num);
    void start_reverse();
    void update(double current_ms);
    bool is_finished();
    void draw();
};
