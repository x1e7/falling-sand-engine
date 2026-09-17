#pragma once
#include "UI/CanvasItem.h"
#include <string>

class Label : public CanvasItem {
public:
    std::string text;
    SDL_Color color{255, 255, 255, 255};
    bool centered = false;

    void render(UIRenderer& r) override {
        if (centered) r.drawTextCentered(text, rect, color);
        else          r.drawText(text, rect.x, rect.y, color);
    }
};
