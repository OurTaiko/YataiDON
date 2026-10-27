#pragma once

#include "../../libs/file/script.h"

class NetworkMsg : public LuaScript {
    sol::protected_function fn_update;
    sol::protected_function fn_draw;
    sol::protected_function fn_show;
    sol::protected_function fn_hide;

public:
    NetworkMsg();
    bool available = false;
    void show(const std::string& label);
    void hide();
    void update(double current_ms);
    void draw();
};
