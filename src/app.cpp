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
#include <algorithm>

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

// ========================
// Call callback with argument list
// ========================
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

class Renderer;

static std::unordered_map<int, std::unique_ptr<App>> apps;
static int next_pid = 1;

// ========================
// Per-app heap-allocated textures (so the vector<uint8_t> survives)
// ========================
static std::unordered_map<int, std::vector<std::unique_ptr<Texture>>> app_textures;
static std::unordered_map<int, std::unique_ptr<Font>> app_fonts;

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
    call_registered_callback(L, "began", {LuaArg(id), LuaArg(x), LuaArg(y)});
}
void onGoing(lua_State* L, int id, int x, int y) {
    call_registered_callback(L, "going", {LuaArg(id), LuaArg(x), LuaArg(y)});
}
void onEnded(lua_State* L, int id, int x, int y) {
    call_registered_callback(L, "ended", {LuaArg(id), LuaArg(x), LuaArg(y)});
}

void dispatch_input_began(int id, int x, int y) {
    for (auto& [pid, app] : apps) onBegan(app->L, id, x, y);
}
void dispatch_input_going(int id, int x, int y) {
    for (auto& [pid, app] : apps) onGoing(app->L, id, x, y);
}
void dispatch_input_ended(int id, int x, int y) {
    for (auto& [pid, app] : apps) onEnded(app->L, id, x, y);
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
    if (it != apps.end()) return it->second.get();
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
    Polygon p;
    p.v = {
        {(float)x, (float)y},
        {(float)(x+w), (float)y},
        {(float)(x+w), (float)(y+h)},
        {(float)x, (float)(y+h)}
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
    Polygon p;
    p.v = {
        {(float)x, (float)y},
        {(float)(x+1), (float)y},
        {(float)(x+1), (float)(y+1)},
        {(float)x, (float)(y+1)}
    };
    ctx->renderer->drawPolygon(p, Gray(gray));
    return 0;
}

int l_polygon(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;
    luaL_checktype(L, 1, LUA_TTABLE);
    int n = lua_rawlen(L, 1);
    if (n < 3) return 0;
    int gray = luaL_optinteger(L, 2, 0);
    Polygon p;
    p.v.reserve(n);
    for (int i = 0; i < n; i++) {
        lua_rawgeti(L, 1, i+1);
        if (!lua_istable(L, -1)) { lua_pop(L, 1); return 0; }
        lua_rawgeti(L, -1, 1);
        float x = luaL_checknumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2);
        float y = luaL_checknumber(L, -1); lua_pop(L, 1);
        p.v.push_back({x, y});
        lua_pop(L, 1);
    }
    ctx->renderer->drawPolygon(p, Gray(gray));
    return 0;
}

// ========================
// FONT LOADING
// ========================
int l_loadFont(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx) return 0;

    const char* path = luaL_checkstring(L, 1);
    int size = luaL_optinteger(L, 2, 20);

    // Find which PID owns this lua state
    int pid = -1;
    lua_getglobal(L, "__pid");
    if (lua_isnumber(L, -1)) pid = lua_tointeger(L, -1);
    lua_pop(L, 1);

    auto font = std::make_unique<Font>();
    if (!font->load(path, size)) {
        lua_pushnil(L);
        return 1;
    }

    Font* raw = font.get();
    if (pid >= 0) {
        app_fonts[pid] = std::move(font);
    } else {
        // fallback: leak (shouldn't happen)
        font.release();
    }

    lua_pushlightuserdata(L, raw);
    return 1;
}

// ========================
// TEXT DRAWING - uses real FreeType font
// ========================
int l_drawText(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    const char* text = luaL_checkstring(L, 3);
    int gray = luaL_optinteger(L, 4, 0);

    // Arg 5: font userdata (optional)
    Font* font = nullptr;
    if (lua_isuserdata(L, 5)) {
        font = (Font*)lua_touserdata(L, 5);
    }

    if (font) {
        ctx->renderer->drawText(*font, text, x, y, (uint8_t)gray);
    }
    return 0;
}

// ========================
// TEXTURE LOADING - heap-allocated, safe
// ========================
int l_loadTexture(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx) return 0;

    const char* path = luaL_checkstring(L, 1);

    int pid = -1;
    lua_getglobal(L, "__pid");
    if (lua_isnumber(L, -1)) pid = lua_tointeger(L, -1);
    lua_pop(L, 1);

    auto tex = std::make_unique<Texture>(loadTexture(path));

    if (tex->data.empty()) {
        std::cerr << "[Texture] Failed to load: " << path << "\n";
        lua_pushnil(L);
        return 1;
    }

    Texture* raw = tex.get();
    if (pid >= 0) {
        app_textures[pid].push_back(std::move(tex));
    } else {
        tex.release(); // leak fallback
    }

    lua_pushlightuserdata(L, raw);
    return 1;
}

