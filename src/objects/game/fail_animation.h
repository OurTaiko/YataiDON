#pragma once

#include "../../libs/file/script.h"

class FailAnimation : public LuaScript {
private:
    sol::protected_function fn_update, fn_draw;

public:
    FailAnimation(bool is_2p);

    void update(double current_ms);
    void draw();
};
