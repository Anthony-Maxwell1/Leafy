#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include "raw_input.hpp"

struct InputEvent {
    int id;
    int x;
    int y;
};

class InputSystem {
public:
    using Callback = std::function<void(const InputEvent&)>;

    void onBegan(Callback cb);
    void onGoing(Callback cb);
    void onEnded(Callback cb);

    // 🔥 this is your frame entry point
    void inputFrame(const RawInputFrame& frame);

private:
    void emitBegan(const InputEvent& e);
    void emitGoing(const InputEvent& e);
    void emitEnded(const InputEvent& e);

    std::vector<Callback> beganSubs;
    std::vector<Callback> goingSubs;
    std::vector<Callback> endedSubs;

    std::unordered_map<int, InputEvent> prev;
    std::unordered_map<int, InputEvent> curr;
};