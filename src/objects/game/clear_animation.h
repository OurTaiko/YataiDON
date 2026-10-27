#pragma once

#include "../../libs/file/script.h"

class ClearAnimation : public LuaScript {
private:
    sol::protected_function fn_update, fn_draw;

public:
    ClearAnimation(bool is_2p);

    void update(double current_ms);
    void draw();
};
