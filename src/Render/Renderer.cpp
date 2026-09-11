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

    buildColorLUT(registry);
}

Renderer::~Renderer() {
    if (m_texture) SDL_DestroyTexture(m_texture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Renderer::buildColorLUT(const ParticleRegistry& reg) {
    for (int id = 0; id < 256; ++id) {
        uint32_t c = reg.get(static_cast<ParticleId>(id)).color;
        uint8_t a = (c >> 24) & 0xFF;
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8) & 0xFF;
        uint8_t b = c & 0xFF;

        for (int br = 0; br < 256; ++br) {
            float bright = 0.9f + (br / 255.0f) * 0.2f;
            uint8_t rr = static_cast<uint8_t>(std::clamp(r * bright, 0.0f, 255.0f));
            uint8_t gg = static_cast<uint8_t>(std::clamp(g * bright, 0.0f, 255.0f));
            uint8_t bb = static_cast<uint8_t>(std::clamp(b * bright, 0.0f, 255.0f));
            m_colorLUT[id][br] = (a << 24) | (rr << 16) | (gg << 8) | bb;
        }
    }
}

void Renderer::createTexture(int width, int height) {
    if (m_texWidth == width && m_texHeight == height && m_texture) return;

    if (m_texture) SDL_DestroyTexture(m_texture);

    m_texWidth = width;
    m_texHeight = height;

    m_texture = SDL_CreateTexture(m_renderer,
                                  SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  width, height);

    SDL_SetTextureScaleMode(m_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);
}

void Renderer::updateViewport(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

void Renderer::render(World& world, Camera& camera) {
    int minX, minY, maxX, maxY;
    camera.getViewBounds(minX, minY, maxX, maxY);

    const int viewW = camera.getLogicalWidth();
    const int viewH = camera.getLogicalHeight();

    createTexture(viewW, viewH);

    uint32_t bg = (0xFF << 24) | (m_registry.get(ParticleRegistry::Empty).color & 0x00FFFFFF);

    void* pixels = nullptr;
    int pitch = 0;
    if (!SDL_LockTexture(m_texture, nullptr, &pixels, &pitch)) {
        return;
    }

    uint32_t* dst = static_cast<uint32_t*>(pixels);
    const int dstPitch = pitch / sizeof(uint32_t);

    const auto& reg = world.getRegistry();
    for (int y = 0; y < viewH; ++y) {
        uint32_t* row = dst + static_cast<size_t>(y) * dstPitch;
        int wy = minY + y;

        if (wy < 0 || wy >= world.getHeight()) {
            for (int x = 0; x < viewW; ++x) row[x] = bg;
            continue;
        }

        for (int x = 0; x < viewW; ++x) {
            int wx = minX + x;
            if (wx < 0 || wx >= world.getWidth()) {
                row[x] = bg;
                continue;
            }

            const ParticleInstance* p = world.getParticlePtr(wx, wy);
            if (p->id == ParticleRegistry::Empty) {
                row[x] = bg;
                continue;
            }

            row[x] = m_colorLUT[p->id][p->brightness];
        }
    }

    SDL_UnlockTexture(m_texture);

    uint8_t bgR = (bg >> 16) & 0xFF;
    uint8_t bgG = (bg >> 8) & 0xFF;
    uint8_t bgB = bg & 0xFF;
    SDL_SetRenderDrawColor(m_renderer, bgR, bgG, bgB, 255);
    SDL_RenderClear(m_renderer);

    SDL_FRect dstRect = {0.0f, 0.0f,
                         static_cast<float>(m_windowWidth),
                         static_cast<float>(m_windowHeight)};
    SDL_RenderTexture(m_renderer, m_texture, nullptr, &dstRect);
}
