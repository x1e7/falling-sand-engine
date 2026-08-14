#pragma once

#include <SDL2/SDL.h>
#include <string>
#include "FontData.h"
#include "Core/Math/Vector2.h"

struct Color {
    uint8_t r, g, b, a;

    Color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    static const Color White;
    static const Color Black;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
};

class Text {
public:
    Text();
    Text(const std::string& text);
    ~Text();

    void setString(const std::string& text);
    const std::string& getString() const { return m_text; }

    void setPosition(float x, float y);
    void setPosition(const Vec2f& position);
    Vec2f getPosition() const { return m_position; }

    void setColor(const Color& color);
    const Color& getColor() const { return m_color; }

    void setCharacterSize(int size);
    int getCharacterSize() const { return m_characterSize; }

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    void render(SDL_Renderer* renderer);

private:
    void createFontTexture(SDL_Renderer* renderer);
    void renderChar(SDL_Renderer* renderer, char c, int x, int y);
    int charToIndex(char c) const;

    std::string m_text;
    Vec2f m_position{0.0f, 0.0f};
    Color m_color{255, 255, 255, 255};
    int m_characterSize = 16;
    bool m_visible = true;

    SDL_Texture* m_fontTexture = nullptr;
    SDL_Texture* m_textTexture = nullptr;
    Vec2f m_size{0.0f, 0.0f};
    bool m_needsUpdate = true;
    SDL_Renderer* m_renderer = nullptr;
};
