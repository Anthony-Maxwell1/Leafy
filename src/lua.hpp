#pragma once

#include <lua.hpp>
#include <string>
#include <vector>
#include <map>

// Lua function wrapper
struct LuaFunction {
    std::string name;
    lua_CFunction func;
};

using NamespaceMap = std::map<std::string, std::vector<LuaFunction>>;

// ========================
// Lua Setup & API
// ========================

// Setup sandbox with whitelisted globals
void setup_sandbox(lua_State* L);

// Register API namespaces (e.g., leafy.circle, leafy.rect)
void register_namespaces(lua_State* L, const NamespaceMap& api);

// Load and run a Lua script file
bool run_script(lua_State* L, const std::string& path);

// ========================
// Lifecycle Callbacks
// ========================

// Call start() function if it exists
void call_start(lua_State* L);

// Call update(dt) function if it exists
void call_update(lua_State* L, float dt);
