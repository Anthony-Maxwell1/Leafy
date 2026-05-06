#include "renderer.hpp"
#include "raster.hpp"
#include <cstring>
#include <vector>
#include <cstdlib>
#include <ft2build.h>
#include FT_FREETYPE_H

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

bool Font::load(const std::string& path, int pixelSize) {
    if (FT_Init_FreeType(&ft)) return false;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) return false;

    FT_Set_Pixel_Sizes(face, 0, pixelSize);
    return true;
}

const Glyph& Font::get(char c) {

    if (cache.count(c)) return cache[c];

    FT_Load_Char(face, c, FT_LOAD_RENDER);
    FT_GlyphSlot g = face->glyph;

    Glyph glyph;

    glyph.width  = g->bitmap.width;
    glyph.height = g->bitmap.rows;

    glyph.bearingX = g->bitmap_left;
    glyph.bearingY = g->bitmap_top;

    glyph.advance = g->advance.x >> 6;

    // Create texture from bitmap
    glyph.tex.w  = glyph.width;
    glyph.tex.h = glyph.height;
    glyph.tex.data.resize(glyph.width * glyph.height);

    for (int i = 0; i < glyph.width * glyph.height; i++) {
        glyph.tex.data[i] = g->bitmap.buffer[i];
    }

    cache[c] = glyph;
    return cache[c];
}

void Renderer::drawText(Font& font, const std::string& text, int x, int y, uint8_t color) {

    int cursorX = x;

    for (char c : text) {

        const Glyph& g = font.get(c);

        int xpos = cursorX + g.bearingX;
        int ypos = y - g.bearingY;

        for (int yy = 0; yy < g.height; yy++) {
            for (int xx = 0; xx < g.width; xx++) {

                int px = xpos + xx;
                int py = ypos + yy;

                if (px < 0 || py < 0 || px >= fb.w || py >= fb.h)
                    continue;

                uint8_t value = g.tex.data[yy * g.width + xx];

                // blend (simple overwrite for now)`
                fb.data[py * fb.stride + px] = value;
            }
        }

        cursorX += g.advance;
    }
}

