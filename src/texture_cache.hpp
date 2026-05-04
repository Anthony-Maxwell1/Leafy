#pragma once
#include "texture.hpp"
#include <unordered_map>
#include <string>

class TextureCache {
public:
    Texture& get(const std::string& path);
    void beginFrame();
    void endFrame();

private:
    std::unordered_map<std::string, Texture> cache;
};