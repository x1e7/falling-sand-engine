#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include "World/World.h"
#include "Render/Renderer.h"
#include "Render/Camera.h"
#include "UI/UIRenderer.h"
#include "UI/UICanvas.h"
#include "UI/Label.h"
#include "UI/Button.h"

class GameLoop {
public:
    GameLoop();
    ~GameLoop();

    void run();
    void paintBrush(int wx, int wy, ParticleId id);

private:
    void handleInput(float deltaTime);
    void render();

    ParticleRegistry m_registry;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Camera> m_camera;

    std::unique_ptr<UIRenderer> m_uiRenderer;
    std::unique_ptr<UICanvas> m_uiCanvas;
    Label* m_fpsLabel = nullptr;
    Button* m_pauseBtn = nullptr;

    bool m_running = true;
    bool m_paused = false;

    ParticleId m_currentBrush;
    int m_brushRadius = 1;

    int m_fps = 0;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    Uint32 m_lastTime = 0;

    float m_msTotal = 0.0f;
    float m_msSim = 0.0f;
    float m_msRender = 0.0f;

    Uint64 m_sumSim = 0;
    Uint64 m_sumRender = 0;
    Uint64 m_sumTotal = 0;
    int m_diagFrames = 0;
    Uint64 m_diagStart = 0;
    Uint64 m_perfFreq = 0;
};
