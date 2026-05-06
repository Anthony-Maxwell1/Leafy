#include "renderer.hpp"
#include "backend.hpp"
#include "texture_cache.hpp"
#include "app.hpp"
#include "input_system.hpp"

#include <cmath>
#include <unistd.h>
#include <cstring>

int main() {

    Framebuffer fb = initFramebuffer();

    Renderer r;
    r.init(fb);

    TextureCache texCache;

    Backlight bl;
    bl.init();

    // =========================
    // NEW INPUT SYSTEM
    // =========================
    TouchEngine touch;
    touch.init();

    const float frameStep = 0.2f; // ~5 FPS

    // Launch the app
    int appPid = launchApp("/mnt/us/test/app.lua", &r);

    if (appPid < 0) {
        return 1;
    }

    while (true) {

        texCache.beginFrame();
        r.beginFrame(255);

        // =========================
        // INPUT (NEW MODEL)
        // =========================
        auto events = touch.consumeFrame();

        // Forward touch events to Lua app
        App* app = getApp(appPid);
        if (app) {
            for (auto& e : events.press) {
                call_registered_callback(app->L, "began", {e.id, e.x, e.y});
            }

            for (auto& e : events.move) {
                call_registered_callback(app->L, "going", {e.id, e.x, e.y});
            }

            for (auto& e : events.release) {
                call_registered_callback(app->L, "ended", {e.id, e.x, e.y});
            }
        }

        // =========================
        // APP UPDATE
        // =========================
        frame(frameStep);

        // =========================
        // PRESENT
        // =========================
        r.present();
        texCache.endFrame();

        usleep(200000); // ~5 FPS logic loop
    }

    closeApp(appPid);

    return 0;
}