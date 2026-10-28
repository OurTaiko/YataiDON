#pragma once

#include <filesystem>

void ps5_init_window(int refresh_hz);
int ps5_apply_output_refresh(int refresh_hz);
void ps5_report_boot_error(const char* message);
extern "C" unsigned char* ps5_load_file_data(const char* path, int* size);

#include "platform.h"
PlatformAudioDevice* ps5_open_audio(PlatformMixCallback callback, void* context, unsigned int frames);
void ps5_close_audio(PlatformAudioDevice* device);
