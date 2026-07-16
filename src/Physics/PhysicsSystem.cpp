#include <Sand2D/Physics/PhysicsSystem.h>
#include <Sand2D/Core/ParticleRegistry.h>
#include <cstdlib>
#include <random>
#include <chrono>

namespace Sand2D {

static std::mt19937& getRng() {
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    return rng;
}

void PhysicsSystem::update(World& world, float deltaTime) {
    int totalCells = world.getWidth() * world.getHeight();
    m_movedThisFrame.assign(totalCells, false);
    m_pendingMoves.clear();

    // Update particle ages
    m_particleAge.resize(totalCells);
    for (int i = 0; i < totalCells; i++) {
        int x = i % world.getWidth();
        int y = i / world.getWidth();
        if (world.getParticleId(x, y) != ParticleRegistry::Empty) {
            m_particleAge[i]++;
        } else {
            m_particleAge[i] = 0;
        }
    }

    // Scan from bottom to top
    for (int y = world.getHeight() - 1; y >= 0; y--) {
        for (int x = 0; x < world.getWidth(); x++) {
            bool leftToRight = (x % 2 == 0);
            int idx;
            if (leftToRight) {
                idx = y * world.getWidth() + x;
            } else {
                idx = y * world.getWidth() + (world.getWidth() - 1 - x);
            }

            if (m_movedThisFrame[idx]) continue;

            Math::Vec2i pos(x, y);
            ParticleId id = world.getParticleId(x, y);
            if (id == ParticleRegistry::Empty) continue;

            Math::Vec2i oldPos = pos;
            updateParticle(world, pos);

            // Check if moved
            if (world.getParticleId(oldPos.x, oldPos.y) == ParticleRegistry::Empty &&
                world.getParticleId(pos.x, pos.y) != ParticleRegistry::Empty) {
                int newIdx = pos.y * world.getWidth() + pos.x;
                m_movedThisFrame[newIdx] = true;
            }
        }
    }
}

void PhysicsSystem::updateParticle(World& world, const Math::Vec2i& pos) {
    ParticleInstance& p = world.getParticle(pos.x, pos.y);
    const auto& def = world.getRegistry().get(p.id);

    switch (def.state) {
        case PhysicalState::Powder:
            updatePowder(world, pos);
            break;
        case PhysicalState::Liquid:
            updateLiquid(world, pos);
            break;
        case PhysicalState::Gas:
            updateGas(world, pos);
            break;
        case PhysicalState::Fire:
            updateFire(world, pos);
            break;
        default:
            break;
    }
}

void PhysicsSystem::updatePowder(World& world, const Math::Vec2i& pos) {
    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(-1, 1);
    int dx = dist(rng);

    Math::Vec2i down(pos.x, pos.y + 1);
    if (tryMove(world, pos, down)) return;

    if (pos.y + 2 < world.getHeight()) {
        Math::Vec2i fastDown(pos.x, pos.y + 2);
        if (world.getParticleId(fastDown.x, fastDown.y) == ParticleRegistry::Empty) {
            if (tryMove(world, pos, fastDown)) return;
        }
    }

    Math::Vec2i downRight(pos.x + dx, pos.y + 1);
    if (tryMove(world, pos, downRight)) return;

    Math::Vec2i downLeft(pos.x - dx, pos.y + 1);
    if (tryMove(world, pos, downLeft)) return;
}

void PhysicsSystem::updateLiquid(World& world, const Math::Vec2i& pos) {
    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(-1, 1);
    int dx = dist(rng);

    Math::Vec2i down(pos.x, pos.y + 1);
    if (tryMove(world, pos, down)) return;

    if (pos.y + 2 < world.getHeight()) {
        Math::Vec2i fastDown(pos.x, pos.y + 2);
        if (world.getParticleId(fastDown.x, fastDown.y) == ParticleRegistry::Empty) {
            if (tryMove(world, pos, fastDown)) return;
        }
    }

    Math::Vec2i right(pos.x + dx, pos.y);
    if (tryMove(world, pos, right)) return;

    Math::Vec2i left(pos.x - dx, pos.y);
    if (tryMove(world, pos, left)) return;

    Math::Vec2i downRight(pos.x + dx, pos.y + 1);
    if (tryMove(world, pos, downRight)) return;

    Math::Vec2i downLeft(pos.x - dx, pos.y + 1);
    if (tryMove(world, pos, downLeft)) return;
}

void PhysicsSystem::updateGas(World& world, const Math::Vec2i& pos) {
    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(-1, 1);
    int dx = dist(rng);

    std::uniform_int_distribution<int> dissipateChance(0, 99);
    if (dissipateChance(rng) < 5) {
        world.setParticle(pos.x, pos.y, ParticleRegistry::Empty);
        return;
    }

    Math::Vec2i up(pos.x, pos.y - 1);
    if (tryMove(world, pos, up)) return;

    Math::Vec2i right(pos.x + dx, pos.y);
    if (tryMove(world, pos, right)) return;

    Math::Vec2i left(pos.x - dx, pos.y);
    if (tryMove(world, pos, left)) return;

    Math::Vec2i upRight(pos.x + dx, pos.y - 1);
    if (tryMove(world, pos, upRight)) return;

    Math::Vec2i upLeft(pos.x - dx, pos.y - 1);
    if (tryMove(world, pos, upLeft)) return;
}

void PhysicsSystem::updateFire(World& world, const Math::Vec2i& pos) {
    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(-1, 1);
    int dx = dist(rng);

    int idx = pos.y * world.getWidth() + pos.x;

    if (m_particleAge[idx] > 3 + (std::rand() % 3)) {
        ParticleId smokeId = world.getRegistry().findId("Smoke");
        if (smokeId != ParticleRegistry::Empty) {
            world.setParticle(pos.x, pos.y, smokeId);
        } else {
            world.setParticle(pos.x, pos.y, ParticleRegistry::Empty);
        }
        return;
    }

    if ((std::rand() % 100) < 60) {
        Math::Vec2i up(pos.x, pos.y - 1);
        if (tryMove(world, pos, up)) return;
    }

    if ((std::rand() % 100) < 40) {
        Math::Vec2i right(pos.x + dx, pos.y);
        if (tryMove(world, pos, right)) return;

        Math::Vec2i left(pos.x - dx, pos.y);
        if (tryMove(world, pos, left)) return;
    }

    if ((std::rand() % 100) < 30) {
        Math::Vec2i upRight(pos.x + dx, pos.y - 1);
        if (tryMove(world, pos, upRight)) return;

        Math::Vec2i upLeft(pos.x - dx, pos.y - 1);
        if (tryMove(world, pos, upLeft)) return;
    }

    if ((std::rand() % 100) < 30) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx2 = -1; dx2 <= 1; dx2++) {
                if (dx2 == 0 && dy == 0) continue;

                Math::Vec2i neighbor(pos.x + dx2, pos.y + dy);
                if (!world.isInside(neighbor.x, neighbor.y)) continue;

                ParticleId neighborId = world.getParticleId(neighbor.x, neighbor.y);
                if (neighborId == ParticleRegistry::Empty) continue;

                const auto& neighborDef = world.getRegistry().get(neighborId);
                if (neighborDef.name == "Oil") {
                    ParticleId fireId = world.getRegistry().findId("Fire");
                    if (fireId != ParticleRegistry::Empty) {
                        world.setParticle(neighbor.x, neighbor.y, fireId);
                    }
                }
            }
        }
    }
}

