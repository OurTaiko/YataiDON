#pragma once

#if defined(YATAIDON_USE_SDL2) || (defined(__has_include) && !__has_include(<SDL3/SDL_main.h>))
#if defined(__has_include) && __has_include(<SDL2/SDL_main.h>)
#include <SDL2/SDL_main.h>
#else
#include <SDL_main.h>
#endif
#else
#include <SDL3/SDL_main.h>
#endif
