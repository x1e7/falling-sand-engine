#pragma once

#include <SDL3/SDL.h>
#include "World/World.h"

class UI {
public:
    UI(SDL_Window* window, SDL_Renderer* renderer);
    ~UI();

    void beginFrame();
    void endFrame(SDL_Renderer* renderer);

    void render(World& world,
                bool& paused, ParticleId& currentBrush, int& brushRadius,
                int fps,
                float msTotal, float msSim, float msRender,
                int worldWidth, int worldHeight);

    bool wantsInput() const;

private:
    void renderMainWindow(World& world,
                          bool& paused, ParticleId& currentBrush, int& brushRadius,
                          int fps, float msTotal, float msSim, float msRender,
                          int worldWidth, int worldHeight);
    void renderControlsWindow();
    void renderDemoWindow();

    bool m_showDemoWindow = false;
};
