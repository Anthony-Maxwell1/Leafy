#include "raster.hpp"
#include <algorithm>
#include <cmath>

static inline uint8_t clampU8(int v) {
    return (v < 0) ? 0 : (v > 255 ? 255 : v);
}

static bool pointInPoly(const Polygon& p, float x, float y) {
    bool inside = false;
    int n = p.v.size();

    for (int i = 0, j = n - 1; i < n; j = i++) {
        auto a = p.v[i];
        auto b = p.v[j];

        bool intersect =
            ((a.y > y) != (b.y > y)) &&
            (x < (b.x - a.x) * (y - a.y) / (b.y - a.y + 0.0001f) + a.x);

        if (intersect) inside = !inside;
    }
    return inside;
}

void drawPolygon(Framebuffer& fb, const Polygon& p, Color c) {
    float minx = fb.w, miny = fb.h;
    float maxx = 0, maxy = 0;

    for (auto& v : p.v) {
        minx = std::min(minx, v.x);
        miny = std::min(miny, v.y);
        maxx = std::max(maxx, v.x);
        maxy = std::max(maxy, v.y);
    }

    uint8_t col = c.v;

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            if (pointInPoly(p, x, y)) {
                fb.data[y * fb.stride + x] = col;
            }
        }
    }
}

void drawCircle(Framebuffer& fb, const Circle& c, Color col) {
    uint8_t color = col.v;

    for (int y = c.c.y - c.r; y <= c.c.y + c.r; y++) {
        for (int x = c.c.x - c.r; x <= c.c.x + c.r; x++) {

            float dx = x - c.c.x;
            float dy = y - c.c.y;
            float d = dx * dx + dy * dy;

            if (c.filled) {
                if (d <= c.r * c.r) {
                    fb.data[y * fb.stride + x] = color;
                }
            } else {
                float r2 = c.r * c.r;
                float inner = (c.r - 1.5f) * (c.r - 1.5f);

                if (d <= r2 && d >= inner) {
                    fb.data[y * fb.stride + x] = color;
                }
            }
        }
    }
}