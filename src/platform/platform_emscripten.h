#pragma once

// Emscripten/WebAssembly-only platform services; see platform.h for the shared ones.

// Runs the frame function using emscripten_set_main_loop. Doesn't return.
int emscripten_run_main_loop(void (*run_frame)());
