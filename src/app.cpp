// app.cpp
#include "app.hpp"
#include "lua.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <map>
#include "renderer.hpp"
#include "core.hpp"

// -------- from lua.cpp ----
void setup_sandbox(lua_State* L);
void register_namespaces(lua_State* L,
    const std::map<std::string, std::vector<struct LuaFunction>>& api);
bool run_script(lua_State* L, const std::string& path);
void call_start(lua_State* L);
void call_update(lua_State* L, float dt);

// -------- LuaArg implementation --------
void LuaArg::push(lua_State* L) const {
    switch (type) {
        case INT: lua_pushinteger(L, value.i); break;
        case FLOAT: lua_pushnumber(L, value.f); break;
        case STRING: lua_pushstring(L, value.s); break;
    }
}

// -------- Callback execution with arguments --------

// Call callback with argument list
void call_registered_callback(lua_State* L, const char* name, const std::vector<LuaArg>& args) {
    lua_getfield(L, LUA_REGISTRYINDEX, "__callbacks");

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, name);

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return;
    }

    lua_remove(L, -2); // remove __callbacks table

    for (const auto& arg : args) {
        arg.push(L);
    }

    if (lua_pcall(L, args.size(), 0, 0) != LUA_OK) {
        std::cerr << "[Lua callback error] " << lua_tostring(L, -1) << "\n";
        lua_pop(L, 1);
    }
}

// ========================
// Forward declarations
// ========================
class Renderer;

static std::unordered_map<int, std::unique_ptr<App>> apps;
static int next_pid = 1;

// ========================
// Context helper
// ========================
AppContext* get_ctx(lua_State* L) {
    lua_getglobal(L, "__ctx");
    AppContext* ctx = (AppContext*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ctx;
}

// ========================
// INPUT EVENT DISPATCH
// ========================
void onBegan(lua_State* L, int id, int x, int y) {
    call_registered_callback(L, "began", {
        LuaArg(id),
        LuaArg(x),
        LuaArg(y)
    });
}

void onGoing(lua_State* L, int id, int x, int y) {
    call_registered_callback(L, "going", {
        LuaArg(id),
        LuaArg(x),
        LuaArg(y)
    });
}

void onEnded(lua_State* L, int id, int x, int y) {
    call_registered_callback(L, "ended", {
        LuaArg(id),
        LuaArg(x),
        LuaArg(y)
    });
}

// ========================
// Multi-app input broadcast
// ========================
void dispatch_input_began(int id, int x, int y) {
    for (auto& [pid, app] : apps)
        onBegan(app->L, id, x, y);
}

void dispatch_input_going(int id, int x, int y) {
    for (auto& [pid, app] : apps)
        onGoing(app->L, id, x, y);
}

void dispatch_input_ended(int id, int x, int y) {
    for (auto& [pid, app] : apps)
        onEnded(app->L, id, x, y);
}

// ========================
// INIT HOOK (your requested function)
// ========================
void registerCallbacksFunction() {
    std::cout << "[Leafy] Input callbacks registered\n";
}

// ========================
// Lua callback registration API
// ========================
int l_registerCallback(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_getfield(L, LUA_REGISTRYINDEX, "__callbacks");

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_setfield(L, LUA_REGISTRYINDEX, "__callbacks");
        lua_getfield(L, LUA_REGISTRYINDEX, "__callbacks");
    }

    lua_pushvalue(L, 2);
    lua_setfield(L, -2, name);

    lua_pop(L, 1);
    return 0;
}

// ========================
// Frame loop
// ========================
void frame(float dt) {
    for (auto& [pid, app] : apps) {
        call_update(app->L, dt);
    }
}
// ========================
// Get App by PID
// ========================
App* getApp(int pid) {
    auto it = apps.find(pid);
    if (it != apps.end()) {
        return it->second.get();
    }
    return nullptr;
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
int l_polygon(lua_State* L) { 
    AppContext* ctx = get_ctx(L); if (!ctx || !ctx->renderer) return 0; 
    // Get the number of vertices 
    int n = luaL_checkinteger(L, 1); 
    if (n < 3) return 0; 
    // Create polygon 
    Polygon p; 
    p.v.reserve(n); 
    // Get vertex coordinates 
    for (int i = 0; i < n; i++) { 
        lua_pushinteger(L, i + 2); 
        lua_gettable(L, -2); 
        if (!lua_istable(L, -1)) { lua_pop(L, 1); return 0; } lua_pushinteger(L, 1); lua_gettable(L, -2); float x = luaL_checknumber(L, -1); lua_pop(L, 1); lua_pushinteger(L, 2); lua_gettable(L, -2); float y = luaL_checknumber(L, -1); lua_pop(L, 1); p.v.push_back({x, y}); } 
        // Get fill color 
        int gray = luaL_optinteger(L, 2, 0); ctx->renderer->drawPolygon(p, Gray(gray)); return 0; }

std::map<std::string, std::vector<LuaFunction>> build_api() {
    return {
        {
            "render",
            {
                {"circle", l_circle},
                {"rect", l_rect},
                {"polygon", l_polygon},
                {"pixel", l_pixel}
            }
        },
        {
            "callbacks",
            {
                {"register", l_registerCallback}
            }
        }
    };
}



// ========================
// Launch app
// ========================
int launchApp(const std::string& path, Renderer* renderer) {
    lua_State* L = luaL_newstate();

    setup_sandbox(L);

    std::map<std::string, std::vector<LuaFunction>> api = build_api();

    register_namespaces(L, api);

    auto app = std::make_unique<App>();
    app->pid = next_pid++;
    app->L = L;
    app->ctx = { path, renderer };

    lua_pushlightuserdata(L, &app->ctx);
    lua_setglobal(L, "__ctx");

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
// Close app
// ========================
void closeApp(int pid) {
    auto it = apps.find(pid);
    if (it == apps.end()) return;

    lua_close(it->second->L);
    apps.erase(it);

    std::cout << "[Leafy] Closed app PID=" << pid << "\n";
}