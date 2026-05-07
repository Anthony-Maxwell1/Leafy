#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declaration of lua_State
extern "C" {
    struct lua_State;
}

// Forward declarations
class Renderer;
class UISystem;
struct lua_State;

// Lua argument wrapper for callbacks
struct LuaArg {
    enum Type { INT, FLOAT, STRING };
    Type type;
    union {
        int i;
        float f;
        const char* s;
    } value;
    
    LuaArg(int v) : type(INT) { value.i = v; }
    LuaArg(float v) : type(FLOAT) { value.f = v; }
    LuaArg(const char* v) : type(STRING) { value.s = v; }
    
    void push(lua_State* L) const;  // Defined in app.cpp
};

// App context for Lua scripts
struct AppContext {
    std::string id;
    Renderer* renderer;
    UISystem* ui;
};

// App instance
struct App {
    int pid;
    lua_State* L;
    AppContext ctx;
    std::unique_ptr<UISystem> ui;
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

// Call registered callback with argument list
void call_registered_callback(lua_State* L, const char* name, const std::vector<LuaArg>& args = {});

// Get app by PID (for direct access if needed)
App* getApp(int pid);
