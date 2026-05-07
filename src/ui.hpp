#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include "renderer.hpp"
#include "input_system.hpp"
#include "lua.hpp"

// ==============================
// FORWARD
// ==============================
class View;

// ==============================
// EVENT
// ==============================
struct UITouchEvent {
    int id;
    int x;
    int y;
};

// ==============================
// VIEW BASE
// ==============================
class View {
public:
    int x = 0, y = 0, w = 0, h = 0;
    int z = 0;
    bool visible = true;

    View* parent = nullptr;
    std::vector<std::unique_ptr<View>> children;

    // Event hooks
    std::function<bool(const UITouchEvent&)> onTouchBegin;
    std::function<bool(const UITouchEvent&)> onTouchGoing;
    std::function<bool(const UITouchEvent&)> onTouchEnd;

    virtual ~View() = default;

    void addChild(std::unique_ptr<View> child);

    virtual void draw(Renderer& r);
    virtual bool handleTouchBegin(const UITouchEvent& e);
    virtual bool handleTouchGoing(const UITouchEvent& e);
    virtual bool handleTouchEnd(const UITouchEvent& e);

    bool contains(int px, int py) const;
};

// ==============================
// FRAME (background, stroke)
// ==============================
class Frame : public View {
public:
    bool hasBackground = true;
    uint8_t bgColor = 200;

    bool hasStroke = false;
    uint8_t strokeColor = 0;

    void draw(Renderer& r) override;
};

// ==============================
// TEXT
// ==============================
class Text : public Frame {
public:
    std::string content;
    Font* font = nullptr;
    uint8_t color = 0;

    void draw(Renderer& r) override;
};

// ==============================
// TEXTURE VIEW
// ==============================
class TextureView : public View {
public:
    Texture* texture = nullptr;

    void draw(Renderer& r) override;
};

// ==============================
// TEXTURE FRAME
// ==============================
class TextureFrame : public Frame {
public:
    Texture* texture = nullptr;

    void draw(Renderer& r) override;
};

// ==============================
// BUTTON
// ==============================
class Button : public Frame {
public:
    bool pressed = false;

    std::function<void()> onClick;

    bool handleTouchBegin(const UITouchEvent& e) override;
    bool handleTouchEnd(const UITouchEvent& e) override;
};

// ==============================
// CHECKBOX
// ==============================
class Checkbox : public Button {
public:
    bool checked = false;

    std::function<void(bool)> onToggle;

    bool handleTouchEnd(const UITouchEvent& e) override;
};

// ==============================
// UI SYSTEM
// ==============================
class UISystem {
public:
    View root;

    void bindInput(InputSystem& input);
    void render(Renderer& r);
    
    // Handle touch events
    void handleTouchBegin(const UITouchEvent& e) {
        dispatchBegin(&root, e);
    }
    void handleTouchGoing(const UITouchEvent& e) {
        dispatchGoing(&root, e);
    }
    void handleTouchEnd(const UITouchEvent& e) {
        dispatchEnd(&root, e);
    }

private:
    bool dispatchBegin(View* v, const UITouchEvent& e);
    bool dispatchGoing(View* v, const UITouchEvent& e);
    bool dispatchEnd(View* v, const UITouchEvent& e);
};

int l_createView(lua_State* L);
int l_set(lua_State* L);
int l_add(lua_State* L);

int l_onClick(lua_State* L);
int l_onToggle(lua_State* L);

int l_onTouchBegin(lua_State* L);
int l_onTouchGoing(lua_State* L);
int l_onTouchEnd(lua_State* L);