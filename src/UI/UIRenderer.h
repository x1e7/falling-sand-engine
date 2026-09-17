#pragma once

#include "SDL3_ttf/SDL_ttf.h"
#include "Core/Math.h"
#include <string>
#include <unordered_map>

class UIRenderer {
public:
    bool init(SDL_Renderer* renderer, const std::string& fontPath, int fontSize);
    void destroy();

    void drawRect(const SDL_FRect& rect, SDL_Color color);
    void drawRectOutline(const SDL_FRect& rect, SDL_Color color, int thickness);
    void drawLine(float x1, float y1, float x2, float y2, SDL_Color color);

    void drawText(const std::string& text, float x, float y, SDL_Color color);
    void drawTextCentered(const std::string& text, const SDL_FRect& rect, SDL_Color color);
    Vec2f measureText(const std::string& text) const;

    void draw9Slice(SDL_Texture* tex, const SDL_FRect& dst, int cornerSize);

    void clearCache();

private:
    struct TextKey {
        std::string text;
        uint32_t color;
        bool operator==(const TextKey& o) const {
            return text == o.text && color == o.color;
        }
    };
    struct TextKeyHash {
        size_t operator()(const TextKey& k) const {
            return std::hash<std::string>{}(k.text) ^ k.color;
        }
    };

    SDL_Texture* getTextTexture(const std::string& text, SDL_Color color);
    uint32_t packColor(SDL_Color c) const {
        return (uint32_t(c.a) << 24) | (uint32_t(c.r) << 16) |
               (uint32_t(c.g) << 8)  | uint32_t(c.b);
    }

    SDL_Renderer* m_renderer = nullptr;
    TTF_Font* m_font = nullptr;
    int m_fontSize = 0;
    std::unordered_map<TextKey, SDL_Texture*, TextKeyHash> m_textCache;
};
