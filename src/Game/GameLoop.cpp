#include <Game/GameLoop.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <Serialization/WorldSerializer.h>

GameLoop::GameLoop() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    registerSand2DParticles(m_registry);
    m_registry.setBackgroundColor(0xFF1A1A2E);

    m_world = std::make_unique<World>(WORLD_WIDTH, WORLD_HEIGHT, m_registry);
    m_physics = std::make_unique<PhysicsSystem>();

    m_renderer = std::make_unique<Renderer>(WORLD_WIDTH, WORLD_HEIGHT,
                                            WINDOW_WIDTH, WINDOW_HEIGHT,
                                            "Sandbox - Physics Demo", m_registry);

    m_ui = std::make_unique<UIRenderer>(m_renderer->getRenderer(),
                                        WINDOW_WIDTH, WINDOW_HEIGHT);

    m_currentBrush = m_registry.findId("Sand");

    WorldSerializer::loadWorld(*m_world, "world.bin");
}

GameLoop::~GameLoop() {
    WorldSerializer::saveWorld(*m_world, "world.bin");
    SDL_Quit();
}

void GameLoop::run() {
    m_lastTime = SDL_GetTicks();

    while (m_running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = std::min((currentTime - m_lastTime) / 1000.0f, 0.05f);
        m_lastTime = currentTime;

        handleInput();

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

void GameLoop::handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_1: m_currentBrush = m_registry.findId("Sand"); break;
                    case SDLK_2: m_currentBrush = m_registry.findId("Water"); break;
                    case SDLK_3: m_currentBrush = m_registry.findId("Fire"); break;
                    case SDLK_4: m_currentBrush = m_registry.findId("Wall"); break;
                    case SDLK_5: m_currentBrush = m_registry.findId("Oil"); break;

                    case SDLK_SPACE: m_paused = !m_paused; break;
                    case SDLK_ESCAPE: m_running = false; break;
                }
                break;

            case SDL_MOUSEWHEEL:
                if (event.wheel.y > 0) {
                    m_brushRadius = std::min(m_brushRadius + 1, 128);
                } else if (event.wheel.y < 0) {
                    m_brushRadius = std::max(m_brushRadius - 1, 1);
                }
                break;
        }
    }

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_renderer->getWindow(), &windowWidth, &windowHeight);

    int wx = static_cast<int>((static_cast<float>(mouseX) / windowWidth) * m_world->getWidth());
    int wy = static_cast<int>((static_cast<float>(mouseY) / windowHeight) * m_world->getHeight());

    wx = std::max(0, std::min(wx, m_world->getWidth() - 1));
    wy = std::max(0, std::min(wy, m_world->getHeight() - 1));

    Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);

    if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        for (int dy = -m_brushRadius; dy <= m_brushRadius; ++dy) {
            for (int dx = -m_brushRadius; dx <= m_brushRadius; ++dx) {
                if (dx*dx + dy*dy > m_brushRadius*m_brushRadius) continue;
                int nx = wx + dx;
                int ny = wy + dy;
                if (m_world->isInside(nx, ny)) {
                    ParticleId current = m_world->getParticleId(nx, ny);
                    if (current == ParticleRegistry::Empty ||
                        current == m_registry.findId("Smoke")) {
                        m_world->setParticle(nx, ny, m_currentBrush);
                    }
                }
            }
        }
    }

    if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
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
    m_physics->update(*m_world, deltaTime);
}

void GameLoop::render() {
    m_ui->setFPS(m_fps);

    std::string brushName = "Unknown";
    if (m_currentBrush == m_registry.findId("Sand")) brushName = "Sand";
    else if (m_currentBrush == m_registry.findId("Water")) brushName = "Water";
    else if (m_currentBrush == m_registry.findId("Fire")) brushName = "Fire";
    else if (m_currentBrush == m_registry.findId("Wall")) brushName = "Wall";
    else if (m_currentBrush == m_registry.findId("Oil")) brushName = "Oil";
    m_ui->setBrushInfo(brushName, m_brushRadius);

    m_renderer->render(*m_world, m_ui.get());
}
