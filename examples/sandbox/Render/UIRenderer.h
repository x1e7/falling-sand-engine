#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <string>
#include "FontData.h"

class UIRenderer {
public:
    UIRenderer(SDL_Renderer* renderer, int screenWidth, int screenHeight);
    ~UIRenderer();

    void renderToTexture();

    void setScreenSize(int width, int height);
    SDL_Texture* getTexture() const { return m_uiTexture; }

    void setFPS(int fps);
    void setBrushInfo(const std::string& name, int size) {
        m_brushName = name;
        m_brushSize = size;
    }

private:
    void createFontTexture();
    void drawChar(char c, int x, int y, uint32_t color);
    void drawText(const std::string& text, int x, int y, uint32_t color);

    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_fontTexture = nullptr;
    SDL_Texture* m_uiTexture = nullptr;

    std::string m_fpsText;

    int m_screenWidth = 0;
    int m_screenHeight = 0;
    int m_textScale = 2;

    int m_fps = 0;
    int m_brushSize = 1;

    std::string m_brushName = "Sand";
};
