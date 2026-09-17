#pragma once

#include "UI/UIRenderer.h"
#include "UI/CanvasItem.h"
#include <vector>
#include <memory>

class UICanvas {
public:
    void addItem(std::unique_ptr<CanvasItem> item);
    void removeItem(CanvasItem* item);
    void clear();

    bool handleSDLEvent(const SDL_Event& e);
    void update(float dt, const Vec2f& mousePos);

    void render(UIRenderer& r);

    bool isMouseCaptured() const;
    CanvasItem* hitTest(const Vec2f& pos);

private:
    std::vector<std::unique_ptr<CanvasItem>> m_items;
    bool m_mouseCaptured = false;
    Vec2f m_lastMousePos{0, 0};
};
