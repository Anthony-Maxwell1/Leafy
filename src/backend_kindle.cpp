#include "backend.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace fs = std::filesystem;

// =====================================
// FRAMEBUFFER
// =====================================

static int fbfd;
static uint8_t* fbmem = nullptr;

static int screenWidth = 600;
static int screenHeight = 800;

Framebuffer initFramebuffer() {
    fbfd = open("/dev/fb0", O_RDWR);

    fb_var_screeninfo v;
    fb_fix_screeninfo f;

    ioctl(fbfd, FBIOGET_VSCREENINFO, &v);
    ioctl(fbfd, FBIOGET_FSCREENINFO, &f);

    int w = v.xres;
    int h = v.yres;
    screenWidth = w;
    screenHeight = h;

    fbmem = (uint8_t*)mmap(nullptr, f.line_length * h,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED, fbfd, 0);

    return { fbmem, fbmem, w, h, (int)f.line_length };
}

void present(Framebuffer fb) {
    std::memcpy(fb.device, fb.data, fb.w * fb.h);
    system("eips '' >/dev/null 2>&1");
}

// =====================================
// BACKLIGHT
// =====================================

static std::string findBacklightPath() {
    std::string base = "/sys/class/backlight";

    for (auto &e : fs::directory_iterator(base)) {
        if (e.is_directory())
            return e.path().string();
    }

    return "";
}

bool Backlight::init() {
    path = findBacklightPath();
    if (path.empty()) return false;

    maxBrightness = getMax();
    return maxBrightness > 0;
}

int Backlight::getMax() const {
    std::ifstream f(path + "/max_brightness");
    int v = 0;
    f >> v;
    return v;
}

int Backlight::getCurrent() const {
    std::ifstream f(path + "/brightness");
    int v = 0;
    f >> v;
    return v;
}

bool Backlight::set(int value) {
    value = std::clamp(value, 0, maxBrightness);

    std::ofstream f(path + "/brightness");
    if (!f.is_open()) return false;

    f << value;
    return true;
}

bool Backlight::setPercent(float pct) {
    pct = std::clamp(pct, 0.0f, 1.0f);
    return set((int)(pct * maxBrightness));
}

// =====================================
// TOUCH ENGINE
// =====================================

// -------- init --------

void TouchEngine::init() {
    if (initialized) return;

    const char* paths[] = {
        "/dev/input/event2",
        "/dev/input/event1",
        "/dev/input/event0"
    };

    for (auto p : paths) {
        int f = open(p, O_RDONLY | O_NONBLOCK);
        if (f >= 0) {
            fd = f;
            std::cerr << "Touch opened: " << p << "\n";
            initialized = true;
            return;
        }
    }

    std::cerr << "Touch init failed\n";
    initialized = true;
}

// -------- low-level poll --------

RawInputFrame TouchEngine::poll() {
    RawInputFrame frame;

    if (fd < 0) return frame;

    input_event ev[64];
    ssize_t n = read(fd, ev, sizeof(ev));

    if (n <= 0) return frame;

    int count = n / sizeof(input_event);

    // Temp state across events
    static int currentX = 0, currentY = 0;
    static int currentId = 0;
    static bool currentDown = false;

    for (int i = 0; i < count; i++) {
        const auto& e = ev[i];

        if (e.type == EV_ABS) {
            // Capture coordinates
            if (e.code == ABS_MT_POSITION_X || e.code == 53) {
                currentX = e.value;
            }
            if (e.code == ABS_MT_POSITION_Y || e.code == 54) {
                currentY = e.value;
            }
            if (e.code == ABS_MT_TRACKING_ID || e.code == 57) {
                currentId = e.value;
            }
        } else if (e.type == EV_KEY) {
            // Capture down state
            if (e.code == 330) { // BTN_TOUCH
                currentDown = (e.value == 1);
            }
        } else if (e.type == EV_SYN && e.code == 0) { // SYN_REPORT
            // Emit complete event with all captured state
            RawInputEvent out;
            out.id = (currentId >= 0) ? 0 : -1;  // Valid touch if tracking ID >= 0
            out.x = currentX;
            out.y = currentY;
            out.down = currentDown && (currentId >= 0);
            
            frame.events.push_back(out);
        }
    }

    return frame;
}

// -------- event ingestion --------

void TouchEngine::handleEvent(const RawInputEvent& e) {
    int id = e.id;

    auto& f = fingers[id];

    if (e.down) {
        if (!f.active) {
            f.active = true;
            f.changed = true;
        }
    }

    if (e.x || e.y) {
        if (f.x != e.x || f.y != e.y) {
            f.x = e.x;
            f.y = e.y;
            f.changed = true;
        }
    }

    if (!e.down && f.active) {
        f.active = false;
        f.changed = true;
    }
}

// -------- frame processing --------

TouchEngine::Events TouchEngine::consumeFrame() {
    Events out;

    RawInputFrame frame = poll();

    for (auto& e : frame.events) {
        handleEvent(e);
    }

    // diff previous vs current
    for (auto& [id, f] : fingers) {

        auto& prev = previous[id];

        if (!prev.active && f.active) {
            out.press.push_back({id, f.x, f.y, true});
        }

        if (prev.active && !f.active) {
            out.release.push_back({id, f.x, f.y, false});
        }

        if (f.active &&
            (f.x != prev.x || f.y != prev.y)) {
            out.move.push_back({id, f.x, f.y, true});
        }

        prev = f;
        f.changed = false;
    }

    return out;
}