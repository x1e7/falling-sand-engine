#include <Physics/PhysicsSystem.h>
#include <Core/ParticleRegistry.h>
#include <random>
#include <chrono>

static std::mt19937& getRng() {
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    return rng;
}

void PhysicsSystem::update(World& world, float deltaTime) {
    m_accumulator += deltaTime;

    m_accumulator = std::min(m_accumulator, 0.1f);

    while (m_accumulator >= FIXED_DT) {
        int worldWidth = world.getWidth();
        int worldHeight = world.getHeight();
        int totalCells = worldWidth * worldHeight;

        if (m_movedThisFrame.size() != totalCells) {
            m_movedThisFrame.resize(totalCells, 0);
        } else {
            std::fill(m_movedThisFrame.begin(), m_movedThisFrame.end(), 0);
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
                if (id == ParticleRegistry::Empty) {
                    world.resetAge(x, y);
                    continue;
                }

                world.incrementAge(x, y);

                const Vec2i oldPos(x, y);
                Vec2i newPos = oldPos;

                updateParticle(world, newPos);

                if (newPos.x != oldPos.x || newPos.y != oldPos.y) {
                    const int newIdx = newPos.y * worldWidth + newPos.x;
                    m_movedThisFrame[newIdx] = 1;
                    m_movedThisFrame[idx] = 1;
                }
            }
        }

        m_accumulator -= FIXED_DT;
    }
}

