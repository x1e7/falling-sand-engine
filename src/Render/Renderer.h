#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "World/World.h"
#include "Render/Camera.h"
#include <string>
#include <vector>
#include <functional>

class Renderer {
public:
    Renderer(int worldWidth, int worldHeight, int windowWidth, int windowHeight,
             const std::string& title, ParticleRegistry& registry);
    ~Renderer();

    void updateViewport(int width, int height);

    void render(World& world, Camera& camera, void* ui = nullptr);

    void setPreserveAspectRatio(bool preserve) { m_preserveAspectRatio = preserve; }
    bool getPreserveAspectRatio() const { return m_preserveAspectRatio; }

    bool isOpen() const { return m_isRunning; }
    SDL_Window* getWindow() { return m_window; }
    SDL_Renderer* getRenderer() const { return m_renderer; }

    void setWheelCallback(std::function<void(int)> callback) {
        m_wheelCallback = callback;
    }

private:
    std::function<void(int)> m_wheelCallback;

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;

    int m_windowWidth, m_windowHeight;
    int m_textureWidth, m_textureHeight;

    std::vector<uint32_t> m_pixelBuffer;
    ParticleRegistry& m_registry;

    bool m_isRunning = true;
    bool m_preserveAspectRatio = false;
};
