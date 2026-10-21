#define YATAIDON_USE_SDL2 1
#define SDL_GetNumAudioDevices test_device_count
#define SDL_malloc test_allocate
#include "../src/libs/sdl_compat.h"
#undef SDL_GetNumAudioDevices
#undef SDL_malloc
#include <cassert>
#include <cstdlib>

static int reported, allocations;
static bool refuse;
extern "C" int test_device_count(int capture) { assert(capture == 0); return reported; }
extern "C" void* test_allocate(size_t size) {
    ++allocations;
    return refuse ? nullptr : std::malloc(size);
}
int main() {
    int count = 99;
    reported = -1;
    assert(!SDL_GetAudioPlaybackDevices(&count) && count == 0 && allocations == 0);
    reported = 0; count = 99;
    assert(!SDL_GetAudioPlaybackDevices(&count) && count == 0 && allocations == 0);
    reported = 3; refuse = true;
    assert(!SDL_GetAudioPlaybackDevices(&count) && count == 0 && allocations == 1);
    refuse = false;
    auto* devices = SDL_GetAudioPlaybackDevices(&count);
    assert(devices && count == 3 && devices[0] == 1 && devices[2] == 3 && devices[3] == 0);
    std::free(devices);
    devices = SDL_GetAudioPlaybackDevices(nullptr);
    assert(devices && devices[3] == 0);
    std::free(devices);
}
