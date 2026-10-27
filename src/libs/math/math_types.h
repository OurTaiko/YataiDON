#pragma once

#include <concepts>
#include <type_traits>

struct Vec2 {
    float x = 0.0f, y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x, float y) : x(x), y(y) {}
    constexpr bool operator==(const Vec2&) const = default;

    template<class T>
        requires(!std::same_as<T, Vec2> && std::is_aggregate_v<T> && sizeof(T) == sizeof(float) * 2 &&
                 requires(const T& t) {
                     t.x;
                     t.y;
                 })
    constexpr Vec2(const T& v) : x(v.x), y(v.y) {}

    template<class T>
        requires(!std::same_as<T, Vec2> && std::is_aggregate_v<T> && sizeof(T) == sizeof(float) * 2 &&
                 requires(T t) {
                     t.x;
                     t.y;
                 })
    constexpr operator T() const {
        return T{x, y};
    }
};

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    constexpr bool operator==(const Vec3&) const = default;

    template<class T>
        requires(!std::same_as<T, Vec3> && std::is_aggregate_v<T> && sizeof(T) == sizeof(float) * 3 &&
                 requires(const T& t) {
                     t.x;
                     t.y;
                     t.z;
                 })
    constexpr Vec3(const T& v) : x(v.x), y(v.y), z(v.z) {}

    template<class T>
        requires(!std::same_as<T, Vec3> && std::is_aggregate_v<T> && sizeof(T) == sizeof(float) * 3 &&
                 requires(T t) {
                     t.x;
                     t.y;
                     t.z;
                 })
    constexpr operator T() const {
        return T{x, y, z};
    }
};

// Plain aggregate (raylib's Vector4 / Quaternion layout).
struct Vec4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
};

struct Rect {
    float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;

    constexpr Rect() = default;
    constexpr Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
    constexpr bool operator==(const Rect&) const = default;

    template<class T>
        requires(!std::same_as<T, Rect> && std::is_aggregate_v<T> && sizeof(T) == sizeof(float) * 4 &&
                 requires(const T& t) {
                     t.x;
                     t.y;
                     t.width;
                     t.height;
                 })
    constexpr Rect(const T& r) : x(r.x), y(r.y), width(r.width), height(r.height) {}

    template<class T>
        requires(!std::same_as<T, Rect> && std::is_aggregate_v<T> && sizeof(T) == sizeof(float) * 4 &&
                 requires(T t) {
                     t.x;
                     t.y;
                     t.width;
                     t.height;
                 })
    constexpr operator T() const {
        return T{x, y, width, height};
    }
};

// 4x4 matrix, column-major: mC R is column C, row R (raylib's field order).
struct Mat4 {
    float m0 = 0, m4 = 0, m8 = 0, m12 = 0;
    float m1 = 0, m5 = 0, m9 = 0, m13 = 0;
    float m2 = 0, m6 = 0, m10 = 0, m14 = 0;
    float m3 = 0, m7 = 0, m11 = 0, m15 = 0;

    constexpr Mat4() = default;

    template<class T>
        requires(!std::same_as<T, Mat4> && std::is_aggregate_v<T> && sizeof(T) == sizeof(float) * 16 &&
                 requires(const T& t) {
                     t.m0;
                     t.m15;
                 })
    constexpr Mat4(const T& m)
        : m0(m.m0), m4(m.m4), m8(m.m8), m12(m.m12), m1(m.m1), m5(m.m5), m9(m.m9), m13(m.m13), m2(m.m2), m6(m.m6),
          m10(m.m10), m14(m.m14), m3(m.m3), m7(m.m7), m11(m.m11), m15(m.m15) {}

    template<class T>
        requires(!std::same_as<T, Mat4> && std::is_aggregate_v<T> && sizeof(T) == sizeof(float) * 16 &&
                 requires(T t) {
                     t.m0;
                     t.m15;
                 })
    constexpr operator T() const {
        return T{m0, m4, m8, m12, m1, m5, m9, m13, m2, m6, m10, m14, m3, m7, m11, m15};
    }
};

// raymath-compatible helpers (same conventions as raylib's MatrixMultiply etc.).
constexpr Mat4 matrix_multiply(const Mat4& l, const Mat4& r) {
    Mat4 m;
    m.m0 = l.m0 * r.m0 + l.m1 * r.m4 + l.m2 * r.m8 + l.m3 * r.m12;
    m.m1 = l.m0 * r.m1 + l.m1 * r.m5 + l.m2 * r.m9 + l.m3 * r.m13;
    m.m2 = l.m0 * r.m2 + l.m1 * r.m6 + l.m2 * r.m10 + l.m3 * r.m14;
    m.m3 = l.m0 * r.m3 + l.m1 * r.m7 + l.m2 * r.m11 + l.m3 * r.m15;
    m.m4 = l.m4 * r.m0 + l.m5 * r.m4 + l.m6 * r.m8 + l.m7 * r.m12;
    m.m5 = l.m4 * r.m1 + l.m5 * r.m5 + l.m6 * r.m9 + l.m7 * r.m13;
    m.m6 = l.m4 * r.m2 + l.m5 * r.m6 + l.m6 * r.m10 + l.m7 * r.m14;
    m.m7 = l.m4 * r.m3 + l.m5 * r.m7 + l.m6 * r.m11 + l.m7 * r.m15;
    m.m8 = l.m8 * r.m0 + l.m9 * r.m4 + l.m10 * r.m8 + l.m11 * r.m12;
    m.m9 = l.m8 * r.m1 + l.m9 * r.m5 + l.m10 * r.m9 + l.m11 * r.m13;
    m.m10 = l.m8 * r.m2 + l.m9 * r.m6 + l.m10 * r.m10 + l.m11 * r.m14;
    m.m11 = l.m8 * r.m3 + l.m9 * r.m7 + l.m10 * r.m11 + l.m11 * r.m15;
    m.m12 = l.m12 * r.m0 + l.m13 * r.m4 + l.m14 * r.m8 + l.m15 * r.m12;
    m.m13 = l.m12 * r.m1 + l.m13 * r.m5 + l.m14 * r.m9 + l.m15 * r.m13;
    m.m14 = l.m12 * r.m2 + l.m13 * r.m6 + l.m14 * r.m10 + l.m15 * r.m14;
    m.m15 = l.m12 * r.m3 + l.m13 * r.m7 + l.m14 * r.m11 + l.m15 * r.m15;
    return m;
}

constexpr Mat4 matrix_translate(float x, float y, float z) {
    Mat4 m;
    m.m0 = m.m5 = m.m10 = m.m15 = 1.0f;
    m.m12 = x;
    m.m13 = y;
    m.m14 = z;
    return m;
}

constexpr Mat4 matrix_scale(float x, float y, float z) {
    Mat4 m;
    m.m0 = x;
    m.m5 = y;
    m.m10 = z;
    m.m15 = 1.0f;
    return m;
}

constexpr Vec3 vector3_transform(Vec3 v, const Mat4& m) {
    return {m.m0 * v.x + m.m4 * v.y + m.m8 * v.z + m.m12, m.m1 * v.x + m.m5 * v.y + m.m9 * v.z + m.m13,
            m.m2 * v.x + m.m6 * v.y + m.m10 * v.z + m.m14};
}