bool PhysicsSystem::tryMove(World& world, const Math::Vec2i& from, const Math::Vec2i& to) {
    if (!world.isInside(to.x, to.y)) return false;

    int toIdx = to.y * world.getWidth() + to.x;
    if (m_movedThisFrame[toIdx]) return false;

    ParticleInstance& fromP = world.getParticle(from.x, from.y);
    ParticleInstance& toP = world.getParticle(to.x, to.y);
    const auto& fromDef = world.getRegistry().get(fromP.id);

    if (toP.id == ParticleRegistry::Empty) {
        std::swap(fromP.id, toP.id);
        std::swap(fromP.temperature, toP.temperature);

        world.markDirty(from.x, from.y);
        world.markDirty(to.x, to.y);

        int fromIdx = from.y * world.getWidth() + from.x;
        m_movedThisFrame[toIdx] = true;
        m_movedThisFrame[fromIdx] = true;
        return true;
    }

    const auto& toDef = world.getRegistry().get(toP.id);

    if (toDef.state == PhysicalState::Solid) return false;

    if ((fromDef.state == PhysicalState::Gas || fromDef.state == PhysicalState::Fire) &&
        (toDef.state == PhysicalState::Liquid || toDef.state == PhysicalState::Powder)) {
        std::swap(fromP.id, toP.id);
        std::swap(fromP.temperature, toP.temperature);

        world.markDirty(from.x, from.y);
        world.markDirty(to.x, to.y);

        int fromIdx = from.y * world.getWidth() + from.x;
        m_movedThisFrame[toIdx] = true;
        m_movedThisFrame[fromIdx] = true;
        return true;
    }

    if (fromDef.state == PhysicalState::Powder && toDef.state == PhysicalState::Liquid) {
        std::swap(fromP.id, toP.id);
        std::swap(fromP.temperature, toP.temperature);

        world.markDirty(from.x, from.y);
        world.markDirty(to.x, to.y);

        int fromIdx = from.y * world.getWidth() + from.x;
        m_movedThisFrame[toIdx] = true;
        m_movedThisFrame[fromIdx] = true;
        return true;
    }

    if (fromDef.state == PhysicalState::Liquid && toDef.state == PhysicalState::Powder) {
        return false;
    }

    if (fromDef.state == PhysicalState::Liquid && toDef.state == PhysicalState::Liquid) {
        if (toDef.density < fromDef.density) {
            std::swap(fromP.id, toP.id);
            std::swap(fromP.temperature, toP.temperature);

            world.markDirty(from.x, from.y);
            world.markDirty(to.x, to.y);

            int fromIdx = from.y * world.getWidth() + from.x;
            m_movedThisFrame[toIdx] = true;
            m_movedThisFrame[fromIdx] = true;
            return true;
        }
        return false;
    }

    if (fromDef.state == PhysicalState::Powder && toDef.state == PhysicalState::Powder) {
        if (toDef.density < fromDef.density) {
            std::swap(fromP.id, toP.id);
            std::swap(fromP.temperature, toP.temperature);

            world.markDirty(from.x, from.y);
            world.markDirty(to.x, to.y);

            int fromIdx = from.y * world.getWidth() + from.x;
            m_movedThisFrame[toIdx] = true;
            m_movedThisFrame[fromIdx] = true;
            return true;
        }
        return false;
    }

    if (toDef.density < fromDef.density) {
        std::swap(fromP.id, toP.id);
        std::swap(fromP.temperature, toP.temperature);

        world.markDirty(from.x, from.y);
        world.markDirty(to.x, to.y);

        int fromIdx = from.y * world.getWidth() + from.x;
        m_movedThisFrame[toIdx] = true;
        m_movedThisFrame[fromIdx] = true;
        return true;
    }

    return false;
}

} // namespace Sand2D
