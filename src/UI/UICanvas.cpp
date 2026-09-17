#include "UI/UICanvas.h"
#include <algorithm>

// ---- SDL -> UIEvent ----
static bool convertSDLEvent(const SDL_Event& sdl, UIEvent& out) {
    switch (sdl.type) {
        case SDL_EVENT_MOUSE_MOTION:
            out.type = UIEventType::MouseMove;
            out.pos  = { sdl.motion.x, sdl.motion.y };
            return true;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            out.type   = UIEventType::MouseDown;
            out.pos    = { sdl.button.x, sdl.button.y };
            out.button = (sdl.button.button == SDL_BUTTON_LEFT)  ? 0 :
                         (sdl.button.button == SDL_BUTTON_RIGHT) ? 1 : 2;
            return true;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            out.type   = UIEventType::MouseUp;
            out.pos    = { sdl.button.x, sdl.button.y };
            out.button = (sdl.button.button == SDL_BUTTON_LEFT)  ? 0 :
                         (sdl.button.button == SDL_BUTTON_RIGHT) ? 1 : 2;
            return true;

        case SDL_EVENT_MOUSE_WHEEL:
            out.type  = UIEventType::MouseWheel;
            out.pos   = { sdl.wheel.mouse_x, sdl.wheel.mouse_y };
            out.wheel = sdl.wheel.y;
            return true;

        case SDL_EVENT_KEY_DOWN:
            out.type = UIEventType::KeyDown;
            out.key  = sdl.key.scancode;
            return true;

        case SDL_EVENT_KEY_UP:
            out.type = UIEventType::KeyUp;
            out.key  = sdl.key.scancode;
            return true;

        default:
            return false;
    }
}

// ---- UICanvas ----
void UICanvas::addItem(std::unique_ptr<CanvasItem> item) {
    m_items.push_back(std::move(item));
}

void UICanvas::removeItem(CanvasItem* item) {
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [item](const std::unique_ptr<CanvasItem>& p) { return p.get() == item; });
    if (it != m_items.end()) m_items.erase(it);
}

void UICanvas::clear() {
    m_items.clear();
    m_mouseCaptured = false;
}

bool UICanvas::handleSDLEvent(const SDL_Event& e) {
    UIEvent ui;
    if (!convertSDLEvent(e, ui)) return false;

    if (ui.type == UIEventType::MouseUp) {
        m_mouseCaptured = false;
    }

    if (ui.type == UIEventType::MouseMove) {
        m_lastMousePos = ui.pos;
    }

    for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
        CanvasItem* item = it->get();
        if (!item->visible || !item->enabled) continue;

        if (ui.type == UIEventType::MouseUp) {
            item->handleEvent(ui);
            continue;
        }

        if (!item->contains(ui.pos)) continue;

        if (item->handleEvent(ui)) {
            if (ui.type == UIEventType::MouseDown) m_mouseCaptured = true;
            return true;
        }
    }

    return false;
}

void UICanvas::update(float dt, const Vec2f& mousePos) {
    m_lastMousePos = mousePos;

    for (auto& item : m_items) {
        if (!item->visible) continue;

        bool nowHovered = item->enabled && item->contains(mousePos);
        if (nowHovered != item->hovered) {
            item->hovered = nowHovered;
            if (nowHovered) item->onMouseEnter();
            else            item->onMouseLeave();
        }
        item->update(dt);
    }
}

void UICanvas::render(UIRenderer& r) {
    for (auto& item : m_items) {
        if (item->visible) item->render(r);
    }
}

bool UICanvas::isMouseCaptured() const {
    return m_mouseCaptured;
}

CanvasItem* UICanvas::hitTest(const Vec2f& pos) {
    for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
        CanvasItem* item = it->get();
        if (item->visible && item->enabled && item->contains(pos))
            return item;
    }
    return nullptr;
}
