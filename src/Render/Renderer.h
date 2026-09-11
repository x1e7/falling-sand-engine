#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <string>
#include "World/World.h"
#include "Render/Camera.h"

class Renderer {
public:
    Renderer(int windowWidth, int windowHeight,
             const std::string& title, ParticleRegistry& registry);
    ~Renderer();

    void buildColorLUT(const ParticleRegistry& reg);

    void render(World& world, Camera& camera);
    void updateViewport(int width, int height);

    SDL_Renderer* getRenderer() const { return m_renderer; }

    SDL_Window* getWindow() const { return m_window; }

private:
    void createTexture(int width, int height);

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;

    int m_windowWidth;
    int m_windowHeight;
    int m_texWidth = 0;
    int m_texHeight = 0;

    std::array<std::array<uint32_t, 256>, 256> m_colorLUT;
    ParticleRegistry& m_registry;
};
