#pragma once

#include "../../libs/file/script.h"

class Indicator : public LuaScript {
public:
    enum class State {
        SKIP = 0,
        SIDE = 1,
        SELECT = 2,
        WAIT = 3,
        CANCEL = 4
    };

private:
    sol::protected_function fn_update;
    sol::protected_function fn_draw;
    sol::protected_function fn_draw_top;
    sol::protected_function fn_screen_pos;

public:
    Indicator(State state);
    void update(double current_ms);
    void draw(float x, float y, float fade = 1.0f);
    // drawn where the skin puts the indicator on `screen` ("song_select", "dan_select", ...)
    void draw_on(const std::string& screen, float fade = 1.0f);

    void draw_top();
};
