#include "Render/Camera.h"
#include <cmath>

Camera::Camera(int viewportWidth, int viewportHeight)
    : m_viewportWidth(viewportWidth)
    , m_viewportHeight(viewportHeight) {}

void Camera::setViewportSize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void Camera::setWorldBounds(int width, int height) {
    m_worldWidth = width;
    m_worldHeight = height;
}

void Camera::setLogicalSize(int width, int height) {
    m_logicalWidth = width;
    m_logicalHeight = height;
}

void Camera::setPosition(const Vec2f& pos) {
    m_position = pos;
    m_target = pos;
    clampPosition();
}

void Camera::setTarget(const Vec2f& target) {
    m_target = target;
}

void Camera::move(const Vec2f& delta) {
    m_target = m_target + delta;
}

void Camera::setZoom(float zoom) {
    m_zoom = std::clamp(zoom, m_minZoom, m_maxZoom);
}

void Camera::zoomIn(float amount) {
    setZoom(m_zoom * amount);
}

void Camera::zoomOut(float amount) {
    setZoom(m_zoom / amount);
}

void Camera::update(float deltaTime) {
    if (m_smoothing > 0.0f && (m_target - m_position).lengthSquared() > 0.0001f) {
        float speed = 1.0f - std::pow(1.0f - m_smoothing, deltaTime * 60.0f);
        speed = std::min(speed, 1.0f);

        m_position.x += (m_target.x - m_position.x) * speed;
        m_position.y += (m_target.y - m_position.y) * speed;

        if ((m_target - m_position).lengthSquared() < 0.0001f) {
            m_position = m_target;
        }

        clampPosition();
    } else {
        m_position = m_target;
        clampPosition();
    }
}

void Camera::snapToTarget() {
    m_position = m_target;
    clampPosition();
}

void Camera::clampPosition() {
    if (!m_clampToWorld) return;

    float halfW = getViewSize().x / 2.0f;
    float halfH = getViewSize().y / 2.0f;

    m_position.x = std::clamp(m_position.x, halfW, m_worldWidth - halfW);
    m_position.y = std::clamp(m_position.y, halfH, m_worldHeight - halfH);

    m_target.x = std::clamp(m_target.x, halfW, m_worldWidth - halfW);
    m_target.y = std::clamp(m_target.y, halfH, m_worldHeight - halfH);
}

void Camera::getViewBounds(int& minX, int& minY, int& maxX, int& maxY) const {
    float halfW = (m_logicalWidth / m_zoom) / 2.0f;
    float halfH = (m_logicalHeight / m_zoom) / 2.0f;

    minX = static_cast<int>(std::floor(m_position.x - halfW));
    minY = static_cast<int>(std::floor(m_position.y - halfH));
    maxX = static_cast<int>(std::ceil(m_position.x + halfW));
    maxY = static_cast<int>(std::ceil(m_position.y + halfH));

    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(m_worldWidth, maxX);
    maxY = std::min(m_worldHeight, maxY);
}

Vec2f Camera::getViewCenter() const {
    return m_position;
}

Vec2f Camera::getViewSize() const {
    return Vec2f(
        m_logicalWidth / m_zoom,
        m_logicalHeight / m_zoom
    );
}

Vec2f Camera::screenToWorld(const Vec2f& screenPos) const {
    float normalizedX = screenPos.x / m_viewportWidth;
    float normalizedY = screenPos.y / m_viewportHeight;

    Vec2f viewSize = getViewSize();

    float worldX = m_position.x - viewSize.x / 2.0f + normalizedX * viewSize.x;
    float worldY = m_position.y - viewSize.y / 2.0f + normalizedY * viewSize.y;

    return Vec2f(worldX, worldY);
}

Vec2f Camera::worldToScreen(const Vec2f& worldPos) const {
    Vec2f viewSize = getViewSize();

    float normalizedX = (worldPos.x - (m_position.x - viewSize.x / 2.0f)) / viewSize.x;
    float normalizedY = (worldPos.y - (m_position.y - viewSize.y / 2.0f)) / viewSize.y;

    return Vec2f(
        normalizedX * m_viewportWidth,
        normalizedY * m_viewportHeight
    );
}
