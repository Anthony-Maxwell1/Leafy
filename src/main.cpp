#include "renderer.hpp"
#include "backend.hpp"
#include "texture_cache.hpp"
#include <cmath>
#include <unistd.h>

// ------------------------------
// Shapes
// ------------------------------
Polygon makeSquare(float x, float y, float s) {
    Polygon p;
    p.v = {
        {x, y},
        {x + s, y},
        {x + s, y + s},
        {x, y + s}
    };
    return p;
}

Polygon makeTriangle(float x, float y, float s) {
    Polygon p;
    p.v = {
        {x, y + s},
        {x + s, y + s},
        {x + s * 0.5f, y}
    };
    return p;
}

// ------------------------------
// Gradient test square
// TL = black, TR = white, BL = white, BR = black
// ------------------------------
void drawGradientSquare(Framebuffer& fb, int x0, int y0, int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {

            float fx = (float)x / size;
            float fy = (float)y / size;

            // corner blending
            float tl = (1.0f - fx) * (1.0f - fy);
            float tr = fx * (1.0f - fy);
            float bl = (1.0f - fx) * fy;
            float br = fx * fy;

            float v = (tl * 0.0f + tr * 255.0f + bl * 255.0f + br * 0.0f);

            int px = x0 + x;
            int py = y0 + y;

            fb.data[py * fb.stride + px] = (uint8_t)v;
        }
    }
}

// ------------------------------
// Main test scene
// ------------------------------
int main() {
    Framebuffer fb = initFramebuffer();

    Renderer r;
    r.init(fb);

    TextureCache texCache;

    float angle = 0.0f;

    while (true) {

        // ------------------------------
        // Cache lifecycle
        // ------------------------------
        texCache.beginFrame();

        // ------------------------------
        // Frame start
        // ------------------------------
        r.beginFrame(255);

        // ------------------------------
        // 1. Gradient square (top-left)
        // ------------------------------
        drawGradientSquare(fb, 20, 20, 120);

        // ------------------------------
        // 2. Static shapes
        // ------------------------------
        r.drawPolygon(makeSquare(200, 30, 80), Gray(0));      // black square
        r.drawPolygon(makeTriangle(320, 30, 80), Gray(80));   // gray triangle

        Circle c;
        c.c = {450, 70};
        c.r = 40;
        c.filled = true;

        r.drawCircle(c, Gray(0));

        // ------------------------------
        // 3. Spinning square
        // ------------------------------
        float cx = 300, cy = 200, s = 40;

        float cs = cos(angle);
        float sn = sin(angle);

        Polygon spin;
        spin.v = {
            {cx + (-s * cs - (-s) * sn), cy + (-s * sn + (-s) * cs)},
            {cx + ( s * cs - (-s) * sn), cy + ( s * sn + (-s) * cs)},
            {cx + ( s * cs - ( s) * sn), cy + ( s * sn + ( s) * cs)},
            {cx + (-s * cs - ( s) * sn), cy + (-s * sn + ( s) * cs)}
        };

        r.drawPolygon(spin, Gray(0));

        angle += 0.05f;

        // ------------------------------
        // 4. Textures
        // ------------------------------
        Texture& png = texCache.get("/mnt/us/test.png");
        Texture& jpg = texCache.get("/mnt/us/test.jpg");
        Texture& bmp = texCache.get("/mnt/us/test.bmp");

        r.drawTexture(png, 50, 250);
        r.drawTexture(jpg, 200, 250);
        r.drawTexture(bmp, 350, 250);

        // ------------------------------
        // Present
        // ------------------------------
        r.present();

        texCache.endFrame();

        usleep(33000); // ~30 FPS
    }

    return 0;
}