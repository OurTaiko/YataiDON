/* SPDX-License-Identifier: GPL-3.0-or-later
 * SDL owns input and the logical window; bgfx owns the Vulkan display surface.
 */
#include "SDL_internal.h"
#include "video/SDL_sysvideo.h"
#include "events/SDL_keyboard_c.h"
extern int yataidon_ps5_display_size(int*, int*);
static int video_init(_THIS) {
    int w = 0, h = 0;
    if (yataidon_ps5_display_size(&w, &h) != 0) return SDL_SetError("Vulkan display discovery failed");
    SDL_DisplayMode mode = {SDL_PIXELFORMAT_ABGR8888, w, h, 60, NULL};
    int index = SDL_AddBasicVideoDisplay(&mode);
    if (index < 0) return -1;
    return SDL_AddDisplayMode(&_this->displays[index], &mode) ? 0 : -1;
}
static int create_window(_THIS, SDL_Window* window) {
    (void)_this;
    SDL_SetKeyboardFocus(window);
    return 0;
}
static void video_quit(_THIS) { (void)_this; }
static void pump_events(_THIS) { (void)_this; }
static void delete_device(SDL_VideoDevice* device) { SDL_free(device); }
static SDL_VideoDevice* create_device(void) {
    SDL_VideoDevice* device = SDL_calloc(1, sizeof(*device));
    if (!device) { SDL_OutOfMemory(); return NULL; }
    device->VideoInit = video_init;
    device->VideoQuit = video_quit;
    device->PumpEvents = pump_events;
    device->CreateSDLWindow = create_window;
    device->free = delete_device;
    return device;
}
VideoBootStrap PS5_bootstrap = {"ps5", "PS5 native Vulkan display", create_device, NULL};
