#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <Sand2D/World/World.h>
#include <string>
#include <vector>

class UIRenderer;

class Renderer {
public:
    Renderer(int worldWidth, int worldHeight, int windowWidth, int windowHeight,
             const std::string& title, Sand2D::ParticleRegistry& registry);
    ~Renderer();

    void render(Sand2D::World& world, UIRenderer* ui);
    void handleEvents();

    bool isOpen() const { return m_isRunning; }
    SDL_Window* getWindow() { return m_window; }
    SDL_Renderer* getRenderer() const { return m_renderer; }
    void getMouseWorldPosition(Sand2D::World& world, int& x, int& y) const;

private:
    void updateTexture(Sand2D::World& world);
    void updateViewport(int width, int height);

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;
    SDL_GLContext m_glContext = nullptr;

    int m_windowWidth, m_windowHeight;
    int m_textureWidth, m_textureHeight;

    std::vector<uint32_t> m_pixelBuffer;
    Sand2D::ParticleRegistry& m_registry;

    bool m_isRunning = true;
};
