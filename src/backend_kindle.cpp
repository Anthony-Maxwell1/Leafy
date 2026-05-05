#include "backend.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <algorithm>

namespace fs = std::filesystem;

static int fbfd;
static uint8_t* fbmem = nullptr;

Framebuffer initFramebuffer() {
    fbfd = open("/dev/fb0", O_RDWR);

    fb_var_screeninfo v;
    fb_fix_screeninfo f;

    ioctl(fbfd, FBIOGET_VSCREENINFO, &v);
    ioctl(fbfd, FBIOGET_FSCREENINFO, &f);

    int w = v.xres;
    int h = v.yres;

    fbmem = (uint8_t*)mmap(nullptr, w * h,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED, fbfd, 0);

    return { fbmem, fbmem, w, h, (int)f.line_length };
}

void present(Framebuffer fb) {
    // full copy (safe baseline; optimize later)
    std::memcpy(fb.device, fb.data, fb.w * fb.h);

    // Kindle refresh trigger (temporary hack)
    system("eips ''");
}

static std::string findBacklightPath() {
    std::string base = "/sys/class/backlight";

    for (auto &entry : fs::directory_iterator(base)) {
        if (entry.is_directory()) {
            return entry.path().string();
        }
    }

    return "";
}

bool Backlight::init() {
    path = findBacklightPath();

    if (path.empty()) {
        std::cerr << "No backlight device found\n";
        return false;
    }

    maxBrightness = getMax();
    return maxBrightness > 0;
}

int Backlight::getMax() const {
    std::ifstream f(path + "/max_brightness");
    int value = 0;
    f >> value;
    return value;
}

int Backlight::getCurrent() const {
    std::ifstream f(path + "/brightness");
    int value = 0;
    f >> value;
    return value;
}

bool Backlight::set(int value) {
    if (maxBrightness > 0 && value > maxBrightness)
        value = maxBrightness;

    if (value < 0)
        value = 0;

    std::ofstream f(path + "/brightness");
    if (!f.is_open()) return false;

    f << value;
    return true;
}

bool Backlight::setPercent(float pct) {
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;

    int value = static_cast<int>(pct * getMax());
    return set(value);
}

// ========================================
// TOUCH INPUT SYSTEM - EVENT BASED
// ========================================

static int touchEventFd = -1;
static bool inputInitialized = false;
static int screenWidth = 600;
static int screenHeight = 800;

// Track current touch state
struct TouchState {
    int x;
    int y;
    bool active;
};

static std::unordered_map<int, TouchState> activeTouches;

static void initTouchInput() {
    if (inputInitialized) return;

    // Try common touch device paths
    const char* paths[] = {
        "/dev/input/event2",  // Most common on Kindle
        "/dev/input/event1",
        "/dev/input/event0"
    };

    for (const char* path : paths) {
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            std::cerr << "Opened touch device: " << path << std::endl;
            touchEventFd = fd;
            inputInitialized = true;
            return;
        }
    }

    std::cerr << "Warning: Could not open any touch device\n";
    inputInitialized = true;
}

RawInputFrame readRawInput() {
    if (!inputInitialized) {
        initTouchInput();
    }

    RawInputFrame frame;

    if (touchEventFd < 0) {
        return frame;  // No input device
    }

    struct input_event ev[64];
    ssize_t n = read(touchEventFd, ev, sizeof(ev));

    if (n <= 0) {
        // No new events - return current state
        for (const auto& [slot, state] : activeTouches) {
            RawInputEvent evt;
            evt.id = slot;
            evt.x = std::clamp(state.x, 0, screenWidth - 1);
            evt.y = std::clamp(state.y, 0, screenHeight - 1);
            evt.down = true;
            frame.events.push_back(evt);
        }
        return frame;
    }

    int numEvents = n / sizeof(struct input_event);

    // Current event being assembled
    static int currentSlot = 0;
    static int currentX = 0, currentY = 0;
    std::vector<int> releasedTouches;

    for (int i = 0; i < numEvents; i++) {
        const input_event& e = ev[i];

        if (e.type == EV_ABS) {
            if (e.code == ABS_MT_SLOT) {
                currentSlot = e.value;
            } else if (e.code == ABS_MT_POSITION_X) {
                currentX = e.value;
            } else if (e.code == ABS_MT_POSITION_Y) {
                currentY = e.value;
            } else if (e.code == ABS_MT_TRACKING_ID) {
                // TRACKING_ID >= 0: touch active
                // TRACKING_ID < 0: touch released
                if (e.value >= 0) {
                    // Touch is down - update or add to active touches
                    activeTouches[currentSlot] = {currentX, currentY, true};
                } else {
                    // Touch released - mark for removal after emitting
                    auto it = activeTouches.find(currentSlot);
                    if (it != activeTouches.end()) {
                        releasedTouches.push_back(currentSlot);
                    }
                }
            }
        }
    }

    // Build frame with all currently active touches (down=true)
    for (const auto& [slot, state] : activeTouches) {
        RawInputEvent evt;
        evt.id = slot;
        evt.x = std::clamp(state.x, 0, screenWidth - 1);
        evt.y = std::clamp(state.y, 0, screenHeight - 1);
        evt.down = true;
        frame.events.push_back(evt);
    }

    // Add released touches with down=false so InputSystem can detect the transition
    for (int slot : releasedTouches) {
        auto it = activeTouches.find(slot);
        if (it != activeTouches.end()) {
            RawInputEvent evt;
            evt.id = slot;
            evt.x = std::clamp(it->second.x, 0, screenWidth - 1);
            evt.y = std::clamp(it->second.y, 0, screenHeight - 1);
            evt.down = false;
            frame.events.push_back(evt);
            
            // Now remove it from active touches
            activeTouches.erase(it);
        }
    }

    return frame;
}