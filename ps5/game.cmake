set(game_root "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(vendor "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
set(PS5_FFMPEG_PREFIX "" CACHE PATH "PS5 FFmpeg prefix (include and lib)")
if(NOT EXISTS "${PS5_FFMPEG_PREFIX}/lib/libavformat.a")
  message(FATAL_ERROR "Set PS5_FFMPEG_PREFIX to a PS5-built FFmpeg installation")
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)
set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_MPEG OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_INSTALL OFF CACHE BOOL "" FORCE)
set(CPU_CLIPS_POSITIVE FALSE CACHE BOOL "" FORCE)
set(CPU_CLIPS_NEGATIVE FALSE CACHE BOOL "" FORCE)
add_subdirectory(${vendor}/libsamplerate ${CMAKE_BINARY_DIR}/deps/samplerate)
add_subdirectory(${vendor}/libsndfile ${CMAKE_BINARY_DIR}/deps/sndfile)
add_subdirectory(${vendor}/miniz ${CMAKE_BINARY_DIR}/deps/miniz)

file(GLOB lua_sources ${vendor}/lua/src/*.c)
list(FILTER lua_sources EXCLUDE REGEX "/lua(c)?\\.c$")
add_library(yataidon_lua STATIC ${lua_sources})
target_include_directories(yataidon_lua PUBLIC ${vendor}/lua/src)
foreach(header lua.h lauxlib.h lualib.h lua.hpp luaconf.h)
  file(CONFIGURE OUTPUT "${CMAKE_BINARY_DIR}/generated/lua/${header}"
       CONTENT "#include \"${vendor}/lua/src/${header}\"\n")
endforeach()

add_library(yataidon_sqlite STATIC ${vendor}/sqlite/sqlite3.c)
target_include_directories(yataidon_sqlite PUBLIC ${vendor}/sqlite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(yataidon_sqlite PRIVATE SQLITE_THREADSAFE=1 SQLITE_CUSTOM_INCLUDE=sqlite-config.h)

file(GLOB_RECURSE game_sources CONFIGURE_DEPENDS ${game_root}/src/*.cpp)
list(FILTER game_sources EXCLUDE REGEX "/src/platform/")
list(FILTER game_sources EXCLUDE REGEX "/src/libs/optional/")
add_library(yataidon_ps5_game STATIC ${game_sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/native-file-data.c
  ${game_root}/src/platform/platform_ps5.cpp ${game_root}/src/platform/platform_ps5_audio.cpp
  ${game_root}/src/platform/platform_ps5_video.cpp)
target_include_directories(yataidon_ps5_game PUBLIC
  ${game_root}/src ${CMAKE_BINARY_DIR}/generated
  ${vendor}/rapidjson/include ${vendor}/tomlplusplus/include
  ${vendor}/spdlog/include ${vendor}/sol2/include ${RAYLIB_SOURCE_DIR}/src/external
  ${PS5_FFMPEG_PREFIX}/include)
target_compile_definitions(yataidon_ps5_game PUBLIC
  YATAIDON_PLATFORM_PS5 YATAIDON_USE_SDL2 SDL_MAIN_HANDLED
  SOL_ALL_SAFETIES_ON=1)
target_compile_options(yataidon_ps5_game PRIVATE -O1 -ffunction-sections -fdata-sections)

# Link descriptions only: the native title imports the console's firmware
# decoder. These stub bodies are never packaged or executed.
add_library(yataidon_videodec_stub SHARED ${CMAKE_CURRENT_SOURCE_DIR}/videodec-link-stub.c)
set_target_properties(yataidon_videodec_stub PROPERTIES OUTPUT_NAME SceVideodec2 NO_SONAME TRUE)
target_link_options(yataidon_videodec_stub PRIVATE -nostdlib LINKER:-soname,libSceVideodec2.prx)
target_link_libraries(yataidon_ps5_game PUBLIC
  yataidon_ps5_raylib yataidon_lua yataidon_sqlite sndfile samplerate miniz SceAudioOut SceSysmodule yataidon_videodec_stub)
foreach(lib avformat avcodec swscale swresample avutil opus)
  target_link_libraries(yataidon_ps5_game PUBLIC ${PS5_FFMPEG_PREFIX}/lib/lib${lib}.a)
endforeach()
# Referencing main pulls the complete game from the archive for this link check.
file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/game-entry.c
     CONTENT "extern int main(int, char**); int (*yataidon_entry)(int, char**) = main;\n")
add_executable(YataiDON-PS5 ${CMAKE_BINARY_DIR}/game-entry.c)
set_property(TARGET YataiDON-PS5 PROPERTY LINKER_LANGUAGE CXX)
target_link_libraries(YataiDON-PS5 PRIVATE yataidon_ps5_game)
target_link_options(YataiDON-PS5 PRIVATE LINKER:--gc-sections)
set_target_properties(YataiDON-PS5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/native-archives.txt CONTENT
"$<TARGET_FILE:yataidon_ps5_game>\n$<TARGET_FILE:yataidon_ps5_raylib>\n$<TARGET_FILE:yataidon_lua>\n$<TARGET_FILE:yataidon_sqlite>\n$<TARGET_FILE:sndfile>\n$<TARGET_FILE:samplerate>\n$<TARGET_FILE:miniz>\n$<TARGET_FILE:SDL2::SDL2>\n")
