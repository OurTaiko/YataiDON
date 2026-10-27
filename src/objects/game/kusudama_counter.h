#pragma once

#include "../../libs/file/script.h"

// Counting, animations and drawing live in game/kusudama_counter.lua.
class KusudamaCounter : public LuaScript {
private:
    bool fx = false;
    sol::protected_function fn_update, fn_miss, fn_draw, fn_popped, fn_missed, fn_finished;

public:
    KusudamaCounter(int total);

    void update(double current_ms, int count);
    void draw();
    bool is_finished();
    bool has_popped();

    // Skins with the effect layers keep the counter on screen while the ball falls after a failed kusudama.
    bool can_miss() const { return fx; }
    bool has_missed();
    void miss(double current_ms);
};
