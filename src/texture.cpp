#include "texture.hpp"
#include "external/stb_image.h"

#include <iostream>

Texture loadTexture(const std::string& path) {
    Texture t;

    int channels;
    unsigned char* img = stbi_load(
        path.c_str(),
        &t.w,
        &t.h,
        &channels,
        1 // force grayscale
    );

    if (!img) {
        std::cerr << "Failed to load texture: " << path << "\n";
        return t;
    }

    t.data.resize(t.w * t.h);  // ✅ now valid

    for (int i = 0; i < t.w * t.h; i++) {
        t.data[i] = img[i];
    }

    stbi_image_free(img);
    return t;
}