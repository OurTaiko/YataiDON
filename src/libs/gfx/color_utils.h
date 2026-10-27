#pragma once

#include <concepts>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <type_traits>

struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 0;

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0) : r(r), g(g), b(b), a(a) {};
    constexpr bool operator==(const Color&) const = default;

    static const Color WHITE;
    static const Color BLACK;
    static const Color BLANK;
    static const Color BLUE;
    static const Color GRAY;
    static const Color GREEN;
    static const Color LIGHTGRAY;
    static const Color LIME;
    static const Color ORANGE;
    static const Color RED;
    static const Color SKYBLUE;
    static const Color YELLOW;

    // Implicit interop with raylib's Color (and its WHITE/BLACK/... macros), without including ray.h.
    template<class T>
        requires(!std::same_as<T, Color> &&
                 requires(const T& t) {
                     t.r;
                     t.g;
                     t.b;
                     t.a;
                 })
    constexpr Color(const T& c) : r(c.r), g(c.g), b(c.b), a(c.a) {}

    template<class T>
        requires(!std::same_as<T, Color> && std::is_aggregate_v<T> && sizeof(T) == 4 &&
                 requires(T t) {
                     t.r;
                     t.g;
                     t.b;
                     t.a;
                 })
    constexpr operator T() const {
        return T{r, g, b, a};
    }
};

inline constexpr Color Color::WHITE{255, 255, 255, 255};
inline constexpr Color Color::BLACK{0, 0, 0, 255};
inline constexpr Color Color::BLANK{0, 0, 0, 0};
inline constexpr Color Color::BLUE{0, 121, 241, 255};
inline constexpr Color Color::GRAY{130, 130, 130, 255};
inline constexpr Color Color::GREEN{0, 228, 48, 255};
inline constexpr Color Color::LIGHTGRAY{200, 200, 200, 255};
inline constexpr Color Color::LIME{0, 158, 47, 255};
inline constexpr Color Color::ORANGE{255, 161, 0, 255};
inline constexpr Color Color::RED{230, 41, 55, 255};
inline constexpr Color Color::SKYBLUE{102, 191, 255, 255};
inline constexpr Color Color::YELLOW{253, 249, 0, 255};

inline Color parse_hex_color(const std::string& input) {
    std::string color = input;
    if (!color.empty() && color[0] == '#') {
        color = color.substr(1);
    }
    if (color.size() == 3) {
        color = std::string{color[0], color[0], color[1], color[1], color[2], color[2]};
    }
    if (color.size() != 6) {
        throw std::invalid_argument("Invalid hex color: " + input);
    }

    auto nibble = [&](char c) -> int {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        throw std::invalid_argument("Invalid hex color: " + input);
    };
    auto byte = [&](std::size_t i) -> unsigned char {
        return static_cast<unsigned char>(nibble(color[i]) * 16 + nibble(color[i + 1]));
    };

    return Color(byte(0), byte(2), byte(4), 255);
}

inline std::string color_to_hex(Color c) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
    return buf;
}
