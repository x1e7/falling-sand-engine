#include "Text.h"
#include <cstring>
#include <iostream>
#include <algorithm>

const Color Color::White(255, 255, 255);
const Color Color::Black(0, 0, 0);
const Color Color::Red(255, 0, 0);
const Color Color::Green(0, 255, 0);
const Color Color::Blue(0, 0, 255);
const Color Color::Yellow(255, 255, 0);

Text::Text() {}

Text::Text(const std::string& text) : m_text(text), m_needsUpdate(true) {}

Text::~Text() {
    if (m_fontTexture) SDL_DestroyTexture(m_fontTexture);
    if (m_textTexture) SDL_DestroyTexture(m_textTexture);
}

void Text::setString(const std::string& text) {
    if (m_text != text) {
        m_text = text;
        m_needsUpdate = true;
    }
}

void Text::setPosition(float x, float y) {
    m_position.x = x;
    m_position.y = y;
}

void Text::setPosition(const Vec2f& position) {
    m_position = position;
}

void Text::setColor(const Color& color) {
    m_color = color;
    m_needsUpdate = true;
}

void Text::setCharacterSize(int size) {
    if (m_characterSize != size) {
        m_characterSize = size;
        m_needsUpdate = true;
    }
}

Vec2f Text::calculateTextSize() const {
    if (m_text.empty()) return Vec2f(0.0f, 0.0f);

    int charWidth = FontData::FONT_CHAR_WIDTH * m_characterSize / 8;
    int charHeight = FontData::FONT_CHAR_HEIGHT * m_characterSize / 8;

    int maxLineWidth = 0;
    int currentLineWidth = 0;
    int lineCount = 1;

    for (char c : m_text) {
        if (c == '\n') {
            maxLineWidth = std::max(maxLineWidth, currentLineWidth);
            currentLineWidth = 0;
            lineCount++;
        } else {
            currentLineWidth += charWidth;
        }
    }
    maxLineWidth = std::max(maxLineWidth, currentLineWidth);

    return Vec2f(
        static_cast<float>(maxLineWidth),
        static_cast<float>(lineCount * charHeight)
    );
}

void Text::createFontTexture(SDL_Renderer* renderer) {
    if (m_fontCreated) return;

    const int textureWidth = FontData::FONT_CHAR_WIDTH * FontData::FONT_CHARS_COUNT;
    const int textureHeight = FontData::FONT_CHAR_HEIGHT;

    m_fontTexture = SDL_CreateTexture(renderer,
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      textureWidth, textureHeight);

    if (!m_fontTexture) {
        std::cerr << "Failed to create font texture: " << SDL_GetError() << std::endl;
        return;
    }

    uint32_t* pixels;
    int pitch;
    SDL_LockTexture(m_fontTexture, nullptr, (void**)&pixels, &pitch);
    std::memset(pixels, 0, static_cast<size_t>(pitch) * textureHeight);

    for (int charIdx = 0; charIdx < FontData::FONT_CHARS_COUNT; ++charIdx) {
        for (int y = 0; y < FontData::FONT_CHAR_HEIGHT; ++y) {
            for (int x = 0; x < FontData::FONT_CHAR_WIDTH; ++x) {
                bool isSet = FontData::FONT_8x8[charIdx][y] & (1 << (7 - x));
                if (isSet) {
                    int pixelX = charIdx * FontData::FONT_CHAR_WIDTH + x;
                    pixels[y * (pitch / 4) + pixelX] = 0xFFFFFFFF;
                }
            }
        }
    }

    SDL_UnlockTexture(m_fontTexture);
    SDL_SetTextureBlendMode(m_fontTexture, SDL_BLENDMODE_BLEND);
    m_fontCreated = true;
}

int Text::charToIndex(char c) const {
    return FontData::charToIndex(c);
}

void Text::renderCharToTexture(SDL_Renderer* renderer, char c, int x, int y) {
    int idx = charToIndex(c);
    if (idx < 0) return;

    SDL_Rect srcRect = {
        idx * FontData::FONT_CHAR_WIDTH,
        0,
        FontData::FONT_CHAR_WIDTH,
        FontData::FONT_CHAR_HEIGHT
    };

    int scaledWidth = FontData::FONT_CHAR_WIDTH * m_characterSize / 8;
    int scaledHeight = FontData::FONT_CHAR_HEIGHT * m_characterSize / 8;

    SDL_Rect dstRect = {x, y, scaledWidth, scaledHeight};

    SDL_SetTextureColorMod(m_fontTexture, m_color.r, m_color.g, m_color.b);
    SDL_SetTextureAlphaMod(m_fontTexture, m_color.a);

    SDL_RenderCopy(renderer, m_fontTexture, &srcRect, &dstRect);
}

void Text::updateTextTexture(SDL_Renderer* renderer) {
    if (!m_needsUpdate) return;

    if (m_textTexture) {
        SDL_DestroyTexture(m_textTexture);
        m_textTexture = nullptr;
    }

    if (m_text.empty()) {
        m_textSize = Vec2f(0.0f, 0.0f);
        m_needsUpdate = false;
        return;
    }

    if (!m_fontCreated) {
        createFontTexture(renderer);
    }

    if (!m_fontTexture) return;

    m_textSize = calculateTextSize();
    if (m_textSize.x == 0.0f || m_textSize.y == 0.0f) {
        m_needsUpdate = false;
        return;
    }

    m_textTexture = SDL_CreateTexture(renderer,
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET,
                                      static_cast<int>(m_textSize.x),
                                      static_cast<int>(m_textSize.y));

    if (!m_textTexture) {
        std::cerr << "Failed to create text texture: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, m_textTexture);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    int charWidth = FontData::FONT_CHAR_WIDTH * m_characterSize / 8;
    int charHeight = FontData::FONT_CHAR_HEIGHT * m_characterSize / 8;

    int x = 0, y = 0;
    for (char c : m_text) {
        if (c == '\n') {
            x = 0;
            y += charHeight;
            continue;
        }
        renderCharToTexture(renderer, c, x, y);
        x += charWidth;
    }

    SDL_SetRenderTarget(renderer, oldTarget);
    SDL_SetTextureBlendMode(m_textTexture, SDL_BLENDMODE_BLEND);

    m_needsUpdate = false;
}

void Text::render(SDL_Renderer* renderer) {
    if (!m_visible || m_text.empty()) return;

    if (m_needsUpdate) {
        updateTextTexture(renderer);
    }

    if (!m_textTexture) return;

    SDL_Rect dstRect = {
        static_cast<int>(m_position.x),
        static_cast<int>(m_position.y),
        static_cast<int>(m_textSize.x),
        static_cast<int>(m_textSize.y)
    };

    SDL_RenderCopy(renderer, m_textTexture, nullptr, &dstRect);
}
