#include "ui.hpp"
#include <algorithm>
#include <iostream>

// ==============================
// VIEW BASE
// ==============================
void View::addChild(std::unique_ptr<View> child) {
    child->parent = this;
    children.push_back(std::move(child));
}

bool View::contains(int px, int py) const {
    return px >= x && py >= y && px <= x + w && py <= y + h;
}

void View::draw(Renderer& r) {
    for (auto& c : children) {
        if (c->visible) c->draw(r);
    }
}

bool View::handleTouchBegin(const UITouchEvent& e) {
    if (onTouchBegin) return onTouchBegin(e);
    return false;
}

bool View::handleTouchGoing(const UITouchEvent& e) {
    if (onTouchGoing) return onTouchGoing(e);
    return false;
}

bool View::handleTouchEnd(const UITouchEvent& e) {
    if (onTouchEnd) return onTouchEnd(e);
    return false;
}

// ==============================
// FRAME
// ==============================
void Frame::draw(Renderer& r) {
    if (hasBackground) {
        Polygon p;
        p.v = {
            {x, y}, {x + w, y},
            {x + w, y + h}, {x, y + h}
        };
        r.drawPolygon(p, {bgColor});
    }

    View::draw(r);
}

// ==============================
// TEXT
// ==============================
void Text::draw(Renderer& r) {
    Frame::draw(r);
    if (font) {
        r.drawText(*font, content, x, y + h, color);
    }
}

// ==============================
// TEXTURE VIEW
// ==============================
void TextureView::draw(Renderer& r) {
    if (texture) {
        r.drawTexture(*texture, x, y);
    }
    View::draw(r);
}

// ==============================
// TEXTURE FRAME
// ==============================
void TextureFrame::draw(Renderer& r) {
    Frame::draw(r);
    if (texture) {
        r.drawTexture(*texture, x, y);
    }
}

// ==============================
// BUTTON
// ==============================
bool Button::handleTouchBegin(const UITouchEvent& e) {
    if (!contains(e.x, e.y)) return false;

    pressed = true;
    return true;
}

bool Button::handleTouchEnd(const UITouchEvent& e) {
    if (pressed && contains(e.x, e.y)) {
        pressed = false;
        if (onClick) onClick();
        return true;
    }
    pressed = false;
    return false;
}

// ==============================
// CHECKBOX
// ==============================
bool Checkbox::handleTouchEnd(const UITouchEvent& e) {
    if (Button::handleTouchEnd(e)) {
        checked = !checked;
        if (onToggle) onToggle(checked);
        return true;
    }
    return false;
}

// ==============================
// UI SYSTEM
// ==============================
void UISystem::bindInput(InputSystem& input) {
    input.onBegan([this](const InputEvent& e) {
        dispatchBegin(&root, {e.id, e.x, e.y});
    });

    input.onGoing([this](const InputEvent& e) {
        dispatchGoing(&root, {e.id, e.x, e.y});
    });

    input.onEnded([this](const InputEvent& e) {
        dispatchEnd(&root, {e.id, e.x, e.y});
    });
}

void UISystem::render(Renderer& r) {
    // sort children by z
    std::sort(root.children.begin(), root.children.end(),
        [](const std::unique_ptr<View>& a, const std::unique_ptr<View>& b) {
            return a->z < b->z;
        });

    root.draw(r);
}

// ==============================
// EVENT DISPATCH
// ==============================
bool UISystem::dispatchBegin(View* v, const UITouchEvent& e) {
    // reverse for top-most first
    for (auto it = v->children.rbegin(); it != v->children.rend(); ++it) {
        if ((*it)->visible && (*it)->contains(e.x, e.y)) {
            if (dispatchBegin(it->get(), e)) return true;
        }
    }
    return v->handleTouchBegin(e);
}

bool UISystem::dispatchGoing(View* v, const UITouchEvent& e) {
    for (auto it = v->children.rbegin(); it != v->children.rend(); ++it) {
        if ((*it)->visible && (*it)->contains(e.x, e.y)) {
            if (dispatchGoing(it->get(), e)) return true;
        }
    }
    return v->handleTouchGoing(e);
}

bool UISystem::dispatchEnd(View* v, const UITouchEvent& e) {
    for (auto it = v->children.rbegin(); it != v->children.rend(); ++it) {
        if ((*it)->visible && (*it)->contains(e.x, e.y)) {
            if (dispatchEnd(it->get(), e)) return true;
        }
    }
    return v->handleTouchEnd(e);
}

#include "ui.hpp"
#include <cstring>

// ==============================
// HELPERS
// ==============================
static View* checkView(lua_State* L, int i) {
    return (View*)lua_touserdata(L, i);
}

// ==============================
// CREATE
// ==============================
int l_createView(lua_State* L) {
    const char* type = luaL_checkstring(L, 1);

    View* v = nullptr;

    if (strcmp(type, "Frame") == 0) v = new Frame();
    else if (strcmp(type, "Button") == 0) v = new Button();
    else if (strcmp(type, "Text") == 0) v = new Text();
    else if (strcmp(type, "Checkbox") == 0) v = new Checkbox();
    else if (strcmp(type, "Texture") == 0) v = new TextureView();
    else if (strcmp(type, "TextureFrame") == 0) v = new TextureFrame();
    else v = new View();

    lua_pushlightuserdata(L, v);
    return 1;
}

// ==============================
// SET
// ==============================
int l_set(lua_State* L) {
    View* v = checkView(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (strcmp(key, "x") == 0) v->x = luaL_checkinteger(L, 3);
    else if (strcmp(key, "y") == 0) v->y = luaL_checkinteger(L, 3);
    else if (strcmp(key, "w") == 0) v->w = luaL_checkinteger(L, 3);
    else if (strcmp(key, "h") == 0) v->h = luaL_checkinteger(L, 3);
    else if (strcmp(key, "z") == 0) v->z = luaL_checkinteger(L, 3);

    // Text-specific
    else if (auto t = dynamic_cast<Text*>(v)) {
        if (strcmp(key, "text") == 0) {
            t->content = luaL_checkstring(L, 3);
        }
    }

    return 0;
}

// ==============================
// ADD CHILD
// ==============================
int l_add(lua_State* L) {
    View* parent = checkView(L, 1);
    View* child = checkView(L, 2);

    parent->addChild(std::unique_ptr<View>(child));
    return 0;
}

// ==============================
// BUTTON CLICK
// ==============================
int l_onClick(lua_State* L) {
    View* v = checkView(L, 1);

    if (auto b = dynamic_cast<Button*>(v)) {
        lua_pushvalue(L, 2);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        lua_State* state = L;

        b->onClick = [ref, state]() {
            lua_rawgeti(state, LUA_REGISTRYINDEX, ref);
            if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
                printf("Lua error: %s\n", lua_tostring(state, -1));
                lua_pop(state, 1);
            }
        };
    }

    return 0;
}

// ==============================
// CHECKBOX TOGGLE
// ==============================
int l_onToggle(lua_State* L) {
    View* v = checkView(L, 1);

    if (auto c = dynamic_cast<Checkbox*>(v)) {
        lua_pushvalue(L, 2);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        lua_State* state = L;

        c->onToggle = [ref, state](bool value) {
            lua_rawgeti(state, LUA_REGISTRYINDEX, ref);
            lua_pushboolean(state, value);

            if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
                printf("Lua error: %s\n", lua_tostring(state, -1));
                lua_pop(state, 1);
            }
        };
    }

    return 0;
}