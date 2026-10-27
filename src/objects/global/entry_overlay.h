#pragma once

#include "../../libs/file/script.h"

class EntryOverlay : public LuaScript {
    sol::protected_function fn_update;
    sol::protected_function fn_draw;

public:
    EntryOverlay();
    bool online = false;
    void update(double current_ms);
    void draw(bool title_screen);
};
