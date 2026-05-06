#include "renderer.hpp"
#include "raster.hpp"
#include <cstring>
#include <vector>
#include <cstdlib>

void Renderer::init(Framebuffer f) {
    fb = f;

    prev.resize(fb.stride * fb.h);
    std::fill(prev.begin(), prev.end(), 255);
}

void Renderer::beginFrame(uint8_t clear) {
    std::memset(fb.data, clear, fb.w * fb.h);
}

void Renderer::drawPolygon(const Polygon& p, Color c) {
    ::drawPolygon(fb, p, c);
}

void Renderer::drawCircle(const Circle& c, Color col) {
    ::drawCircle(fb, c, col);
}

void Renderer::present() {
    int size = fb.stride * fb.h;

    for (int i = 0; i < size; i++) {
       uint8_t oldPx = prev[i];
uint8_t newPx = fb.data[i];

// Case 1: pixel is drawn this frame → always draw it
if (newPx != 255) {
    fb.data[i] = newPx;
    prev[i] = newPx;
}
// Case 2: pixel is NOT drawn this frame
else {
    // Only erase if it WAS previously non-white
    if (oldPx != 255) {
        fb.data[i] = 255;
        prev[i] = 255;
    }
    // else: do nothing (already white)
}
    }

    system("eips '' >/dev/null 2>&1");
}

void Renderer::drawTexture(const Texture& t, int x0, int y0) {
    for (int y = 0; y < t.h; y++) {
        for (int x = 0; x < t.w; x++) {

            int dx = x0 + x;
            int dy = y0 + y;

            if (dx < 0 || dy < 0 || dx >= fb.w || dy >= fb.h)
                continue;

            uint8_t src = t.data[y * t.w + x];

            fb.data[dy * fb.stride + dx] = src;
        }
    }
}