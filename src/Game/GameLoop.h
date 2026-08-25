#pragma once

#include <memory>
#include "World/World.h"
#include "Render/Renderer.h"
#include "Render/Camera.h"

class GameLoop {
public:
    GameLoop();
    ~GameLoop();

    void run();

private:
    void handleInput(float deltaTime);
    void update(float deltaTime);
    void render();

    ParticleRegistry m_registry;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Camera> m_camera;

    bool m_running = true;
    bool m_paused = false;

    ParticleId m_currentBrush;
    int m_brushRadius = 1;

    int m_fps = 0;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    Uint32 m_lastTime = 0;

    static constexpr int WORLD_WIDTH = 4000;
    static constexpr int WORLD_HEIGHT = 2000;
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
};
