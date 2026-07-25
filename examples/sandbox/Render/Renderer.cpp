#include "Renderer.h"
#include "UIRenderer.h"
#include <iostream>
#include <algorithm>

Renderer::Renderer(int worldWidth, int worldHeight, int windowWidth, int windowHeight,
                   const std::string& title, Sand2D::ParticleRegistry& registry)
    : m_windowWidth(windowWidth)
    , m_windowHeight(windowHeight)
    , m_textureWidth(worldWidth)
    , m_textureHeight(worldHeight)
    , m_registry(registry)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    m_window = SDL_CreateWindow(title.c_str(),
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                m_windowWidth, m_windowHeight,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (!m_window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_GL_SetSwapInterval(1);

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);


    if (!m_renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    m_texture = SDL_CreateTexture(m_renderer,
                                  SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  m_textureWidth, m_textureHeight);
    if (!m_texture) {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    m_pixelBuffer.resize(m_textureWidth * m_textureHeight, 0);
}

Renderer::~Renderer() {
    if (m_texture) SDL_DestroyTexture(m_texture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Renderer::updateViewport(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

void Renderer::updateTexture(Sand2D::World& world) {
    const int width = world.getWidth();
    const int height = world.getHeight();
    const auto& registry = world.getRegistry();

    if (m_pixelBuffer.size() != static_cast<size_t>(width * height)) {
        m_pixelBuffer.resize(width * height, 0);
        m_textureWidth = width;
        m_textureHeight = height;
    }

    world.forEachDirtyCell([&](int x, int y) {
        const auto& particle = world.getParticle(x, y);
        uint32_t color = registry.get(particle.id).color;
        m_pixelBuffer[y * width + x] = color & 0x00FFFFFF;
    });
    world.clearDirty();
}

void Renderer::render(Sand2D::World& world, UIRenderer* ui) {
    updateTexture(world);

    SDL_UpdateTexture(m_texture, nullptr, m_pixelBuffer.data(), m_textureWidth * sizeof(uint32_t));
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);

    if (ui) {
        ui->setScreenSize(m_windowWidth, m_windowHeight);
        ui->renderToTexture();

        SDL_SetTextureBlendMode(ui->getTexture(), SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(m_renderer, ui->getTexture(), nullptr, nullptr);
    }

    SDL_RenderPresent(m_renderer);
}

void Renderer::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_isRunning = false;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    updateViewport(event.window.data1, event.window.data2);
                }
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        m_isRunning = false;
                        break;
                    case SDLK_PLUS:
                    case SDLK_KP_PLUS:
                        m_wheelCallback(1);
                        break;
                    case SDLK_MINUS:
                    case SDLK_KP_MINUS:
                        m_wheelCallback(-1);
                        break;
                    case SDLK_EQUALS:
                        m_wheelCallback(1);
                        break;
                }
                break;
            case SDL_MOUSEWHEEL:
                if (m_wheelCallback) {
                    m_wheelCallback(event.wheel.y);
                }
                break;
        }
    }
}

void Renderer::getMouseWorldPosition(Sand2D::World& world, int& x, int& y) const {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    int windowWidth, windowHeight;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);

    float worldX = (static_cast<float>(mouseX) / windowWidth) * world.getWidth();
    float worldY = (static_cast<float>(mouseY) / windowHeight) * world.getHeight();

    x = static_cast<int>(worldX);
    y = static_cast<int>(worldY);

    x = std::max(0, std::min(x, world.getWidth() - 1));
    y = std::max(0, std::min(y, world.getHeight() - 1));
}
