#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK}"
: "${PS5_FFMPEG_PREFIX:?Set PS5_FFMPEG_PREFIX to PS5 FFmpeg include/lib prefix}"
gl_prefix=${PS5_OPENGL_PREFIX:-$root/ps5/vendor/opengl-sdk/ps5-opengl-sdk-1.0.2/sdk}
sdl_build=${PS5_SDL_BUILD:-$root/ps5/vendor/ps5-opengl/build/yataidon-sdl2}
# Installed SDL SDKs are relocatable; reuse one with the matching profile.
if [[ ! -f "$sdl_build/sdk/lib/libSDL2.a" ]] || \
   ! cmp -s "$root/ps5/sdl-display.h" "$sdl_build/yataidon-display.h"; then
  if [[ ! -f "$sdl_build/receipt.json" ]]; then
    python3 "$root/ps5/vendor/ps5-opengl/integration/SDL2/build.py" native \
      --sdl-source "$root/ps5/vendor/SDL" --sdk-prefix "$gl_prefix" \
      --out "$sdl_build" --payload-sdk "$PS5_PAYLOAD_SDK" \
      --compiler-wrapper "$PS5_PAYLOAD_SDK/bin/prospero-clang"
  fi
  cp "$root/ps5/sdl-display.h" "$sdl_build/integration/ps5g19_display.h"
  cmake --build "$sdl_build/cmake" --target install --parallel "${BUILD_JOBS:-4}"
  cp "$root/ps5/sdl-display.h" "$sdl_build/yataidon-display.h"
fi
cmake -S "$root/ps5" -B "$root/ps5/build-game" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PS5_PAYLOAD_SDK/toolchain/prospero.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DYATAIDON_PS5_FULL_GAME=ON \
  -DPS5_FFMPEG_PREFIX="$PS5_FFMPEG_PREFIX" \
  -DPS5OpenGL_DIR="$gl_prefix/lib/cmake/PS5OpenGL" \
  -DPS5OpenGLCore33_DIR="$gl_prefix/lib/cmake/PS5OpenGLCore33" \
  -DSDL2_DIR="$sdl_build/sdk/lib/cmake/SDL2"
cmake --build "$root/ps5/build-game" --target YataiDON-PS5 --parallel "${BUILD_JOBS:-4}"
