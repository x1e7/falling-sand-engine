#pragma once

#include "Core/ParticleTypes.h"
#include "Core/ParticleRegistry.h"
#include "Core/Math/Vector2.h"
#include <vector>

struct ParticleInstance {
    ParticleId id;
    uint8_t age;
    Vec2f velocity {0.0f, 0.0f};
};

class World {
public:
    World(int width, int height, ParticleRegistry& registry);

    void tick(float deltaTime);

    void setParticle(int x, int y, ParticleId id);
    ParticleId getParticleId(int x, int y) const;
    ParticleInstance& getParticle(int x, int y);
    const ParticleInstance& getParticle(int x, int y) const;
    bool isInside(int x, int y) const;

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

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    ParticleRegistry& getRegistry() { return m_registry; }
    const ParticleRegistry& getRegistry() const { return m_registry; }
private:
    int m_width;
    int m_height;
    std::vector<ParticleInstance> m_grid;
    std::vector<uint8_t> m_movedThisFrame;
    ParticleRegistry& m_registry;

    void updatePowder(const Vec2i& pos);
    void updateLiquid(const Vec2i& pos);
    void updateGas(const Vec2i& pos);
    void updateFire(const Vec2i& pos);

    bool canMove(const Vec2i& from, const Vec2i& to);
    void performSwap(const Vec2i& from, const Vec2i& to);

    void applyVelocity(ParticleInstance& p, const Vec2i& pos);
    void updateVelocity(ParticleInstance& p, const Vec2i& direction);

    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
    static constexpr float VELOCITY_DAMPING = 0.98f;
    static constexpr float GRAVITY = 0.1f;
};
