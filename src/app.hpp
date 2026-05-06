#pragma once

#include <string>

// Forward declarations
class Renderer;
struct lua_State;

// App context for Lua scripts
struct AppContext {
    std::string id;
    Renderer* renderer;
};

// App instance
struct App {
    int pid;
    lua_State* L;
    AppContext ctx;
};

// ========================
// App Management API
// ========================

// Launch a Lua app from a file path
// Returns app PID on success, -1 on failure
int launchApp(const std::string& path, Renderer* renderer);

// Close an app by PID
void closeApp(int pid);

// Update all running apps by dt (in seconds)
void frame(float dt);

// Get app by PID (for direct access if needed)
App* getApp(int pid);
