#pragma once

#include <filesystem>

void ps5_init_window(int refresh_hz);
int ps5_output_refresh_hz();
int ps5_apply_output_refresh(int refresh_hz);
void ps5_report_boot_error(const char* message);
extern "C" unsigned char* ps5_load_file_data(const char* path, int* size);
extern "C" char* ps5_load_file_text(const char* path);

struct PS5AudioDevice;
using PS5MixCallback = void (*)(float*, unsigned int, void*);
PS5AudioDevice* ps5_open_audio(PS5MixCallback callback, void* context, unsigned int frames);
void ps5_close_audio(PS5AudioDevice* device);
