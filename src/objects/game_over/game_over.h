#pragma once

#include "../../libs/file/script.h"

class GameOverSequence : public LuaScript {
    sol::protected_function fn_update;
    sol::protected_function fn_draw;
    sol::protected_function fn_is_finished;

public:
    GameOverSequence();
    void update(double current_ms);
    void draw();
    bool is_finished();
};
