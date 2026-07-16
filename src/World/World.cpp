#include <Sand2D/World/World.h>
#include <cstring>

namespace Sand2D {

World::World(int width, int height, ParticleRegistry& registry)
    : m_width(width), m_height(height)
    , m_grid(width * height)
    , m_dirty(width * height, 1)
    , m_registry(registry)
{
    for (auto& p : m_grid) {
        p.id = ParticleRegistry::Empty;
    }
}

void World::setParticle(int x, int y, ParticleId id, uint8_t temp) {
    if (!isInside(x, y)) return;
    auto& p = m_grid[y * m_width + x];
    p.id = id;
    p.temp = temp;

    markDirty(x, y);
}

ParticleId World::getParticleId(int x, int y) const {
    if (!isInside(x, y)) return ParticleRegistry::Empty;
    return m_grid[y * m_width + x].id;
}

ParticleInstance& World::getParticle(int x, int y) {
    return m_grid[y * m_width + x];
}

const ParticleInstance& World::getParticle(int x, int y) const {
    return m_grid[y * m_width + x];
}

void World::markDirty(int x, int y)
{
    if (!isInside(x, y)) return;
    size_t idx = y * m_width + x;
    m_dirty[idx] = 1;

    if (x > 0) {
        m_dirty[idx - 1] = 1;
        if (y > 0) m_dirty[idx - m_width - 1] = 1;
        if (y + 1 < m_height) m_dirty[idx + m_width - 1] = 1;
    }
    if (x + 1 < m_width) {
        m_dirty[idx + 1] = 1;
        if (y > 0) m_dirty[idx - m_width + 1] = 1;
        if (y + 1 < m_height) m_dirty[idx + m_width + 1] = 1;
    }
    if (y > 0) m_dirty[idx - m_width] = 1;
    if (y + 1 < m_height) m_dirty[idx + m_width] = 1;
}

bool World::isDirty(int x, int y) const
{
    size_t idx = y * m_width + x;
    return m_dirty[idx];
}

void World::clearDirty()
{
    memset(m_dirty.data(), 0, m_dirty.size());
}

bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

}
