#pragma once

#include "../../libs/file/script.h"
#include "../../libs/global_data.h"
#include "../../libs/math/animation.h"

class ComboAnnounce : public LuaScript {
private:
    PlayerNum player_num;
    int combo;
    double wait;
    FadeAnimation* fade;
    bool audio_played{false};
    TextureObject* t_announce_bg = nullptr;
    TextureObject* t_announce_digit = nullptr;
    TextureObject* t_announce_text = nullptr;
    TextureObject* t_announce_number = nullptr;
    TextureObject* t_announce_add = nullptr;

    sol::protected_function fn_draw;

public:
    bool is_finished{false};

    ComboAnnounce(int combo, double current_ms, PlayerNum player_num);

    void update(double current_ms);
    void draw(float y);
};
