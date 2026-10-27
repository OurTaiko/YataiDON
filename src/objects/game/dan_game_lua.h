#pragma once

#include "../../libs/file/script.h"

struct DanExamInfo;
struct DanInfoCache;

class DanGameLua : public LuaScript {
    sol::protected_function fn_draw;

public:
    DanGameLua() = default;
    explicit DanGameLua(const std::string& title);

    void draw(const DanInfoCache& cache, const std::vector<bool>& exam_failed, int song_index, int dan_color,
              int dan_rank, double frame_ms);
};
