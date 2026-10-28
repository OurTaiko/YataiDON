#include <sol/sol.hpp>

#include "../../libs/global_data.h"

class Background {
private:
    sol::table lua_object;
    sol::protected_function fn_update;
    sol::protected_function fn_handle_good;
    sol::protected_function fn_handle_ok;
    sol::protected_function fn_handle_bad;
    sol::protected_function fn_handle_drumroll;
    sol::protected_function fn_handle_balloon;
    sol::protected_function fn_handle_kusudama;
    sol::protected_function fn_handle_gauge;
    sol::protected_function fn_handle_song_end;
    sol::protected_function fn_handle_dan;
    sol::protected_function fn_handle_skip;
    sol::protected_function fn_handle_score;
    sol::protected_function fn_handle_ending;
    sol::protected_function fn_draw_ending;
    sol::protected_function fn_draw_back;
    sol::protected_function fn_draw_fore;
    sol::protected_function fn_draw_gauge;
    sol::protected_function fn_handle_balloon_end;
    sol::protected_function fn_handle_gogo;
    sol::protected_function fn_handle_miss_streak;

public:
    Background(PlayerNum player_num, float bpm, const std::string& scene_preset);
    Background(const Background&) = delete;
    Background& operator=(const Background&) = delete;
    ~Background();
    void update(double current_ms, float bpm);
    void handle_good(PlayerNum player_num);
    void handle_ok(PlayerNum player_num);
    void handle_bad(PlayerNum player_num);
    void handle_drumroll(PlayerNum player_num);
    // remaining: hits left after this one (-1 = not known, e.g. a kusudama)
    void handle_balloon(PlayerNum player_num, int remaining = -1);
    void handle_gauge(PlayerNum player_num, float progress, bool is_clear, bool is_rainbow, float clear_progress = 1.0f,
                      float flash = 0.0f);
    void handle_song_end(PlayerNum player_num, int good, int ok, int bad, int total_notes);
    void handle_kusudama(PlayerNum player_num);

    void handle_dan(PlayerNum player_num, const sol::table& state);
    bool wants_dan() const { return fn_handle_dan.valid(); }
    void handle_skip(PlayerNum player_num, const sol::table& state);
    bool wants_skip() const { return fn_handle_skip.valid(); }
    void handle_score(PlayerNum player_num, int score);
    void handle_ending(PlayerNum player_num, const std::string& kind);
    void draw_ending(PlayerNum player_num);
    bool wants_ending() const { return fn_handle_ending.valid() && fn_draw_ending.valid(); }
    bool wants_handle_ending() const { return fn_handle_ending.valid(); }
    // Optional: a balloon reached its end without bursting (a burst plays "balloon_pop")
    void handle_balloon_end(PlayerNum player_num);
    // GOGO time started (true) / ended (false) for that player
    void handle_gogo(PlayerNum player_num, bool on);
    void handle_miss_streak(PlayerNum player_num, bool on);
    bool wants_draw_ending() const { return fn_draw_ending.valid(); }
    void draw_back();
    void draw_fore();

    void draw_gauge(PlayerNum player_num);
    bool wants_gauge_draw() const { return fn_draw_gauge.valid(); }
};
