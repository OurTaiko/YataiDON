#include "platform_emscripten.h"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <thread>
#include <unistd.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

#include "../libs/gfx/sdl_compat.h"
#endif

namespace {

    void signal_handler(int signal) {
        if (signal == SIGINT) {
            _exit(0);
        }
    }

} // namespace

std::filesystem::path emscripten_get_working_directory() {
#ifdef __EMSCRIPTEN__
    spdlog::info("Emscripten: using virtual FS root as working directory");
    return std::filesystem::path("/");
#else
    return std::filesystem::current_path();
#endif
}

void emscripten_install_crash_handlers() {
#ifdef __EMSCRIPTEN__
    // Emscripten doesn't have traditional signal handling
    // Errors are caught by JS console/window.onerror
    // We just set up basic signal handlers for completeness
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
#else
    (void)signal_handler; // suppress unused warning
#endif
}

int emscripten_run_main_loop(void (*run_frame)()) {
#ifdef __EMSCRIPTEN__
    // emscripten_set_main_loop never returns
    emscripten_set_main_loop(run_frame, 0, 1);
    return 0;
#else
    (void)run_frame;
    return -1;
#endif
}

void emscripten_shutdown() {
#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
    emscripten_sync_filesystem();
#else
    // Nothing to do on non-Emscripten
#endif
}

void emscripten_sync_filesystem() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (typeof FS != = 'undefined' && FS.syncfs) {
            FS.syncfs(function(err) {
                if (err) {
                    console.error('FS sync failed:', err);
                } else {
                    console.log('FS synced successfully');
                }
            });
        }
    });
#else
    // Nothing to do on non-Emscripten
#endif
}
