#pragma once

#include "../../libs/gfx.h"
#include "../../libs/gfx/texture.h"
#include "../../libs/math/animation.h"

class SongSelectScript;

class SearchBox {
private:
    FadeAnimation* diff_fade_in = nullptr;
    TextureResizeAnimation* bg_resize = nullptr;

public:
    std::string current_search;
    SearchBox();
    void update(double current_ms);
    void draw(SongSelectScript* script); // painted by the skin's SongSelect:draw_search_box
};
