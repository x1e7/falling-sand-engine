#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <World/World.h>
#include <string>
#include <vector>
#include <functional>

class UIRenderer;

class Renderer {
public:
    Renderer(int worldWidth, int worldHeight, int windowWidth, int windowHeight,
             const std::string& title, ParticleRegistry& registry);
    ~Renderer();

    void render(World& world, UIRenderer* ui);

    bool isOpen() const { return m_isRunning; }
    SDL_Window* getWindow() { return m_window; }
    SDL_Renderer* getRenderer() const { return m_renderer; }

    void handleEvents(World* world = nullptr);

    void setWheelCallback(std::function<void(int)> callback) {
        m_wheelCallback = callback;
    }

    void getMouseWorldPosition(World& world, int& x, int& y) const;

private:
    void updateTexture(World& world);
    void updateViewport(int width, int height);

    std::function<void(int)> m_wheelCallback;

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;

    int m_windowWidth, m_windowHeight;
    int m_textureWidth, m_textureHeight;

    std::vector<uint32_t> m_pixelBuffer;
    ParticleRegistry& m_registry;

    bool m_isRunning = true;
};
