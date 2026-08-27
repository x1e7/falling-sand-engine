#include "Renderer.h"
#include <iostream>
#include <algorithm>

Renderer::Renderer(int windowWidth, int windowHeight,
                   const std::string& title, ParticleRegistry& registry)
    : m_windowWidth(windowWidth)
    , m_windowHeight(windowHeight)
    , m_registry(registry) {

    SDL_Init(SDL_INIT_VIDEO);

    m_window = SDL_CreateWindow(title.c_str(),
                                m_windowWidth, m_windowHeight,
                                SDL_WINDOW_RESIZABLE);

    if (!m_window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);

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
    m_pixels.resize(static_cast<size_t>(width) * height);

    m_texture = SDL_CreateTexture(m_renderer,
                                  SDL_PIXELFORMAT_XRGB8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  width, height);

    SDL_SetTextureScaleMode(m_texture, SDL_SCALEMODE_NEAREST);
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
                uint32_t baseColor = reg.get(p->id).color;

                uint8_t r = (baseColor >> 16) & 0xFF;
                uint8_t g = (baseColor >> 8) & 0xFF;
                uint8_t b = baseColor & 0xFF;

                float bright = 0.9f + (p->brightness / 255.0f) * 0.2f;

                r = static_cast<uint8_t>(std::clamp(r * bright, 0.0f, 255.0f));
                g = static_cast<uint8_t>(std::clamp(g * bright, 0.0f, 255.0f));
                b = static_cast<uint8_t>(std::clamp(b * bright, 0.0f, 255.0f));
                m_pixels[row + (x - minX)] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    SDL_UpdateTexture(m_texture, nullptr, m_pixels.data(), viewW * sizeof(uint32_t));

    SDL_FRect dst = {0.0f, 0.0f, static_cast<float>(m_windowWidth), static_cast<float>(m_windowHeight)};
    SDL_RenderTexture(m_renderer, m_texture, nullptr, &dst);
}
