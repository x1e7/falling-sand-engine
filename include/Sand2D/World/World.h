#pragma once

#include <Sand2D/Core/ParticleTypes.h>
#include <Sand2D/Core/ParticleRegistry.h>
#include <Sand2D/Core/Math/Vector2.h>
#include <vector>

namespace Sand2D {

struct ParticleInstance {
    ParticleId id;
    float temperature = 20.0f;
    //Math::Vec2f velocity {0.0f, 0.0f};
};

class World {
public:
    World(int width, int height, ParticleRegistry& registry);

    void setParticle(int x, int y, ParticleId id, float temperature = 20.0f);

    ParticleId getParticleId(int x, int y) const;

    ParticleInstance& getParticle(int x, int y);
    const ParticleInstance& getParticle(int x, int y) const;

    void markDirty(int x, int y);
    bool isDirty(int x, int y) const;
    void clearDirty();

    bool isInside(int x, int y) const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    ParticleRegistry& getRegistry() { return m_registry; }
    const ParticleRegistry& getRegistry() const { return m_registry; }

private:
    int m_width;
    int m_height;
    std::vector<ParticleInstance> m_grid;
    std::vector<uint8_t> m_dirty;
    ParticleRegistry& m_registry;
};

}
