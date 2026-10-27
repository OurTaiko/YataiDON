#include <assert.h>
#include <stdint.h>
static int supported = 1, fail_mode, calls, last_mode, waits;
int __real_sceVideoOutOpen(int32_t u, int32_t t, int32_t i, const void *p) { return 7; }
int __real_sceVideoOutClose(int32_t h) { assert(h == 7); return 0; }
int sceVideoOutIsOutputSupported(int32_t h, uint32_t m, const void *a, const void *b, const void *c) { return supported; }
int sceVideoOutConfigureOutput(int32_t h, uint32_t m, const void *a, const void *b, const void *c) {
    assert(h == 7); ++calls; last_mode = (int)m;
    return (int)m == fail_mode ? -10 : 0;
}
int sceVideoOutWaitVblank(int32_t h) { assert(h == 7); ++waits; return 0; }
#include "native-display.c"
int main(void) {
    assert(ps5_set_output_refresh(120) == -1);
    assert(__wrap_sceVideoOutOpen(0xff, 0, 0, 0) == 7);
    assert(ps5_set_output_refresh(90) == -1);
    assert(ps5_set_output_refresh(60) == 60 && calls == 0);
    supported = 0;
    assert(ps5_set_output_refresh(120) == 60 && calls == 0);
    supported = 1; fail_mode = 15;
    assert(ps5_set_output_refresh(120) == 60 && calls == 2 && last_mode == 1);
    assert(ps5_scanout_active_fps == 60);
    fail_mode = 0;
    assert(ps5_set_output_refresh(120) == 120 && last_mode == 15 && waits == 2);
    assert(ps5_scanout_active_fps == 120);
    assert(ps5_set_output_refresh(60) == 60 && last_mode == 1 && waits == 4);
    assert(ps5_set_output_refresh(120) == 120);
    assert(__wrap_sceVideoOutClose(7) == 0 && last_mode == 1);
    assert(ps5_set_output_refresh(60) == -1);
}
