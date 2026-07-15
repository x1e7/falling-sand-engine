#pragma once

#include <Sand2D/World/World.h>
#include <Sand2D/Core/Math/Vector2.h>
#include <vector>

namespace Sand2D {

class PhysicsSystem {
public:
    PhysicsSystem() = default;
    void update(World& world, float deltaTime = 1.0f);

private:
    void updateParticle(World& world, const Math::Vec2i& pos);
    void updatePowder(World& world, const Math::Vec2i& pos);
    void updateLiquid(World& world, const Math::Vec2i& pos);
    void updateGas(World& world, const Math::Vec2i& pos);
    void updateFire(World& world, const Math::Vec2i& pos);

    bool tryMove(World& world, const Math::Vec2i& from, const Math::Vec2i& to);

    struct PendingMove {
        Math::Vec2i from;
        Math::Vec2i to;
    };

    std::vector<PendingMove> m_pendingMoves;
    std::vector<bool> m_movedThisFrame;
    std::vector<int> m_particleAge;
};

} // namespace Sand2D
