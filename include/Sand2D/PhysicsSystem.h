#pragma once

#include <Sand2D/World.h>
#include <vector>

namespace Sand2D {

class PhysicsSystem {
public:
    void update(World& world);
    
private:
    void updateParticle(World& world, int x, int y);
    void updatePowder(World& world, int x, int y);
    void updateLiquid(World& world, int x, int y);
    void updateGas(World& world, int x, int y);
    void updateFire(World& world, int x, int y);
    
    bool tryMove(World& world, int fromX, int fromY, int toX, int toY);
    
    struct PendingMove {
        int fromX, fromY;
        int toX, toY;
    };
    std::vector<PendingMove> m_pendingMoves;
    std::vector<bool> m_movedThisFrame;
    std::vector<int> m_particleAge;
};

}