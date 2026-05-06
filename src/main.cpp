#include "renderer.hpp"
#include "backend.hpp"
#include "texture_cache.hpp"
#include "app.hpp"

#include <cmath>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <cstdio>

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
// Gradient square
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

            if (px >= 0 && py >= 0 && px < fb.stride && py < fb.h) {
                fb.data[py * fb.stride + px] = (uint8_t)v;
            }
        }
    }
}

// ------------------------------
// Persistent drawing
// ------------------------------
struct Point {
    int x;
    int y;
};

struct Stroke {
    std::vector<Point> points;
};

static std::vector<Stroke> strokes;
static Stroke currentStroke;
static bool drawing = false;

// ------------------------------
// Draw line helper
// ------------------------------
static void drawLine(Framebuffer& fb, Point a, Point b) {
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    int steps = std::max(std::abs(dx), std::abs(dy));

    if (steps == 0) return;

    for (int i = 0; i <= steps; i++) {
        int x = a.x + dx * i / steps;
        int y = a.y + dy * i / steps;

        if (x >= 0 && y >= 0 && x < fb.w && y < fb.h) {
            fb.data[y * fb.stride + x] = 0; // black ink
        }
    }
}

// ------------------------------
// Render strokes
// ------------------------------
static void renderStroke(Framebuffer& fb, const Stroke& s) {
    for (size_t i = 1; i < s.points.size(); i++) {
        drawLine(fb, s.points[i - 1], s.points[i]);
    }
}

// ------------------------------
// MAIN
// ------------------------------
int main() {

    Framebuffer fb = initFramebuffer();

    Renderer r;
    r.init(fb);

    TextureCache texCache;

    float angle = 0.0f;

    TouchEngine touch;
    touch.init();

    while (true) {

        texCache.beginFrame();
        r.beginFrame(255);

        // ------------------------------
        // Background scene
        // ------------------------------
        drawGradientSquare(fb, 20, 20, 120);

        r.drawPolygon(makeSquare(200, 30, 80), Gray(0));
        r.drawPolygon(makeTriangle(320, 30, 80), Gray(80));

        Circle c;
        c.c = {450, 70};
        c.r = 40;
        c.filled = true;
        r.drawCircle(c, Gray(0));

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

        Texture& png = texCache.get("/mnt/us/test.png");
        Texture& jpg = texCache.get("/mnt/us/test.jpg");
        Texture& bmp = texCache.get("/mnt/us/test.bmp");

        r.drawTexture(png, 50, 250);
        r.drawTexture(jpg, 200, 250);
        r.drawTexture(bmp, 350, 250);

        // ------------------------------
        // INPUT (single consume per frame)
        // ------------------------------
        auto events = touch.consumeFrame();

        for (auto& e : events.press) {
            drawing = true;
            currentStroke = {};
            currentStroke.points.push_back({e.x, e.y});
        }

        for (auto& e : events.move) {
            if (drawing) {
                currentStroke.points.push_back({e.x, e.y});
            }
        }

        for (auto& e : events.release) {
            if (drawing) {
                currentStroke.points.push_back({e.x, e.y});
                strokes.push_back(currentStroke);
                currentStroke = {};
                drawing = false;
            }
        }

        // ------------------------------
        // Render persistent strokes
        // ------------------------------
        for (auto& s : strokes) {
            renderStroke(fb, s);
        }

        // live stroke (while finger down)
        if (drawing) {
            renderStroke(fb, currentStroke);
        }

        // ------------------------------
        // Present
        // ------------------------------
        r.present();
        texCache.endFrame();

        usleep(33000); // ~30 FPS
    }

    return 0;
}