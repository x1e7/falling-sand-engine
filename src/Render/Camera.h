#pragma once

#include "Core/Math/Vector2.h"
#include <algorithm>

class Camera {
public:
    Camera(int viewportWidth = 1280, int viewportHeight = 720);

    void setViewportSize(int width, int height);
    void setWorldBounds(int width, int height);
    void setLogicalSize(int width, int height);

    void setPosition(const Vec2f& pos);
    void setTarget(const Vec2f& target);
    void move(const Vec2f& delta);

    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }
    void zoomIn(float amount = 1.1f);
    void zoomOut(float amount = 1.1f);
    void setMinZoom(float minZoom) { m_minZoom = minZoom; }
    void setMaxZoom(float maxZoom) { m_maxZoom = maxZoom; }

    void update(float deltaTime);
    void snapToTarget();

    void getViewBounds(int& minX, int& minY, int& maxX, int& maxY) const;
    Vec2f getViewCenter() const;
    Vec2f getViewSize() const;

    Vec2f screenToWorld(const Vec2f& screenPos) const;
    Vec2f worldToScreen(const Vec2f& worldPos) const;

    const Vec2f& getPosition() const { return m_position; }
    const Vec2f& getTarget() const { return m_target; }
    int getViewportWidth() const { return m_viewportWidth; }
    int getViewportHeight() const { return m_viewportHeight; }
    int getLogicalWidth() const { return m_logicalWidth; }
    int getLogicalHeight() const { return m_logicalHeight; }
    bool isSmoothing() const { return m_smoothing > 0.0f; }

    void setSmoothing(float smoothing) { m_smoothing = std::clamp(smoothing, 0.0f, 1.0f); }
    float getSmoothing() const { return m_smoothing; }
    void setClampToWorld(bool clamp) { m_clampToWorld = clamp; }

private:
    Vec2f m_position{0.0f, 0.0f};
    Vec2f m_target{0.0f, 0.0f};

    float m_zoom = 1.0f;
    float m_minZoom = 0.1f;
    float m_maxZoom = 10.0f;
    float m_smoothing = 0.1f;

    int m_viewportWidth = 1280;
    int m_viewportHeight = 720;
    int m_logicalWidth = 200;
    int m_logicalHeight = 150;

    int m_worldWidth = 800;
    int m_worldHeight = 600;
    bool m_clampToWorld = true;

    void clampPosition();
};
