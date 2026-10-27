#pragma once

#include "../../libs/file/script.h"

// Keeps the judgment counts; the skin's JudgeCounter (game/judge_counter.lua) draws them.
class JudgeCounter : public LuaScript {
private:
    int good = 0, ok = 0, bad = 0, drumrolls = 0;
    sol::protected_function fn_draw;

public:
    JudgeCounter() {
        if (load("JudgeCounter", "judge_counter")) {
            fn_draw = lua_object["draw"];
        }
    }

    void update(int good, int ok, int bad, int drumrolls) {
        this->good = good;
        this->ok = ok;
        this->bad = bad;
        this->drumrolls = drumrolls;
    }

    void draw() { call(fn_draw, "JudgeCounter:draw", good, ok, bad, drumrolls); }
};
