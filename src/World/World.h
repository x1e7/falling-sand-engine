#pragma once

#include "Core/ParticleTypes.h"
#include "Core/ParticleRegistry.h"
#include "Core/Math/Vector2.h"
#include <vector>

struct ParticleInstance {
    ParticleId id;
    uint8_t age;
};

class World {
public:
    World(int width, int height, ParticleRegistry& registry);
    ~World();

    void tick(float deltaTime);

    void setParticle(int x, int y, ParticleId id);

    bool isInside(int x, int y) const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    ParticleRegistry& getRegistry() { return m_registry; }
    const ParticleRegistry& getRegistry() const { return m_registry; }

    ParticleInstance* getGrid() { return m_grid; }
    const ParticleInstance* getGrid() const { return m_grid; }
private:
    int m_width;
    int m_height;
    ParticleInstance* m_grid;
    bool* m_movedThisFrame;
    ParticleRegistry& m_registry;

    void updatePowder(const Vec2i& pos);
    void updateLiquid(const Vec2i& pos);
    void updateGas(const Vec2i& pos);
    void updateFire(const Vec2i& pos);

    bool canMove(const Vec2i& from, const Vec2i& to);
    void performSwap(const Vec2i& from, const Vec2i& to);

    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
    static constexpr float VELOCITY_DAMPING = 0.98f;
    static constexpr float GRAVITY = 0.1f;
};
