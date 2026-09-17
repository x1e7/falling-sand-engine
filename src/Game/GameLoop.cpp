#include "Game/GameLoop.h"
#include "Game/Config.h"
#include "Serialization/WorldSerializer.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>

GameLoop::GameLoop() {
    registerSand2DParticles(m_registry);
    m_registry.setBackgroundColor(0xFF2D2D2D);

    m_world = std::make_unique<World>(WORLD_WIDTH, WORLD_HEIGHT, m_registry);

    m_renderer = std::make_unique<Renderer>(WINDOW_WIDTH, WINDOW_HEIGHT,
                                            "Sandbox - Physics Demo", m_registry);

    m_camera = std::make_unique<Camera>(WINDOW_WIDTH, WINDOW_HEIGHT);
    m_camera->setWorldBounds(WORLD_WIDTH, WORLD_HEIGHT);
    m_camera->setLogicalSize(WINDOW_WIDTH / 3, WINDOW_HEIGHT / 3);
    m_camera->setPosition(Vec2f(WORLD_WIDTH / 2.0f, WORLD_HEIGHT - (m_camera->getLogicalHeight() / 2.0f)));

    m_currentBrush = m_registry.findId("Sand");

    WorldSerializer::loadWorld(*m_world, "world.bin");

    m_uiRenderer = std::make_unique<UIRenderer>();
    m_uiRenderer->init(m_renderer->getRenderer(), "assets/fonts/Pixuf.ttf", 16);

    m_uiCanvas = std::make_unique<UICanvas>();

    // --- FPS label ---
    auto fpsLabel = std::make_unique<Label>();
    fpsLabel->rect = {10, 10, 200, 24};
    fpsLabel->text = "FPS: 0";
    m_fpsLabel = fpsLabel.get();
    m_uiCanvas->addItem(std::move(fpsLabel));

    // --- ms label ---
    auto msLabel = std::make_unique<Label>();
    msLabel->rect = {10, 34, 260, 60};
    msLabel->text = "total: 0.00 ms\nsim: 0.00\nrender: 0.00";
    m_msLabel = msLabel.get();
    m_uiCanvas->addItem(std::move(msLabel));

    // --- Pause button ---
    auto pauseBtn = std::make_unique<Button>();
    pauseBtn->rect = {10, 100, 100, 32};
    pauseBtn->text = "Pause";
    pauseBtn->onClick = [this]() {
        m_paused = !m_paused;
        if (m_pauseBtn) m_pauseBtn->text = m_paused ? "Resume" : "Pause";
    };
    m_pauseBtn = pauseBtn.get();
    m_uiCanvas->addItem(std::move(pauseBtn));

    // --- Brush buttons ---
    static const char* kBrushNames[] = {
        "Sand", "Water", "Fire", "Wall", "Oil",
        "Stone", "Lava", "Wood", "Acid", "Dust",
        "Plant", "Seed", "Gas"
    };

    constexpr float kBtnW = 60.0f;
    constexpr float kBtnH = 28.0f;
    constexpr float kBtnPad = 4.0f;
    constexpr int   kBtnPerRow = 5;

    for (int i = 0; i < static_cast<int>(std::size(kBrushNames)); ++i) {
        ParticleId id = m_registry.findId(kBrushNames[i]);
        if (id == ParticleRegistry::Empty) continue;

        auto btn = std::make_unique<Button>();
        btn->rect = {
            10.0f + (i % kBtnPerRow) * (kBtnW + kBtnPad),
            145.0f + (i / kBtnPerRow) * (kBtnH + kBtnPad),
            kBtnW, kBtnH
        };
        btn->text = kBrushNames[i];

        const std::string name = kBrushNames[i];
        btn->onClick = [this, id, name]() {
            setBrush(id, name);
        };

        Button* raw = btn.get();
        m_brushButtons.push_back({ raw, id });
        m_uiCanvas->addItem(std::move(btn));
    }

    refreshBrushButtons();
}

GameLoop::~GameLoop() {
    WorldSerializer::saveWorld(*m_world, "world.bin");
    if (m_uiRenderer) m_uiRenderer->destroy();
}

void GameLoop::refreshBrushButtons() {
    for (auto& bb : m_brushButtons) {
        bb.button->active = (bb.id == m_currentBrush);
    }
}

void GameLoop::setBrush(ParticleId id, const std::string& name) {
    m_currentBrush = id;
    m_currentBrushName = name;
    refreshBrushButtons();
}

