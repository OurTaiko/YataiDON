#pragma once
#include <algorithm>
#include <cmath>

#include "../global_data.h"

inline Camera2D compute_camera2d(int virtual_width, int virtual_height) {
    int vw = std::max(virtual_width, 1);
    int vh = std::max(virtual_height, 1);
    int sw = std::max(get_render_width(), 1);
    int sh = std::max(get_render_height(), 1);

    float scale = std::min(static_cast<float>(sw) / vw, static_cast<float>(sh) / vh);
    float base_ox = (sw - vw * scale) * 0.5f;
    float base_oy = (sh - vh * scale) * 0.5f;
    float eff_zoom = scale * global_data.camera.zoom;
    float zoom_ox = (vw * scale * (global_data.camera.zoom - 1.0f)) * 0.5f;
    float zoom_oy = (vh * scale * (global_data.camera.zoom - 1.0f)) * 0.5f;

    float h_scale = global_data.camera.h_scale;
    float v_scale = global_data.camera.v_scale;
    float hscale_ox = (vw * scale * (h_scale - 1.0f)) * 0.5f;
    float vscale_oy = (vh * scale * (v_scale - 1.0f)) * 0.5f;
    float offset_x = base_ox - zoom_ox - hscale_ox + (global_data.camera.offset.x * scale);
    float offset_y = base_oy - zoom_oy - vscale_oy + (global_data.camera.offset.y * scale);

    Vec2 target = {vw * 0.5f, vh * 0.5f};
    offset_x += eff_zoom * target.x;
    offset_y += eff_zoom * target.y;

    return {{offset_x, offset_y}, target, global_data.camera.rotation, eff_zoom};
}

inline Camera3D camera2d_to_3d(Camera2D cam) {
    float sw = get_render_width();
    float sh = get_render_height();
    float zoom = std::max(cam.zoom, 0.0001f); // guard divide-by-zero/negative zoom
    float rot = cam.rotation * deg2rad;
    float cx = (sw * 0.5f - cam.offset.x) / zoom + cam.target.x;
    float cy = (sh * 0.5f - cam.offset.y) / zoom + cam.target.y;
    return {{cx, cy, -100.0f},
            {cx, cy, 0.0f},
            {std::sin(rot), -std::cos(rot), 0.0f},
            sh / zoom,
            camera_projection::ORTHOGRAPHIC};
}
