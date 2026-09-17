#include "UI/UIRenderer.h"
#include <iostream>

bool UIRenderer::init(SDL_Renderer* renderer, const std::string& fontPath, int fontSize) {
    if (!renderer) {
        std::cerr << "[UIRenderer] renderer is null\n";
        return false;
    }
    m_renderer = renderer;

    if (TTF_WasInit() == 0) {
        if (!TTF_Init()) {
            std::cerr << "[UIRenderer] TTF_Init failed: " << SDL_GetError() << "\n";
            return false;
        }
    }

    m_font = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (!m_font) {
        std::cerr << "[UIRenderer] TTF_OpenFont failed: " << SDL_GetError() << "\n";
        return false;
    }

    m_fontSize = fontSize;
    return true;
}

void UIRenderer::destroy() {
    clearCache();

    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    m_renderer = nullptr;

    if (TTF_WasInit()) TTF_Quit();
}

void UIRenderer::clearCache() {
    for (auto& [key, tex] : m_textCache) {
        if (tex) SDL_DestroyTexture(tex);
    }
    m_textCache.clear();
}

void UIRenderer::drawRect(const SDL_FRect& rect, SDL_Color color) {
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(m_renderer, &rect);
}

void UIRenderer::drawRectOutline(const SDL_FRect& rect, SDL_Color color, int thickness) {
    if (thickness <= 0) return;

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);

    float t = static_cast<float>(thickness);
    SDL_FRect top    = { rect.x, rect.y, rect.w, t };
    SDL_FRect bottom = { rect.x, rect.y + rect.h - t, rect.w, t };
    SDL_FRect left   = { rect.x, rect.y, t, rect.h };
    SDL_FRect right  = { rect.x + rect.w - t, rect.y, t, rect.h };

    SDL_RenderFillRect(m_renderer, &top);
    SDL_RenderFillRect(m_renderer, &bottom);
    SDL_RenderFillRect(m_renderer, &left);
    SDL_RenderFillRect(m_renderer, &right);
}

void UIRenderer::drawLine(float x1, float y1, float x2, float y2, SDL_Color color) {
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderLine(m_renderer, x1, y1, x2, y2);
}

Vec2f UIRenderer::measureText(const std::string& text) const {
    if (!m_font || text.empty()) return {0.0f, 0.0f};

    int w = 0, h = 0;
    if (!TTF_GetStringSize(m_font, text.c_str(), text.size(), &w, &h)) {
        return {0.0f, 0.0f};
    }
    return { static_cast<float>(w), static_cast<float>(h) };
}

SDL_Texture* UIRenderer::getTextTexture(const std::string& text, SDL_Color color) {
    TextKey key{ text, packColor(color) };

    auto it = m_textCache.find(key);
    if (it != m_textCache.end()) return it->second;

    if (text.empty()) {
        m_textCache[key] = nullptr;
        return nullptr;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), text.size(), color);
    if (!surface) {
        std::cerr << "[UIRenderer] TTF_RenderText_Blended failed: " << SDL_GetError() << "\n";
        m_textCache[key] = nullptr;
        return nullptr;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_DestroySurface(surface);

    if (!tex) {
        std::cerr << "[UIRenderer] SDL_CreateTextureFromSurface failed: " << SDL_GetError() << "\n";
        m_textCache[key] = nullptr;
        return nullptr;
    }

    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    m_textCache[key] = tex;
    return tex;
}

void UIRenderer::drawText(const std::string& text, float x, float y, SDL_Color color) {
    SDL_Texture* tex = getTextTexture(text, color);
    if (!tex) return;

    float w = 0, h = 0;
    SDL_GetTextureSize(tex, &w, &h);

    SDL_FRect dst{ x, y, w, h };
    SDL_RenderTexture(m_renderer, tex, nullptr, &dst);
}

void UIRenderer::drawTextCentered(const std::string& text, const SDL_FRect& rect, SDL_Color color) {
    SDL_Texture* tex = getTextTexture(text, color);
    if (!tex) return;

    float w = 0, h = 0;
    SDL_GetTextureSize(tex, &w, &h);

    SDL_FRect dst{
        rect.x + (rect.w - w) * 0.5f,
        rect.y + (rect.h - h) * 0.5f,
        w, h
    };
    SDL_RenderTexture(m_renderer, tex, nullptr, &dst);
}

void UIRenderer::draw9Slice(SDL_Texture* tex, const SDL_FRect& dst, int cornerSize) {
    if (!tex || cornerSize <= 0) return;

    float texW = 0, texH = 0;
    SDL_GetTextureSize(tex, &texW, &texH);

    const float c = static_cast<float>(cornerSize);
    const float srcMidW = texW - c * 2.0f;
    const float srcMidH = texH - c * 2.0f;
    const float dstMidW = dst.w - c * 2.0f;
    const float dstMidH = dst.h - c * 2.0f;

    if (srcMidW <= 0 || srcMidH <= 0 || dstMidW < 0 || dstMidH < 0) return;

    SDL_FRect srcTL{0, 0, c, c};
    SDL_FRect dstTL{dst.x, dst.y, c, c};
    SDL_RenderTexture(m_renderer, tex, &srcTL, &dstTL);

    SDL_FRect srcTR{texW - c, 0, c, c};
    SDL_FRect dstTR{dst.x + dst.w - c, dst.y, c, c};
    SDL_RenderTexture(m_renderer, tex, &srcTR, &dstTR);

    SDL_FRect srcBL{0, texH - c, c, c};
    SDL_FRect dstBL{dst.x, dst.y + dst.h - c, c, c};
    SDL_RenderTexture(m_renderer, tex, &srcBL, &dstBL);

    SDL_FRect srcBR{texW - c, texH - c, c, c};
    SDL_FRect dstBR{dst.x + dst.w - c, dst.y + dst.h - c, c, c};
    SDL_RenderTexture(m_renderer, tex, &srcBR, &dstBR);

    SDL_FRect srcT{c, 0, srcMidW, c};
    SDL_FRect dstT{dst.x + c, dst.y, dstMidW, c};
    SDL_RenderTexture(m_renderer, tex, &srcT, &dstT);

    SDL_FRect srcB{c, texH - c, srcMidW, c};
    SDL_FRect dstB{dst.x + c, dst.y + dst.h - c, dstMidW, c};
    SDL_RenderTexture(m_renderer, tex, &srcB, &dstB);

    SDL_FRect srcL{0, c, c, srcMidH};
    SDL_FRect dstL{dst.x, dst.y + c, c, dstMidH};
    SDL_RenderTexture(m_renderer, tex, &srcL, &dstL);

    SDL_FRect srcR{texW - c, c, c, srcMidH};
    SDL_FRect dstR{dst.x + dst.w - c, dst.y + c, c, dstMidH};
    SDL_RenderTexture(m_renderer, tex, &srcR, &dstR);

    SDL_FRect srcC{c, c, srcMidW, srcMidH};
    SDL_FRect dstC{dst.x + c, dst.y + c, dstMidW, dstMidH};
    SDL_RenderTexture(m_renderer, tex, &srcC, &dstC);
}
