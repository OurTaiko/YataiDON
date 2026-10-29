// platform.h hooks that only some platforms (currently PS5) override; every other build uses these.
#include "../libs/file/config.h"
#include "../libs/gfx.h"
#include "../libs/input/ime_ui.h"
#include "platform.h"

int platform_run_main(int (*game_main)(int, char**), int argc, char** argv) {
    return game_main(argc, argv);
}

unsigned platform_window_flags(unsigned flags) {
    return flags | window_flag::RESIZABLE;
}

float platform_supersample() {
    return 2.0f;
}

void platform_init_window() {
    ime_ui_set_hint();
    init_window(1280, 720, "YataiDON");
    ime_ui_install();
}

int platform_apply_target_fps(int fps) {
    return fps;
}

PlatformAudioDevice* platform_open_audio(PlatformMixCallback, void*, double&, unsigned long&) {
    return nullptr;
}

void platform_close_audio(PlatformAudioDevice*) {}

std::unique_ptr<PlatformVideoDecoder> platform_open_video_decoder(const std::filesystem::path&,
                                                                  const std::atomic<bool>&) {
    return nullptr;
}

bool platform_take_text_replacement() {
    return false;
}

bool platform_read_file(const char*, std::vector<unsigned char>&) {
    return false;
}

bool platform_preload_song_previews() {
    return false;
}

bool platform_offers_setting(std::string_view, std::string_view) {
    return true;
}

void platform_load_config(Config&, const toml::table&) {}

void platform_save_config(const Config&, toml::table&) {}
