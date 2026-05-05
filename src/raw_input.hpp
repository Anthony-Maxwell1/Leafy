#pragma once
#include <vector>

struct RawInputEvent {
    int id;
    int x;
    int y;
    bool down;
};

struct RawInputFrame {
    std::vector<RawInputEvent> events;
};