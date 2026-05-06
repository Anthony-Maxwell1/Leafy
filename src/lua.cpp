// lua.cpp
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include <string>
#include <vector>
#include <map>
#include <iostream>

struct LuaFunction {
    std::string name;
    lua_CFunction func;
};

using NamespaceMap = std::map<std::string, std::vector<LuaFunction>>;

// ------------------------
// Sandbox (whitelist only)
// ------------------------
void setup_sandbox(lua_State* L) {
    lua_newtable(L); // global env

    // Allow basic functions
    lua_getglobal(L, "print");
    lua_setfield(L, -2, "print");

    // math lib
    luaL_requiref(L, "math", luaopen_math, 1);
    lua_setfield(L, -2, "math");

    // string lib
    luaL_requiref(L, "string", luaopen_string, 1);
    lua_setfield(L, -2, "string");

    // table lib
    luaL_requiref(L, "table", luaopen_table, 1);
    lua_setfield(L, -2, "table");

    lua_setglobal(L, "_G");
}

// ------------------------
// Register namespaced API
// ------------------------
void register_namespaces(lua_State* L, const NamespaceMap& api) {
    for (const auto& [ns, funcs] : api) {
        lua_newtable(L); // namespace table

        for (const auto& f : funcs) {
            lua_pushcfunction(L, f.func);
            lua_setfield(L, -2, f.name.c_str());
        }

        lua_setglobal(L, ns.c_str()); // e.g. leafy
    }
}

// ------------------------
// Load + run script
// ------------------------
bool run_script(lua_State* L, const std::string& path) {
    if (luaL_loadfile(L, path.c_str()) != LUA_OK) {
        std::cerr << lua_tostring(L, -1) << "\n";
        return false;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        std::cerr << lua_tostring(L, -1) << "\n";
        return false;
    }

    return true;
}

// ------------------------
// Call lifecycle functions
// ------------------------
void call_start(lua_State* L) {
    lua_getglobal(L, "start");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            std::cerr << lua_tostring(L, -1) << "\n";
        }
    } else {
        lua_pop(L, 1);
    }
}

void call_update(lua_State* L, float dt) {
    lua_getglobal(L, "update");
    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, dt);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            std::cerr << lua_tostring(L, -1) << "\n";
        }
    } else {
        lua_pop(L, 1);
    }
}