void PhysicsSystem::updateParticle(World& world, const Vec2i& pos) {
    ParticleInstance& p = world.getParticle(pos.x, pos.y);
    const auto& registry = world.getRegistry();
    const auto& def = registry.get(p.id);

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

void PhysicsSystem::updatePowder(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(-1, 1);
    int dx = dist(rng);

    Vec2i down(pos.x, pos.y + 1);
    if (canMove(world, pos, down)) {
        performSwap(world, pos, down);
        return;
    }

    if (pos.y + 2 < world.getHeight()) {
        Vec2i fastDown(pos.x, pos.y + 2);
        if (world.getParticleId(fastDown.x, fastDown.y) == ParticleRegistry::Empty) {
            if (canMove(world, pos, fastDown)) {
                performSwap(world, pos, fastDown);
                return;
            }
        }
    }

    Vec2i downRight(pos.x + dx, pos.y + 1);
    if (canMove(world, pos, downRight)) {
        performSwap(world, pos, downRight);
        return;
    }

    Vec2i downLeft(pos.x - dx, pos.y + 1);
    if (canMove(world, pos, downLeft)) {
        performSwap(world, pos, downLeft);
        return;
    }
}

void PhysicsSystem::updateLiquid(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    Vec2i down(pos.x, pos.y + 1);
    if (canMove(world, pos, down)) {
        performSwap(world, pos, down);
        return;
    }

    if (pos.y + 2 < world.getHeight()) {
        Vec2i fastDown(pos.x, pos.y + 2);
        if (world.getParticleId(fastDown.x, fastDown.y) == ParticleRegistry::Empty) {
            if (canMove(world, pos, fastDown)) {
                performSwap(world, pos, fastDown);
                return;
            }
        }
    }

    Vec2i right(pos.x + dx, pos.y);
    if (canMove(world, pos, right)) {
        performSwap(world, pos, right);
        return;
    }

    if (dx != 0) {
        Vec2i left(pos.x - dx, pos.y);
        if (canMove(world, pos, left)) {
            performSwap(world, pos, left);
            return;
        }
    }

    Vec2i downRight(pos.x + dx, pos.y + 1);
    if (canMove(world, pos, downRight)) {
        performSwap(world, pos, downRight);
        return;
    }

    if (dx != 0) {
        Vec2i downLeft(pos.x - dx, pos.y + 1);
        if (canMove(world, pos, downLeft)) {
            performSwap(world, pos, downLeft);
            return;
        }
    }
}

void PhysicsSystem::updateGas(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(-1, 1);
    int dx = dist(rng);

    std::uniform_int_distribution<int> dissipateChance(0, 99);
    if (dissipateChance(rng) < 5) {
        world.setParticle(pos.x, pos.y, ParticleRegistry::Empty);
        return;
    }

    Vec2i up(pos.x, pos.y - 1);
    if (canMove(world, pos, up)) {
        performSwap(world, pos, up);
        return;
    }

    Vec2i right(pos.x + dx, pos.y);
    if (canMove(world, pos, right)) {
        performSwap(world, pos, right);
        return;
    }

    Vec2i left(pos.x - dx, pos.y);
    if (canMove(world, pos, left)) {
        performSwap(world, pos, left);
        return;
    }

    Vec2i upRight(pos.x + dx, pos.y - 1);
    if (canMove(world, pos, upRight)) {
        performSwap(world, pos, upRight);
        return;
    }

    Vec2i upLeft(pos.x - dx, pos.y - 1);
    if (canMove(world, pos, upLeft)) {
        performSwap(world, pos, upLeft);
        return;
    }
}

void PhysicsSystem::updateFire(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    int age = world.getAge(pos.x, pos.y);

    // Fire turns to smoke based on age
    float smokeChance = 5.0f + age;

    if (age > 20 && std::uniform_int_distribution<>(0, 99)(rng) < static_cast<int>(smokeChance)) {
        ParticleId smokeId = world.getRegistry().findId("Smoke");
        world.setParticle(pos.x, pos.y, smokeId != ParticleRegistry::Empty ? smokeId : ParticleRegistry::Empty);
        return;
    }

    static const Vec2i directions[] = {
        {0, -1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}, {0, 1}
    };

    int indices[] = {0, 1, 2, 3, 4, 5};
    for (int i = 5; i > 0; --i) {
        int j = std::uniform_int_distribution<>(0, i)(rng);
        std::swap(indices[i], indices[j]);
    }

    for (int i = 0; i < 6; ++i) {
        const auto& dir = directions[indices[i]];
        Vec2i newPos(pos.x + dir.x, pos.y + dir.y);
        if (canMove(world, pos, newPos)) {
            performSwap(world, pos, newPos);
            return;
        }
    }

    // Ignite oil and other flammable materials nearby
    ParticleId oilId = world.getRegistry().findId("Oil");
    if (oilId != ParticleRegistry::Empty && std::uniform_int_distribution<>(0, 99)(rng) < 15) {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                Vec2i neighbor(pos.x + dx, pos.y + dy);
                if (world.isInside(neighbor.x, neighbor.y)) {
                    ParticleId neighborId = world.getParticleId(neighbor.x, neighbor.y);
                    if (neighborId != ParticleRegistry::Empty) {
                        const auto& def = world.getRegistry().get(neighborId);
                        // Ignite if material has ignition temperature
                        if (def.canIgnite) {
                            ParticleId fireId = world.getRegistry().findId("Fire");
                            world.setParticle(neighbor.x, neighbor.y, fireId);
                        }
                    }
                }
            }
        }
    }
}

bool PhysicsSystem::canMove(World& world, const Vec2i& from, const Vec2i& to) {
    if (!world.isInside(to.x, to.y)) return false;

    const int toIdx = to.y * world.getWidth() + to.x;
    if (m_movedThisFrame[toIdx]) return false;

    const auto& registry = world.getRegistry();
    const auto& fromP = world.getParticle(from.x, from.y);
    const auto& toP = world.getParticle(to.x, to.y);

    if (toP.id == ParticleRegistry::Empty) {
        return true;
    }

    const auto& fromDef = registry.get(fromP.id);
    const auto& toDef = registry.get(toP.id);

    if (toDef.state == PhysicalState::Solid) return false;

    if (fromDef.density > toDef.density) {
        return true;
    }

    return false;
}

void PhysicsSystem::performSwap(World& world, const Vec2i& from, const Vec2i& to) {
    ParticleInstance& fromP = world.getParticle(from.x, from.y);
    ParticleInstance& toP = world.getParticle(to.x, to.y);

    std::swap(fromP, toP);

    world.markDirty(from.x, from.y);
    world.markDirty(to.x, to.y);

    int fromIdx = from.y * world.getWidth() + from.x;
    int toIdx = to.y * world.getWidth() + to.x;

    m_movedThisFrame[toIdx] = true;
    m_movedThisFrame[fromIdx] = true;
}
