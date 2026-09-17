#pragma once

#include "UI/UIEvent.h"
#include "UI/UIRenderer.h"
#include "Core/Math.h"

class CanvasItem {
public:
    virtual ~CanvasItem() = default;

    SDL_FRect rect{0, 0, 0, 0};
    bool visible = true;
    bool enabled = true;
    bool hovered = false;

    virtual bool handleEvent(const UIEvent&) { return false; }
    virtual void update(float) {}
    virtual void render(UIRenderer&) = 0;
    virtual bool contains(const Vec2f& p) const {
        return p.x >= rect.x && p.x <= rect.x + rect.w &&
               p.y >= rect.y && p.y <= rect.y + rect.h;
    }

    virtual void onMouseEnter() {}
    virtual void onMouseLeave() {}
};
