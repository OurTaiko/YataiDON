#pragma once

#include "../../../libs/gfx/color_utils.h"
#include "box_base.h"

class BackBox : public BaseBox {
public:
    static constexpr Color COLOR = Color(170, 115, 35);

    BackBox(const fs::path& path, const BoxDef& box_def);

    const char* lua_kind() const override { return "back"; }
};
