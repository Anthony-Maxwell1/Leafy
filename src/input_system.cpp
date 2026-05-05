#include "input_system.hpp"

void InputSystem::onBegan(Callback cb) { beganSubs.push_back(cb); }
void InputSystem::onGoing(Callback cb) { goingSubs.push_back(cb); }
void InputSystem::onEnded(Callback cb) { endedSubs.push_back(cb); }

void InputSystem::emitBegan(const InputEvent& e) {
    for (auto& cb : beganSubs) cb(e);
}

void InputSystem::emitGoing(const InputEvent& e) {
    for (auto& cb : goingSubs) cb(e);
}

void InputSystem::emitEnded(const InputEvent& e) {
    for (auto& cb : endedSubs) cb(e);
}

void InputSystem::inputFrame(const RawInputFrame& frame) {
    // move current → previous
    prev = curr;
    curr.clear();

    // build current state
    for (const auto& e : frame.events) {
        if (e.down) {
            curr[e.id] = { e.id, e.x, e.y };
        }
    }

    // Began + Going
    for (auto& [id, e] : curr) {
        if (!prev.count(id)) {
            emitBegan(e);
        } else {
            emitGoing(e);
        }
    }

    // Ended
    for (auto& [id, e] : prev) {
        if (!curr.count(id)) {
            emitEnded(e);
        }
    }
}