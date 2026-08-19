#include "World/World.h"
#include <cstring>
#include <random>
#include <chrono>
#include <cmath>

static std::mt19937& getRng() {
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    return rng;
}

World::World(int width, int height, ParticleRegistry& registry)
    : m_width(width), m_height(height)
    , m_grid(width * height)
    , m_registry(registry)
{
    for (auto& p : m_grid) {
        p.id = ParticleRegistry::Empty;
        p.age = 0;
    }
}

void World::setParticle(int x, int y, ParticleId id) {
    if (!isInside(x, y)) return;
    auto& p = m_grid[y * m_width + x];
    p.id = id;
    p.age = 0;
}

ParticleId World::getParticleId(int x, int y) const {
    if (!isInside(x, y)) return ParticleRegistry::Empty;
    return m_grid[y * m_width + x].id;
}

ParticleInstance& World::getParticle(int x, int y) {
    return m_grid[y * m_width + x];
}

const ParticleInstance& World::getParticle(int x, int y) const {
    return m_grid[y * m_width + x];
}

bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

void World::tick(float deltaTime) {
    m_accumulator += std::min(deltaTime, 0.05f);
    m_accumulator = std::min(m_accumulator, 0.1f);

    while (m_accumulator >= FIXED_DT) {
        int w = getWidth(), h = getHeight();
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

                ParticleId id = getParticleId(x, y);
                if (id == ParticleRegistry::Empty) {
                    resetAge(x, y);
                    continue;
                }

                incrementAge(x, y);

                const auto& def = getRegistry().get(getParticleId(x, y));
                switch (def.state) {
                    case PhysicalState::Powder: updatePowder({x, y}); break;
                    case PhysicalState::Liquid: updateLiquid({x, y}); break;
                    case PhysicalState::Gas: updateGas({x, y}); break;
                    case PhysicalState::Fire: updateFire({x, y}); break;
                    default: break;
                }
            }
        }
        m_accumulator -= FIXED_DT;
    }
}

void World::applyVelocity(ParticleInstance& p, const Vec2i& pos) {
    p.velocity.x *= VELOCITY_DAMPING;
    p.velocity.y *= VELOCITY_DAMPING;

    const auto& def = getRegistry().get(p.id);
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

void World::updateVelocity(ParticleInstance& p, const Vec2i& direction) {
    p.velocity.x += direction.x * 0.05f;
    p.velocity.y += direction.y * 0.05f;
}

void World::updatePowder(const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = getParticle(pos.x, pos.y);
    applyVelocity(p, pos);

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
        if (canMove(pos, target)) {
            performSwap(pos, target);
            updateVelocity(p, dir);
            return;
        }
    }
}

void World::updateLiquid(const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = getParticle(pos.x, pos.y);
    applyVelocity(p, pos);

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
        if (canMove(pos, target)) {
            performSwap(pos, target);
            updateVelocity(p, dir);
            return;
        }
    }

    // Random diffusion
    if (std::uniform_int_distribution<>(0, 99)(rng) < 10) {
        p.velocity.x += (std::uniform_int_distribution<>(-1, 1)(rng)) * 0.01f;
    }
}

void World::updateGas(const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = getParticle(pos.x, pos.y);
    applyVelocity(p, pos);

    if (std::uniform_int_distribution<>(0, 99)(rng) < 5) {
        setParticle(pos.x, pos.y, ParticleRegistry::Empty);
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
        if (canMove(pos, target)) {
            performSwap(pos, target);
            updateVelocity(p, dir);
            return;
        }
    }
}

void World::updateFire(const Vec2i& pos) {
    auto& rng = getRng();
    int age = getAge(pos.x, pos.y);

    if (age > 20 && std::uniform_int_distribution<>(0, 99)(rng) < 5 + age) {
        ParticleId smokeId = getRegistry().findId("Smoke");
        setParticle(pos.x, pos.y, smokeId != ParticleRegistry::Empty ? smokeId : ParticleRegistry::Empty);
        return;
    }

    Vec2i dirs[] = {{0, -1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}, {0, 1}};
    for (int i = 5; i > 0; --i) {
        int j = std::uniform_int_distribution<>(0, i)(rng);
        std::swap(dirs[i], dirs[j]);
    }

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(pos, target)) {
            performSwap(pos, target);
            return;
        }
    }

    if (std::uniform_int_distribution<>(0, 99)(rng) < 15) {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                Vec2i n(pos.x + dx, pos.y + dy);
                if (isInside(n.x, n.y)) {
                    ParticleId pid = getParticleId(n.x, n.y);
                    if (pid != ParticleRegistry::Empty && getRegistry().get(pid).canIgnite) {
                        ParticleId fireId = getRegistry().findId("Fire");
                        setParticle(n.x, n.y, fireId);
                    }
                }
            }
        }
    }
}

bool World::canMove(const Vec2i& from, const Vec2i& to) {
    if (!isInside(to.x, to.y)) return false;

    int toIdx = to.y * getWidth() + to.x;
    if (m_movedThisFrame[toIdx]) return false;

    ParticleId fromId = getParticleId(from.x, from.y);
    ParticleId toId = getParticleId(to.x, to.y);

    if (toId == ParticleRegistry::Empty) return true;

    const auto& fromDef = getRegistry().get(fromId);
    const auto& toDef = getRegistry().get(toId);

    return toDef.state != PhysicalState::Solid && fromDef.density > toDef.density;
}

void World::performSwap(const Vec2i& from, const Vec2i& to) {
    ParticleInstance& fromP = getParticle(from.x, from.y);
    ParticleInstance& toP = getParticle(to.x, to.y);
    std::swap(fromP, toP);

    int fromIdx = from.y * getWidth() + from.x;
    int toIdx = to.y * getWidth() + to.x;
    m_movedThisFrame[toIdx] = m_movedThisFrame[fromIdx] = 1;
}
