#pragma once

#include "../../libs/file/script.h"

// The combo counter on the lane. The skin's Combo (game/combo.lua) owns the animations and draws it.
class Combo : public LuaScript {
private:
    sol::protected_function fn_update, fn_draw;

public:
    Combo() = default;
    Combo(int combo, double current_ms) {
        if (!load("Combo", "combo", combo, current_ms)) {
            return;
        }
        fn_update = lua_object["update"];
        fn_draw = lua_object["draw"];
    }

    void update(double current_ms, int curr_combo) { call(fn_update, "Combo:update", current_ms, curr_combo); }
    void draw(float y) { call(fn_draw, "Combo:draw", y); }
};
