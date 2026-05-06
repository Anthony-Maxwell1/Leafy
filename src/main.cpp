#include "renderer.hpp"
#include "backend.hpp"
#include "texture_cache.hpp"
#include "input_system.hpp"
#include "app.hpp"
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

    InputSystem input;

    const float frameStep = 0.2f; // ~5 FPS

    // Launch the app
    int appPid = launchApp("/test/app.lua", &r);

    if (appPid < 0) {
        // Failed to launch
        return 1;
    }

    while (true) {

        texCache.beginFrame();
        r.beginFrame(255);

        // Read raw input from device
        RawInputFrame rawFrame = readRawInput();
        
        // Process input through the system
        input.inputFrame(rawFrame);

        // Update app (calls Lua update function)
        frame(frameStep);

        // Present the framebuffer
        r.present();

        texCache.endFrame();

        usleep(200000); // ~5 FPS logic loop
    }

    closeApp(appPid);

    return 0;
}