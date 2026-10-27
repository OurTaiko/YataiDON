#pragma once

#include <string>

#include "../../libs/file/script.h"

class NameEntry : public LuaScript {
public:
    enum Action {
        NONE = 0,
        LEFT = 1,
        RIGHT = 2,
        DECIDE = 3
    };

    explicit NameEntry(bool is_2p);
    bool unavailable() const { return !fn_input.valid(); }
    void update(double current_ms);
    void input(Action action);
    void draw();
    bool is_done();
    std::string get_name(); // empty: keep the default name

private:
    sol::protected_function fn_update;
    sol::protected_function fn_input;
    sol::protected_function fn_draw;
    sol::protected_function fn_is_done;
    sol::protected_function fn_get_name;
};
