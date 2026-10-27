#pragma once

#include <filesystem>

// Services every platform provides. CMake compiles exactly one platform's sources,
// so callers include this header and call these without platform guards.

// Prepares the directory the game runs from (assets, config, scores) and returns it.
// An empty path keeps the current working directory.
std::filesystem::path platform_get_working_directory();

// Installs signal/exception handlers that report the crash before exiting.
void platform_install_crash_handlers();

// Destroys the main window, working around platform driver teardown issues.
void platform_close_window();

// Runs once the window exists (e.g. hooking mobile app lifecycle events).
void platform_initialize_after_window();

// Monotonic milliseconds since an arbitrary epoch, frozen while the app is
// suspended (backgrounded on iOS/Android). Safe to call from any thread.
double platform_game_time_ms();

// True while the OS has the app backgrounded; frames should be skipped.
bool platform_is_suspended();
