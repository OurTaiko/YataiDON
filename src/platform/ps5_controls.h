#pragma once
#include <array>
#include <algorithm>
#include <span>
#include <string_view>

namespace ps5_controls {
inline constexpr std::array left_ka{1, 4, 9, 10};
inline constexpr std::array left_don{2, 3, 16};
inline constexpr std::array right_ka{5, 6, 11, 12};
inline constexpr std::array right_don{7, 8, 17};
// Separate per-pad song events from the shared menu/key-binding events.
inline constexpr int player_button_key(int slot, int button) { return 50000 + slot * 100 + button; }
inline bool separate_players(std::string_view screen) { return screen == "GAME_2P"; }

class ControllerSlots {
    std::array<int, 4> ids{-1, -1, -1, -1};
public:
    void update(std::span<const int> connected) {
        for (auto& id : ids)
            if (std::ranges::find(connected, id) == connected.end()) id = -1;
        for (int id : connected) {
            if (slot(id) >= 0) continue;
            auto empty = std::ranges::find(ids, -1);
            if (empty != ids.end()) *empty = id;
        }
    }
    int slot(int id) const {
        if (id < 0) return -1;
        auto found = std::ranges::find(ids, id);
        return found == ids.end() ? -1 : int(found - ids.begin());
    }
    int device(int slot) const { return ids[slot]; }
};

template<class Layout>
bool apply_default_layout_if_empty(Layout& layout) {
    if (!layout.left_kat.empty() || !layout.left_don.empty() ||
        !layout.right_kat.empty() || !layout.right_don.empty()) return false;
    layout.left_kat.assign(left_ka.begin(), left_ka.end());
    layout.left_don.assign(left_don.begin(), left_don.end());
    layout.right_kat.assign(right_ka.begin(), right_ka.end());
    layout.right_don.assign(right_don.begin(), right_don.end());
    return true;
}

enum class MenuAction { None, Left, Right, Confirm, Back, Pause };
inline constexpr MenuAction menu_action(int button) {
    switch (button) {
    case 1: case 4: case 9: return MenuAction::Left;
    case 2: case 3: case 11: return MenuAction::Right;
    case 7: return MenuAction::Confirm;
    case 6: case 18: return MenuAction::Back;
    default: return MenuAction::None;
    }
}
inline bool gameplay(std::string_view screen) {
    return screen == "GAME" || screen == "GAME_2P" || screen == "GAME_DAN" ||
           screen == "GAME_PRACTICE" || screen == "AI_GAME" || screen == "INPUT_TEST" || screen == "INPUT_CALI";
}
}
