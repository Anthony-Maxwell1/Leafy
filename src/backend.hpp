#pragma once

#include "core.hpp"
#include "raw_input.hpp"

#include <vector>
#include <string>
#include <unordered_map>

Framebuffer initFramebuffer();
void present(Framebuffer fb);

// =====================================
// BACKLIGHT
// =====================================

class Backlight {
public:
    bool init();

    int getMax() const;
    int getCurrent() const;

    bool set(int value);
    bool setPercent(float pct);

private:
    std::string path;
    int maxBrightness = -1;
};

// =====================================
// TOUCH ENGINE (NEW LAYER)
// =====================================

struct TouchPoint {
    int id;
    int x;
    int y;
    bool active;
    bool changed;
};

class TouchEngine {
public:
    void init();
    RawInputFrame poll();   // optional low-level access

    struct Events {
        std::vector<RawInputEvent> press;
        std::vector<RawInputEvent> move;
        std::vector<RawInputEvent> release;
    };

    Events consumeFrame();   // HIGH LEVEL API

private:
    void handleEvent(const RawInputEvent& e);
    void processFrame();

private:
    int fd = -1;

    std::unordered_map<int, TouchPoint> fingers;
    std::unordered_map<int, TouchPoint> previous;

    bool initialized = false;
};