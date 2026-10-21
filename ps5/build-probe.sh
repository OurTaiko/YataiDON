#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public PS5 payload SDK}"
gl_root=${PS5_OPENGL_SOURCE:-$root/ps5/vendor/ps5-opengl}
gl_prefix=${PS5_OPENGL_PREFIX:-$root/ps5/vendor/opengl-sdk/ps5-opengl-sdk-1.0.2/sdk}
sdl_build=${PS5_SDL_BUILD:-$gl_root/build/yataidon-sdl2}
if [[ ! -f "$sdl_build/receipt.json" ]]; then
  if [[ -e "$sdl_build" ]]; then
    echo "Incomplete SDL build at $sdl_build; set PS5_SDL_BUILD to a new directory under $gl_root/build." >&2
    exit 1
  fi
  python3 "$gl_root/integration/SDL2/build.py" native \
    --sdl-source "$root/ps5/vendor/SDL" --sdk-prefix "$gl_prefix" \
    --out "$sdl_build" --payload-sdk "$PS5_PAYLOAD_SDK" \
    --compiler-wrapper "$PS5_PAYLOAD_SDK/bin/prospero-clang"
fi
cmake -S "$root/ps5" -B "$root/ps5/build" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PS5_PAYLOAD_SDK/toolchain/prospero.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DPS5OpenGL_DIR="$gl_prefix/lib/cmake/PS5OpenGL" \
  -DPS5OpenGLCore33_DIR="$gl_prefix/lib/cmake/PS5OpenGLCore33" \
  -DSDL2_DIR="$sdl_build/sdk/lib/cmake/SDL2"
cmake --build "$root/ps5/build" --parallel 8
