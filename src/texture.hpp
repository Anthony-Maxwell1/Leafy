#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Texture {
    int w = 0;
    int h = 0;
    std::vector<uint8_t> data;
    bool alive = false; // used this frame marker
};

Texture loadTexture(const std::string& path);