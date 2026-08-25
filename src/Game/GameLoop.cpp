#include "Game/GameLoop.h"
#include "Serialization/WorldSerializer.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <algorithm>

GameLoop::GameLoop() {
    registerSand2DParticles(m_registry);
    m_registry.setBackgroundColor(0xFF1A1A2E);

    m_world = std::make_unique<World>(WORLD_WIDTH, WORLD_HEIGHT, m_registry);

    m_renderer = std::make_unique<Renderer>(WINDOW_WIDTH, WINDOW_HEIGHT,
                                            "Sandbox - Physics Demo", m_registry);

    m_camera = std::make_unique<Camera>(WINDOW_WIDTH, WINDOW_HEIGHT);
    m_camera->setWorldBounds(WORLD_WIDTH, WORLD_HEIGHT);
    m_camera->setLogicalSize(WINDOW_WIDTH / 3, WINDOW_HEIGHT / 3);
    m_camera->setPosition(Vec2f(WORLD_WIDTH / 2.0f, WORLD_HEIGHT - (m_camera->getLogicalHeight() / 2)));

    m_currentBrush = m_registry.findId("Sand");

    WorldSerializer::loadWorld(*m_world, "world.bin");
    m_world->initActiveChunks();

    m_ui = std::make_unique<UI>(m_renderer->getWindow(), m_renderer->getRenderer());
}

GameLoop::~GameLoop() {
    WorldSerializer::saveWorld(*m_world, "world.bin");
    SDL_Quit();
}

void GameLoop::run() {
    m_lastTime = SDL_GetTicks();

    while (m_running) {
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = std::min(static_cast<float>(currentTime - m_lastTime) / 1000.0f, 0.05f);
        m_lastTime = static_cast<Uint32>(currentTime);

        handleInput(deltaTime);

        if (!m_paused) {
            update(deltaTime);
        }

        render();

        m_frameCount++;
        m_fpsTimer += deltaTime;
        if (m_fpsTimer >= 1.0f) {
            m_fps = m_frameCount;
            m_frameCount = 0;
            m_fpsTimer = 0.0f;
        }
    }
}

void GameLoop::handleInput(float deltaTime) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                m_running = false;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                {
                    int newWidth = event.window.data1;
                    int newHeight = event.window.data2;
                    m_camera->setViewportSize(newWidth, newHeight);
                    m_renderer->updateViewport(newWidth, newHeight);
                }
                break;

            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key) {
                    case SDLK_1: m_currentBrush = m_registry.findId("Sand"); break;
                    case SDLK_2: m_currentBrush = m_registry.findId("Water"); break;
                    case SDLK_3: m_currentBrush = m_registry.findId("Fire"); break;
                    case SDLK_4: m_currentBrush = m_registry.findId("Wall"); break;
                    case SDLK_5: m_currentBrush = m_registry.findId("Oil"); break;

                    case SDLK_SPACE: m_paused = !m_paused; break;
                    case SDLK_ESCAPE: m_running = false; break;
                    case SDLK_F1: break; // handled by UI
                }
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                if (event.wheel.y > 0) {
                    m_brushRadius = std::min(m_brushRadius + 1, 128);
                } else if (event.wheel.y < 0) {
                    m_brushRadius = std::max(m_brushRadius - 1, 1);
                }
                break;

            default:
                break;
        }
    }

    if (m_ui->wantsInput()) return;

    const bool* keys = SDL_GetKeyboardState(nullptr);
    Vec2f moveDelta{0.0f, 0.0f};
    float moveSpeed = 800.0f / m_camera->getZoom();

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
        moveDelta.y -= moveSpeed;
    }
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
        moveDelta.y += moveSpeed;
    }
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
        moveDelta.x -= moveSpeed;
    }
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
        moveDelta.x += moveSpeed;
    }
    if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
        moveDelta = moveDelta * 2.0f;
    }

    m_camera->move(moveDelta * deltaTime);

    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    Vec2f worldPos = m_camera->screenToWorld(Vec2f(mouseX, mouseY));
    int wx = static_cast<int>(worldPos.x);
    int wy = static_cast<int>(worldPos.y);

    SDL_MouseButtonFlags mouseState = SDL_GetMouseState(nullptr, nullptr);

    if (mouseState & SDL_BUTTON_LMASK) {
        for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy) {
            for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx) {
                if (dx*dx + dy*dy > m_brushRadius*m_brushRadius) continue;
                int nx = wx + dx;
                int ny = wy + dy;

                if (!m_world->isInside(nx, ny)) continue;

                ParticleId current = m_world->getParticlePtr(nx, ny)->id;

                if (current == ParticleRegistry::Empty || current == m_registry.findId("Smoke")) {
                    m_world->setParticle(nx, ny, m_currentBrush);
                }
            }
        }
    }

    if (mouseState & SDL_BUTTON_RMASK) {
        for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy) {
            for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx) {
                if (dx*dx + dy*dy > m_brushRadius*m_brushRadius) continue;
                int nx = wx + dx;
                int ny = wy + dy;
                if (m_world->isInside(nx, ny)) {
                    m_world->setParticle(nx, ny, ParticleRegistry::Empty);
                }
            }
        }
    }
}

void GameLoop::update(float deltaTime) {
    m_camera->update(deltaTime);
    m_world->tick(deltaTime);
}

void GameLoop::render() {
    SDL_Renderer* renderer = m_renderer->getRenderer();

    m_ui->beginFrame();

    SDL_RenderClear(renderer);
    m_renderer->render(*m_world, *m_camera);

    m_ui->render(*m_world, *m_renderer, m_paused, m_currentBrush, m_brushRadius,
                 m_fps, WORLD_WIDTH, WORLD_HEIGHT);

    m_ui->endFrame(renderer);

    SDL_RenderPresent(renderer);
}
