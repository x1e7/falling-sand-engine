#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "World/World.h"
#include "Render/Camera.h"

class Renderer {
public:
    Renderer(int windowWidth, int windowHeight,
             const std::string& title, ParticleRegistry& registry);
    ~Renderer();

    void render(World& world, Camera& camera);
    void updateViewport(int width, int height);

    SDL_Renderer* getRenderer() const { return m_renderer; }

private:
    void createTexture(int width, int height);

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;

    int m_windowWidth;
    int m_windowHeight;
    int m_texWidth = 0;
    int m_texHeight = 0;

    std::vector<uint32_t> m_pixels;
    ParticleRegistry& m_registry;
};
