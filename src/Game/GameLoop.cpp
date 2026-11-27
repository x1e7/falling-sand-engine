#include "Game/GameLoop.h"
#include "Game/Config.h"
#include "Serialization/WorldSerializer.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <iterator>

static std::string basePath(const char* relative) {
    const char* base = SDL_GetBasePath();
    if (!base) return relative;
    return std::string(base) + relative;
}

GameLoop::GameLoop() {
    registerSand2DParticles(m_registry);
    m_registry.setBackgroundColor(0xFF2D2D2D);
    m_smokeId = m_registry.findId("Smoke");
    m_savePath = basePath("world.bin");

    m_world = std::make_unique<World>(WORLD_WIDTH, WORLD_HEIGHT, m_registry);
    m_renderer = std::make_unique<Renderer>(WINDOW_WIDTH, WINDOW_HEIGHT,
                                            "Sandbox - Physics Demo", m_registry);
    m_camera = std::make_unique<Camera>(WINDOW_WIDTH, WINDOW_HEIGHT);
    m_camera->setWorldBounds(WORLD_WIDTH, WORLD_HEIGHT);
    m_camera->setLogicalSize(WINDOW_WIDTH / 3, WINDOW_HEIGHT / 3);
    m_camera->setPosition(Vec2f(WORLD_WIDTH / 2.0f,
                                WORLD_HEIGHT - (m_camera->getLogicalHeight() / 2.0f)));

    m_currentBrush = cachedId("Sand");
    WorldSerializer::loadWorld(*m_world, m_savePath);

    m_uiRenderer = std::make_unique<UIRenderer>();
    m_uiRenderer->init(m_renderer->getRenderer(), basePath("assets/fonts/Pixuf.ttf"), 16);
    m_uiCanvas = std::make_unique<UICanvas>();

    m_fpsLabel    = addLabel(10.0f,  10.0f, 360.0f, "FPS: 0");
    m_totalLabel  = addLabel(10.0f,  34.0f, 420.0f, "total   : 0.00 ms");
    m_simLabel    = addLabel(10.0f,  54.0f, 420.0f, "sim     : 0.00 ms | 0 us/tick");
    m_renderLabel = addLabel(10.0f,  74.0f, 420.0f, "render  : 0.00 ms | present: 0.00 ms");
    m_chunksLabel = addLabel(10.0f,  94.0f, 420.0f, "chunks  : 0 / 0");
    m_brushLabel  = addLabel(10.0f, 114.0f, 420.0f, "brush   : Sand | radius: 1");

    auto pauseBtn = std::make_unique<Button>();
    pauseBtn->rect = {10.0f, 146.0f, 100.0f, 32.0f};
    pauseBtn->text = "Pause";
    pauseBtn->onClick = [this]() { togglePause(); };
    m_pauseBtn = pauseBtn.get();
    m_uiCanvas->addItem(std::move(pauseBtn));

    static const char* kBrushNames[] = {
        "Sand", "Water", "Fire", "Wall", "Oil",
        "Stone", "Lava", "Wood", "Acid", "Dust",
        "Plant", "Seed", "Gas"
    };

    constexpr float kBtnW = 60.0f;
    constexpr float kBtnH = 28.0f;
    constexpr float kBtnPad = 4.0f;
    constexpr int kBtnPerRow = 5;

    for (int i = 0; i < static_cast<int>(std::size(kBrushNames)); ++i) {
        ParticleId id = cachedId(kBrushNames[i]);
        if (id == ParticleRegistry::Empty) continue;

        auto btn = std::make_unique<Button>();
        btn->rect = {
            10.0f + static_cast<float>(i % kBtnPerRow) * (kBtnW + kBtnPad),
            190.0f + static_cast<float>(i / kBtnPerRow) * (kBtnH + kBtnPad),
            kBtnW, kBtnH
        };
        btn->text = kBrushNames[i];

        const std::string name = kBrushNames[i];
        btn->onClick = [this, id, name]() { setBrush(id, name); };

        m_brushButtons.push_back({btn.get(), id});
        m_uiCanvas->addItem(std::move(btn));
    }

    refreshBrushButtons();
    updateBrushLabel();
}

GameLoop::~GameLoop() {
    WorldSerializer::saveWorld(*m_world, m_savePath);
    if (m_uiRenderer) m_uiRenderer->destroy();
}

