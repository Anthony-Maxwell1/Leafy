#pragma once
#include "core.hpp"
#include "raw_input.hpp"
#include <vector>
#include <string>

Framebuffer initFramebuffer();
void present(Framebuffer fb);

RawInputFrame readRawInput();

class Backlight {
public:
    bool init();

    int getMax() const;
    int getCurrent() const;

    bool set(int value);          // raw value
    bool setPercent(float pct);   // 0.0 - 1.0

private:
    std::string path;
    int maxBrightness = -1;
};