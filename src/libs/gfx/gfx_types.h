#pragma once

#include <bit>
#include <concepts>
#include <type_traits>

#include "../math/math_types.h"
#include "color_utils.h"

// Backend-neutral GPU texture handle. Layout matches raylib's Texture, so it
// converts implicitly to/from it (same pattern as Vec2 in math_types.h).
struct Texture {
    unsigned int id = 0;
    int width = 0, height = 0, mipmaps = 0, format = 0;

    constexpr Texture() = default;
    constexpr bool operator==(const Texture&) const = default;

    template<class T>
        requires(!std::same_as<T, Texture> && std::is_aggregate_v<T> &&
                 sizeof(T) == sizeof(unsigned int) + 4 * sizeof(int) &&
                 requires(const T& t) {
                     t.id;
                     t.width;
                     t.height;
                     t.mipmaps;
                     t.format;
                 })
    constexpr Texture(const T& t) : id(t.id), width(t.width), height(t.height), mipmaps(t.mipmaps), format(t.format) {}

    template<class T>
        requires(!std::same_as<T, Texture> && std::is_aggregate_v<T> &&
                 sizeof(T) == sizeof(unsigned int) + 4 * sizeof(int) &&
                 requires(T t) {
                     t.id;
                     t.width;
                     t.height;
                     t.mipmaps;
                     t.format;
                 })
    constexpr operator T() const {
        return T{id, width, height, mipmaps, format};
    }
};

// Same-layout bit conversion to/from a backend struct (evaluated lazily, so the
// class is complete when sizeof is checked).
template<class Self, class T>
concept bit_convertible = !std::same_as<Self, T> && std::is_trivially_copyable_v<T> && sizeof(T) == sizeof(Self);

struct Camera2D {
    Vec2 offset;
    Vec2 target;
    float rotation = 0.0f;
    float zoom = 1.0f;

    constexpr Camera2D() = default;
    constexpr Camera2D(Vec2 offset, Vec2 target, float rotation, float zoom)
        : offset(offset), target(target), rotation(rotation), zoom(zoom) {}
    template<class T>
        requires bit_convertible<Camera2D, T>
    constexpr Camera2D(const T& t) {
        *this = std::bit_cast<Camera2D>(t);
    }
    template<class T>
        requires bit_convertible<Camera2D, T>
    constexpr operator T() const {
        return std::bit_cast<T>(*this);
    }
};

namespace camera_projection {
    inline constexpr int PERSPECTIVE = 0;
    inline constexpr int ORTHOGRAPHIC = 1;
} // namespace camera_projection

struct Camera3D {
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fovy = 0.0f;
    int projection = camera_projection::PERSPECTIVE;

    constexpr Camera3D() = default;
    constexpr Camera3D(Vec3 position, Vec3 target, Vec3 up, float fovy, int projection)
        : position(position), target(target), up(up), fovy(fovy), projection(projection) {}
    template<class T>
        requires bit_convertible<Camera3D, T>
    constexpr Camera3D(const T& t) {
        *this = std::bit_cast<Camera3D>(t);
    }
    template<class T>
        requires bit_convertible<Camera3D, T>
    constexpr operator T() const {
        return std::bit_cast<T>(*this);
    }
};

struct Shader {
    unsigned int id = 0;
    int* locs = nullptr;

    constexpr Shader() = default;
    template<class T>
        requires bit_convertible<Shader, T>
    constexpr Shader(const T& t) {
        *this = std::bit_cast<Shader>(t);
    }
    template<class T>
        requires bit_convertible<Shader, T>
    constexpr operator T() const {
        return std::bit_cast<T>(*this);
    }
};

struct RenderTarget {
    unsigned int id = 0;
    Texture texture;
    Texture depth;

    constexpr RenderTarget() = default;
    template<class T>
        requires bit_convertible<RenderTarget, T>
    constexpr RenderTarget(const T& t) {
        *this = std::bit_cast<RenderTarget>(t);
    }
    template<class T>
        requires bit_convertible<RenderTarget, T>
    constexpr operator T() const {
        return std::bit_cast<T>(*this);
    }
};

// CPU-side and 3D data types. Plain aggregates whose layout matches raylib's, so the
// raylib backend reinterprets them directly (layout checked in gfx_raylib.cpp).
// Arrays inside them are malloc'd and owned by the matching unload_* call.

struct Image {
    void* data = nullptr;
    int width = 0, height = 0, mipmaps = 0, format = 0;
};

struct GlyphInfo {
    int value = 0;
    int offsetX = 0, offsetY = 0, advanceX = 0;
    Image image;
};

struct Font {
    int baseSize = 0, glyphCount = 0, glyphPadding = 0;
    Texture texture;
    Rect* recs = nullptr;
    GlyphInfo* glyphs = nullptr;
};

struct Transform {
    Vec3 translation;
    Vec4 rotation; // quaternion
    Vec3 scale;
};

struct BoneInfo {
    char name[32] = {};
    int parent = 0;
};

struct Mesh {
    int vertexCount = 0, triangleCount = 0;
    float* vertices = nullptr;
    float* texcoords = nullptr;
    float* texcoords2 = nullptr;
    float* normals = nullptr;
    float* tangents = nullptr;
    unsigned char* colors = nullptr;
    unsigned short* indices = nullptr;
    int boneCount = 0;
    unsigned char* boneIndices = nullptr;
    float* boneWeights = nullptr;
    float* animVertices = nullptr; // CPU-skinned positions
    float* animNormals = nullptr;
    unsigned int vaoId = 0; // backend buffer handles
    unsigned int* vboId = nullptr;
};

struct MaterialMap {
    Texture texture;
    Color color;
    float value = 0.0f;
};

struct Material {
    Shader shader;
    MaterialMap* maps = nullptr;
    float params[4] = {};
};

struct ModelSkeleton {
    unsigned int boneCount = 0;
    BoneInfo* bones = nullptr;
    Transform* bindPose = nullptr;
};

struct Model {
    Mat4 transform;
    int meshCount = 0, materialCount = 0;
    Mesh* meshes = nullptr;
    Material* materials = nullptr;
    int* meshMaterial = nullptr;
    ModelSkeleton skeleton;
    Transform* currentPose = nullptr;
    Mat4* boneMatrices = nullptr;
};

struct ModelAnimation {
    char name[32] = {};
    unsigned int boneCount = 0;
    int keyframeCount = 0;
    Transform** keyframePoses = nullptr; // [keyframe][bone]
};
