#pragma once

#include "../../libs/file/script.h"

// Skin-side draw for the texture-only parts of the dan_select screen (DanSelect in dan_select.lua).
class DanSelectScene : public LuaScript {
    sol::protected_function fn_draw_back;
    sol::protected_function fn_draw_confirm;
    sol::protected_function fn_draw_title;
    sol::protected_function fn_ribbon;

public:
    DanSelectScene();
    bool valid() const { return lua_object.valid() && fn_draw_back.valid(); }

    void draw_back();
    void draw_confirm(float fade, int confirm_index);
    void draw_title();
    // { center, spacing, side_l, side_r } of the dan ribbon, or nothing if the skin gives none
    std::optional<std::array<float, 4>> ribbon();
};
