#include <Physics/PhysicsSystem.h>
#include <Core/ParticleRegistry.h>
#include <random>
#include <chrono>
#include <cmath>

static std::mt19937& getRng() {
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    return rng;
}

void PhysicsSystem::update(World& world, float deltaTime) {
    m_accumulator += std::min(deltaTime, 0.05f);
    m_accumulator = std::min(m_accumulator, 0.1f);

    while (m_accumulator >= FIXED_DT) {
        int w = world.getWidth(), h = world.getHeight();
        int total = w * h;

        if (m_movedThisFrame.size() != total) {
            m_movedThisFrame.resize(total, 0);
        } else {
            std::fill(m_movedThisFrame.begin(), m_movedThisFrame.end(), 0);
        }

        for (int y = h - 1; y >= 0; --y) {
            int startX = (y % 2 == 0) ? 0 : w - 1;
            int endX = (y % 2 == 0) ? w : -1;
            int stepX = (y % 2 == 0) ? 1 : -1;

            for (int x = startX; x != endX; x += stepX) {
                int idx = y * w + x;
                if (m_movedThisFrame[idx]) continue;

                ParticleId id = world.getParticleId(x, y);
                if (id == ParticleRegistry::Empty) {
                    world.resetAge(x, y);
                    continue;
                }

                world.incrementAge(x, y);
                updateParticle(world, {x, y});
            }
        }
        m_accumulator -= FIXED_DT;
    }
}

void PhysicsSystem::updateParticle(World& world, const Vec2i& pos) {
    const auto& def = world.getRegistry().get(world.getParticleId(pos.x, pos.y));
    switch (def.state) {
        case PhysicalState::Powder: updatePowder(world, pos); break;
        case PhysicalState::Liquid: updateLiquid(world, pos); break;
        case PhysicalState::Gas: updateGas(world, pos); break;
        case PhysicalState::Fire: updateFire(world, pos); break;
        default: break;
    }
}

void PhysicsSystem::applyVelocity(World& world, ParticleInstance& p, const Vec2i& pos) {
    p.velocity.x *= VELOCITY_DAMPING;
    p.velocity.y *= VELOCITY_DAMPING;

    const auto& def = world.getRegistry().get(p.id);
    if (def.state != PhysicalState::Gas) {
        p.velocity.y += GRAVITY * (def.density / 1000.0f);
    } else {
        p.velocity.y -= GRAVITY * 0.5f;
    }

    if (p.velocity.x > 1.0f) {
        p.velocity.x = 1.0f;
    } else if (p.velocity.x < -1.0f) {
        p.velocity.x = -1.0f;
    }

    if (p.velocity.y > 1.0f) {
        p.velocity.y = 1.0f;
    } else if (p.velocity.y < -1.0f) {
        p.velocity.y = -1.0f;
    }
}

void PhysicsSystem::updateVelocity(ParticleInstance& p, const Vec2i& direction) {
    p.velocity.x += direction.x * 0.05f;
    p.velocity.y += direction.y * 0.05f;
}

void PhysicsSystem::updatePowder(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = world.getParticle(pos.x, pos.y);
    applyVelocity(world, p, pos);

    int dx = std::uniform_int_distribution<>(-1, 1)(rng);
    int biasX = (p.velocity.x > 0.5f) ? 1 : (p.velocity.x < -0.5f) ? -1 : 0;

    Vec2i dirs[] = {
        {0, 1},
        {dx + biasX, 1},
        {-dx - biasX, 1},
        {biasX, 0}
    };

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(world, pos, target)) {
            performSwap(world, pos, target);
            updateVelocity(p, dir);
            return;
        }
    }
}

