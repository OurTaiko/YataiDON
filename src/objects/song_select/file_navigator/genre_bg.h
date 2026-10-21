#pragma once

#include "box_folder.h"

class GenreBG {
public:
    GenreBG(const std::string& text_name, std::optional<ray::Color> color, TextureIndex texture_index, float distance);
    ~GenreBG() {
    }

    void update(double current_ms, FolderBox* box);
    void exit(float left_position, float right_position, FolderBox* center_box);
    void fade_out();
    void fade_in();
    bool is_finished();
    bool is_complete();

    int texture_frame() const { return (int)texture_index; }
    OutlinedText* name_text() const { return name.get(); }
    bool has_recolor() const { return shader_loaded; }
    void begin_recolor() {
        if (shader_loaded) {
            ray::BeginShaderMode(shader);
        }
    }
    void end_recolor() {
        if (shader_loaded) {
            ray::EndShaderMode();
        }
    }

    std::unique_ptr<MoveAnimation> stretch;
    std::unique_ptr<TextureResizeAnimation> scale;
    std::unique_ptr<MoveAnimation> move;
    std::unique_ptr<FadeAnimation> fade;
    std::unique_ptr<MoveAnimation> move_left;
    std::unique_ptr<MoveAnimation> move_right;

private:
    ray::Shader shader;
    std::shared_ptr<ray::Shader> shader_owner;
    bool shader_loaded = false;
    std::unique_ptr<OutlinedText> name;
    TextureIndex texture_index;
};
