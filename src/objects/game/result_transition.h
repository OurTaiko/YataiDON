#pragma once

#include "../../libs/file/script.h"
#include "../../libs/global_data.h"
#include "../../libs/math/animation.h"

class ResultTransition : public LuaScript {
private:
    MoveAnimation* move = nullptr;

    sol::protected_function fn_start, fn_update, fn_draw, fn_is_finished;

public:
    bool is_finished = false;
    bool is_started = false;

    ResultTransition() = default;

    ResultTransition(PlayerNum player_num);

    void start();
    void update(double current_ms);
    void draw();
};
