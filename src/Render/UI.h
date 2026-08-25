#pragma once

#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "World/World.h"
#include "Render/Renderer.h"
#include "Serialization/WorldSerializer.h"

class UI {
public:
    UI(SDL_Window* window, SDL_Renderer* renderer);
    ~UI();

    void beginFrame();
    void endFrame(SDL_Renderer* renderer);

    void render(World& world, Renderer& renderer,
                bool& paused, ParticleId& currentBrush, int& brushRadius,
                int fps, int worldWidth, int worldHeight);

    bool wantsInput() const;

private:
    void renderMainWindow(World& world, Renderer& renderer,
                          bool& paused, ParticleId& currentBrush, int& brushRadius,
                          int fps, int worldWidth, int worldHeight);
    void renderControlsWindow();
    void renderDemoWindow();

    bool m_showDemoWindow = false;
};
