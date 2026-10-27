#include "search_box.h"

#include <algorithm>

#include "song_select_script.h"

SearchBox::SearchBox() {
    current_search = "";
    bg_resize = dynamic_cast<TextureResizeAnimation*>(tex.get_animation(19));
    diff_fade_in = dynamic_cast<FadeAnimation*>(tex.get_animation(20));
    bg_resize->start();
    diff_fade_in->start();
}

void SearchBox::update(double current_ms) {
    bg_resize->update(current_ms);
    diff_fade_in->update(current_ms);
}

void SearchBox::draw(SongSelectScript* script) {
    if (script) {
        script->draw_search_box(current_search, bg_resize->attribute, diff_fade_in->attribute);
    }
}
