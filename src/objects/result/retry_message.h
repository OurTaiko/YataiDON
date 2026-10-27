#pragma once

#include "../../libs/file/script.h"

// "Revive!" message between a failed result and the retry mini-game (Scripts/result/result_retry_message.lua)
class ResultRetryMessage : public LuaScript {
    sol::protected_function fn_update, fn_draw, fn_is_finished;

public:
    ResultRetryMessage();
    void update(double current_ms);
    void draw();
    bool is_finished();
};
