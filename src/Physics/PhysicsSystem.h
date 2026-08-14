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

    bool canMove(World& world, const Vec2i& from, const Vec2i& to);
    void performSwap(World& world, const Vec2i& from, const Vec2i& to);

    void applyVelocity(World& world, ParticleInstance& p, const Vec2i& pos);
    void updateVelocity(ParticleInstance& p, const Vec2i& direction);

    std::vector<uint8_t> m_movedThisFrame;
    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
    static constexpr float VELOCITY_DAMPING = 0.98f;
    static constexpr float GRAVITY = 0.1f;
};
