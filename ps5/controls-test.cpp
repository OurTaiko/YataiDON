#include "../src/platform/ps5_controls.h"
#include <cassert>
#include <set>
int main() {
    using namespace ps5_controls;
    std::set<int> hits;
    auto add = [&hits](auto group) {
        for (int button : group) assert(hits.insert(button).second);
    };
    add(left_ka); add(left_don); add(right_ka); add(right_don);
    assert(hits.size() == 14 && !hits.count(15) && !hits.count(18));
    // The D-pad's left-Don gameplay bindings must never confirm a menu.
    assert(menu_action(2) == MenuAction::Right && menu_action(3) == MenuAction::Right);
    assert(menu_action(7) == MenuAction::Confirm && menu_action(6) == MenuAction::Back);
    assert(menu_action(15) == MenuAction::None && menu_action(18) == MenuAction::Back);
    assert(menu_action(16) == MenuAction::None && menu_action(17) == MenuAction::None);
    assert(gameplay("GAME") && gameplay("GAME_2P") && gameplay("INPUT_TEST"));
    assert(!gameplay("ENTRY") && !gameplay("SETTINGS") && !gameplay("SONG_SELECT"));
}
