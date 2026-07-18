#include "UIRenderer.h"
#include <iostream>
#include <cstring>

UIRenderer::UIRenderer(SDL_Renderer* renderer, int screenWidth, int screenHeight)
    : m_renderer(renderer)
    , m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight) {
    createFontTexture();

    m_uiTexture = SDL_CreateTexture(m_renderer,
                                    SDL_PIXELFORMAT_RGB888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    m_screenWidth, m_screenHeight);
    if (!m_uiTexture) {
        std::cerr << "Failed to create UI texture: " << SDL_GetError() << std::endl;
    }
}

UIRenderer::~UIRenderer() {
    if (m_fontTexture) SDL_DestroyTexture(m_fontTexture);
    if (m_uiTexture) SDL_DestroyTexture(m_uiTexture);
}

void UIRenderer::setScreenSize(int width, int height) {
    if (m_screenWidth != width || m_screenHeight != height) {
        m_screenWidth = width;
        m_screenHeight = height;

        if (m_uiTexture) {
            SDL_DestroyTexture(m_uiTexture);
        }
        m_uiTexture = SDL_CreateTexture(m_renderer,
                                       SDL_PIXELFORMAT_RGB888,
                                       SDL_TEXTUREACCESS_TARGET,
                                       m_screenWidth, m_screenHeight);
    }
}

void UIRenderer::setFPS(int fps) {
    if (m_fps != fps) {
        m_fps = fps;
        m_fpsText = "FPS: " + std::to_string(fps);
    }
}

void UIRenderer::createFontTexture() {
    const int textureWidth = FontData::FONT_CHAR_WIDTH * FontData::FONT_CHARS_COUNT;
    const int textureHeight = FontData::FONT_CHAR_HEIGHT;

    m_fontTexture = SDL_CreateTexture(m_renderer,
                                      SDL_PIXELFORMAT_RGB888,
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
                    pixels[y * (pitch / 4) + pixelX] = 0xFFFFFF;
                }
            }
        }
    }

    SDL_UnlockTexture(m_fontTexture);
    SDL_SetTextureBlendMode(m_fontTexture, SDL_BLENDMODE_BLEND);
}

void UIRenderer::drawChar(char c, int x, int y, uint32_t color) {
    int idx = FontData::charToIndex(c);
    if (idx < 0) return;

    SDL_Rect srcRect = {
        idx * FontData::FONT_CHAR_WIDTH,
        0,
        FontData::FONT_CHAR_WIDTH,
        FontData::FONT_CHAR_HEIGHT
    };

    int scaledWidth = FontData::FONT_CHAR_WIDTH * m_textScale;
    int scaledHeight = FontData::FONT_CHAR_HEIGHT * m_textScale;
    SDL_Rect dstRect = {x, y, scaledWidth, scaledHeight};

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    SDL_SetTextureColorMod(m_fontTexture, r, g, b);

    SDL_RenderCopy(m_renderer, m_fontTexture, &srcRect, &dstRect);
}

void UIRenderer::drawText(const std::string& text, int x, int y, uint32_t color) {
    int charWidth = FontData::FONT_CHAR_WIDTH * m_textScale;
    int xPos = x;
    for (size_t i = 0; i < text.length(); ++i) {
        drawChar(text[i], xPos, y, color);
        xPos += charWidth;
    }
}

void UIRenderer::renderToTexture() {
    SDL_Texture* oldTarget = SDL_GetRenderTarget(m_renderer);

    SDL_SetRenderTarget(m_renderer, m_uiTexture);

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);

    // FPS
    drawText(m_fpsText, 10, 10, 0xFFCCCCCC);

    // Brush info
    std::string info = "Brush: " + m_brushName + " (" + std::to_string(m_brushSize) + ")";
    drawText(info, 10, m_screenHeight - (FontData::FONT_CHAR_HEIGHT * m_textScale) - 10, 0xFFCCCCCC);

    SDL_SetRenderTarget(m_renderer, oldTarget);
}
