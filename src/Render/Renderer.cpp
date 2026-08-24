#include "Renderer.h"
#include <iostream>
#include <algorithm>

Renderer::Renderer(int windowWidth, int windowHeight,
                   const std::string& title, ParticleRegistry& registry)
    : m_windowWidth(windowWidth)
    , m_windowHeight(windowHeight)
    , m_registry(registry) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    m_window = SDL_CreateWindow(title.c_str(),
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                m_windowWidth, m_windowHeight,
                                SDL_WINDOW_RESIZABLE);

    if (!m_window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    m_renderer = SDL_CreateRenderer(m_window, -1,
                                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!m_renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

Renderer::~Renderer() {
    if (m_texture) SDL_DestroyTexture(m_texture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Renderer::createTexture(int width, int height) {
    if (m_texWidth == width && m_texHeight == height && m_texture) return;

    if (m_texture) SDL_DestroyTexture(m_texture);

    m_texWidth = width;
    m_texHeight = height;
    m_pixels.resize(width * height);

    m_texture = SDL_CreateTexture(m_renderer,
                                  SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  width, height);
}

void Renderer::updateViewport(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

void Renderer::render(World& world, Camera& camera) {
    int minX, minY, maxX, maxY;
    camera.getViewBounds(minX, minY, maxX, maxY);

    int viewW = maxX - minX;
    int viewH = maxY - minY;

    createTexture(viewW, viewH);

    uint32_t bg = m_registry.get(ParticleRegistry::Empty).color & 0x00FFFFFF;
    std::fill(m_pixels.begin(), m_pixels.end(), bg);

    const auto& reg = world.getRegistry();
    for (int y = minY; y < maxY; ++y) {
        size_t row = static_cast<size_t>(y - minY) * viewW;
        for (int x = minX; x < maxX; ++x) {
            const ParticleInstance* p = world.getParticlePtr(x, y);
            if (p->id != ParticleRegistry::Empty) {
                m_pixels[row + (x - minX)] = reg.get(p->id).color & 0x00FFFFFF;
            }
        }
    }

    SDL_UpdateTexture(m_texture, nullptr, m_pixels.data(), viewW * sizeof(uint32_t));

    SDL_Rect dst = {0, 0, m_windowWidth, m_windowHeight};
    SDL_RenderCopy(m_renderer, m_texture, nullptr, &dst);
}
