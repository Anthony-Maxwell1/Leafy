#include "renderer.hpp"
#include "backend.hpp"
#include "texture_cache.hpp"
#include "app.hpp"
#include "input_system.hpp"
#include "ui.hpp"

#include <unistd.h>

int main() {

    Framebuffer fb = initFramebuffer();

    Renderer r;
    r.init(fb);

    TextureCache texCache;

    Backlight bl;
    bl.init();

    TouchEngine touch;
    touch.init();

    const float frameStep = 0.2f;

    int appPid = launchApp("/mnt/us/test/app.lua", &r);

    if (appPid < 0) return 1;

    App* app = getApp(appPid);

    while (true) {

        texCache.beginFrame();
        r.beginFrame(255);

        // =========================
        // INPUT (correct pipeline)
        // =========================
        auto events = touch.consumeFrame();

        if (app) {
            // Route to Lua callbacks
            for (auto& e : events.press) {
                call_registered_callback(app->L, "began", {
                    LuaArg(e.id),
                    LuaArg(e.x),
                    LuaArg(e.y)
                });
            }

            for (auto& e : events.move) {
                call_registered_callback(app->L, "going", {
                    LuaArg(e.id),
                    LuaArg(e.x),
                    LuaArg(e.y)
                });
            }

            for (auto& e : events.release) {
                call_registered_callback(app->L, "ended", {
                    LuaArg(e.id),
                    LuaArg(e.x),
                    LuaArg(e.y)
                });
            }
            
            // Route to UI system
            if (app->ui) {
                for (auto& e : events.press) {
                    app->ui->handleTouchBegin({e.id, e.x, e.y});
                }
                for (auto& e : events.move) {
                    app->ui->handleTouchGoing({e.id, e.x, e.y});
                }
                for (auto& e : events.release) {
                    app->ui->handleTouchEnd({e.id, e.x, e.y});
                }
            }
        }

        // =========================
        // LUA FRAME
        // =========================
        frame(frameStep);

        // =========================
        // PRESENT
        // =========================
        r.present();
        texCache.endFrame();

        usleep(200000); // ~5 FPS
    }

    closeApp(appPid);
    return 0;
}