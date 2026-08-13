#pragma once

#include <Core/ParticleTypes.h>
#include <Core/ParticleRegistry.h>
#include <Core/Math/Vector2.h>
#include <vector>

struct ParticleInstance {
    ParticleId id;
    uint8_t age;
    Vec2f velocity {0.0f, 0.0f};
};

class World {
public:
    World(int width, int height, ParticleRegistry& registry);

    void setParticle(int x, int y, ParticleId id, uint8_t temp = 55);

    ParticleId getParticleId(int x, int y) const;

    ParticleInstance& getParticle(int x, int y);
    const ParticleInstance& getParticle(int x, int y) const;

    void incrementAge(int x, int y) {
        if (!isInside(x, y)) return;
        m_grid[y * m_width + x].age++;
    }

    int getAge(int x, int y) const {
        if (!isInside(x, y)) return 0;
        return m_grid[y * m_width + x].age;
    }

    void resetAge(int x, int y) {
        if (!isInside(x, y)) return;
        m_grid[y * m_width + x].age = 0;
    }

    void markDirty(int x, int y);
    void markAllDirty() {
        std::fill(m_dirty.begin(), m_dirty.end(), 1);
    }
    bool isDirty(int x, int y) const;
    void clearDirty();

    bool isInside(int x, int y) const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    ParticleRegistry& getRegistry() { return m_registry; }
    const ParticleRegistry& getRegistry() const { return m_registry; }

    template<typename Func>
    void forEachDirtyCell(Func&& func) const {
        for (int y = 0; y < m_height; ++y) {
            size_t rowOffset = y * m_width;
            for (int x = 0; x < m_width; ++x) {
                if (m_dirty[rowOffset + x]) {
                    func(x, y);
                }
            }
        }
    }
private:
    int m_width;
    int m_height;
    std::vector<ParticleInstance> m_grid;
    std::vector<uint8_t> m_dirty;
    ParticleRegistry& m_registry;
};
