#include "renderer.hpp"
#include "backend.hpp"
#include "texture_cache.hpp"
#include "input_system.hpp"
#include <cmath>
#include <unistd.h>
#include <cstring>


// ------------------------------
// Gradient test square (unchanged)
// ------------------------------
void drawGradientSquare(Framebuffer& fb, int x0, int y0, int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {

            float fx = (float)x / size;
            float fy = (float)y / size;

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
// Grid squares renderer
// ------------------------------
void drawSquares(Framebuffer& fb, int count) {
    int size = 20;
    int cols = fb.stride / size;

    for (int i = 0; i < count; i++) {
        int x = (i % cols) * size;
        int y = (i / cols) * size;

        for (int yy = 0; yy < size; yy++) {
            for (int xx = 0; xx < size; xx++) {
                int px = x + xx;
                int py = y + yy;

                fb.data[py * fb.stride + px] = 0; // black
            }
        }
    }
}

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
// MAIN
// ------------------------------
int main() {

    Framebuffer fb = initFramebuffer();

    Renderer r;
    r.init(fb);

    TextureCache texCache;

    Backlight bl;
    bl.init();

    InputSystem input;

    // Track touch state for the display logic
    bool touching = false;
    float holdTime = 0.0f;
    float lastTick = 0.0f;
    int squares = 0;

    const float frameStep = 0.2f; // matches sleep

    // Setup input callbacks
    input.onBegan([&](const InputEvent& e) {
        touching = true;
        holdTime = 0.0f;
        lastTick = 0.0f;
        squares = 0;

        // black screen on touch
        for (int i = 0; i < fb.stride * 600; i++) {
            fb.data[i] = 0;
        }
    });

    input.onGoing([&](const InputEvent& e) {
        if (touching) {
            holdTime += frameStep;

            if (holdTime - lastTick >= 1.0f) {
                squares++;
                lastTick = holdTime;
            }

            drawSquares(fb, squares);
        }
    });

    input.onEnded([&](const InputEvent& e) {
        touching = false;

        // white screen on release
        for (int i = 0; i < fb.stride * 600; i++) {
            fb.data[i] = 255;
        }
    });

    while (true) {

        texCache.beginFrame();
        r.beginFrame(255);

        // Read raw input from device
        RawInputFrame rawFrame = readRawInput();
        
        // Process input through the system
        input.inputFrame(rawFrame);

        // Set backlight based on touch state
        if (touching) {
            bl.setPercent(1.0f);
        } else {
            bl.setPercent(0.7f);
        }

        // Present the framebuffer
        r.present();

        texCache.endFrame();

        usleep(200000); // ~5 FPS logic loop
    }

    return 0;
}