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
    int worldWidth = world.getWidth();
    int worldHeight = world.getHeight();
    int totalCells = worldWidth * worldHeight;
    m_movedThisFrame.assign(totalCells, 0);

    for (int y = 0; y < worldHeight; y++) {
        for (int x = 0; x < worldWidth; x++) {
            if (world.getParticleId(x, y) != ParticleRegistry::Empty) {
                world.incrementAge(x, y);
            } else {
                world.resetAge(x, y);
            }
        }
    }

    for (int y = worldHeight - 1; y >= 0; --y) {
        const bool leftToRight = (y % 2 == 0);
        const int startX = leftToRight ? 0 : worldWidth - 1;
        const int endX = leftToRight ? worldWidth : -1;
        const int stepX = leftToRight ? 1 : -1;

        for (int x = startX; x != endX; x += stepX) {
            const int idx = y * worldWidth + x;
            if (m_movedThisFrame[idx]) continue;

            const ParticleId id = world.getParticleId(x, y);
            if (id == ParticleRegistry::Empty) continue;

            const Math::Vec2i oldPos(x, y);
            Math::Vec2i newPos = oldPos;

            updateParticle(world, newPos);

            if (newPos.x != oldPos.x || newPos.y != oldPos.y) {
                const int newIdx = newPos.y * worldWidth + newPos.x;
                m_movedThisFrame[newIdx] = 1;
                m_movedThisFrame[idx] = 1;
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

    int age = world.getAge(pos.x, pos.y);

    std::uniform_int_distribution<int> lifetimeDist(30, 120);
    if (age > lifetimeDist(rng)) {
        ParticleId smokeId = world.getRegistry().findId("Smoke");
        if (smokeId != ParticleRegistry::Empty) {
            world.setParticle(pos.x, pos.y, smokeId);
        } else {
            world.setParticle(pos.x, pos.y, ParticleRegistry::Empty);
        }
        return;
    }

    std::uniform_int_distribution<int> chanceDist(0, 99);

    if (chanceDist(rng) < 60) {
        Math::Vec2i up(pos.x, pos.y - 1);
        if (tryMove(world, pos, up)) return;
    }

    if (chanceDist(rng) < 40) {
        Math::Vec2i right(pos.x + dx, pos.y);
        if (tryMove(world, pos, right)) return;

        Math::Vec2i left(pos.x - dx, pos.y);
        if (tryMove(world, pos, left)) return;
    }

    if (chanceDist(rng) < 30) {
        Math::Vec2i upRight(pos.x + dx, pos.y - 1);
        if (tryMove(world, pos, upRight)) return;

        Math::Vec2i upLeft(pos.x - dx, pos.y - 1);
        if (tryMove(world, pos, upLeft)) return;
    }

    if (chanceDist(rng) < 30) {
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
        return performSwap(world, from, to);
    }

    const auto& toDef = world.getRegistry().get(toP.id);

    if (toDef.state == PhysicalState::Solid) return false;

    if ((fromDef.state == PhysicalState::Gas || fromDef.state == PhysicalState::Fire) &&
        (toDef.state == PhysicalState::Liquid || toDef.state == PhysicalState::Powder)) {
            return performSwap(world, from, to);
    }

    if (fromDef.state == PhysicalState::Fire && toDef.state == PhysicalState::Gas) {
        return performSwap(world, from, to);
    }

    if (fromDef.state == PhysicalState::Powder && toDef.state == PhysicalState::Liquid) {
        return performSwap(world, from, to);
    }

    if (fromDef.state == PhysicalState::Liquid && toDef.state == PhysicalState::Powder) {
        return false;
    }

    if (fromDef.state == PhysicalState::Liquid && toDef.state == PhysicalState::Liquid) {
        if (toDef.density < fromDef.density) {
            return performSwap(world, from, to);
        }
        return false;
    }

    if (fromDef.state == PhysicalState::Powder && toDef.state == PhysicalState::Powder) {
        if (toDef.density < fromDef.density) {
            return performSwap(world, from, to);
        }
        return false;
    }

    if (toDef.density < fromDef.density) {
        return performSwap(world, from, to);
    }

    return false;
}

bool PhysicsSystem::performSwap(World& world, const Math::Vec2i& from, const Math::Vec2i& to) {
    ParticleInstance& fromP = world.getParticle(from.x, from.y);
    ParticleInstance& toP = world.getParticle(to.x, to.y);

    std::swap(fromP, toP);

    world.markDirty(from.x, from.y);
    world.markDirty(to.x, to.y);

    int fromIdx = from.y * world.getWidth() + from.x;
    int toIdx = to.y * world.getWidth() + to.x;

    m_movedThisFrame[toIdx] = true;
    m_movedThisFrame[fromIdx] = true;

    return true;
}

} // namespace Sand2D
