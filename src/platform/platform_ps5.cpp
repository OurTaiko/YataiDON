#include "platform_ps5.h"
#include "../libs/sdl_compat.h"
#include "../libs/ray.h"
#include <stdexcept>
#include <cstdio>
#include <ps5_opengl_display_modes.h>
#include "ps5_video.h"

extern "C" int sceKernelDebugOutText(int channel, const char* text);
extern "C" void av_log_set_level(int level);

void ps5_report_boot_error(const char* message) {
    char text[2048];
    std::snprintf(text, sizeof(text), "[yataidon-ps5] boot error: %s\n", message);
    std::fputs(text, stderr);
    std::fflush(stderr);
    sceKernelDebugOutText(0, text);
}

std::filesystem::path unix_get_executable_dir() {
    // Assets, configuration and databases live together in writable storage.
    const std::filesystem::path root = "/data/YataiDON";
    std::filesystem::create_directories(root);
    return root;
}

void unix_install_crash_handlers() {
    // logging.cpp installs the common C++ terminate handler.
}

void ps5_init_window(int refresh_hz) {
    ps5_preload_video_decoder();
    ray::SetLoadFileDataCallback(ps5_load_file_data);
    ray::SetLoadFileTextCallback(ps5_load_file_text);
    // Avoid hundreds of benign Vorbis timestamp warnings during scene loads.
    av_log_set_level(16); // AV_LOG_ERROR; decoding errors remain visible.
    // This must precede SDL's EGL initialization. The native backend checks
    // HDMI support and retains 60 Hz when the high-refresh request is rejected.
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglSetDisplayModePS5(display, 3840, 2160))
        throw std::runtime_error("PS5 4K render mode selection failed");
    if (!eglSetDisplayRefreshPS5(display, refresh_hz == 60 ? 60 : 120))
        std::fprintf(stderr, "[yataidon-ps5] refresh request failed: EGL=%04x\n", eglGetError());
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0)
        throw std::runtime_error(SDL_GetError());
    SDL_DisplayMode mode{};
    if (SDL_GetDesktopDisplayMode(0, &mode) < 0)
        throw std::runtime_error(SDL_GetError());
    ray::InitWindow(mode.w, mode.h, "YataiDON");
    if (!ray::IsWindowReady()) throw std::runtime_error("PS5 OpenGL window initialization failed");
    if (ray::GetRenderWidth() != 3840 || ray::GetRenderHeight() != 2160)
        throw std::runtime_error("PS5 renderer did not create the requested 3840x2160 drawable");
    if (SDL_GL_SetSwapInterval(1) < 0)
        throw std::runtime_error("PS5 VSync initialization failed");
    // Present once before querying: the accepted rate is final only after the
    // native video port has opened. This also avoids showing an uninitialized frame.
    ray::BeginDrawing();
    ray::ClearBackground(ray::BLACK);
    ray::EndDrawing();
    ray::SwapScreenBuffer();
}

// The native title supplies the video-port bridge; the payload link probe does not.
extern "C" int ps5_set_output_refresh(int hz) __attribute__((weak));
int ps5_apply_output_refresh(int refresh_hz) {
    // Finish all rendering before changing the video port's presentation rate.
    using Finish = void (*)();
    auto finish = reinterpret_cast<Finish>(eglGetProcAddress("glFinish"));
    if (finish) finish();
    return ps5_set_output_refresh ? ps5_set_output_refresh(refresh_hz) : -1;
}

int ps5_output_refresh_hz() {
    EGLint width = 0, height = 0, refresh = 60;
    if (!eglGetDisplayModePS5(eglGetDisplay(EGL_DEFAULT_DISPLAY), &width, &height, &refresh))
        refresh = 60;
    std::fprintf(stderr, "[yataidon-ps5] output=%dx%d refresh=%d Hz model-resolution=100%%\n",
                 width, height, refresh);
    return refresh == 120 ? 120 : 60;
}

