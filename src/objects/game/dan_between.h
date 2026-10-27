#pragma once

#include <string>

#include "../../libs/audio/audio.h"
#include "../../libs/file/script.h"

// The song intro between dan courses: doors close over the lane, the song title shows, the doors open
// on the next chart. This keeps the timing and the door sounds; the skin's DanBetween
// (game/dan_between.lua) draws the frame for the time elapsed so far.
class DanBetween : public LuaScript {
public:
    static constexpr double FPS = 60.0;
    static constexpr double F_START = 2.0;
    static constexpr double F_OPEN = 201.0;
    static constexpr double F_END = 366.0;

    static constexpr double SONG_OPEN_MS = (F_OPEN - F_START) * 1000.0 / FPS;
    static constexpr double TOTAL_MS = (F_END - F_START) * 1000.0 / FPS;

    void start(double now, const std::string& title, const std::string& subtitle, bool show_subtitle);
    void stop();

    void update(double now);
    void draw(float lane_y);

    bool is_active() const { return active; }
    bool song_may_start() const { return !active || elapsed >= SONG_OPEN_MS; }

private:
    bool active = false;
    bool open_fired = false;
    double start_ms = 0.0;
    double elapsed = 0.0;
    sol::protected_function fn_draw;
};

inline void DanBetween::start(double now, const std::string& title, const std::string& subtitle, bool show_subtitle) {
    active = true;
    open_fired = false;
    start_ms = now;
    elapsed = 0.0;

    fn_draw = {};
    if (load("DanBetween", "dan_between")) {
        fn_draw = lua_object["draw"];
        sol::protected_function fn_start = lua_object["start"];
        call(fn_start, "DanBetween:start", title, subtitle, show_subtitle);
    }

    audio.play_sound("fusuma_close", VolumePreset::SOUND);
}

inline void DanBetween::stop() {
    active = false;
    open_fired = false;
    elapsed = 0.0;
    fn_draw = {};
    lua_object = sol::table(); // never outlive the Lua state across a skin reload
}

inline void DanBetween::update(double now) {
    if (!active) {
        return;
    }
    elapsed = now - start_ms;
    if (!open_fired && elapsed >= SONG_OPEN_MS) {
        open_fired = true;
        audio.play_sound("fusuma_open", VolumePreset::SOUND);
    }
    if (elapsed >= TOTAL_MS) {
        stop();
    }
}

inline void DanBetween::draw(float lane_y) {
    if (!active) {
        return;
    }
    call(fn_draw, "DanBetween:draw", lane_y, elapsed);
}
