cmake_minimum_required(VERSION 3.24)
project(YataiDONPS5Probe LANGUAGES C CXX)

if(NOT PS5)
  message(FATAL_ERROR "Use the public PS5 SDK toolchain/prospero.cmake")
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(RAYLIB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/raylib" CACHE PATH "raylib 6.2 sources")
find_package(PS5OpenGL CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
if(NOT TARGET SDL2::SDL2)
  message(FATAL_ERROR "A PS5 SDL2 package exporting SDL2::SDL2 is required")
endif()

# The upstream SDL CMake path also searches for desktop X11/OpenGL on UNIX.
# Compile the same raylib modules directly, with the PS5 SDK owning those links.
add_library(yataidon_ps5_raylib STATIC
  ${RAYLIB_SOURCE_DIR}/src/rcore.c
  ${RAYLIB_SOURCE_DIR}/src/rshapes.c
  ${RAYLIB_SOURCE_DIR}/src/rtextures.c
  ${RAYLIB_SOURCE_DIR}/src/rtext.c
  ${RAYLIB_SOURCE_DIR}/src/rmodels.c)
target_include_directories(yataidon_ps5_raylib PUBLIC ${RAYLIB_SOURCE_DIR}/src)
target_compile_definitions(yataidon_ps5_raylib PRIVATE
  PLATFORM_DESKTOP_SDL GRAPHICS_API_OPENGL_33 USING_SDL2_PROJECT
  SUPPORT_MODULE_RAUDIO=0 SUPPORT_SCREEN_CAPTURE=0 SUPPORT_FILEFORMAT_JPG=1)
target_link_libraries(yataidon_ps5_raylib PUBLIC SDL2::SDL2 PS5OpenGL::OpenGL)

# A payload-toolchain link probe, NOT a packaged native title.
add_executable(yataidon-ps5-probe smoke.cpp)
target_compile_definitions(yataidon-ps5-probe PRIVATE SDL_MAIN_HANDLED YATAIDON_USE_SDL2)
target_link_libraries(yataidon-ps5-probe PRIVATE yataidon_ps5_raylib)

option(YATAIDON_PS5_FULL_GAME "Build the full game for PS5" OFF)
if(YATAIDON_PS5_FULL_GAME)
  # YataiDON owns polling/presentation in its main loop; EndDrawing must not
  # swap a second time. The standalone probe retains automatic frame control.
  target_compile_definitions(yataidon_ps5_raylib PRIVATE SUPPORT_CUSTOM_FRAME_CONTROL=1)
  include(${CMAKE_CURRENT_SOURCE_DIR}/game.cmake)
endif()
