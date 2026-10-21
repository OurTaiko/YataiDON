#pragma once
#include "asset_cache.h"
#include "ray.h"

inline AssetCache<ray::Shader>& color_shader_cache() {
    static AssetCache<ray::Shader> cache(64);
    return cache;
}

inline std::shared_ptr<ray::Shader> load_color_shader(ray::Color color) {
    const std::string key = std::to_string((unsigned(color.r) << 16) | (unsigned(color.g) << 8) | color.b);
#ifdef YATAIDON_PLATFORM_PS5
    if (auto shader = color_shader_cache().get(key)) return shader;
#endif
    auto shader = std::shared_ptr<ray::Shader>(
        new ray::Shader(load_shader("shader/dummy.vs", "shader/colortransform.fs")),
        [](ray::Shader* shader) { ray::UnloadShader(*shader); delete shader; });
    float source[3] = {142 / 255.0f, 212 / 255.0f, 30 / 255.0f};
    float target[3] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f};
    ray::SetShaderValue(*shader, ray::GetShaderLocation(*shader, "sourceColor"), source, ray::SHADER_UNIFORM_VEC3);
    ray::SetShaderValue(*shader, ray::GetShaderLocation(*shader, "targetColor"), target, ray::SHADER_UNIFORM_VEC3);
#ifdef YATAIDON_PLATFORM_PS5
    color_shader_cache().put(key, shader, 1);
#endif
    return shader;
}
