#pragma once
#include "core.hpp"
#include "texture.hpp"
#include <string>
#include <unordered_map>

// Forward declare FreeType types
struct FT_LibraryRec_;
struct FT_FaceRec_;

typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;

// ==============================
// GLYPH
// ==============================
struct Glyph {
    Texture tex;

    int width;
    int height;

    int bearingX;
    int bearingY;

    int advance;
};

// ==============================
// FONT
// ==============================
class Font {
public:
    bool load(const std::string& path, int pixelSize);

    const Glyph& get(char c);
private:
    FT_Library ft = nullptr;
    FT_Face face = nullptr;

    std::unordered_map<char, Glyph> cache;
};

class Renderer {
public:
    void init(Framebuffer f);

    void beginFrame(uint8_t clear = 255);

    void drawPolygon(const Polygon& p, Color c);
    void drawCircle(const Circle& c, Color col);

    void drawText(Font& font, const std::string& text, int x, int y, uint8_t color);

    void present();
void drawTexture(const Texture& t, int x, int y);

private:
    Framebuffer fb;
    std::vector<uint8_t> prev;
};