void PhysicsSystem::updateLiquid(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = world.getParticle(pos.x, pos.y);
    applyVelocity(world, p, pos);

    int biasX = (p.velocity.x > 0.3f) ? 1 : (p.velocity.x < -0.3f) ? -1 : 0;
    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    // Build direction list with priority
    Vec2i dirs[] = {
        {0, 1},
        {biasX, 0},
        {biasX, 1},
        {dx, 1},
        {-dx, 1},
        {dx, 0}
    };

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(world, pos, target)) {
            performSwap(world, pos, target);
            updateVelocity(p, dir);
            return;
        }
    }

    // Random diffusion
    if (std::uniform_int_distribution<>(0, 99)(rng) < 10) {
        p.velocity.x += (std::uniform_int_distribution<>(-1, 1)(rng)) * 0.01f;
    }
}

void PhysicsSystem::updateGas(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = world.getParticle(pos.x, pos.y);
    applyVelocity(world, p, pos);

    if (std::uniform_int_distribution<>(0, 99)(rng) < 5) {
        world.setParticle(pos.x, pos.y, ParticleRegistry::Empty);
        return;
    }

    int biasX = (p.velocity.x > 0.3f) ? 1 : (p.velocity.x < -0.3f) ? -1 : 0;
    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    Vec2i dirs[] = {
        {0, -1},
        {dx, -1},
        {-dx, -1},
        {biasX, 0},
        {dx, 0},
        {-dx, 0}
    };

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(world, pos, target)) {
            performSwap(world, pos, target);
            updateVelocity(p, dir);
            return;
        }
    }
}

void PhysicsSystem::updateFire(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    int age = world.getAge(pos.x, pos.y);

    if (age > 20 && std::uniform_int_distribution<>(0, 99)(rng) < 5 + age) {
        ParticleId smokeId = world.getRegistry().findId("Smoke");
        world.setParticle(pos.x, pos.y, smokeId != ParticleRegistry::Empty ? smokeId : ParticleRegistry::Empty);
        return;
    }

    Vec2i dirs[] = {{0, -1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}, {0, 1}};
    for (int i = 5; i > 0; --i) {
        int j = std::uniform_int_distribution<>(0, i)(rng);
        std::swap(dirs[i], dirs[j]);
    }

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(world, pos, target)) {
            performSwap(world, pos, target);
            return;
        }
    }

    if (std::uniform_int_distribution<>(0, 99)(rng) < 15) {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                Vec2i n(pos.x + dx, pos.y + dy);
                if (world.isInside(n.x, n.y)) {
                    ParticleId pid = world.getParticleId(n.x, n.y);
                    if (pid != ParticleRegistry::Empty && world.getRegistry().get(pid).canIgnite) {
                        ParticleId fireId = world.getRegistry().findId("Fire");
                        world.setParticle(n.x, n.y, fireId);
                    }
                }
            }
        }
    }
}

bool PhysicsSystem::canMove(World& world, const Vec2i& from, const Vec2i& to) {
    if (!world.isInside(to.x, to.y)) return false;

    int toIdx = to.y * world.getWidth() + to.x;
    if (m_movedThisFrame[toIdx]) return false;

    ParticleId fromId = world.getParticleId(from.x, from.y);
    ParticleId toId = world.getParticleId(to.x, to.y);

    if (toId == ParticleRegistry::Empty) return true;

    const auto& fromDef = world.getRegistry().get(fromId);
    const auto& toDef = world.getRegistry().get(toId);

    return toDef.state != PhysicalState::Solid && fromDef.density > toDef.density;
}

void PhysicsSystem::performSwap(World& world, const Vec2i& from, const Vec2i& to) {
    ParticleInstance& fromP = world.getParticle(from.x, from.y);
    ParticleInstance& toP = world.getParticle(to.x, to.y);
    std::swap(fromP, toP);

    world.markDirty(from.x, from.y);
    world.markDirty(to.x, to.y);

    int fromIdx = from.y * world.getWidth() + from.x;
    int toIdx = to.y * world.getWidth() + to.x;
    m_movedThisFrame[toIdx] = m_movedThisFrame[fromIdx] = 1;
}
