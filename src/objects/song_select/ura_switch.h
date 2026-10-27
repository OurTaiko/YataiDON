#pragma once

#include "../../libs/file/script.h"

class UraSwitchAnimation : public LuaScript {
private:
    sol::protected_function fn_start, fn_update, fn_draw;

public:
    UraSwitchAnimation();

    void start(bool is_backwards);
    void update(double current_ms);
    void draw();
};
