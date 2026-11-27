#include "Renderer.h"

#include <iostream>
#include <algorithm>

Renderer::Renderer(int windowWidth, int windowHeight,
                   const std::string& title, ParticleRegistry& registry)
    : m_windowWidth(windowWidth)
    , m_windowHeight(windowHeight)
    , m_registry(registry) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

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

    setVSync(false);
    buildColorLUT(registry);
}

Renderer::~Renderer() {
    if (m_texture) SDL_DestroyTexture(m_texture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Renderer::setVSync(bool enabled) {
    m_vsync = enabled;
    if (!SDL_SetRenderVSync(m_renderer, enabled ? 1 : 0)) {
        std::cerr << "SDL_SetRenderVSync failed: " << SDL_GetError() << std::endl;
    }
}

void Renderer::buildColorLUT(const ParticleRegistry& reg) {
    for (int id = 0; id < 256; ++id) {
        uint32_t c = reg.get(static_cast<ParticleId>(id)).color;
        uint8_t a = static_cast<uint8_t>((c >> 24) & 0xFF);
        uint8_t r = static_cast<uint8_t>((c >> 16) & 0xFF);
        uint8_t g = static_cast<uint8_t>((c >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>(c & 0xFF);

        for (int br = 0; br < 256; ++br) {
            float bright = 0.9f + (static_cast<float>(br) / 255.0f) * 0.2f;
            uint8_t rr = static_cast<uint8_t>(std::clamp(static_cast<float>(r) * bright, 0.0f, 255.0f));
            uint8_t gg = static_cast<uint8_t>(std::clamp(static_cast<float>(g) * bright, 0.0f, 255.0f));
            uint8_t bb = static_cast<uint8_t>(std::clamp(static_cast<float>(b) * bright, 0.0f, 255.0f));
            m_colorLUT[id][br] = (static_cast<uint32_t>(a) << 24)
                               | (static_cast<uint32_t>(rr) << 16)
                               | (static_cast<uint32_t>(gg) << 8)
                               | static_cast<uint32_t>(bb);
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
    if (!m_texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
        return;
    }
    SDL_SetTextureScaleMode(m_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_NONE);
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
    if (!m_texture) return;

    const uint32_t bg = 0xFF000000u | (m_registry.get(ParticleRegistry::Empty).color & 0x00FFFFFFu);

    void* pixels = nullptr;
    int pitch = 0;
    if (!SDL_LockTexture(m_texture, nullptr, &pixels, &pitch)) return;

    uint32_t* dst = static_cast<uint32_t*>(pixels);
    const size_t dstPitch = static_cast<size_t>(pitch) / sizeof(uint32_t);
    const int worldW = world.getWidth();
    const int worldH = world.getHeight();

    for (int y = 0; y < viewH; ++y) {
        uint32_t* row = dst + static_cast<size_t>(y) * dstPitch;
        int wy = minY + y;

        if (wy < 0 || wy >= worldH) {
            std::fill(row, row + viewW, bg);
            continue;
        }

        int x = 0;
        int leftFill = std::clamp(-minX, 0, viewW);
        if (leftFill > 0) {
            std::fill(row, row + leftFill, bg);
            x = leftFill;
        }

        int rightStart = std::clamp(worldW - minX, 0, viewW);
        for (; x < rightStart; ++x) {
            const ParticleInstance* p = world.getParticlePtr(minX + x, wy);
            row[x] = (p->id == ParticleRegistry::Empty) ? bg : m_colorLUT[p->id][p->brightness];
        }
        if (x < viewW) std::fill(row + x, row + viewW, bg);
    }

    SDL_UnlockTexture(m_texture);

    SDL_SetRenderDrawColor(m_renderer,
                           static_cast<uint8_t>((bg >> 16) & 0xFF),
                           static_cast<uint8_t>((bg >> 8) & 0xFF),
                           static_cast<uint8_t>(bg & 0xFF),
                           255);
    SDL_RenderClear(m_renderer);

    SDL_FRect dstRect = {0.0f, 0.0f,
                         static_cast<float>(m_windowWidth),
                         static_cast<float>(m_windowHeight)};
    SDL_RenderTexture(m_renderer, m_texture, nullptr, &dstRect);
}
