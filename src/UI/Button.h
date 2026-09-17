#pragma once
#include "UI/CanvasItem.h"
#include <string>
#include <functional>

class Button : public CanvasItem {
public:
    std::string text;
    std::function<void()> onClick;
    std::function<void(float)> onHold;

    bool active = false;

    struct Style {
        SDL_Color fill{40, 40, 40, 255};
        SDL_Color hoverFill{60, 60, 60, 255};
        SDL_Color pressedFill{30, 30, 30, 255};
        SDL_Color activeFill{80, 120, 80, 255};
        SDL_Color border{136, 153, 170, 255};
        SDL_Color textColor{255, 255, 255, 255};
        int borderThickness = 2;
    } style;

    bool handleEvent(const UIEvent& e) override {
        if (e.type == UIEventType::MouseDown && e.button == 0) {
            if (contains(e.pos)) {
                m_pressed = true;
                return true;
            }
        }
        if (e.type == UIEventType::MouseUp && e.button == 0) {
            bool wasPressed = m_pressed;
            m_pressed = false;
            if (wasPressed && contains(e.pos) && onClick) onClick();
            return contains(e.pos);
        }
        return false;
    }

    void update(float dt) override {
        if (m_pressed && onHold) onHold(dt);
    }

    void render(UIRenderer& r) override {
        SDL_Color fill = m_pressed ? style.pressedFill
                       : active    ? style.activeFill
                       : hovered   ? style.hoverFill
                       :             style.fill;
        r.drawRect(rect, fill);
        r.drawRectOutline(rect, style.border, style.borderThickness);
        r.drawTextCentered(text, rect, style.textColor);
    }

    bool isPressed() const { return m_pressed; }

private:
    bool m_pressed = false;
};
