#pragma once

#include <World/World.h>
#include <Core/Math/Vector2.h>
#include <vector>

class PhysicsSystem {
public:
    PhysicsSystem() = default;
    void update(World& world, float deltaTime = 1.0f);

    // Temperature system control
    void setTemperatureUpdateRate(float rate) { m_tempUpdateRate = rate; } // Updates per second

private:
    void updateParticle(World& world, const Vec2i& pos);
    void updatePowder(World& world, const Vec2i& pos);
    void updateLiquid(World& world, const Vec2i& pos);
    void updateGas(World& world, const Vec2i& pos);
    void updateFire(World& world, const Vec2i& pos);

    // Temperature system
    void updateTemperatures(World& world, float dt);
    float getAverageNeighborTemp(const World& world, int x, int y) const;
    void applyIgnition(World& world, int x, int y, float temp);
    void applyPhaseChange(World& world, int x, int y, float temp, const ParticleDefinition& def);

    bool tryMove(World& world, const Vec2i& from, const Vec2i& to);
    bool performSwap(World& world, const Vec2i& from, const Vec2i& to);

    std::vector<uint8_t> m_movedThisFrame;

    // Physics timing
    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;

    // Temperature timing (update less frequently for performance)
    float m_tempAccumulator = 0.0f;
    float m_tempUpdateRate = 30.0f; // Update temperatures 30 times per second
};
