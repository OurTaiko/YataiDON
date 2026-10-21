#pragma once
#include <array>
#include <string_view>

namespace ps5_controls {
inline constexpr std::array left_ka{1, 4, 9, 10};
inline constexpr std::array left_don{2, 3, 16};
inline constexpr std::array right_ka{5, 6, 11, 12};
inline constexpr std::array right_don{7, 8, 17};
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
