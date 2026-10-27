// App lifecycle shared by iOS and Android: the OS can background the app at any
// time, so the game clock and audio stop until it returns.
#include "platform_mobile.h"

#include <chrono>
#include <mutex>

#include "../libs/audio/audio.h"
#include "../libs/input/input.h"
#include "platform.h"

namespace {
    std::mutex clock_mutex;
    bool suspended = false;
    double paused_at = 0;
    double paused_duration = 0;

    double monotonic_ms() {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void set_suspended(bool value) {
        std::lock_guard<std::mutex> lock(clock_mutex);
        if (value == suspended) {
            return;
        }
        if (value) {
            paused_at = monotonic_ms();
        } else {
            paused_duration += monotonic_ms() - paused_at;
        }
        suspended = value;
    }
} // namespace

double platform_game_time_ms() {
    std::lock_guard<std::mutex> lock(clock_mutex);
    return (suspended ? paused_at : monotonic_ms()) - paused_duration;
}

bool platform_is_suspended() {
    std::lock_guard<std::mutex> lock(clock_mutex);
    return suspended;
}

void mobile_install_lifecycle_handlers() {
    SDL_AddEventWatch(
        [](void*, SDL_Event* event) -> bool {
            if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND) {
                set_suspended(true);
                audio.set_suspended(true);
                clear_input_buffers();
            } else if (event->type == SDL_EVENT_DID_ENTER_FOREGROUND) {
                clear_input_buffers();
                set_suspended(false);
                audio.set_suspended(false);
            }
            return true;
        },
        nullptr);
}
