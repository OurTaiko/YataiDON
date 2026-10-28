#pragma once
#include <array>
#include <algorithm>

namespace ps5_controls {
inline constexpr std::array left_ka{1, 4, 9, 10};
inline constexpr std::array left_don{2, 3, 16};
inline constexpr std::array right_ka{5, 6, 11, 12};
inline constexpr std::array right_don{7, 8, 17};
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

}
