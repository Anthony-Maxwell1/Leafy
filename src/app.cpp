// app.cpp
#include "app.hpp"
#include "lua.hpp"
#include "ui.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <map>
#include <cstring>
#include "renderer.hpp"
#include "texture.hpp"
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
        // Render UI for this app
        if (app->ui && app->ctx.renderer) {
            app->ui->render(*app->ctx.renderer);
        }
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
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    // Arg 1: table of {x, y} vertices
    luaL_checktype(L, 1, LUA_TTABLE);
    int n = lua_rawlen(L, 1);  // Get table length
    if (n < 3) return 0;

    // Arg 2: gray value (optional)
    int gray = luaL_optinteger(L, 2, 0);

    // Create polygon from table
    Polygon p;
    p.v.reserve(n);

    for (int i = 0; i < n; i++) {
        lua_rawgeti(L, 1, i + 1);  // Get vertices[i+1]
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return 0;
        }

        // Get x coordinate (first element of vertex table)
        lua_rawgeti(L, -1, 1);
        float x = luaL_checknumber(L, -1);
        lua_pop(L, 1);

        // Get y coordinate (second element of vertex table)
        lua_rawgeti(L, -1, 2);
        float y = luaL_checknumber(L, -1);
        lua_pop(L, 1);

        p.v.push_back({x, y});
        lua_pop(L, 1);  // Pop the vertex table
    }

    ctx->renderer->drawPolygon(p, Gray(gray));
    return 0;
}

int l_drawText(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    const char* text = luaL_checkstring(L, 3);
    int size = luaL_optinteger(L, 4, 16);
    int gray = luaL_optinteger(L, 5, 0);

    // TODO: Font support - for now, render text as simple geometric boxes
    // This is a placeholder that renders text as blocks
    for (int i = 0; i < (int)std::strlen(text); i++) {
        int px = x + i * (size / 2);
        Polygon p;
        p.v = {
            {(float)px, (float)y},
            {(float)(px + size/2 - 2), (float)y},
            {(float)(px + size/2 - 2), (float)(y + size)},
            {(float)px, (float)(y + size)}
        };
        ctx->renderer->drawPolygon(p, Gray(gray));
    }
    return 0;
}

int l_loadTexture(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    const char* path = luaL_checkstring(L, 1);
    
    // Load texture from file
    Texture t = loadTexture(path);
    
    if (t.data.empty()) {
        lua_pushnil(L);
        return 1;
    }
    
    // Create texture userdata
    Texture* tex = (Texture*)lua_newuserdata(L, sizeof(Texture));
    *tex = t;
    
    return 1;
}

int l_drawTexture(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    Texture* tex = (Texture*)lua_touserdata(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    
    if (!tex || tex->data.empty()) return 0;
    
    ctx->renderer->drawTexture(*tex, x, y);
    return 0;
}

int l_onTouchBegin(lua_State* L) {
    View* v = (View*)lua_touserdata(L, 1);

    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    v->onTouchBegin = [ref, L](const UITouchEvent& e) -> bool {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_pushinteger(L, e.id);
        lua_pushinteger(L, e.x);
        lua_pushinteger(L, e.y);

        if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
            return false;
        }

        return lua_toboolean(L, -1);
    };

    return 0;
}

int l_onTouchGoing(lua_State* L) {
    View* v = (View*)lua_touserdata(L, 1);

    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    v->onTouchGoing = [ref, L](const UITouchEvent& e) -> bool {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_pushinteger(L, e.id);
        lua_pushinteger(L, e.x);
        lua_pushinteger(L, e.y);

        if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
            return false;
        }

        return lua_toboolean(L, -1);
    };

    return 0;
}

int l_onTouchEnd(lua_State* L) {
    View* v = (View*)lua_touserdata(L, 1);

    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    v->onTouchEnd = [ref, L](const UITouchEvent& e) -> bool {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_pushinteger(L, e.id);
        lua_pushinteger(L, e.x);
        lua_pushinteger(L, e.y);

        if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
            return false;
        }

        return lua_toboolean(L, -1);
    };

    return 0;
}

// Add view to UISystem root
int l_addToRoot(lua_State* L) {
    View* v = (View*)lua_touserdata(L, 1);
    AppContext* ctx = get_ctx(L);
    
    if (!ctx || !ctx->ui) return 0;
    
    ctx->ui->root.addChild(std::unique_ptr<View>(v));
    return 0;
}

std::map<std::string, std::vector<LuaFunction>> build_api() {
    return {
        {
            "render",
            {
                {"circle", l_circle},
                {"rect", l_rect},
                {"polygon", l_polygon},
                {"pixel", l_pixel},
                {"text", l_drawText},
                {"loadTexture", l_loadTexture},
                {"drawTexture", l_drawTexture}
            }
        },
        {
            "callbacks",
            {
                {"register", l_registerCallback}
            }
        },
        {
            "ui",
            {
                {"create", l_createView},
                {"set", l_set},
                {"add", l_add},
                {"addToRoot", l_addToRoot},

                // events
                {"onClick", l_onClick},
                {"onToggle", l_onToggle},

                // 🔥 generic touch hooks (important)
                {"onTouchBegin", l_onTouchBegin},
                {"onTouchGoing", l_onTouchGoing},
                {"onTouchEnd", l_onTouchEnd}
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
    app->ui = std::make_unique<UISystem>();
    app->ctx = { path, renderer, app->ui.get() };

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