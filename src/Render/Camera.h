#pragma once

#include "Core/Math/Vector2.h"

class Camera {
    public:
        Camera(int viewportWidth = 1280, int viewportHeight = 720);

        void setViewportSize(int width, int height);

        void setPosition(const Vec2f& pos);
        void setTarget(const Vec2f& target);
        void move(const Vec2f& delta);

        void update(float deltaTime);

        void getViewBounds(int& minX, int& minY, int& maxX, int& maxY,
                               int worldWidth, int worldHeight) const;

        Vec2f screenToWorld(const Vec2f& screenPos) const;

        const Vec2f& getPosition() const { return m_position; }
        int getViewportWidth() const { return m_viewportWidth; }
        int getViewportHeight() const { return m_viewportHeight; }
    private:
        Vec2f m_position{0.0f, 0.0f};
        Vec2f m_target{0.0f, 0.0f};

        float m_smoothing = 0.1f;

        int m_viewportWidth = 1280;
        int m_viewportHeight = 720;
};
