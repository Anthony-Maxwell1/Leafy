// app.cpp
#include "lua.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include "renderer.hpp"
#include "core.hpp"

// ---- from lua.cpp ----
void setup_sandbox(lua_State* L);
void register_namespaces(lua_State* L,
    const std::map<std::string, std::vector<struct LuaFunction>>& api);
bool run_script(lua_State* L, const std::string& path);
void call_start(lua_State* L);
void call_update(lua_State* L, float dt);

// ------------------------
// API function struct
// ------------------------
struct LuaFunction {
    std::string name;
    lua_CFunction func;
};

// Forward declaration
class Renderer;

// ------------------------
// App Context
// ------------------------
struct AppContext {
    std::string id;
    Renderer* renderer;
};

// ------------------------
// App Instance
// ------------------------
struct App {
    int pid;
    lua_State* L;
    AppContext ctx;
};

// ------------------------
// Global app registry
// ------------------------
static std::unordered_map<int, std::unique_ptr<App>> apps;
static int next_pid = 1;

// ------------------------
// Helpers
// ------------------------
AppContext* get_ctx(lua_State* L) {
    lua_getglobal(L, "__ctx");
    return (AppContext*)lua_touserdata(L, -1);
}

// ========================
// Rendering API
// ========================
int l_circle(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    int gray = luaL_optinteger(L, 4, 0);

    Circle c;
    c.c = {(float)x, (float)y};
    c.r = (float)r;
    c.filled = true;

    ctx->renderer->drawCircle(c, Gray(gray));
    return 0;
}

int l_rect(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);
    int gray = luaL_optinteger(L, 5, 0);

    // Draw a filled rectangle as a polygon
    Polygon p;
    p.v = {
        {(float)x, (float)y},
        {(float)(x + w), (float)y},
        {(float)(x + w), (float)(y + h)},
        {(float)x, (float)(y + h)}
    };

    ctx->renderer->drawPolygon(p, Gray(gray));
    return 0;
}

int l_pixel(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int gray = luaL_optinteger(L, 3, 0);

    // Draw single pixel (1x1 rect)
    Polygon p;
    p.v = {
        {(float)x, (float)y},
        {(float)(x + 1), (float)y},
        {(float)(x + 1), (float)(y + 1)},
        {(float)x, (float)(y + 1)}
    };

    ctx->renderer->drawPolygon(p, Gray(gray));
    return 0;
}

// ========================
// API builder
// ========================
std::map<std::string, std::vector<LuaFunction>> build_api() {
    return {
        {
            "leafy",
            {
                {"circle", l_circle},
                {"rect", l_rect},
                {"pixel", l_pixel}
            }
        }
    };
}

// ------------------------
// Attach context
// ------------------------
void attach_context(lua_State* L, AppContext* ctx) {
    lua_pushlightuserdata(L, ctx);
    lua_setglobal(L, "__ctx");
}

// ========================
// 🚀 Launch App
// ========================
int launchApp(const std::string& path, Renderer* renderer) {
    lua_State* L = luaL_newstate();

    setup_sandbox(L);

    auto api = build_api();
    register_namespaces(L, api);

    // Create app
    auto app = std::make_unique<App>();
    app->pid = next_pid++;
    app->L = L;
    app->ctx = { path, renderer };

    attach_context(L, &app->ctx);

    if (!run_script(L, path)) {
        std::cerr << "Failed to load app\n";
        lua_close(L);
        return -1;
    }

    call_start(L);

    int pid = app->pid;
    apps[pid] = std::move(app);

    std::cout << "[Leafy] Launched app PID=" << pid << "\n";
    return pid;
}

// ========================
// ❌ Close App
// ========================
void closeApp(int pid) {
    auto it = apps.find(pid);
    if (it == apps.end()) return;

    lua_close(it->second->L);
    apps.erase(it);

    std::cout << "[Leafy] Closed app PID=" << pid << "\n";
}

// ========================
// 🎯 Frame Loop
// ========================
void frame(float dt) {
    for (auto& [pid, app] : apps) {
        call_update(app->L, dt);
    }
}