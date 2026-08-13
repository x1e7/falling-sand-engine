#pragma once

#include <World/World.h>
#include <Core/Math/Vector2.h>
#include <vector>

class PhysicsSystem {
public:
    PhysicsSystem() = default;
    void update(World& world, float deltaTime = 1.0f);
private:
    void updateParticle(World& world, const Vec2i& pos);
    void updatePowder(World& world, const Vec2i& pos);
    void updateLiquid(World& world, const Vec2i& pos);
    void updateGas(World& world, const Vec2i& pos);
    void updateFire(World& world, const Vec2i& pos);

    bool tryMove(World& world, const Vec2i& from, const Vec2i& to);
    bool performSwap(World& world, const Vec2i& from, const Vec2i& to);

    std::vector<uint8_t> m_movedThisFrame;

    // Physics timing
    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
};
