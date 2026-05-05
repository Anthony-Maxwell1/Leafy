#pragma once
#include <cstdint>
#include <vector>

struct Color {
    uint8_t v; // single-channel grayscale (0–255)
};

inline Color Gray(uint8_t v) {
    return Color{v};
}

struct Framebuffer {
    uint8_t* data;   // CPU-side buffer
    uint8_t* device; // /dev/fb0 mapped memory
    int w;
    int h;
    int stride;
};

struct Vec2 {
    float x, y;
};

struct Polygon {
    std::vector<Vec2> v;
};

struct Circle {
    Vec2 c;
    float r;
    bool filled;
};