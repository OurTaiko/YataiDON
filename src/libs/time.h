#pragma once

#include <chrono>

#include "../platform/platform.h"

inline double get_current_ms() {
    return platform_game_time_ms();
}

extern double g_frame_ms;

inline double get_frame_ms() {
    return (g_frame_ms > 0.0) ? g_frame_ms : get_current_ms();
}
