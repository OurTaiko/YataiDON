#include "platform_ps5.h"
#include "platform.h"
#include "../libs/gfx/sdl_compat.h"
#include "../libs/gfx.h"
#include <stdexcept>
#include <cstdio>
#include <cstdlib>

#include "ps5_video.h"

extern "C" int sceSystemServiceHideSplashScreen();
extern "C" int sceKernelDebugOutText(int channel, const char* text);
extern "C" void av_log_set_level(int level);

void ps5_report_boot_error(const char* message) {
    char text[2048];
    std::snprintf(text, sizeof(text), "[yataidon-ps5] boot error: %s\n", message);
    std::fputs(text, stderr);
    std::fflush(stderr);
    sceKernelDebugOutText(0, text);
}

std::filesystem::path platform_get_working_directory() {
    // Assets, configuration and databases live together in writable storage.
    const std::filesystem::path root = "/data/YataiDON";
    std::filesystem::create_directories(root);
    return root;
}

void platform_install_crash_handlers() {
    // logging.cpp installs the common C++ terminate handler.
}

extern "C" int yataidon_ps5_display_size(int* width, int* height);
extern "C" int ps5_set_output_refresh(int hz) __attribute__((weak));
static int output_refresh = 60;

void ps5_init_window(int refresh_hz) {
    // Set this before display discovery creates the first Vulkan instance.
    // RADV otherwise defaults to the installed application's /app0 directory.
    constexpr const char* shader_cache = "/data/YataiDON/radv-shader-cache";
    std::filesystem::create_directories(shader_cache);
    if (setenv("MESA_SHADER_CACHE_DIR", shader_cache, 1) != 0)
        throw std::runtime_error("PS5 RADV shader cache configuration failed");
    std::fprintf(stderr, "[yataidon-ps5] RADV shader cache: %s\n", shader_cache);

    ps5_preload_video_decoder();
    av_log_set_level(16);
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0)
        throw std::runtime_error(SDL_GetError());
    int width = 0, height = 0;
    if (yataidon_ps5_display_size(&width, &height) != 0 || width <= 0 || height <= 0)
        throw std::runtime_error("PS5 Vulkan display discovery failed");
    init_window(width, height, "YataiDON");
    begin_drawing();
    clear_background(Color::BLACK);
    end_drawing();
    swap_screen_buffer();
    sceSystemServiceHideSplashScreen();
    const int accepted = ps5_apply_output_refresh(refresh_hz);
    if (accepted < 0) std::fprintf(stderr, "[yataidon-ps5] Vulkan refresh request failed; retaining 60 Hz\n");
}

int ps5_apply_output_refresh(int refresh_hz) {
    // The native bridge owns VideoOut and validates HDMI 120 Hz capability.
    const int accepted = ps5_set_output_refresh ? ps5_set_output_refresh(refresh_hz) : -1;
    if (accepted > 0) output_refresh = accepted;
    return accepted;
}

int ps5_output_refresh_hz() { return output_refresh; }

void platform_close_window() {
    close_window();
}

bool platform_retains_assets() {
    return true;
}
