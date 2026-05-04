#include "stb_image.h"

#include "texture.hpp"
#include <iostream>

Texture loadTexture(const std::string& path) {
    Texture t;

    int channels;
    unsigned char* img = stbi_load(path.c_str(), &t.w, &t.h, &channels, 1);

    if (!img) {
        std::cerr << "Failed to load texture: " << path << "\n";
        return t;
    }

    t.data.resize(t.w * t.h);

    for (int i = 0; i < t.w * t.h; i++) {
        t.data[i] = img[i]; // already grayscale (forced via 1 channel)
    }

    stbi_image_free(img);

    t.alive = true;
    return t;
}