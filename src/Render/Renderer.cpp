#include "Renderer.h"
#include <iostream>
#include <algorithm>

Renderer::Renderer(int worldWidth, int worldHeight, int windowWidth, int windowHeight,
                   const std::string& title, ParticleRegistry& registry)
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

void Renderer::render(World& world, Camera& camera, void* ui) {
    int minX, minY, maxX, maxY;
    camera.getViewBounds(minX, minY, maxX, maxY);

    uint32_t bgColor = m_registry.get(ParticleRegistry::Empty).color & 0x00FFFFFF;
    std::fill(m_pixelBuffer.begin(), m_pixelBuffer.end(), bgColor);

    const auto& registry = world.getRegistry();

    for (int y = minY; y < maxY; ++y) {
        size_t rowOffset = static_cast<size_t>(y) * m_textureWidth;

        for (int x = minX; x < maxX; ++x) {
            const auto& particle = world.getParticle(x, y);

            if (particle.id != ParticleRegistry::Empty) {
                uint32_t color = registry.get(particle.id).color;
                m_pixelBuffer[rowOffset + x] = color & 0x00FFFFFF;
            } else {
                m_pixelBuffer[rowOffset + x] = bgColor;
            }
        }
    }

    SDL_UpdateTexture(m_texture, nullptr, m_pixelBuffer.data(),
                      m_textureWidth * sizeof(uint32_t));

    SDL_RenderClear(m_renderer);

    int srcW = maxX - minX;
    int srcH = maxY - minY;

    SDL_Rect srcRect = {
        minX, minY,
        srcW, srcH
    };

    SDL_Rect dstRect = {
        0, 0,
        m_windowWidth,
        m_windowHeight
    };

    SDL_RenderCopy(m_renderer, m_texture, &srcRect, &dstRect);
}