Label* GameLoop::addLabel(float x, float y, float w, const char* text) {
    auto label = std::make_unique<Label>();
    label->rect = {x, y, w, 20.0f};
    label->text = text;
    Label* raw = label.get();
    m_uiCanvas->addItem(std::move(label));
    return raw;
}

ParticleId GameLoop::cachedId(const char* name) {
    auto it = m_idCache.find(name);
    if (it != m_idCache.end()) return it->second;
    ParticleId id = m_registry.findId(name);
    m_idCache.emplace(name, id);
    return id;
}

void GameLoop::refreshBrushButtons() {
    for (auto& bb : m_brushButtons) {
        bb.button->active = (bb.id == m_currentBrush);
    }
}

void GameLoop::updateBrushLabel() {
    if (!m_brushLabel) return;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "brush   : %s | radius: %d",
                  m_currentBrushName.c_str(), m_brushRadius);
    m_brushLabel->text = buf;
}

void GameLoop::setBrush(ParticleId id, const std::string& name) {
    m_currentBrush = id;
    m_currentBrushName = name;
    refreshBrushButtons();
    updateBrushLabel();
}

void GameLoop::togglePause() {
    m_paused = !m_paused;
    if (m_pauseBtn) m_pauseBtn->text = m_paused ? "Resume" : "Pause";
}

void GameLoop::saveWorld() {
    WorldSerializer::saveWorld(*m_world, m_savePath);
    m_autosaveTimer = 0.0f;
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
        if (!m_paused) m_world->tick(deltaTime);
        Uint64 t2 = SDL_GetPerformanceCounter();

        render();
        Uint64 t3 = SDL_GetPerformanceCounter();

        SDL_RenderPresent(m_renderer->getRenderer());
        Uint64 t4 = SDL_GetPerformanceCounter();

        m_autosaveTimer += deltaTime;
        if (m_autosaveTimer >= AUTOSAVE_INTERVAL) saveWorld();

        updateDiagnostics(frameStart, t1, t2, t3, t4, deltaTime);
    }
}

void GameLoop::updateDiagnostics(Uint64 frameStart, Uint64 t1, Uint64 t2,
                                 Uint64 t3, Uint64 t4, float deltaTime) {
    m_sumSim     += (t2 - t1);
    m_sumRender  += (t3 - t2);
    m_sumPresent += (t4 - t3);
    m_sumTotal   += (t4 - frameStart);
    m_sumTicks   += static_cast<Uint64>(m_world->getTicksLastFrame());
    m_activeChunks = m_world->getActiveChunks();
    m_totalChunks  = m_world->getTotalChunks();
    m_diagFrames++;
    m_frameCount++;
    m_fpsTimer += deltaTime;

    if (m_fpsTimer >= 1.0f) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
        if (m_fpsLabel) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "FPS: %d  [%s]", m_fps,
                          m_renderer->isVSync() ? "vsync" : "free");
            m_fpsLabel->text = buf;
        }
    }

    Uint64 now = SDL_GetPerformanceCounter();
    if (now - m_diagStart >= m_perfFreq && m_diagFrames > 0) {
        const float f = static_cast<float>(m_perfFreq);
        const float frames = static_cast<float>(m_diagFrames);

        m_msTotal   = static_cast<float>(m_sumTotal)   * 1000.0f / f / frames;
        m_msSim     = static_cast<float>(m_sumSim)     * 1000.0f / f / frames;
        m_msRender  = static_cast<float>(m_sumRender)  * 1000.0f / f / frames;
        m_msPresent = static_cast<float>(m_sumPresent) * 1000.0f / f / frames;
        m_usPerTick = (m_sumTicks > 0)
            ? static_cast<float>(m_sumSim) * 1000000.0f / f / static_cast<float>(m_sumTicks)
            : 0.0f;

        char buf[160];

        if (m_totalLabel) {
            std::snprintf(buf, sizeof(buf), "total   : %.2f ms", m_msTotal);
            m_totalLabel->text = buf;
        }
        if (m_simLabel) {
            std::snprintf(buf, sizeof(buf), "sim     : %.2f ms | %.0f us/tick",
                          m_msSim, m_usPerTick);
            m_simLabel->text = buf;
        }
        if (m_renderLabel) {
            std::snprintf(buf, sizeof(buf), "render  : %.2f ms | present: %.2f ms",
                          m_msRender, m_msPresent);
            m_renderLabel->text = buf;
        }
        if (m_chunksLabel) {
            std::snprintf(buf, sizeof(buf), "chunks  : %d / %d",
                          m_activeChunks, m_totalChunks);
            m_chunksLabel->text = buf;
        }

        if (m_uiRenderer) m_uiRenderer->clearCache();

        m_sumSim = m_sumRender = m_sumPresent = m_sumTotal = m_sumTicks = 0;
        m_diagFrames = 0;
        m_diagStart = now;
    }
}

