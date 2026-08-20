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
    : m_width(width), m_height(height), m_registry(registry) {
    m_grid = new ParticleInstance[width * height];
    m_movedThisFrame = new bool[width * height];

    for (int i = 0; i < width * height; ++i) {
        m_grid[i].id = ParticleRegistry::Empty;
        m_grid[i].age = 0;
        m_movedThisFrame[i] = false;
    }
}

World::~World() {
    delete[] m_grid;
    delete[] m_movedThisFrame;
}

void World::setParticle(int x, int y, ParticleId id) {
    if (!isInside(x, y)) return;
    auto& p = m_grid[y * m_width + x];
    p.id = id;
    p.age = 0;
}

bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

void World::tick(float deltaTime) {
    m_accumulator += std::min(deltaTime, 0.05f);
    m_accumulator = std::min(m_accumulator, 0.1f);

    while (m_accumulator >= FIXED_DT) {
        int w = m_width, h = m_height;
        int total = w * h;

        std::memset(m_movedThisFrame, 0, total * sizeof(bool));

        for (int y = h - 1; y >= 0; --y) {
            int startX = (y % 2 == 0) ? 0 : w - 1;
            int endX = (y % 2 == 0) ? w : -1;
            int stepX = (y % 2 == 0) ? 1 : -1;

            for (int x = startX; x != endX; x += stepX) {
                int idx = y * w + x;
                if (m_movedThisFrame[idx]) continue;

                ParticleInstance& p = m_grid[idx];
                if (p.id == ParticleRegistry::Empty) {
                    p.age = 0;
                    continue;
                }

                p.age++;

                const auto& def = m_registry.get(p.id);
                switch (def.state) {
                    case PhysicalState::Powder: updatePowder({x,y}); break;
                    case PhysicalState::Liquid:  updateLiquid({x,y}); break;
                    case PhysicalState::Gas:    updateGas({x,y}); break;
                    case PhysicalState::Fire:   updateFire({x,y}); break;
                    default: break;
                }
            }
        }
        m_accumulator -= FIXED_DT;
    }
}

void World::updatePowder(const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = m_grid[pos.y * m_width + pos.x];

    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    Vec2i dirs[] = {
        {0, 1},
        {dx, 1},
        {-dx, 1},
    };

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(pos, target)) {
            performSwap(pos, target);
            return;
        }
    }
}

void World::updateLiquid(const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = m_grid[pos.y * m_width + pos.x];

    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    // Build direction list with priority
    Vec2i dirs[] = {
        {0, 1},
        {dx, 1},
        {-dx, 1},
        {dx, 0}
    };

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(pos, target)) {
            performSwap(pos, target);
            return;
        }
    }
}

void World::updateGas(const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = m_grid[pos.y * m_width + pos.x];

    if (std::uniform_int_distribution<>(0, 99)(rng) < 5) {
        setParticle(pos.x, pos.y, ParticleRegistry::Empty);
        return;
    }

    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    Vec2i dirs[] = {
        {0, -1},
        {dx, -1},
        {-dx, -1},
        {dx, 0},
        {-dx, 0}
    };

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(pos, target)) {
            performSwap(pos, target);
            return;
        }
    }
}

void World::updateFire(const Vec2i& pos) {
    auto& rng = getRng();
    int age = m_grid[pos.y * m_width + pos.x].age;

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
                    ParticleId pid = m_grid[pos.y * m_width + pos.x].id;
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

    ParticleId fromId = m_grid[from.y * m_width + from.x].id;
    ParticleId toId = m_grid[to.y * m_width + to.x].id;

    if (toId == ParticleRegistry::Empty) return true;

    const auto& fromDef = getRegistry().get(fromId);
    const auto& toDef = getRegistry().get(toId);

    return toDef.state != PhysicalState::Solid && fromDef.density > toDef.density;
}

void World::performSwap(const Vec2i& from, const Vec2i& to) {
    ParticleInstance& fromP = m_grid[from.y * m_width + from.x];
    ParticleInstance& toP = m_grid[to.y * m_width + to.x];
    std::swap(fromP, toP);

    int fromIdx = from.y * getWidth() + from.x;
    int toIdx = to.y * getWidth() + to.x;
    m_movedThisFrame[toIdx] = m_movedThisFrame[fromIdx] = 1;
}
