#pragma once

#include "../../../libs/global_data.h"
#include "box_base.h"

class FolderBox : public BaseBox {
public:
    int tja_count{0};
    bool is_osu_folder = false;
    std::map<int, Crown> crown;
    std::map<int, Crown> crown_p2;
    bool entered = false;
    bool genre_voice_started = false;
    std::unique_ptr<FadeAnimation> enter_fade;
    std::optional<Texture> box_texture;

    FolderBox(const fs::path& path, const BoxDef& box_def,
              std::map<std::pair<std::string, std::string>, fs::path>& song_files);
    ~FolderBox() override;

    void load_text() override;
    void update(double current_time) override;

    void enter_box() override;
    void exit_box() override;

    void refresh_scores(const std::map<std::pair<std::string, std::string>, fs::path>& song_files);
    static void run_deferred_scans(std::atomic<bool>& abort_flag);
    bool scan_pending = false;
    static void invalidate_scan_cache();

    const char* lua_kind() const override { return "folder"; }
};