void GameLoop::paintBrush(int wx, int wy, ParticleId id) {
    const int r2 = m_brushRadius * m_brushRadius;
    const bool erase = (id == ParticleRegistry::Empty);

    for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy) {
        for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx) {
            if (dx * dx + dy * dy > r2) continue;
            const int nx = wx + dx, ny = wy + dy;
            if (!m_world->isInside(nx, ny)) continue;

            ParticleId current = m_world->getParticlePtr(nx, ny)->id;
            if (erase || current == ParticleRegistry::Empty || current == m_smokeId) {
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

            case SDL_EVENT_WINDOW_RESIZED: {
                int newWidth = event.window.data1;
                int newHeight = event.window.data2;
                m_camera->setViewportSize(newWidth, newHeight);
                m_renderer->updateViewport(newWidth, newHeight);
                break;
            }

            case SDL_EVENT_KEY_DOWN: {
                if (event.key.repeat) break;
                const bool ctrl = (event.key.mod & SDL_KMOD_CTRL) != 0;
                switch (event.key.scancode) {
                    case SDL_SCANCODE_1: setBrush(cachedId("Sand"),  "Sand");  break;
                    case SDL_SCANCODE_2: setBrush(cachedId("Water"), "Water"); break;
                    case SDL_SCANCODE_3: setBrush(cachedId("Fire"),  "Fire");  break;
                    case SDL_SCANCODE_4: setBrush(cachedId("Wall"),  "Wall");  break;
                    case SDL_SCANCODE_5: setBrush(cachedId("Oil"),   "Oil");   break;
                    case SDL_SCANCODE_6: setBrush(cachedId("Stone"), "Stone"); break;
                    case SDL_SCANCODE_7: setBrush(cachedId("Lava"),  "Lava");  break;
                    case SDL_SCANCODE_8: setBrush(cachedId("Wood"),  "Wood");  break;
                    case SDL_SCANCODE_9: setBrush(cachedId("Acid"),  "Acid");  break;
                    case SDL_SCANCODE_0: setBrush(cachedId("Dust"),  "Dust");  break;
                    case SDL_SCANCODE_SPACE: togglePause(); break;
                    case SDL_SCANCODE_ESCAPE: m_running = false; break;
                    case SDL_SCANCODE_F5: m_renderer->setVSync(!m_renderer->isVSync()); break;
                    case SDL_SCANCODE_S: if (ctrl) saveWorld(); break;
                    default: break;
                }
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
                if (event.wheel.y > 0) {
                    m_brushRadius = std::min(m_brushRadius + 1, 128);
                } else if (event.wheel.y < 0) {
                    m_brushRadius = std::max(m_brushRadius - 1, 1);
                }
                updateBrushLabel();
                break;

            default:
                break;
        }
    }

    float mx, my;
    SDL_MouseButtonFlags mouseState = SDL_GetMouseState(&mx, &my);
    Vec2f mouse{mx, my};

    m_uiCanvas->update(deltaTime, mouse);
    if (m_uiCanvas->isMouseCaptured()) return;

    const bool* keys = SDL_GetKeyboardState(nullptr);
    Vec2f moveDelta{0.0f, 0.0f};
    const float moveSpeed = 800.0f;

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    moveDelta.y -= moveSpeed;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  moveDelta.y += moveSpeed;
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  moveDelta.x -= moveSpeed;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) moveDelta.x += moveSpeed;
    if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) moveDelta = moveDelta * 2.0f;

    m_camera->move(moveDelta * deltaTime);

    Vec2f worldPos = m_camera->screenToWorld(Vec2f(mouse.x, mouse.y));
    int wx = static_cast<int>(worldPos.x);
    int wy = static_cast<int>(worldPos.y);

    if (mouseState & SDL_BUTTON_LMASK) paintBrush(wx, wy, m_currentBrush);
    if (mouseState & SDL_BUTTON_RMASK) paintBrush(wx, wy, ParticleRegistry::Empty);
}

void GameLoop::render() {
    m_renderer->render(*m_world, *m_camera);
    m_uiCanvas->render(*m_uiRenderer);
}
