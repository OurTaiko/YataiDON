// Platforms the OS never suspends underneath the game (Windows, Linux, macOS, web).
#include "platform.h"

#include <chrono>

double platform_game_time_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool platform_is_suspended() {
    return false;
}

void platform_initialize_after_window() {}
