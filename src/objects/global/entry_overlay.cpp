#include "entry_overlay.h"

#include "../../libs/network/network.h"

EntryOverlay::EntryOverlay() {
    if (!load("EntryOverlay", "entry_overlay", network.is_online())) {
        return;
    }
    fn_update = lua_object["update"];
    fn_draw = lua_object["draw"];
}

void EntryOverlay::update(double current_ms) {
    online = network.is_online();
    call(fn_update, "EntryOverlay:update", current_ms, online);
}
void EntryOverlay::draw(bool title_screen) {
    call(fn_draw, "EntryOverlay:draw", title_screen);
}
