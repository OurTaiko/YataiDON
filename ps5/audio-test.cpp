// Host contract test: native output conversion, failure cleanup and worker lifetime.
#define YATAIDON_PLATFORM_PS5
#include "../src/platform/platform_ps5.h"
#include "../src/libs/atomic_ref_compat.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <thread>

static std::atomic<int> blocks{0}, closes{0}, flushes{0};
static int open_result = 7;
extern "C" int sceAudioOutInit() { return 0; }
extern "C" int sceAudioOutOpen(int user, int type, int index, uint32_t frames, uint32_t rate, uint32_t format) {
    assert(user == 0xff && type == 0 && index == 0);
    assert(frames == 256 && rate == 48000 && format == 1);
    return open_result;
}
extern "C" int sceAudioOutOutput(int port, const void* data) {
    assert(port == 7);
    if (!data) { ++flushes; return 0; }
    const auto* samples = static_cast<const int16_t*>(data);
    assert(samples[0] == 32767 && samples[1] == -32767);
    assert(samples[2] == 0 && samples[3] == 16383);
    ++blocks;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return 0;
}
extern "C" int sceAudioOutClose(int port) { assert(port == 7); ++closes; return 0; }
static void mix(float* output, unsigned frames, void* context) {
    assert(frames == 256 && context == &blocks);
    output[0] = 2.0f;
    output[1] = -2.0f;
    output[2] = std::numeric_limits<float>::quiet_NaN();
    output[3] = 0.5f;
}
int main() {
    unsigned long long counter = 0;
    auto add = [&] { for (int i = 0; i < 10000; ++i) yd::atomic_ref(counter).fetch_add(1, std::memory_order_relaxed); };
    std::thread first(add), second(add);
    first.join(); second.join();
    assert(yd::atomic_ref(counter).load() == 20000);
    float gain = 0;
    yd::atomic_ref(gain).store(0.75f, std::memory_order_release);
    assert(yd::atomic_ref(gain).load(std::memory_order_acquire) == 0.75f);
    assert(!ps5_open_audio(mix, &blocks, 255));
    assert(!ps5_open_audio(nullptr, &blocks, 256));
    open_result = -1;
    assert(!ps5_open_audio(mix, &blocks, 256));
    assert(closes == 0);
    open_result = 7;
    auto* device = ps5_open_audio(mix, &blocks, 256);
    assert(device);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (blocks == 0 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    assert(blocks > 0);
    ps5_close_audio(device);
    assert(closes == 1 && flushes == 1);
    const int stopped = blocks;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    assert(blocks == stopped);
    ps5_close_audio(nullptr);
}
