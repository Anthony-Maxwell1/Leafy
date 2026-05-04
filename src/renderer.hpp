#pragma once
#include "core.hpp"
#include "texture.hpp"

class Renderer {
public:
    void init(Framebuffer f);

    void beginFrame(uint8_t clear = 255);

    void drawPolygon(const Polygon& p, Color c);
    void drawCircle(const Circle& c, Color col);

    void present();
void drawTexture(const Texture& t, int x, int y);

private:
    Framebuffer fb;
    std::vector<uint8_t> prev;
};
