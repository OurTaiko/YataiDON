#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>

#include "platform_ps5.h"

extern "C" {
int sceAudioOutInit();
int sceAudioOutOpen(int, int, int, uint32_t, uint32_t, uint32_t);
int sceAudioOutOutput(int, const void*);
int sceAudioOutClose(int);
}

struct PlatformAudioDevice {
    int port = -1;
    std::atomic<bool> running{true};
    std::thread worker;
};

PlatformAudioDevice* ps5_open_audio(PlatformMixCallback callback, void* context, unsigned int frames) {
    if (!callback || frames < 256 || frames > 2048 || frames % 256) {
        return nullptr;
    }
    (void)sceAudioOutInit();
    // Main output, 48 kHz, signed 16-bit stereo (public PS5 AudioOut ABI).
    int port = sceAudioOutOpen(0xff, 0, 0, frames, 48000, 1);
    if (port < 0) {
        spdlog::error("sceAudioOutOpen failed: {:#x}", static_cast<unsigned>(port));
        return nullptr;
    }
    auto* device = new PlatformAudioDevice;
    device->port = port;
    try {
        device->worker = std::thread([device, callback, context, frames] {
            std::vector<float> mixed(frames * 2);
            std::vector<int16_t> output(frames * 2);
            while (device->running.load()) {
                std::fill(mixed.begin(), mixed.end(), 0.0f);
                callback(mixed.data(), frames, context);
                for (size_t i = 0; i < mixed.size(); ++i) {
                    const float sample = std::isfinite(mixed[i]) ? std::clamp(mixed[i], -1.0f, 1.0f) : 0.0f;
                    output[i] = static_cast<int16_t>(sample * 32767.0f);
                }
                const int result = sceAudioOutOutput(device->port, output.data());
                if (result < 0) {
                    spdlog::error("PS5 audio output stopped: {:#x}", static_cast<unsigned>(result));
                    device->running.store(false);
                }
            }
        });
    } catch (...) {
        sceAudioOutClose(port);
        delete device;
        throw;
    }
    return device;
}

void ps5_close_audio(PlatformAudioDevice* device) {
    if (!device) {
        return;
    }
    device->running.store(false);
    if (device->worker.joinable()) {
        device->worker.join();
    }
    sceAudioOutOutput(device->port, nullptr);
    sceAudioOutClose(device->port);
    delete device;
}