int l_drawTexture(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->renderer) return 0;

    if (!lua_isuserdata(L, 1)) return 0;
    Texture* tex = (Texture*)lua_touserdata(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    if (!tex || tex->data.empty()) return 0;

    ctx->renderer->drawTexture(*tex, x, y);
    return 0;
}

// ========================
// TOUCH HOOKS ON VIEWS
// ========================
int l_onTouchBegin(lua_State* L) {
    View* v = (View*)lua_touserdata(L, 1);
    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    v->onTouchBegin = [ref, L](const UITouchEvent& e) -> bool {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_pushinteger(L, e.id);
        lua_pushinteger(L, e.x);
        lua_pushinteger(L, e.y);
        if (lua_pcall(L, 3, 1, 0) != LUA_OK) { lua_pop(L, 1); return false; }
        bool r = lua_toboolean(L, -1); lua_pop(L, 1);
        return r;
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
        if (lua_pcall(L, 3, 1, 0) != LUA_OK) { lua_pop(L, 1); return false; }
        bool r = lua_toboolean(L, -1); lua_pop(L, 1);
        return r;
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
        if (lua_pcall(L, 3, 1, 0) != LUA_OK) { lua_pop(L, 1); return false; }
        bool r = lua_toboolean(L, -1); lua_pop(L, 1);
        return r;
    };
    return 0;
}

// ========================
// addToRoot - safe: uses a tracked raw pointer registry
// Views created by l_createView are owned by Lua userdata lifetime;
// We register them in a per-app set to prevent double-free.
// ========================
static std::unordered_map<int, std::vector<View*>> rooted_views;

int l_addToRoot(lua_State* L) {
    View* v = (View*)lua_touserdata(L, 1);
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->ui || !v) return 0;

    int pid = -1;
    lua_getglobal(L, "__pid");
    if (lua_isnumber(L, -1)) pid = lua_tointeger(L, -1);
    lua_pop(L, 1);

    // Prevent double-add
    if (pid >= 0) {
        auto& rv = rooted_views[pid];
        for (auto* existing : rv) {
            if (existing == v) return 0; // already added
        }
        rv.push_back(v);
    }

    ctx->ui->root.addChild(std::unique_ptr<View>(v));
    return 0;
}

// ========================
// removeFromRoot - remove a view from the UI tree
// ========================
int l_removeFromRoot(lua_State* L) {
    View* v = (View*)lua_touserdata(L, 1);
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->ui || !v) return 0;

    auto& children = ctx->ui->root.children;
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (it->get() == v) {
            it->release(); // don't delete — Lua still owns userdata
            children.erase(it);

            // Remove from tracked list too
            int pid = -1;
            lua_getglobal(L, "__pid");
            if (lua_isnumber(L, -1)) pid = lua_tointeger(L, -1);
            lua_pop(L, 1);
            if (pid >= 0) {
                auto& rv = rooted_views[pid];
                rv.erase(std::remove(rv.begin(), rv.end(), v), rv.end());
            }
            break;
        }
    }
    return 0;
}

// ========================
// clearRoot - remove all views from root
// ========================
int l_clearRoot(lua_State* L) {
    AppContext* ctx = get_ctx(L);
    if (!ctx || !ctx->ui) return 0;

    // Release ownership without deleting (Lua userdata owns the pointers)
    for (auto& child : ctx->ui->root.children) {
        child.release();
    }
    ctx->ui->root.children.clear();

    int pid = -1;
    lua_getglobal(L, "__pid");
    if (lua_isnumber(L, -1)) pid = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (pid >= 0) rooted_views[pid].clear();

    return 0;
}

// ========================
// Build API table
// ========================
std::map<std::string, std::vector<LuaFunction>> build_api() {
    return {
        {
            "render",
            {
                {"circle",      l_circle},
                {"rect",        l_rect},
                {"polygon",     l_polygon},
                {"pixel",       l_pixel},
                {"text",        l_drawText},
                {"loadTexture", l_loadTexture},
                {"drawTexture", l_drawTexture},
                {"loadFont",    l_loadFont},
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
                {"create",        l_createView},
                {"set",           l_set},
                {"add",           l_add},
                {"addToRoot",     l_addToRoot},
                {"removeFromRoot",l_removeFromRoot},
                {"clearRoot",     l_clearRoot},
                {"onClick",       l_onClick},
                {"onToggle",      l_onToggle},
                {"onTouchBegin",  l_onTouchBegin},
                {"onTouchGoing",  l_onTouchGoing},
                {"onTouchEnd",    l_onTouchEnd},
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

    // Expose PID to Lua for texture/font ownership tracking
    lua_pushinteger(L, app->pid);
    lua_setglobal(L, "__pid");

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
    app_textures.erase(pid);
    app_fonts.erase(pid);
    rooted_views.erase(pid);

    std::cout << "[Leafy] Closed app PID=" << pid << "\n";
}