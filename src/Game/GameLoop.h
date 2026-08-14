#pragma once

#include <memory>
#include <World/World.h>
#include <Physics/PhysicsSystem.h>
#include <Render/Renderer.h>
#include <Render/UIRenderer.h>

class GameLoop {
public:
    GameLoop();
    ~GameLoop();

    void run();

private:
    void handleInput();
    void update(float deltaTime);
    void render();

    ParticleRegistry m_registry;
    std::unique_ptr<World> m_world;
    std::unique_ptr<PhysicsSystem> m_physics;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<UIRenderer> m_ui;

    bool m_running = true;
    bool m_paused = false;
    bool m_panning = false;
    Vec2f m_lastMousePos;

    ParticleId m_currentBrush;
    int m_brushRadius = 1;

    int m_fps = 0;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    Uint32 m_lastTime = 0;

    static constexpr int WORLD_WIDTH = 400;
    static constexpr int WORLD_HEIGHT = 300;
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
};
