#include "platform_ios.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <mutex>

#include "../libs/gfx/sdl_compat.h"
#include "../libs/global_data.h"
#include "../libs/input/input.h"

// Platform code that needs no Objective-C. Apple's Foundation headers declare
// a global QuickDraw Rect that clashes with ours, so keep gfx.h out of the .mm.

namespace {
    std::mutex clock_mutex;
    bool suspended = false;
    double paused_at = 0;
    double paused_duration = 0;
    void (*g_ios_frame_callback)() = nullptr;

    double monotonic_ms() {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
} // namespace

double ios_game_time_ms() {
    std::lock_guard<std::mutex> lock(clock_mutex);
    return (suspended ? paused_at : monotonic_ms()) - paused_duration;
}

void ios_set_suspended(bool value) {
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

bool ios_is_suspended() {
    std::lock_guard<std::mutex> lock(clock_mutex);
    return suspended;
}

void ios_configure_window_flags(unsigned int& flags) {
    // UIKit owns the event loop. SDL_WaitEvent while minimized would prevent
    // UIKit from delivering the foreground event that wakes the game again.
    flags = window_flag::VSYNC | window_flag::ALWAYS_RUN;
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "playback");
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "0");
}

void ios_initialize_after_window() {
    ios_request_audio_buffer();

    // Register lifecycle event handler
    SDL_AddEventWatch(
        [](void* userdata, SDL_Event* event) -> bool {
            if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND) {
                ios_on_background();
            } else if (event->type == SDL_EVENT_DID_ENTER_FOREGROUND) {
                ios_on_foreground();
            }
            return true;
        },
        nullptr);
}

int ios_run_main_loop(void (*run_frame)()) {
    poll_touch_once();
    g_ios_frame_callback = run_frame;

    SDL_Window* win = main_window();
    if (!win) {
        return 1;
    }

    bool registered = SDL_SetiOSAnimationCallback(win, 1, [](void*) { g_ios_frame_callback(); }, nullptr);

    if (!registered) {
        return 1;
    }

    // UIKit owns the loop. No background input thread may outlive main().
    return 0;
}

void ios_shutdown() {
    // iOS cleanup - UIKit manages the main loop
}

void ios_set_keyboard_visible(bool visible) {
    SDL_Window* win = main_window();
    if (!win) {
        return;
    }

    if (visible) {
        SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "1");
        SDL_StartTextInput(win);
    } else {
        SDL_StopTextInput(win);
        SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "0");
    }
}

void ios_draw_touch_navigation_labels(float screen_width, float screen_height) {
    int font_size = std::max(16, static_cast<int>(screen_height * 0.04f));
    const char* labels[] = {"Back", "Pause"};
    for (int i = 0; i < 2; ++i) {
        float x = screen_width * (0.36f + i * 0.15f);
        draw_rectangle({x, screen_height * 0.025f, screen_width * 0.13f, screen_height * 0.10f},
                       fade(Color::BLACK, 0.6f));
        draw_text(labels[i], static_cast<int>(x + (screen_width * 0.13f - measure_text(labels[i], font_size)) / 2),
                  static_cast<int>(screen_height * 0.075f - font_size / 2), font_size, Color::WHITE);
    }
}

bool ios_is_suspended_state() {
    return ios_is_suspended();
}

void ios_on_background() {
    ios_set_suspended(true);
    ios_suspend_audio(true);
    clear_input_buffers();
}

void ios_on_foreground() {
    clear_input_buffers();
    ios_set_suspended(false);
    ios_suspend_audio(false);
}

extern SDL_AudioStream* g_ios_audio_stream; // Defined in audio.cpp

void ios_suspend_audio(bool suspended) {
    if (!g_ios_audio_stream) {
        return;
    }
    if (suspended) {
        SDL_PauseAudioStreamDevice(g_ios_audio_stream);
    } else {
        SDL_ResumeAudioStreamDevice(g_ios_audio_stream);
    }
}

int ios_handle_touch_navigation(int quadrant_vkey, float touch_x, float touch_y) {
    // The two top-center controls remain clear of landscape notches.
    bool navigation = touch_y >= 0.025f && touch_y <= 0.125f;
    if (navigation && touch_x >= 0.36f && touch_x <= 0.49f) {
        // Back button area
        if (global_data.config) {
            return global_data.config->keys.back_key;
        }
    } else if (navigation && touch_x >= 0.51f && touch_x <= 0.64f) {
        // Pause button area
        if (global_data.config) {
            return global_data.config->keys.pause_key;
        }
    }
    return quadrant_vkey;
}
