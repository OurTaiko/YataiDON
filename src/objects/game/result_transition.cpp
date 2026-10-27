#include "result_transition.h"

#include <stdexcept>

#include "../../libs/gfx/texture.h"

ResultTransition::ResultTransition(PlayerNum player_num) {

    move = dynamic_cast<MoveAnimation*>(global_tex.get_animation(5));
    if (!move) {
        spdlog::error("ResultTransition: animation 5 is not a MoveAnimation");
        throw std::runtime_error("ResultTransition: animation 5 is not a MoveAnimation");
    }
    move->reset();

    if (!load("ResultTransition", "result_transition", static_cast<int>(player_num))) {
        return;
    }
    fn_start = lua_object["start"];
    fn_update = lua_object["update"];
    fn_draw = lua_object["draw"];
    fn_is_finished = lua_object["is_finished"];
}

void ResultTransition::start() {
    move->start();
    call(fn_start, "ResultTransition:start");
}

void ResultTransition::update(double current_ms) {
    move->update(current_ms);
    is_started = move->is_started;
    is_finished = move->is_finished;

    if (!is_started) {
        is_finished = false;
        return;
    }

    call(fn_update, "ResultTransition:update", current_ms);
    auto done = call_r<bool>(fn_is_finished, "ResultTransition:is_finished");
    if (done.has_value()) {
        is_finished = is_finished || done.value();
    }
}

void ResultTransition::draw() {
    call(fn_draw, "ResultTransition:draw");
}
