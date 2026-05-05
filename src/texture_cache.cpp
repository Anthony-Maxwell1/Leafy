#include "texture_cache.hpp"

Texture& TextureCache::get(const std::string& path) {
    auto it = cache.find(path);

    if (it != cache.end()) {
        it->second.alive = true;
        return it->second;
    }

    Texture t = loadTexture(path);
    cache[path] = t;
    cache[path].alive = true;

    return cache[path];
}

void TextureCache::beginFrame() {
    for (auto& [k, v] : cache) {
        v.alive = false;
    }
}

void TextureCache::endFrame() {
    for (auto it = cache.begin(); it != cache.end(); ) {
        if (!it->second.alive) {
            it = cache.erase(it);
        } else {
            ++it;
        }
    }
}