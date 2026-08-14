#include "Render/Camera.h"
#include <algorithm>
#include <cmath>

Camera::Camera(int viewportWidth, int viewportHeight)
    : m_viewportWidth(viewportWidth)
    , m_viewportHeight(viewportHeight) {}

void Camera::setViewportSize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void Camera::setPosition(const Vec2f& pos) {
    m_position = pos;
    m_target = pos;
}

void Camera::setTarget(const Vec2f& target) {
    m_target = target;
}

void Camera::move(const Vec2f& delta) {
    m_target = m_target + delta;
}

void Camera::update(float deltaTime) {
    if (m_target != m_position) {
        float speed = 1.0f - std::pow(1.0f - m_smoothing, deltaTime * 60.0f);
        m_position.x += (m_target.x - m_position.x) * speed;
        m_position.y += (m_target.y - m_position.y) * speed;

        if (std::abs(m_position.x - m_target.x) < 0.1f &&
            std::abs(m_position.y - m_target.y) < 0.1f) {
            m_position = m_target;
        }
    }
}

void Camera::getViewBounds(int& minX, int& minY, int& maxX, int& maxY,
                           int worldWidth, int worldHeight) const {
    float halfW = m_viewportWidth / 2.0f;
    float halfH = m_viewportHeight / 2.0f;

    minX = std::max(0, static_cast<int>(std::floor(m_position.x - halfW)));
    minY = std::max(0, static_cast<int>(std::floor(m_position.y - halfH)));
    maxX = std::min(worldWidth, static_cast<int>(std::ceil(m_position.x + halfW)));
    maxY = std::min(worldHeight, static_cast<int>(std::ceil(m_position.y + halfH)));
}

Vec2f Camera::screenToWorld(const Vec2f& screenPos) const {
    return Vec2f(
        screenPos.x + m_position.x - m_viewportWidth / 2.0f,
        screenPos.y + m_position.y - m_viewportHeight / 2.0f
    );
}