void GameLoop::run() {
    m_perfFreq = SDL_GetPerformanceFrequency();
    m_diagStart = SDL_GetPerformanceCounter();
    m_lastTime = SDL_GetTicks();

    while (m_running) {
        Uint64 frameStart = SDL_GetPerformanceCounter();

        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = std::min(static_cast<float>(currentTime - m_lastTime) / 1000.0f, 0.05f);
        m_lastTime = static_cast<Uint32>(currentTime);

        handleInput(deltaTime);

        Uint64 t1 = SDL_GetPerformanceCounter();
        m_camera->update(deltaTime);
        if (!m_paused) {
            m_world->tick(deltaTime);
        }
        Uint64 t2 = SDL_GetPerformanceCounter();

        render();
        Uint64 t3 = SDL_GetPerformanceCounter();

        SDL_RenderPresent(m_renderer->getRenderer());
        Uint64 t4 = SDL_GetPerformanceCounter();

        m_sumSim    += (t2 - t1);
        m_sumRender += (t3 - t2);
        m_sumTotal  += (t4 - frameStart);
        m_diagFrames++;

        m_frameCount++;
        m_fpsTimer += deltaTime;
        if (m_fpsTimer >= 1.0f) {
            m_fps = m_frameCount;
            m_frameCount = 0;
            m_fpsTimer = 0.0f;

            if (m_fpsLabel) {
                m_fpsLabel->text = "FPS: " + std::to_string(m_fps);
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        if (now - m_diagStart >= m_perfFreq) {
            float f = static_cast<float>(m_perfFreq);
            m_msSim    = m_sumSim    * 1000.0f / f / m_diagFrames;
            m_msRender = m_sumRender * 1000.0f / f / m_diagFrames;
            m_msTotal  = m_sumTotal  * 1000.0f / f / m_diagFrames;

            if (m_msLabel) {
                char buf[128];
                std::snprintf(buf, sizeof(buf),
                              "total: %.2f ms\nsim: %.2f\nrender: %.2f",
                              m_msTotal, m_msSim, m_msRender);
                m_msLabel->text = buf;
            }

            m_sumSim = m_sumRender = m_sumTotal = 0;
            m_diagFrames = 0;
            m_diagStart = now;
        }
    }
}

void GameLoop::paintBrush(int wx, int wy, ParticleId id) {
    const int r2 = m_brushRadius * m_brushRadius;
    for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy) {
        for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx) {
            if (dx*dx + dy*dy > r2) continue;
            const int nx = wx + dx, ny = wy + dy;
            if (!m_world->isInside(nx, ny)) continue;

            ParticleId current = m_world->getParticlePtr(nx, ny)->id;

            if (id == ParticleRegistry::Empty || (current == ParticleRegistry::Empty || current == m_registry.findId("Smoke"))) {
                m_world->setParticle(nx, ny, id);
            }
        }
    }
}

void GameLoop::handleInput(float deltaTime) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (m_uiCanvas->handleSDLEvent(event)) continue;

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
                    case SDLK_1: setBrush(m_registry.findId("Sand"),  "Sand");  break;
                    case SDLK_2: setBrush(m_registry.findId("Water"), "Water"); break;
                    case SDLK_3: setBrush(m_registry.findId("Fire"),  "Fire");  break;
                    case SDLK_4: setBrush(m_registry.findId("Wall"),  "Wall");  break;
                    case SDLK_5: setBrush(m_registry.findId("Oil"),   "Oil");   break;

                    case SDLK_SPACE:
                        m_paused = !m_paused;
                        if (m_pauseBtn) m_pauseBtn->text = m_paused ? "Resume" : "Pause";
                        break;
                    case SDLK_ESCAPE: m_running = false; break;
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

    float mx, my;
    SDL_MouseButtonFlags mouseState = SDL_GetMouseState(&mx, &my);
    Vec2f mouse{ mx, my };

    m_uiCanvas->update(deltaTime, mouse);
    if (m_uiCanvas->isMouseCaptured()) return;

    const bool* keys = SDL_GetKeyboardState(nullptr);
    Vec2f moveDelta{0.0f, 0.0f};
    float moveSpeed = 800.0f;

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    moveDelta.y -= moveSpeed;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  moveDelta.y += moveSpeed;
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  moveDelta.x -= moveSpeed;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) moveDelta.x += moveSpeed;
    if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
        moveDelta = moveDelta * 2.0f;
    }

    m_camera->move(moveDelta * deltaTime);

    Vec2f worldPos = m_camera->screenToWorld(Vec2f(mouse.x, mouse.y));
    int wx = static_cast<int>(worldPos.x);
    int wy = static_cast<int>(worldPos.y);

    if (mouseState & SDL_BUTTON_LMASK) {
        paintBrush(wx, wy, m_currentBrush);
    }
    if (mouseState & SDL_BUTTON_RMASK) {
        paintBrush(wx, wy, ParticleRegistry::Empty);
    }
}

void GameLoop::render() {
    m_renderer->render(*m_world, *m_camera);
    m_uiCanvas->render(*m_uiRenderer);
}
