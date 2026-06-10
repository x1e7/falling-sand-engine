#include <Sand2D/World.h>

namespace Sand2D {

World::World(int width, int height, ParticleRegistry& registry) 
    : m_width(width), m_height(height)
    , m_grid(width * height)
    , m_registry(registry)
{
    for (auto& p : m_grid) {
        p.id = ParticleRegistry::Empty;
        p.temperature = 20.0f;
    }
}

ParticleId World::getParticleId(int x, int y) const {
    if (!isInside(x, y)) return ParticleRegistry::Empty;
    return m_grid[y * m_width + x].id;
}

ParticleInstance& World::getParticle(int x, int y) {
    return m_grid[y * m_width + x];
}

void World::setParticle(int x, int y, ParticleId id, float temperature) {
    if (!isInside(x, y)) return;
    auto& p = m_grid[y * m_width + x];
    p.id = id;
    p.temperature = temperature;
}

bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

}